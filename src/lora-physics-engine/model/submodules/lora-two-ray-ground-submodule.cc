#include "lora-two-ray-ground-submodule.h"
#include <cmath>

namespace ns3 {

TwoRayGroundSubmodule::TwoRayGroundSubmodule(const TwoRayGroundParams& params, EnvironmentClass env)
    : m_params(params), m_env(env) {
}

void TwoRayGroundSubmodule::Compute(LinkPhysicsState& state, double distanceM, double txHeightM, double rxHeightM, double frequencyHz) {
    if (!m_params.enabled || m_env != ENV_RURAL_OPEN) {
        state.isTwoRay = false;
        return;
    }

    // Only applies if LOS
    if (state.isNLOS) {
        state.isTwoRay = false;
        return;
    }

    state.isTwoRay = true;
    double lambda = 299792458.0 / frequencyHz;
    
    // Cross-over distance (breakpoint)
    double dc = (4.0 * M_PI * txHeightM * rxHeightM) / lambda;
    
    double lossDb = 0.0;
    if (distanceM <= dc) {
        // Free space path loss
        lossDb = 20.0 * std::log10(4.0 * M_PI * distanceM / lambda);
    } else {
        // Two-ray ground reflection loss
        lossDb = 40.0 * std::log10(distanceM) - 20.0 * std::log10(txHeightM) - 20.0 * std::log10(rxHeightM);
    }
    
    // We overwrite the propagation loss entirely for true open field two-ray
    state.propagationLossDb = lossDb;
}

} // namespace ns3
