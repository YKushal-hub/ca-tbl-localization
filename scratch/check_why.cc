#include "ns3/core-module.h"
#include "ns3/lora-physics-engine.h"
#include "ns3/lora-physics-parameters.h"
using namespace ns3;

int main() {
    LogComponentEnable("LoRaPhysicsEngine", LOG_LEVEL_ALL);
    LoRaPhysicsConfig config = LoRaPhysicsConfig::ForScenario(ENV_RURAL_OPEN, WEATHER_CLEAR_DRY, FADING_NONE, VEG_NONE, MOB_STATIC, false);
    config.terrain.enabled = false;
    config.propagation.pathLossExponent = DistributionParam::Fixed(2.08);
    config.nlos.nlosAttenuationDb = DistributionParam::Normal(35.0, 0.0, 35.0, 35.0);
    config.nlos.nlosScaleDistanceM = DistributionParam::Fixed(1.0);
    config.shadowing.shadowingStddevDb = DistributionParam::Fixed(10.0);
    config.verboseLogging = true;
    
    LoRaPhysicsEngine engine;
    engine.Initialize(config);
    
    Vector txPos(0, 0, 24.0);
    Vector rxPos(300.0, 0, 1.5);
    engine.ProcessTransmission(1, 1, txPos, rxPos, 14.0, 7, 125000.0, 0, Seconds(0));
    return 0;
}
