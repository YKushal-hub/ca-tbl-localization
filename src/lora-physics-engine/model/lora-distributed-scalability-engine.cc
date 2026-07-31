#include "lora-distributed-scalability-engine.h"
#include "ns3/log.h"
#include <cmath>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("LoRaDistributedScalabilityEngine");

DistributedScalabilityEngine::DistributedScalabilityEngine(const DistributedScalabilityConfigParams& params)
    : m_params(params)
{
    NS_LOG_FUNCTION(this);
}

uint32_t DistributedScalabilityEngine::GetRegionId(Vector pos) const
{
    // Hierarchical spatial partitioning based on region size (default 1000m x 1000m grids)
    int32_t gridX = (int32_t)std::floor(pos.x / m_params.regionPartitionSizeM);
    int32_t gridY = (int32_t)std::floor(pos.y / m_params.regionPartitionSizeM);
    
    // Simple 2D spatial hash
    return (uint32_t)(gridX * 73856093 ^ gridY * 19349663);
}

DistributedScalabilityResult DistributedScalabilityEngine::EvaluatePartitionAndBatch(
    Vector txPosition, uint32_t activeTransmissionsInRegion, bool gpuBatchingRequested)
{
    NS_LOG_FUNCTION(this << txPosition << activeTransmissionsInRegion << gpuBatchingRequested);

    DistributedScalabilityResult res;
    res.regionSchedulerState = REGION_STATE_IDLE;
    res.gpuBatchSize = 0;
    res.sparsePropagationUsed = false;

    if (!m_params.enabled) {
        return res;
    }

    uint32_t regionId = GetRegionId(txPosition);
    (void)regionId; // Conceptually routes to the correct distributed compute node

    // 1. Hierarchical Region Scheduling
    if (activeTransmissionsInRegion == 0) {
        res.regionSchedulerState = REGION_STATE_IDLE;
        res.sparsePropagationUsed = true;
    } else if (activeTransmissionsInRegion < 10) {
        res.regionSchedulerState = REGION_STATE_SPARSE;
        // 2. Sparse Event Propagation: Stable regions use cached channel states
        res.sparsePropagationUsed = true;
    } else if (activeTransmissionsInRegion < 100) {
        res.regionSchedulerState = REGION_STATE_DENSE;
        res.sparsePropagationUsed = false;
    } else {
        res.regionSchedulerState = REGION_STATE_CRITICAL;
        res.sparsePropagationUsed = false; // Require full continuous-time micro-simulation
    }

    // 6. GPU Compute Pipeline Batching
    if (m_params.gpuBatchingEnabled && gpuBatchingRequested) {
        // Collect into simulated Structure-of-Arrays (SoA) batch for SIMD / CUDA
        if (activeTransmissionsInRegion > 0) {
            // Emulate batching multiples of 32 threads (warp size) for GPU efficiency
            res.gpuBatchSize = ((activeTransmissionsInRegion + 31) / 32) * 32;
        } else {
            res.gpuBatchSize = 32; // Minimum single warp
        }
    }

    return res;
}

} // namespace ns3
