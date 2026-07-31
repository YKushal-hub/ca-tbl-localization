#ifndef LORA_RANGING_SUBMODULE_H
#define LORA_RANGING_SUBMODULE_H

// RangingParams is defined in lora-physics-parameters.h (same location as
// all other *Params structs). It is not redefined here to avoid a circular
// include: this header includes lora-physics-parameters.h, so defining
// RangingParams here would make it unavailable when lora-physics-parameters.h
// is included first by other translation units.
#include "../lora-physics-parameters.h"
#include "../lora-distribution-sampler.h"

namespace ns3 {

/**
 * @brief CA-TBL three-tier ranging model selector.
 *
 * TIER_868  - RSSI-inversion ranging. Error derives from the shadowing sigma via
 *             the algebraic inverse of ComputePathLossDb(). NOT a time-of-arrival
 *             estimator, so it is NOT bounded by the ToA Cramer-Rao bound and
 *             deliberately does not get a CRLB term.
 * TIER_2P4  - Symmetric two-way ToF. CRLB-derived from occupied bandwidth and
 *             post-despread SNR, floored at the SX1280 hardware limit.
 * TIER_UWB  - Symmetric two-way ToF. CRLB-derived from occupied bandwidth and
 *             post-accumulation SNR, floored at the DW3000 hardware limit.
 */
enum RangingTier {
    RANGING_TIER_868,
    RANGING_TIER_2P4,
    RANGING_TIER_UWB
};

/**
 * @brief Compute physics-derived ranging error (RANGING_PHYSICS_DERIVED mode).
 *
 * Writes state.estimatedRangeM, state.rangeErrorM, state.positionErrorM,
 * state.propagationLossBiasDb -- same four fields Sub.21 writes, so the
 * orchestrator epilogue and CSV writer need no changes in either mode.
 *
 * Returns ranging sigma in metres for the EKF R-matrix diagonal.
 *
 * CALL ORDERING. The two tiers are NOT called at the same point in the pipeline,
 * and that asymmetry is forced, not stylistic:
 *
 *   TIER_868 must be called between Step 4 (Fading) and Step 5 (Delay Spread).
 *     It reads state.propagationLossDb, state.shadowingStdEff, state.isNLOS and
 *     state.trueRangeM, all of which are stale before Step 4; and it WRITES
 *     state.propagationLossBiasDb, which Step 14 adds into additional_loss. It
 *     therefore cannot be moved after the link budget, which is also why it is
 *     passed snrDbInBand = 0.0 -- the link budget has not run yet. The 868 branch
 *     ignores bandwidthHz, snrDbInBand and spreadingFactor entirely.
 *
 *   TIER_2P4 and TIER_UWB must be called at Step 15, after internalSnrDb is
 *     final (i.e. after the IQ-impairment degradation is applied), because their
 *     sigma is a function of that SNR. They set state.propagationLossBiasDb to
 *     zero, which is a no-op at that point in the pipeline -- see the note in the
 *     engine call sites about why the zeroing must ALSO happen at Step 4.5.
 *
 * propParams and frequencyHz are passed explicitly so the TIER_868 inversion
 * cannot drift from what ComputePathLossDb() computed for this packet.
 *
 * @param bandwidthHz     Occupied bandwidth of the ranging waveform. Sets
 *                        beta_rms = bandwidthHz/sqrt(12). Ignored by TIER_868.
 * @param snrDbInBand     Receiver SNR in the occupied bandwidth, PRE-integration
 *                        (rxPowerDbm - noiseFloorDbm). The integration gain is
 *                        applied inside this function, not by the caller.
 *                        Ignored by TIER_868.
 * @param spreadingFactor Used only by TIER_2P4, for the CSS dechirp gain
 *                        10*log10(2^SF). Ignored by TIER_868 and TIER_UWB.
 */
double ComputeRangingErrorM(LinkPhysicsState& state,
                             RangingTier tier,
                             const RangingParams& params,
                             const PropagationParams& propParams,
                             double frequencyHz,
                             double bandwidthHz,
                             double snrDbInBand,
                             uint8_t spreadingFactor,
                             LoRaDistributionSampler& sampler);

} // namespace ns3

#endif // LORA_RANGING_SUBMODULE_H
