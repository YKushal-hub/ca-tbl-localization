#ifndef LORA_PHYSICS_ENGINE_HELPER_H
#define LORA_PHYSICS_ENGINE_HELPER_H

#include "ns3/ptr.h"
#include "ns3/vector.h"
#include "ns3/nstime.h"
#include "../model/lora-physics-engine.h"
#include <string>
#include <fstream>

namespace ns3 {

/**
 * @brief Helper class to instantiate, configure, and intercept transmissions in ns-3
 * Provides convenient results logging to a detailed CSV telemetry file
 */
class LoRaPhysicsEngineHelper {
public:
    LoRaPhysicsEngineHelper();
    ~LoRaPhysicsEngineHelper();
    
    // Initialize the engine with a configuration block
    void Initialize(const LoRaPhysicsConfig& config);
    
    // Initialize the engine with a YAML config file path
    void Initialize(const std::string& yamlPath);
    
    // Fetch the underlying physics engine pointer
    Ptr<LoRaPhysicsEngine> GetEngine() const;
    
    // Interface to process transmission and auto-log results
    PhysicsResult Process(
        uint32_t txNodeId, uint32_t rxGatewayId,
        Vector txPosition, Vector rxPosition,
        double txPowerDbm, uint8_t spreadingFactor,
        double bandwidthHz, uint8_t relayHops,
        Time currentTime);
    
    // Enable detailed transmission telemetry logging to CSV
    void EnableResultLogging(const std::string& csvPath);
    
    // Flush and close the telemetry log file
    void CloseResultLogging();

private:
    Ptr<LoRaPhysicsEngine> m_engine;
    std::string m_csvPath;
    std::ofstream m_csvFile;
    bool m_loggingEnabled;
};

} // namespace ns3

#endif // LORA_PHYSICS_ENGINE_HELPER_H
