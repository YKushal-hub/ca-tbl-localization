#ifndef LORA_SDR_VALIDATOR_H
#define LORA_SDR_VALIDATOR_H

#include <string>
#include <vector>

namespace ns3 {

/**
 * @brief Telemetry metrics from SDR trace validation pipeline
 */
struct SdrValidationMetrics {
    double klDivergenceCfo;
    double rmseDelaySpread;
    double timingDriftError;
    double symbolTimingVariance;
    double evmRmse;
    bool validationPassed;
};

/**
 * @brief Validates simulation outputs against empirical SDR traces (GNU Radio, Semtech SX1276 logs, TTN traces)
 * calculating KL divergence, RMSE, and logging results to CSV.
 */
class LoRaSdrValidator {
public:
    LoRaSdrValidator();
    
    SdrValidationMetrics ValidateAgainstSdrTrace(const std::string& tracePath);
    void WriteValidationMetricsCsv(const std::string& outputPath, const SdrValidationMetrics& metrics);
};

} // namespace ns3

#endif // LORA_SDR_VALIDATOR_H
