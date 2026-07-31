#ifndef LORA_TWO_RAY_GROUND_SUBMODULE_H
#define LORA_TWO_RAY_GROUND_SUBMODULE_H

#include "ns3/lora-physics-parameters.h"
#include "ns3/lora-distribution-sampler.h"
#include "ns3/core-module.h"

namespace ns3 {

class TwoRayGroundSubmodule {
public:
    TwoRayGroundSubmodule(const TwoRayGroundParams& params, EnvironmentClass env);
    void Compute(LinkPhysicsState& state, double distanceM, double txHeightM, double rxHeightM, double frequencyHz);

private:
    TwoRayGroundParams m_params;
    EnvironmentClass m_env;
};

} // namespace ns3

#endif // LORA_TWO_RAY_GROUND_SUBMODULE_H
