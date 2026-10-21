#include "lora-interference-submodule.h"
#include <cmath>
#include <algorithm>
#include <random>

namespace ns3 {

InterferenceSubmodule::InterferenceSubmodule()
    : m_interfererMapInitialized(false),
      m_wasCollision(false),
      m_wasCaptured(false),
      m_strongestInterfererPowerDbm(-200.0),
      m_strongestInterfererSF(7),
      m_overlapProbability(0.0),
      m_collisionOverlapFraction(0.0),
      m_interfererCount(0),
      m_captureMarginDb(0.0),
      m_sicRecovered(false) {}

void InterferenceSubmodule::InitializeInterfererMap(
    Vector centerPosition, double areaRadiusM,
    const InterferenceParams& params,
    LoRaDistributionSampler& sampler)
{
    m_interferers.clear();
    double density = sampler.Sample(params.externalInterfererDensityPerKm2);
    double areaKm2 = M_PI * (areaRadiusM / 1000.0) * (areaRadiusM / 1000.0);
    int nInterferers = (int)std::round(density * areaKm2);
    if (nInterferers < 0) nInterferers = 0;

    for (int i = 0; i < nInterferers; ++i) {
        double r = areaRadiusM * std::sqrt(sampler.Sample(DistributionParam::Uniform(0.0, 1.0)));
        double theta = sampler.Sample(DistributionParam::Uniform(0.0, 2.0 * M_PI));
        
        ExternalInterferer node;
        node.position.x = centerPosition.x + r * std::cos(theta);
        node.position.y = centerPosition.y + r * std::sin(theta);
        node.position.z = centerPosition.z;
        node.txPowerDbm = sampler.Sample(params.externalTxPowerDbm);
        node.dutyCycle = sampler.Sample(params.externalDutyCycle);
        node.currentlyActive = false;
        node.nextStateChange = Time::FromDouble(0.0, Time::S);
        node.spreadingFactor = (uint8_t)std::min(12, std::max(7, (int)std::round(sampler.Sample(DistributionParam::Uniform(7.0, 12.0)))));
        m_interferers.push_back(node);
    }
    m_interfererMapInitialized = true;
}

void InterferenceSubmodule::RegisterActiveTransmission(
    uint32_t txNodeId, uint32_t rxGatewayId,
    Time startTime, Time airtime,
    uint8_t sf, double bw, uint8_t cr,
    double txPowerDbm, Vector txPosition, double ownLinkDistM)
{
    ActiveTransmission t;
    t.ownLinkDistM = ownLinkDistM;
    t.txNodeId = txNodeId;
    t.startTime = startTime;
    t.endTime = startTime + airtime;
    t.spreadingFactor = sf;
    t.bandwidthHz = bw;
    t.codingRateDenom = cr;
    t.rxPowerDbmAtGateway = txPowerDbm; // Actually preliminaryRxPowerDbm passed here
    t.txPosition = txPosition;
    t.rxGatewayId = rxGatewayId;
    t.isCanceled = false;
    m_activeRegistry.push_back(t);
    if (m_journalOn)
    {
        m_journal.push_back(t);
        while (!m_journal.empty() && m_journal.front().endTime + Seconds(10.0) < startTime)
        {
            m_journal.pop_front();
        }
    }
}

uint32_t
InterferenceSubmodule::ReplayJournal(Time from, Time to)
{
    uint32_t n = 0;
    for (const ActiveTransmission& j : m_journal)
    {
        if (!(j.startTime < to && j.endTime > from))
        {
            continue;
        }
        bool present = false;
        for (const ActiveTransmission& a : m_activeRegistry)
        {
            if (a.txNodeId == j.txNodeId && a.rxGatewayId == j.rxGatewayId && a.startTime == j.startTime)
            {
                present = true;
                break;
            }
        }
        if (!present)
        {
            m_activeRegistry.push_back(j);
            ++n;
        }
    }
    return n;
}

void InterferenceSubmodule::NotifySuccessfulDecode(uint32_t txNodeId, uint32_t rxGatewayId, double decodedPowerDbm) {
    for (auto& t : m_activeRegistry) {
        if (t.txNodeId == txNodeId && t.rxGatewayId == rxGatewayId) {
            t.isCanceled = true;
            break;
        }
    }
}

double InterferenceSubmodule::ComputeInterferencePenaltyDb(
    uint32_t txNodeId, uint8_t rxSpreadingFactor, double bandwidthHz,
    Time desiredStartTime, Time desiredAirtime,
    Vector txPosition, Vector rxPosition, Time currentTime,
    const InterferenceParams& params,
    LoRaDistributionSampler& sampler,
    double desiredSignalPowerDbm,
    double noiseFloorDbm)
{
    m_wasCollision = false;
    m_wasCaptured = false;
    m_strongestInterfererPowerDbm = -200.0;
    m_strongestInterfererSF = 7;
    m_overlapProbability = 0.0;
    m_collisionOverlapFraction = 0.0;
    m_interfererCount = 0;
    m_captureMarginDb = 0.0;
    m_sicRecovered = false;

    if (!params.enabled) {
        return 0.0;
    }

    if (!m_interfererMapInitialized) {
        if (LoRaDistributionSampler::KeyedMode()) {
            // --rngKeyed (plan v3 V1): the external-interferer field is laid out once per
            // run on its own keyed sub-sequence and centred on the deployment origin, not
            // drawn inside -- and centred on -- whichever link happens to be evaluated first.
            sampler.BeginSubContext(0x494E54464D4150ULL, 0);
            InitializeInterfererMap(Vector(0.0, 0.0, rxPosition.z), 5000.0, params, sampler);
            sampler.EndSubContext();
        } else {
            InitializeInterfererMap(rxPosition, 5000.0, params, sampler);
        }
    }

    // 1. Purge expired active transmissions from registry
    m_activeRegistry.remove_if([currentTime](const ActiveTransmission& t) {
        return t.endTime <= currentTime;
    });

    double totalInterferenceMw = 0.0;
    double maxOverlapFrac = 0.0;
    Time earliestInterfererStart = Time::Max();

    // 2. Evaluate active packet-level interference registry
    for (const auto& node : m_activeRegistry) {
        if (node.txNodeId == txNodeId) continue; // Do not self-collide

        Time overlapStart = std::max(desiredStartTime, node.startTime);
        Time overlapEnd = std::min(desiredStartTime + desiredAirtime, node.endTime);

        if (overlapEnd > overlapStart) {
            double overlapDurationS = (overlapEnd - overlapStart).GetSeconds();
            double desiredAirtimeS = desiredAirtime.GetSeconds();
            double overlapFrac = (desiredAirtimeS > 0.0 ? overlapDurationS / desiredAirtimeS : 0.0);
            
            double dx = node.txPosition.x - rxPosition.x;
            double dy = node.txPosition.y - rxPosition.y;
            double dz = node.txPosition.z - rxPosition.z;
            double dist = std::max(1.0, std::sqrt(dx*dx + dy*dy + dz*dz));
            
            int rIdx = std::min(5, std::max(0, (int)rxSpreadingFactor - 7));
            int iIdx = std::min(5, std::max(0, (int)node.spreadingFactor - 7));
            double isolation = params.sfIsolationDb[rIdx][iIdx];
            
            // Use the actual received power recorded at the gateway for raw
            // collision telemetry, then apply SIC only to the residual penalty.
            double rawRxPowerAtGw = node.rxPowerDbmAtGateway - isolation;
            if (params.attenuateByDistance && node.ownLinkDistM > 0.0) {
                // T29: the interferer as heard HERE, not at its own receiver.
                rawRxPowerAtGw -= 10.0 * params.attenuationExponent * std::log10(dist / node.ownLinkDistM);
            }
            double rxPowerAtGw = rawRxPowerAtGw;
            if (node.isCanceled) {
                rxPowerAtGw -= 30.0; // 30 dB SIC cancellation depth
                m_sicRecovered = true;
            }
            
            if (rawRxPowerAtGw > noiseFloorDbm) {
                m_interfererCount++;
                if (overlapFrac > maxOverlapFrac) maxOverlapFrac = overlapFrac;
                
                if (rawRxPowerAtGw > m_strongestInterfererPowerDbm) {
                    m_strongestInterfererPowerDbm = rawRxPowerAtGw;
                    m_strongestInterfererSF = node.spreadingFactor;
                }
                
                if (node.startTime < earliestInterfererStart) {
                    earliestInterfererStart = node.startTime;
                }
                
                if (rxPowerAtGw > noiseFloorDbm) {
                    totalInterferenceMw += std::pow(10.0, rxPowerAtGw / 10.0) * overlapFrac;
                }
            }
        }
    }

    // 3. Evaluate external synthetic PPP interferers
    for (auto& node : m_interferers) {
        double probActive = node.dutyCycle;
        double u = sampler.Sample(DistributionParam::Uniform(0.0, 1.0));
        
        if (u < probActive) {
            double dx = node.position.x - rxPosition.x;
            double dy = node.position.y - rxPosition.y;
            double dist = std::max(1.0, std::sqrt(dx*dx + dy*dy));
            double pl = 20.0 * std::log10(dist) + 38.0;
            
            int rIdx = std::min(5, std::max(0, (int)rxSpreadingFactor - 7));
            int iIdx = std::min(5, std::max(0, (int)node.spreadingFactor - 7));
            double rxPowerAtGw = node.txPowerDbm - pl - params.sfIsolationDb[rIdx][iIdx];
            
            if (rxPowerAtGw > noiseFloorDbm) {
                m_interfererCount++;
                double oFrac = sampler.Sample(DistributionParam::Uniform(0.1, 1.0));
                if (oFrac > maxOverlapFrac) maxOverlapFrac = oFrac;
                
                if (rxPowerAtGw > m_strongestInterfererPowerDbm) {
                    m_strongestInterfererPowerDbm = rxPowerAtGw;
                    m_strongestInterfererSF = node.spreadingFactor;
                }
                
                totalInterferenceMw += std::pow(10.0, rxPowerAtGw / 10.0) * oFrac;
            }
        }
    }

    m_collisionOverlapFraction = maxOverlapFrac;
    m_wasCollision = (m_interfererCount > 0);
    m_overlapProbability = (m_wasCollision ? maxOverlapFrac : 0.0);
    
    m_captureMarginDb = desiredSignalPowerDbm - m_strongestInterfererPowerDbm;

    // Preamble Capture Model: if desired packet preamble arrives clean before any collision
    bool preambleCaptureSucceeded = false;
    Time preambleDuration = Time::FromDouble(0.015, Time::S); // Nominal LoRa preamble ~15ms
    if (desiredStartTime + preambleDuration < earliestInterfererStart) {
        preambleCaptureSucceeded = true;
    }

    // Capture effect: logistic sigmoid model (Croce et al. 2018)
    // Produces soft probabilistic transition instead of hard binary threshold.
    // k=0.8 fitted to Croce 2018 Fig.2: ~0% at 0 dB, ~50% at 6 dB, ~95% at 10 dB.
    if (params.captureEffectEnabled && m_wasCollision) {
        int rIdx = std::min(5, std::max(0, (int)rxSpreadingFactor - 7));
        int iIdx = std::min(5, std::max(0, (int)m_strongestInterfererSF - 7));
        
        // Use the explicit 6x6 Croce 2018 capture threshold matrix directly against RAW SIR
        // Recompute raw SIR for the strongest interferer (removing isolation)
        double rawStrongestInterfererPowerDbm = m_strongestInterfererPowerDbm + params.sfIsolationDb[rIdx][iIdx];
        double rawSir = desiredSignalPowerDbm - rawStrongestInterfererPowerDbm;
        
        double requiredMargin = params.captureThresholdDb[rIdx][iIdx];
        if (preambleCaptureSucceeded) {
            requiredMargin -= 3.0; // Preamble advantage
        }
        
        double captureProbability = 1.0 / (1.0 + std::exp(-0.8 * (rawSir - requiredMargin)));
        double u = sampler.Sample(DistributionParam::Uniform(0.0, 1.0));
        m_wasCaptured = (u < captureProbability);
    } else {
        m_wasCaptured = false;
    }

    if (!m_wasCollision) {
        return 0.0;
    }

    // Compute composite interference penalty
    double desiredSignalMw = std::pow(10.0, desiredSignalPowerDbm / 10.0);
    double sinrLinear = desiredSignalMw / (std::pow(10.0, noiseFloorDbm / 10.0) + totalInterferenceMw);
    double snrCleanLinear = desiredSignalMw / std::pow(10.0, noiseFloorDbm / 10.0);
    
    double penaltyDb = 10.0 * std::log10(snrCleanLinear / sinrLinear);
    penaltyDb = std::max(0.0, std::min(25.0, penaltyDb)); // Bounded composite penalty

    if (m_wasCaptured) {
        return 0.0;
    }

    return penaltyDb;
}

bool InterferenceSubmodule::CheckCaptureEffect(
    double desiredPowerDbm, double interfererPowerDbm,
    uint8_t desiredSF, uint8_t interfererSF,
    const InterferenceParams& params)
{
    int rIdx = std::min(5, std::max(0, (int)desiredSF - 7));
    int iIdx = std::min(5, std::max(0, (int)interfererSF - 7));
    double effectiveInterfererPower = interfererPowerDbm - params.sfIsolationDb[rIdx][iIdx];
    double sir = desiredPowerDbm - effectiveInterfererPower;
    
    // Logistic sigmoid capture probability (Croce 2018)
    double threshold = params.captureThresholdDb[rIdx][iIdx];
    double captureProbability = 1.0 / (1.0 + std::exp(-0.8 * (sir - threshold)));
    
    // Deterministic for the public API; callers needing stochastic behavior
    // should use the main ComputeInterferencePenaltyDb path instead.
    return (captureProbability >= 0.5);
}

} // namespace ns3
