#include "lora-microdsp-submodule.h"
#include "ns3/log.h"
#include <cmath>
// <chrono> deliberately NOT included. Wall-clock time must never reach simulation
// state -- see the note in EvaluateMicroDspWindow(). Removing the include makes a
// reintroduction a compile error rather than a silent reproducibility failure.

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("LoRaMicroDspSubmodule");

MicroDspSubmodule::MicroDspSubmodule(const MicroDspConfigParams& params, LoRaDistributionSampler& sampler)
    : m_params(params), m_sampler(sampler)
{
    NS_LOG_FUNCTION(this);
}

MicroDspResult MicroDspSubmodule::EvaluateMicroDspWindow(
    double snrDb, double collisionOverlapFraction,
    bool pllLocked, double cfoInstabilityHz,
    bool sicAmbiguity, double symbolicFftConfidence)
{
    NS_LOG_FUNCTION(this << snrDb << collisionOverlapFraction << pllLocked << cfoInstabilityHz << sicAmbiguity << symbolicFftConfidence);

    MicroDspResult res;
    res.microDspActive = false;
    res.microDspFftConfidence = symbolicFftConfidence;
    res.microDspSymbolAgreement = 1.0;
    res.microDspRuntimeUs = 0.0;

    if (!m_params.enabled) {
        return res;
    }

    // 1. Event-Triggered Activation Logic
    bool triggerSnr = (snrDb < m_params.activationSnrThresholdDb);
    bool triggerCollision = (collisionOverlapFraction > m_params.collisionOverlapThreshold);
    bool triggerPll = !pllLocked || (cfoInstabilityHz > 100.0);
    
    if (!triggerSnr && !triggerCollision && !triggerPll && !sicAmbiguity) {
        return res; // Stable packet, remain in coarse symbolic mode
    }

    res.microDspActive = true;

    // 2. Adaptive Window Resolution & 3. Localized FFT Demodulation
    // Simulate a high-resolution micro-window DSP extraction
    double windowFftConfidence = symbolicFftConfidence;
    
    if (triggerCollision) {
        // High-resolution FFT can sometimes rescue symbols from partial collisions better than statistical averages
        double microRecoveryFactor = m_sampler.Sample(DistributionParam::Uniform(0.8, 1.2));
        windowFftConfidence = std::clamp(symbolicFftConfidence * microRecoveryFactor, 0.0, 1.0);
    }
    
    if (triggerPll) {
        // Micro-DSP tracks phase evolution explicitly, often penalizing unstable PLLs more accurately
        double microPenaltyFactor = m_sampler.Sample(DistributionParam::Uniform(0.5, 0.95));
        windowFftConfidence = std::clamp(symbolicFftConfidence * microPenaltyFactor, 0.0, 1.0);
    }

    // 4. Windowed DSP Validation
    // Calculate agreement between coarse statistical engine and micro-window engine
    double diff = std::abs(symbolicFftConfidence - windowFftConfidence);
    res.microDspSymbolAgreement = std::clamp(1.0 - diff, 0.0, 1.0);
    res.microDspFftConfidence = windowFftConfidence;

    // Simulated DSP windowing overhead on the NODE's micro-controller.
    //
    // This was previously measured with std::chrono::high_resolution_clock around the
    // block above, and the measurement then GATED this sample:
    //     if (res.microDspRuntimeUs < 15.0) { res.microDspRuntimeUs = Sample(...); }
    // The timed block is a handful of multiplications and clamps, so it normally
    // completes in under a microsecond, count() truncates to 0, the branch is taken
    // and one sample is consumed. When the OS preempted the thread or a page fault
    // landed inside that window, elapsed time exceeded 15 us, the branch was SKIPPED,
    // and no sample was drawn -- shifting this engine's RNG stream by one draw for the
    // remainder of the run.
    //
    // The simulation was therefore not reproducible from its seed, and the divergence
    // point depended on machine load. Measured consequence: seed 7 produced 157 UWB
    // fixes on one run and 171 on the next, and static_868 RMSE ranged over
    // 128.3-149.1 m across four runs of the same seed. Every arm comparison taken
    // before this fix is void.
    //
    // The quantity is also conceptually wrong as a measurement: it models the runtime
    // of an embedded DSP, which has no relationship to how fast the host CPU executes
    // this function. It is now sampled unconditionally, which both restores
    // determinism (exactly one draw whenever this path runs) and stops the number
    // pretending to be an observation.
    res.microDspRuntimeUs = m_sampler.Sample(DistributionParam::Uniform(15.0, 45.0));

    return res;
}

} // namespace ns3
