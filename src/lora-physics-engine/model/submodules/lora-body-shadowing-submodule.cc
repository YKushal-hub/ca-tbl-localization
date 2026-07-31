#include "lora-body-shadowing-submodule.h"

namespace ns3 {

BodyShadowingSubmodule::BodyShadowingSubmodule(const BodyShadowingParams& params, EnvironmentClass env, LoRaDistributionSampler& sampler)
    : m_params(params), m_env(env), m_sampler(sampler) {
}

void BodyShadowingSubmodule::Compute(LinkPhysicsState& state) {
    if (!m_params.enabled) {
        state.bodyShadowingDb = 0.0;
        return;
    }

    double dx = state.rxPos.x - state.txPos.x;
    double dy = state.rxPos.y - state.txPos.y;
    double angleToGateway = std::atan2(dy, dx);
    if (angleToGateway < 0) angleToGateway += 2 * M_PI;

    double relAngle = std::abs(angleToGateway - state.nodeOrientationRadians);
    if (relAngle > M_PI) relAngle = 2 * M_PI - relAngle;

    if (!m_params.nodeIsBodyWorn) {
        state.bodyShadowingDb = 0.0;
        return;
    }

    const double MU_BODY = 6.0;  // dB — produces mean=3.0, max=6.0
    double bodyLoss = MU_BODY * (1.0 - std::cos(relAngle)) / 2.0;
    // Range: [0, 6] dB. Mean over Uniform(0,2π) = MU_BODY/2 = 3.0 dB ✓
    state.bodyShadowingDb = bodyLoss;
}

} // namespace ns3
