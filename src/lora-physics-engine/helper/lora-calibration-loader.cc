#include "lora-calibration-loader.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <iomanip>

namespace ns3 {

LoRaCalibrationLoader::LoRaCalibrationLoader() {}

bool LoRaCalibrationLoader::LoadCSV(const std::string& csvPath) {
    m_records.clear();
    std::ifstream file(csvPath);
    if (!file.is_open()) {
        std::cerr << "LoRaCalibrationLoader: Failed to open dataset CSV at " << csvPath << std::endl;
        return false;
    }
    
    std::string line;
    bool isHeader = true;
    while (std::getline(file, line)) {
        if (isHeader) {
            isHeader = false;
            continue;
        }
        if (line.empty()) continue;
        
        std::stringstream ss(line);
        std::string cell;
        CalibrationRecord rec;
        
        try {
            std::getline(ss, cell, ','); rec.timestampS = std::stod(cell);
            std::getline(ss, cell, ','); rec.gpsLat = std::stod(cell);
            std::getline(ss, cell, ','); rec.gpsLon = std::stod(cell);
            std::getline(ss, cell, ','); rec.gatewayId = std::stoul(cell);
            std::getline(ss, cell, ','); rec.rssiDbm = std::stod(cell);
            std::getline(ss, cell, ','); rec.snrDb = std::stod(cell);
            std::getline(ss, cell, ','); rec.sf = (uint8_t)std::stoul(cell);
            std::getline(ss, cell, ','); rec.frequencyHz = std::stod(cell);
            std::getline(ss, cell, ','); rec.distanceM = std::stod(cell);
            m_records.push_back(rec);
        } catch (const std::exception& e) {
            continue; // Ignore malformed rows
        }
    }
    file.close();
    return !m_records.empty();
}

CalibrationSummary LoRaCalibrationLoader::RunCalibrationFit(
    Ptr<LoRaPhysicsEngine> engine,
    const LoRaPhysicsConfig& baseConfig,
    const std::string& outputCsvPath)
{
    CalibrationSummary bestSummary;
    bestSummary.rmseRssi = 999.0;
    bestSummary.rmseSnr = 999.0;
    
    if (m_records.empty()) {
        std::cerr << "LoRaCalibrationLoader: No calibration records loaded to evaluate." << std::endl;
        return bestSummary;
    }
    
    // Parameter grid search across shadowing, fading, foliage, and NLOS scale
    std::vector<double> shadOptions = {4.0, 8.0, 12.0};
    std::vector<double> fadeOptions = {0.2, 0.5, 0.8};
    std::vector<double> folOptions = {5.0, 15.0, 25.0};
    std::vector<double> nlosOptions = {50.0, 200.0, 500.0};
    
    for (double shad : shadOptions) {
        for (double fade : fadeOptions) {
            for (double fol : folOptions) {
                for (double nlos : nlosOptions) {
                    LoRaPhysicsConfig c = baseConfig;
                    c.shadowing.shadowingStddevDb = DistributionParam::Fixed(shad);
                    c.foliage.maxAttenuationDb = DistributionParam::Fixed(fol);
                    c.nlos.nlosScaleDistanceM = DistributionParam::Fixed(nlos);
                    engine->Initialize(c);
                    
                    double sumSqRssi = 0.0;
                    double sumSqSnr = 0.0;
                    double sumBiasRssi = 0.0;
                    double sumBiasSnr = 0.0;
                    double sumPlErr = 0.0;
                    
                    uint32_t fakeNodeId = 100;
                    
                    for (const auto& rec : m_records) {
                        Vector txPos(0.0, 0.0, 1.0);
                        Vector rxPos(rec.distanceM, 0.0, 30.0);
                        
                        PhysicsResult pRes = engine->ProcessTransmission(
                            fakeNodeId++, rec.gatewayId, txPos, rxPos,
                            14.0, rec.sf, 125000.0, 0,
                            Time::FromDouble(rec.timestampS, Time::S));
                            
                        double diffRssi = pRes.measuredRssiDbm - rec.rssiDbm;
                        double diffSnr = pRes.finalSNRDb - rec.snrDb;
                        
                        sumSqRssi += diffRssi * diffRssi;
                        sumSqSnr += diffSnr * diffSnr;
                        sumBiasRssi += diffRssi;
                        sumBiasSnr += diffSnr;
                        sumPlErr += std::abs(pRes.pathLossDb - (14.0 - rec.rssiDbm));
                    }
                    
                    double n = (double)m_records.size();
                    double curRmseRssi = std::sqrt(sumSqRssi / n);
                    double curRmseSnr = std::sqrt(sumSqSnr / n);
                    
                    if ((curRmseRssi + curRmseSnr) < (bestSummary.rmseRssi + bestSummary.rmseSnr)) {
                        bestSummary.rmseRssi = curRmseRssi;
                        bestSummary.rmseSnr = curRmseSnr;
                        bestSummary.biasRssi = sumBiasRssi / n;
                        bestSummary.biasSnr = sumBiasSnr / n;
                        bestSummary.pathlossFitErrorDb = sumPlErr / n;
                        bestSummary.optimizedShadowingStddevDb = shad;
                        bestSummary.optimizedFadingSeverity = fade;
                        bestSummary.optimizedFoliageAttenuationDb = fol;
                        bestSummary.optimizedNlosScaleM = nlos;
                    }
                }
            }
        }
    }
    
    // Write out calibration_metrics.csv
    if (!outputCsvPath.empty()) {
        std::ofstream file(outputCsvPath);
        if (file.is_open()) {
            file << "metric,value\n";
            file << "rmse_rssi," << std::fixed << std::setprecision(4) << bestSummary.rmseRssi << "\n";
            file << "rmse_snr," << bestSummary.rmseSnr << "\n";
            file << "bias_rssi," << bestSummary.biasRssi << "\n";
            file << "bias_snr," << bestSummary.biasSnr << "\n";
            file << "pathloss_fit_error," << bestSummary.pathlossFitErrorDb << "\n";
            file << "optimized_shadowing_stddev_db," << bestSummary.optimizedShadowingStddevDb << "\n";
            file << "optimized_fading_severity," << bestSummary.optimizedFadingSeverity << "\n";
            file << "optimized_foliage_attenuation_db," << bestSummary.optimizedFoliageAttenuationDb << "\n";
            file << "optimized_nlos_scale_m," << bestSummary.optimizedNlosScaleM << "\n";
            file.close();
        }
    }
    
    return bestSummary;
}

CalibrationSummary LoRaCalibrationLoader::LoadSDRTraceAndFit(
    Ptr<LoRaPhysicsEngine> engine,
    const LoRaPhysicsConfig& baseConfig,
    const std::string& sdrTraceCsvPath)
{
    if (!LoadCSV(sdrTraceCsvPath)) {
        std::cerr << "LoRaCalibrationLoader: Failed to load SDR trace CSV at " << sdrTraceCsvPath << std::endl;
        CalibrationSummary emptySummary;
        emptySummary.rmseRssi = 999.0;
        emptySummary.rmseSnr = 999.0;
        return emptySummary;
    }
    
    // Ensure SDR realism mode is active during calibration fit
    LoRaPhysicsConfig sdrConfig = baseConfig;
    sdrConfig.fidelityMode = FIDELITY_SDR_GRADE_REALISM;
    sdrConfig.iqImpairment.enabled = true;
    
    return RunCalibrationFit(engine, sdrConfig, sdrTraceCsvPath + "_optimized_fit.csv");
}

} // namespace ns3
