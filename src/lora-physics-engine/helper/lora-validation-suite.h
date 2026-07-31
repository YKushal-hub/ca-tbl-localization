#ifndef LORA_VALIDATION_SUITE_H
#define LORA_VALIDATION_SUITE_H

#include "ns3/object.h"
#include "../model/lora-physics-parameters.h"
#include <string>
#include <vector>

namespace ns3 {

/**
 * @brief Output metrics from comprehensive SDR validation framework
 */
struct SdrValidationMetricsSummary {
    double rmseCfoHz;
    double klDivergenceEvm;
    double jensenShannonDivergenceTiming;
    double temporalCorrelationError;
    double spatialCorrelationError;
    double psdSimilarityScore;
    double symbolErrorDistributionMismatch;
    bool passedValidationThreshold;
};

/**
 * @brief Empirical SDR trace observation entry
 */
struct EmpiricalSdrTraceRecord {
    double cfoHz;
    double timingDriftNs;
    double evmPercent;
    double delaySpreadNs;
    double dopplerHz;
    double symbolReliability;
};

/**
 * @brief Comprehensive SDR validation suite computing KL divergence and RMSE against real-world SDR datasets
 */
class LoRaValidationSuite : public Object {
public:
    static TypeId GetTypeId();
    LoRaValidationSuite();
    virtual ~LoRaValidationSuite() override = default;

    void Initialize(const AdvancedValidationConfigParams& params);

    /**
     * @brief Load empirical SDR dataset from CSV/trace logs
     */
    bool LoadEmpiricalDataset(const std::string& path);

    /**
     * @brief Register simulated telemetry observation for comparative benchmarking
     */
    void RegisterSimulatedObservation(double cfoHz, double timingDriftNs, double evmPercent, 
                                      double delaySpreadNs, double dopplerHz, double symbolReliability);

    /**
     * @brief Compute comprehensive SDR validation metrics and write to advanced_validation_metrics.csv
     */
    SdrValidationMetricsSummary EvaluateValidationMetrics(const std::string& outputPath);

private:
    AdvancedValidationConfigParams m_params;
    std::vector<EmpiricalSdrTraceRecord> m_empiricalRecords;
    std::vector<EmpiricalSdrTraceRecord> m_simulatedRecords;
};

} // namespace ns3

#endif // LORA_VALIDATION_SUITE_H
