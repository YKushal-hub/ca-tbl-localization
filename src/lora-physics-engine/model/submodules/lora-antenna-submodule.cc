#include "lora-antenna-submodule.h"
#include <algorithm>

namespace ns3 {

AntennaSubmodule::AntennaSubmodule() {}

double AntennaSubmodule::ComputeAntennaGainDb(
    const AntennaParams& params,
    Vector txPosition, Vector rxGatewayPosition,
    Vector gatewayOrientation)
{
    if (!params.directional) {
        return params.peakGainDbi - params.polarizationMismatchLossDb;
    }
    
    // Vector from gateway to end-device node
    double vx = txPosition.x - rxGatewayPosition.x;
    double vy = txPosition.y - rxGatewayPosition.y;
    double vz = txPosition.z - rxGatewayPosition.z;
    double dHoriz = std::sqrt(vx*vx + vy*vy);
    if (dHoriz < 0.1) dHoriz = 0.1;
    
    // Gateway orientation azimuth angle (in horizontal plane)
    double ox = gatewayOrientation.x;
    double oy = gatewayOrientation.y;
    double oNorm = std::sqrt(ox*ox + oy*oy);
    if (oNorm < 0.1) { ox = 1.0; oy = 0.0; oNorm = 1.0; }
    
    // Node direction azimuth angle
    double nodeAzimuth = std::atan2(vy, vx);
    double gwAzimuth = std::atan2(oy, ox);
    
    double thetaRad = nodeAzimuth - gwAzimuth;
    // Normalize angle difference to [-pi, pi]
    while (thetaRad > M_PI) thetaRad -= 2.0 * M_PI;
    while (thetaRad < -M_PI) thetaRad += 2.0 * M_PI;
    
    double thetaDeg = thetaRad * 180.0 / M_PI;
    
    // WARNING: The 3GPP directional sectoral antenna model (designed for cellular base stations)
    // is non-standard for LoRa gateways and should not be enabled by default in standard scenarios.
    // Standard LoRa systems rely on omnidirectional antenna patterns.
    
    // 3GPP Azimuth attenuation model
    double beamwidth = (params.beamwidthDeg > 0.0 ? params.beamwidthDeg : 65.0);
    double aAzimuth = -12.0 * std::pow(thetaDeg / beamwidth, 2.0);
    aAzimuth = std::max(aAzimuth, -params.frontToBackRatioDb);
    
    // Node elevation angle
    double elevationRad = std::atan2(vz, dHoriz);
    double downtiltRad = params.downtiltDeg * M_PI / 180.0;
    
    double phiRad = elevationRad - (-downtiltRad); // negative elevation = downward
    double phiDeg = phiRad * 180.0 / M_PI;
    
    // 3GPP Elevation attenuation model
    double aElevation = -12.0 * std::pow(phiDeg / beamwidth, 2.0);
    aElevation = std::max(aElevation, -params.frontToBackRatioDb);
    
    double compositeAttenuation = aAzimuth + aElevation;
    compositeAttenuation = std::max(compositeAttenuation, -params.frontToBackRatioDb);
    
    double netGain = params.peakGainDbi + compositeAttenuation - params.polarizationMismatchLossDb;
    return netGain;
}

} // namespace ns3
