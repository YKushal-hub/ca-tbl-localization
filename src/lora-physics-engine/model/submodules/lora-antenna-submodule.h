#ifndef LORA_ANTENNA_SUBMODULE_H
#define LORA_ANTENNA_SUBMODULE_H

#include "ns3/vector.h"
#include "../../model/lora-physics-parameters.h"
#include <cmath>

namespace ns3 {

/**
 * @brief Submodule for modeling physical antenna directivity patterns,
 * including 3GPP directional sector azimuth and elevation attenuation.
 */
class AntennaSubmodule {
public:
    AntennaSubmodule();
    
    double ComputeAntennaGainDb(
        const AntennaParams& params,
        Vector txPosition, Vector rxGatewayPosition,
        Vector gatewayOrientation); // Azimuth pointing vector
};

} // namespace ns3

#endif // LORA_ANTENNA_SUBMODULE_H
