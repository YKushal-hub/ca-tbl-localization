#ifndef LORA_DISTRIBUTION_SAMPLER_H
#define LORA_DISTRIBUTION_SAMPLER_H

#include "ns3/random-variable-stream.h"
#include "ns3/ptr.h"
#include "lora-physics-parameters.h"
#include <map>
#include <string>
#include <utility>

namespace ns3 {

/**
 * @brief Thread-safe (for single-threaded ns-3) random distribution sampler.
 * Integrates with ns-3's RandomVariableStream architecture for repeatable runs.
 */
class LoRaDistributionSampler {
public:
    LoRaDistributionSampler(uint32_t seed = 42);

    /**
     * Five submodules (symbol sync, PLL, RF nonlinearity, DSP chain, IQ stream) hold a COPY
     * of their engine's sampler, made by this constructor. The ns-3 streams are shared Ptrs,
     * so unkeyed behaviour is unchanged; the copy also remembers its source so that, under
     * --rngKeyed, it draws in the source's link context. Assignment (how an engine (re)builds
     * its own sampler) does not set a source.
     */
    LoRaDistributionSampler(const LoRaDistributionSampler& other);
    LoRaDistributionSampler& operator=(const LoRaDistributionSampler& other);
    
    // Sample once from a distribution, clamped to [min, max]
    double Sample(const DistributionParam& param);
    
    // Sample and cache a persistent value per nodeId
    // Returns the same value every call for the same (nodeId, paramName)
    double SamplePersistent(const DistributionParam& param,
                             uint64_t nodeId,
                             const std::string& paramName);
    
    // Clear the persistent cache for a specific node
    void ClearNodeCache(uint64_t nodeId);
    
    // Clear all persistent caches
    void ClearAllCaches();

    /**
     * \brief Keyed draws (--rngKeyed, TECH_DEBT 1.1). Default OFF: every draw then comes
     * from the ns-3 streams exactly as before, byte for byte.
     *
     * ON: a draw is a pure function of (run seed, this sampler's salt, the current link
     * context, the draw's index within that context), computed with a SplitMix64 hash.
     * A link evaluation therefore sees the same channel whatever else the run did before
     * it -- an extra transmission elsewhere no longer displaces every later draw. This is
     * the common-random-numbers construction of simulation output analysis.
     */
    static void SetKeyedMode(bool on, uint64_t runSeed);
    static bool KeyedMode();
    /** Draws made in keyed mode outside any link context (sequential fallback). */
    static uint64_t UnkeyedDraws();
    /** Distinguishes engines and samplers sharing a run seed (868, 2.4, UWB, fading). */
    void SetKeyedSalt(uint64_t salt) { m_salt = salt; }
    /** Enter / leave a link context. Nested Begin calls are not supported. */
    void BeginContext(uint32_t txId, uint32_t rxId, int64_t timeNs);
    void EndContext();
    /**
     * A keyed sub-sequence that belongs to no link (e.g. the weather of one update epoch):
     * saves the current link context, draws on key (tag, index), and EndSubContext restores
     * it. One level deep. No-op unless --rngKeyed.
     */
    void BeginSubContext(uint64_t tag, uint64_t index);
    void EndSubContext();

private:
    Ptr<UniformRandomVariable> m_uniform;
    Ptr<NormalRandomVariable> m_normal;
    Ptr<ExponentialRandomVariable> m_exponential;
    Ptr<NormalRandomVariable> m_lognormalUnderlying;
    Ptr<GammaRandomVariable> m_gamma; // Used for Nakagami generation
    
    // Persistent cache: key = (nodeId, paramName) -> cached value
    std::map<std::pair<uint64_t, std::string>, double> m_persistentCache;
    
    double SampleNormal(double mean, double stddev, double min, double max);
    double SampleLogNormal(double mean, double stddev, double min, double max);
    double SampleUniform(double min, double max);
    double SampleExponential(double lambda, double min, double max);
    double SampleNakagami(double shape, double mean, double min, double max);
    double Clamp(double value, double min, double max);

    // keyed mode
    LoRaDistributionSampler* m_source = nullptr; // set by the copy constructor only
    uint64_t m_salt = 0;
    bool m_inContext = false;
    uint64_t m_ctxKey = 0;
    uint64_t m_ctxDraw = 0;
    int64_t m_lastCtxTimeNs = -1;
    std::map<std::pair<uint64_t, int64_t>, uint32_t> m_ctxOccurrence; // (link, t) -> times seen at t
    uint64_t m_fallbackCounter = 0;
    bool m_savedInContext = false;
    uint64_t m_savedKey = 0;
    uint64_t m_savedDraw = 0;
    double KeyedUniform01();                 // in (0, 1)
    double KeyedStdNormal();
    double KeyedGamma(double shape);         // scale 1
};

/**
 * \brief Scope of one link evaluation for keyed draws: begins a context on the engine's
 * main and fading samplers, ends both on every exit path. A no-op unless --rngKeyed.
 */
class LoRaSamplerContextGuard
{
  public:
    LoRaSamplerContextGuard(LoRaDistributionSampler& a,
                            LoRaDistributionSampler& b,
                            uint32_t txId,
                            uint32_t rxId,
                            int64_t timeNs)
        : m_a(a),
          m_b(b)
    {
        m_a.BeginContext(txId, rxId, timeNs);
        m_b.BeginContext(txId, rxId, timeNs);
    }

    ~LoRaSamplerContextGuard()
    {
        m_a.EndContext();
        m_b.EndContext();
    }

    LoRaSamplerContextGuard(const LoRaSamplerContextGuard&) = delete;
    LoRaSamplerContextGuard& operator=(const LoRaSamplerContextGuard&) = delete;

  private:
    LoRaDistributionSampler& m_a;
    LoRaDistributionSampler& m_b;
};

} // namespace ns3

#endif // LORA_DISTRIBUTION_SAMPLER_H
