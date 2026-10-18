#include "lora-nlos-submodule.h"
#include <cmath>
#include <algorithm>

namespace ns3 {

/**
 * @brief Normal(μ, σ) NLOS Loss Model per 3GPP TR 38.901; values OURS (the cited 'Hofer 2015' was not found)
 *
 * NLOS attenuation is drawn from the per-environment Normal distribution
 * already configured in NLOSParams::DefaultsFor() (lora-physics-parameters.cc).
 * A hard 3σ cap is enforced to prevent over-long Poisson compound tails.
 *
 * Reference spec caps:
 *   ENV_URBAN_DENSE:    Normal(28, 14) → 3σ cap = 70 dB
 *   ENV_URBAN_MODERATE: Normal(25, 12) → 3σ cap = 61 dB
 *   ENV_SUBURBAN:       Normal(19,  9) → 3σ cap = 46 dB
 *
 * NLOS probability still uses the exponential model: P_nlos = 1 - exp(-d/scale)
 */

double ComputeNLOSPenaltyDb(
    Vector txPos, Vector rxPos,
    const NLOSParams& params, LoRaDistributionSampler& sampler)
{
    if (!params.enabled) {
        return 0.0;
    }

    // 1. Compute 3D distance
    double dx = txPos.x - rxPos.x;
    double dy = txPos.y - rxPos.y;
    double dz = txPos.z - rxPos.z;
    double d = std::sqrt(dx*dx + dy*dy + dz*dz);
    if (d < 1.0) {
        d = 1.0;
    }

    // V2a-2 --nlos3gppRMa: 3GPP TR 38.901 v17.0.0, RMa, for links with a >= 10 m end (see NLOSParams::rma3gpp).
    {
        const double hBS = std::max(txPos.z, rxPos.z);
        const double hUT = std::min(10.0, std::max(1.0, std::min(txPos.z, rxPos.z)));
        if (params.rma3gpp && hBS >= 10.0) {
            const double d2D = std::max(1.0, std::sqrt(dx * dx + dy * dy));
            const double prLos = (d2D <= 10.0) ? 1.0 : std::exp(-(d2D - 10.0) / 1000.0); // Table 7.4.2-1
            const double u3 = sampler.Sample(DistributionParam::Uniform(0.0, 1.0));
            if (u3 >= 1.0 - prLos) {
                return 0.0; // LOS
            }
            const double fc = params.frequencyHz / 1e9; // GHz (note 6)
            const double h = 5.0, W = 20.0;             // RMa defaults, Table 7.4.1-1
            const double dBP = 2.0 * M_PI * hBS * hUT * params.frequencyHz / 3.0e8; // note 5
            auto pl1 = [&](double dd) {
                return 20.0 * std::log10(40.0 * M_PI * dd * fc / 3.0) +
                       std::min(0.03 * std::pow(h, 1.72), 10.0) * std::log10(dd) -
                       std::min(0.044 * std::pow(h, 1.72), 14.77) + 0.002 * std::log10(h) * dd;
            };
            const double d3 = std::max(d, 1.0);
            const double dBP3 = std::sqrt(dBP * dBP + (hBS - hUT) * (hBS - hUT));
            const double plLos = (d2D <= dBP) ? pl1(d3) : pl1(dBP3) + 40.0 * std::log10(d3 / dBP3);
            const double plN = 161.04 - 7.1 * std::log10(W) + 7.5 * std::log10(h) -
                               (24.37 - 3.7 * (h / hBS) * (h / hBS)) * std::log10(hBS) +
                               (43.42 - 3.1 * std::log10(hBS)) * (std::log10(d3) - 3.0) + 20.0 * std::log10(fc) -
                               (3.2 * std::pow(std::log10(11.75 * hUT), 2.0) - 4.97);
            return std::max(plLos, plN) - plLos; // excess over LOS, >= 0
        }
    }

    // 2. Compute NLOS probability
    double P_nlos = 0.0;
    if (params.envClass == ENV_URBAN_DENSE || params.envClass == ENV_URBAN_MODERATE) {
        // 3GPP TR 38.901 UMa LOS probability
        double p_los = std::min(18.0 / d, 1.0) * (1.0 - std::exp(-d / 63.0)) + std::exp(-d / 63.0);
        P_nlos = 1.0 - p_los;
    } else {
        double scale = params.nlosScaleDistanceM.mean;
        if (scale <= 0.0) scale = 1.0;
        P_nlos = 1.0 - std::exp(-d / scale);
    }

    // 3. Sample uniform(0, 1) to decide LOS/NLOS
    double u = sampler.Sample(DistributionParam::Uniform(0.0, 1.0));

    // 4. If NLOS: draw loss from Normal(mu, sigma) with hard 3σ cap
    // This replaces the Poisson compound accumulation which had unbounded tails.
    // Spec reference: 3GPP TR 38.901. The cited 'Hofer 2015 IEEE VTC' was not found: values OURS.
    if (u < P_nlos) {
        if (params.envClass == ENV_URBAN_DENSE || params.envClass == ENV_URBAN_MODERATE) {
            double f_GHz = params.frequencyHz / 1e9;
            double d_break = 1000.0;
            double pl_los_d = 0.0;
            double pl_nlos_d = 0.0;
            if (d <= d_break) {
                pl_los_d = 28.0 + 22.0 * std::log10(d) + 20.0 * std::log10(f_GHz);
                double pl_nlos_formula = 13.54 + 39.08 * std::log10(d) + 20.0 * std::log10(f_GHz); // h_UT assumed 1.5m
                pl_nlos_d = std::max(pl_los_d, pl_nlos_formula);
            } else {
                double pl_los_break = 28.0 + 22.0 * std::log10(d_break) + 20.0 * std::log10(f_GHz);
                double pl_nlos_formula_break = 13.54 + 39.08 * std::log10(d_break) + 20.0 * std::log10(f_GHz);
                double pl_nlos_break = std::max(pl_los_break, pl_nlos_formula_break);
                
                pl_los_d = pl_los_break + 10.0 * 2.0 * std::log10(d / d_break);
                pl_nlos_d = pl_nlos_break + 10.0 * 1.58 * std::log10(d / d_break);
            }
            double nlos_det_delta = pl_nlos_d - pl_los_d;
            double sigma_nlos = 6.0;
            double stochastic = sampler.Sample(DistributionParam::Normal(0.0, sigma_nlos, -4.0 * sigma_nlos, 4.0 * sigma_nlos));
            
            const double spec_mu = (params.envClass == ENV_URBAN_DENSE) ? 28.0 : 25.0;
            const double cap = spec_mu + 3.0 * sigma_nlos;
            
            double total_nlos = std::min(nlos_det_delta + stochastic, cap);
            return std::max(total_nlos, 0.0);
        } else {
            double nlosAtten = sampler.Sample(params.nlosAttenuationDb);
            double diffraction = sampler.Sample(params.diffractionLossDb);

            // Hard 3σ cap based on the original combined spec parameters (mu + 3*sigma)
            double combined_mu = params.nlosAttenuationDb.mean + params.diffractionLossDb.mean;
            double cap = combined_mu + 3.0 * params.nlosAttenuationDb.stddev;
            double totalNLOS = std::min(nlosAtten + diffraction, cap);

            return std::max(0.0, totalNLOS);
        }
    }

    return 0.0; // Link is LOS, zero penalty
}

} // namespace ns3
