#include "lora-propagation-submodule.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace ns3 {

double ComputePathLossDb(
    Vector txPosition, Vector rxPosition,
    double txHeightM, double rxHeightM,
    const PropagationParams& params,
    LoRaDistributionSampler& sampler)
{
    // 1. Compute 3D distance
    double dx = txPosition.x - rxPosition.x;
    double dy = txPosition.y - rxPosition.y;
    double dz = txPosition.z - rxPosition.z;
    double d = std::sqrt(dx*dx + dy*dy + dz*dz);
    if (d < 1.0) d = 1.0;

    double c = 299792458.0; 
    double f = params.frequencyHz;
    double f_MHz = f / 1e6;
    double lambda = c / f;

    double d0 = params.referenceDistanceM;
    if (d0 < 1.0) d0 = 1.0;
    
    double FSPL_d0 = 20.0 * std::log10(4.0 * M_PI * d0 / lambda);
    
    double n1 = sampler.Sample(params.pathLossExponent);
    double PL_total = 0.0;
    
    if (params.envClass == ENV_URBAN_DENSE || params.envClass == ENV_URBAN_MODERATE) {
        double f_GHz = params.frequencyHz / 1e9;
        double d_break = 1000.0;
        if (d <= d_break) {
            PL_total = 28.0 + 22.0 * std::log10(d) + 20.0 * std::log10(f_GHz) - 6.5;
        } else {
            double pl_los_break = 28.0 + 22.0 * std::log10(d_break) + 20.0 * std::log10(f_GHz);
            PL_total = pl_los_break + 10.0 * 2.0 * std::log10(d / d_break) - 6.5;
        }
    } else if (params.enableDualSlope && d > params.dualSlopeBreakpointM) {
        double n2 = sampler.Sample(params.pathLossExponent2);
        double d_break = params.dualSlopeBreakpointM;
        double pl_at_break = FSPL_d0 + 10.0 * n1 * std::log10(d_break / d0);
        PL_total = pl_at_break + 10.0 * n2 * std::log10(d / d_break);
    } else {
        PL_total = FSPL_d0 + 10.0 * n1 * std::log10(d / d0);
    }
    
    return PL_total;
}

} // namespace ns3
