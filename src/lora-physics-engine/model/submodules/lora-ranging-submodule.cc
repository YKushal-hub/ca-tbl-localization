#include "lora-ranging-submodule.h"
#include <cmath>
#include <algorithm>
#include <limits>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace ns3 {

namespace {

constexpr double kSpeedOfLightMps = 299792458.0;

/**
 * Cramer-Rao lower bound on the standard deviation of a time-of-arrival range
 * estimate.
 *
 *   sigma_t = 1 / (2*pi*beta_rms*sqrt(2*SNR))
 *   beta_rms = bandwidthHz / sqrt(12)        (flat spectrum over the band)
 *   sigma_r = c * sigma_t
 *
 * snrDbEffective is E/N0 at the correlator output, NOT in-band SNR. The caller's
 * integration gain must already be folded in.
 *
 * twoWay divides by sqrt(2): a symmetric two-way exchange averages an outbound
 * and a return timestamp, which are independent. Tier 1 is one-way (TDoA off a
 * single broadcast) and does not get this.
 *
 * Calibration check: bandwidthHz = 125e3 gives beta_rms = 36,084 Hz, which
 * reproduces simstatus.md section 4.1 exactly. If that number moves, this
 * function is miswired.
 *
 * This duplicates CrlbRangeStdM() in scratch/ca_tbl_causal_sim.cc deliberately.
 * That function is the probe's independent audit of THIS model; if both called one
 * shared implementation, a wrong formula would agree with itself and the check
 * would pass while measuring nothing.
 */
static double CrlbRangeStdM(double bandwidthHz, double snrDbEffective, bool twoWay)
{
    if (!(bandwidthHz > 0.0)) {
        return std::numeric_limits<double>::infinity();
    }
    const double snrLin = std::pow(10.0, snrDbEffective / 10.0);
    if (!(snrLin > 0.0) || !std::isfinite(snrLin)) {
        return std::numeric_limits<double>::infinity();
    }
    const double betaRms = bandwidthHz / std::sqrt(12.0);
    const double sigmaT  = 1.0 / (2.0 * M_PI * betaRms * std::sqrt(2.0 * snrLin));
    const double sigmaM  = kSpeedOfLightMps * sigmaT;
    return twoWay ? sigmaM / std::sqrt(2.0) : sigmaM;
}

/**
 * sigma = max(CRLB, hardware floor), with a defensive fallback.
 *
 * The fallback only triggers if the caller passed a non-positive bandwidth or an
 * SNR that underflows to zero. The engines always pass a positive effBw (their
 * own default is 125 kHz) and a finite SNR, so it is unreachable in normal
 * operation; it exists so a caller error produces a floor-valued sigma instead of
 * a non-finite one propagating into the EKF R-matrix.
 */
static double RangingSigmaM(double bandwidthHz,
                             double snrDbEffective,
                             double floorM,
                             bool twoWay)
{
    const double crlbM = CrlbRangeStdM(bandwidthHz, snrDbEffective, twoWay);
    if (!std::isfinite(crlbM)) {
        return floorM;
    }
    return std::max(crlbM, floorM);
}

/**
 * Algebraic inverse of ComputePathLossDb() (lora-propagation-submodule.cc).
 * This is NOT an independent model -- it is the exact inverse of that
 * function's confirmed three branches, including the -6.5 dB calibration
 * offset in the urban 3GPP UMa branch.
 *
 * If ComputePathLossDb() changes, this MUST change with it.
 *
 * All field names confirmed against PropagationParams in
 * lora-physics-parameters.h lines 295-313:
 *   pathLossExponent, frequencyHz, referenceDistanceM,
 *   enableDualSlope, dualSlopeBreakpointM, pathLossExponent2, envClass.
 */
static double InvertPropagationLossDb(double plDb,
                                       const PropagationParams& propParams,
                                       double freqHz)
{
    double d0     = propParams.referenceDistanceM;
    double lambda = kSpeedOfLightMps / freqHz;
    double fsplD0 = 20.0 * std::log10(4.0 * M_PI * d0 / lambda);
    double n1     = propParams.pathLossExponent.mean;

    if (propParams.envClass == ENV_URBAN_DENSE ||
        propParams.envClass == ENV_URBAN_MODERATE)
    {
        // 3GPP UMa two-segment, d_break = 1000 m, -6.5 dB calibration offset.
        // Exact mirror of the confirmed ComputePathLossDb() branch.
        double f_GHz   = freqHz / 1e9;
        double dBreak  = 1000.0;
        double plBreak = 28.0 + 22.0 * std::log10(dBreak)
                       + 20.0 * std::log10(f_GHz) - 6.5;

        if (plDb <= plBreak) {
            return std::pow(10.0,
                (plDb - 28.0 - 20.0 * std::log10(f_GHz) + 6.5) / 22.0);
        }
        return dBreak * std::pow(10.0, (plDb - plBreak) / 20.0);
    }

    if (propParams.enableDualSlope) {
        // Generic params-driven dual-slope -- covers any environment with
        // enableDualSlope set, not just urban.
        double dBreak  = propParams.dualSlopeBreakpointM;
        double n2      = propParams.pathLossExponent2.mean;
        double plBreak = fsplD0 + 10.0 * n1 * std::log10(dBreak / d0);

        if (plDb <= plBreak) {
            return d0 * std::pow(10.0, (plDb - fsplD0) / (10.0 * n1));
        }
        return dBreak * std::pow(10.0, (plDb - plBreak) / (10.0 * n2));
    }

    // Single-slope, all other environments.
    return d0 * std::pow(10.0, (plDb - fsplD0) / (10.0 * n1));
}

} // anonymous namespace

double ComputeRangingErrorM(LinkPhysicsState& state,
                             RangingTier tier,
                             const RangingParams& params,
                             const PropagationParams& propParams,
                             double frequencyHz,
                             double bandwidthHz,
                             double snrDbInBand,
                             uint8_t spreadingFactor,
                             LoRaDistributionSampler& sampler)
{
    if (!params.enabled) {
        state.rangingSigmaM = 0.0;
        return 0.0;
    }

    switch (tier) {

        case RANGING_TIER_868: {
            // RSSI-inversion ranging. bandwidthHz, snrDbInBand and
            // spreadingFactor are unused here by design: this is not a
            // time-of-arrival estimator, so the ToA Cramer-Rao bound does not
            // apply to it and applying one would be a category error. Its error
            // source is the shadowing standard deviation, and it already varies
            // correctly with range (52 m at 100 m to 2973 m at 6 km in the probe).
            (void)bandwidthHz;
            (void)snrDbInBand;
            (void)spreadingFactor;

            double estD = InvertPropagationLossDb(state.propagationLossDb,
                                                   propParams, frequencyHz);
            estD = std::max(estD, 1.0);

            // Error propagation: shadowing sigma (dB) -> metres via
            // d(log10(d))/d(PL) = 1/(10*n). Conservative beyond breakpoint.
            double n      = propParams.pathLossExponent.mean;
            double sigmaM = estD * std::log(10.0) / (10.0 * n)
                          * state.shadowingStdEff;
            sigmaM = std::max(sigmaM, 0.5); // 0.5 m floor prevents near-zero EKF R

            double err = sampler.Sample(
                DistributionParam::Normal(0.0, sigmaM,
                                          -3.0 * sigmaM, 3.0 * sigmaM));

            state.estimatedRangeM = state.trueRangeM + err;
            state.rangeErrorM     = err;
            state.positionErrorM  = std::abs(err);
            if (state.estimatedRangeM > 0.0 && state.trueRangeM > 0.0) {
                state.propagationLossBiasDb =
                    10.0 * n * std::log10(state.estimatedRangeM
                                          / state.trueRangeM);
            } else {
                state.propagationLossBiasDb = 0.0;
            }
            state.rangingSigmaM = sigmaM;
            return sigmaM;
        }

        case RANGING_TIER_2P4: {
            // LoRa CSS dechirp processing gain. snrDbInBand is pre-despread
            // (rxPowerDbm - noiseFloorDbm over the occupied bandwidth), so the
            // 10*log10(2^SF) matched-filter gain is added here to obtain the E/N0
            // the ToA bound is a function of.
            const double despreadGainDb =
                10.0 * std::log10(std::pow(2.0, static_cast<double>(spreadingFactor)));
            const double floorM = state.isNLOS ? params.tofNlosFloorM
                                               : params.tofLosFloorM;
            const double sigmaM = RangingSigmaM(bandwidthHz,
                                                 snrDbInBand + despreadGainDb,
                                                 floorM,
                                                 true);

            const double bias  = state.isNLOS ? sampler.Sample(params.tofNlosBias)
                                              : 0.0;
            const double noise = sampler.Sample(
                DistributionParam::Normal(0.0, sigmaM,
                                          -3.0 * sigmaM, 3.0 * sigmaM));

            // --tofFieldBiasM: a measured field over-estimate, deterministic, so it moves
            // no draw. Skipped entirely at its default of 0.
            state.rangeErrorM           = (params.tofFieldBiasM != 0.0)
                                              ? bias + noise + params.tofFieldBiasM
                                              : bias + noise;
            state.estimatedRangeM       = state.trueRangeM + state.rangeErrorM;
            state.positionErrorM        = std::abs(state.rangeErrorM);
            state.propagationLossBiasDb = 0.0;
            state.rangingSigmaM         = sigmaM;
            return sigmaM;
        }

        case RANGING_TIER_UWB: {
            // HRP UWB preamble accumulation gain, 10*log10(PSR). Not optional:
            // raw UWB SNR is negative because the thermal noise floor over
            // 499.2 MHz sits well above the sensitivity gate, so without this term
            // the CRLB is metres and contradicts the cited 10-33 cm hardware
            // performance. spreadingFactor is meaningless for HRP UWB -- there is
            // no chirp to despread -- so it is explicitly not used here.
            (void)spreadingFactor;
            const double psr = (params.uwbPreambleSymbols > 0.0)
                                   ? params.uwbPreambleSymbols
                                   : 1.0;
            const double accumGainDb = 10.0 * std::log10(psr);
            const double floorM = state.isNLOS ? params.uwbNlosFloorM
                                               : params.uwbLosFloorM;
            const double sigmaM = RangingSigmaM(bandwidthHz,
                                                 snrDbInBand + accumGainDb,
                                                 floorM,
                                                 true);

            const double bias  = state.isNLOS ? sampler.Sample(params.uwbNlosBias)
                                              : 0.0;
            const double noise = sampler.Sample(
                DistributionParam::Normal(0.0, sigmaM,
                                          -3.0 * sigmaM, 3.0 * sigmaM));

            state.rangeErrorM           = bias + noise;
            state.estimatedRangeM       = state.trueRangeM + state.rangeErrorM;
            state.positionErrorM        = std::abs(state.rangeErrorM);
            state.propagationLossBiasDb = 0.0;
            state.rangingSigmaM         = sigmaM;
            return sigmaM;
        }

    } // switch

    return 0.0;
}

} // namespace ns3
