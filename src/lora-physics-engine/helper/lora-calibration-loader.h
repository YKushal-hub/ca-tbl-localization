#ifndef LORA_CALIBRATION_LOADER_H
#define LORA_CALIBRATION_LOADER_H

#include "ns3/ptr.h"
#include "../model/lora-physics-engine.h"
#include <string>
#include <vector>

namespace ns3 {

/**
 * @brief Empirical field measurement record loaded from an external trace CSV
 */
struct CalibrationRecord {
    double timestampS;
    double gpsLat;
    double gpsLon;
    uint32_t gatewayId;
    double rssiDbm;
    double snrDb;
    uint8_t sf;
    double frequencyHz;
    double distanceM;
};

/**
 * @brief Output container for empirical calibration fit metrics
 */
struct CalibrationSummary {
    double rmseRssi;
    double rmseSnr;
    double biasRssi;
    double biasSnr;
    double pathlossFitErrorDb;
    
    double optimizedShadowingStddevDb;
    double optimizedFadingSeverity;
    double optimizedFoliageAttenuationDb;
    double optimizedNlosScaleM;
};

/**
 * @brief Helper class to load real-world LoRa traces, evaluate simulation accuracy against field telemetry,
 * and auto-adjust latent parameters (shadowing, fading, foliage, NLOS) to minimize empirical RMSE.
 */
class LoRaCalibrationLoader {
public:
    LoRaCalibrationLoader();
    
    bool LoadCSV(const std::string& csvPath);
    
    CalibrationSummary RunCalibrationFit(
        Ptr<LoRaPhysicsEngine> engine,
        const LoRaPhysicsConfig& baseConfig,
        const std::string& outputCsvPath);
        
    CalibrationSummary LoadSDRTraceAndFit(
        Ptr<LoRaPhysicsEngine> engine,
        const LoRaPhysicsConfig& baseConfig,
        const std::string& sdrTraceCsvPath);

private:
    std::vector<CalibrationRecord> m_records;
};

} // namespace ns3

#endif // LORA_CALIBRATION_LOADER_H
