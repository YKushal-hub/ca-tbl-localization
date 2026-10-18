#include "lora-distribution-sampler.h"
#include "ns3/double.h"
#include <cmath>
#include <algorithm>

namespace ns3 {

namespace {
bool g_keyed = false;
uint64_t g_keyedRunSeed = 0;
uint64_t g_unkeyedDraws = 0;

uint64_t SplitMix64(uint64_t x)
{
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

uint64_t Fnv1a(const std::string& s)
{
    uint64_t h = 0xcbf29ce484222325ULL;
    for (unsigned char ch : s)
    {
        h = (h ^ ch) * 0x100000001b3ULL;
    }
    return h;
}

uint64_t Mix(uint64_t h, uint64_t v)
{
    return SplitMix64(h ^ SplitMix64(v));
}
} // namespace

LoRaDistributionSampler::LoRaDistributionSampler(const LoRaDistributionSampler& other)
    : m_uniform(other.m_uniform),
      m_normal(other.m_normal),
      m_exponential(other.m_exponential),
      m_lognormalUnderlying(other.m_lognormalUnderlying),
      m_gamma(other.m_gamma),
      m_persistentCache(other.m_persistentCache),
      m_source(other.m_source != nullptr ? other.m_source : const_cast<LoRaDistributionSampler*>(&other)),
      m_salt(other.m_salt)
{
}

LoRaDistributionSampler&
LoRaDistributionSampler::operator=(const LoRaDistributionSampler& other)
{
    if (this != &other)
    {
        m_uniform = other.m_uniform;
        m_normal = other.m_normal;
        m_exponential = other.m_exponential;
        m_lognormalUnderlying = other.m_lognormalUnderlying;
        m_gamma = other.m_gamma;
        m_persistentCache = other.m_persistentCache;
        m_source = nullptr;
        m_salt = other.m_salt;
        m_inContext = false;
        m_ctxKey = 0;
        m_ctxDraw = 0;
        m_lastCtxTimeNs = -1;
        m_ctxOccurrence.clear();
        m_fallbackCounter = 0;
    }
    return *this;
}

void LoRaDistributionSampler::SetKeyedMode(bool on, uint64_t runSeed)
{
    g_keyed = on;
    g_keyedRunSeed = runSeed; // the unkeyed counter is cumulative over the process
}

bool LoRaDistributionSampler::KeyedMode()
{
    return g_keyed;
}

uint64_t LoRaDistributionSampler::UnkeyedDraws()
{
    return g_unkeyedDraws;
}

void LoRaDistributionSampler::BeginContext(uint32_t txId, uint32_t rxId, int64_t timeNs)
{
    if (!g_keyed)
    {
        return;
    }
    // Simulated time restarts at 0 for every arm; the occurrence table is per instant,
    // so it is cleared whenever time moves (forwards or back).
    if (timeNs != m_lastCtxTimeNs)
    {
        m_ctxOccurrence.clear();
        m_lastCtxTimeNs = timeNs;
    }
    const uint64_t link = (static_cast<uint64_t>(txId) << 32) | rxId;
    const uint32_t occ = m_ctxOccurrence[std::make_pair(link, timeNs)]++;
    uint64_t h = Mix(g_keyedRunSeed, m_salt);
    h = Mix(h, link);
    h = Mix(h, static_cast<uint64_t>(timeNs));
    m_ctxKey = Mix(h, occ);
    m_ctxDraw = 0;
    m_inContext = true;
}

void LoRaDistributionSampler::EndContext()
{
    m_inContext = false;
}

void LoRaDistributionSampler::BeginSubContext(uint64_t tag, uint64_t index)
{
    if (!g_keyed)
    {
        return;
    }
    LoRaDistributionSampler* s = (m_source != nullptr) ? m_source : this;
    s->m_savedInContext = s->m_inContext;
    s->m_savedKey = s->m_ctxKey;
    s->m_savedDraw = s->m_ctxDraw;
    s->m_ctxKey = Mix(Mix(Mix(g_keyedRunSeed, s->m_salt), tag), index);
    s->m_ctxDraw = 0;
    s->m_inContext = true;
}

void LoRaDistributionSampler::EndSubContext()
{
    if (!g_keyed)
    {
        return;
    }
    LoRaDistributionSampler* s = (m_source != nullptr) ? m_source : this;
    s->m_inContext = s->m_savedInContext;
    s->m_ctxKey = s->m_savedKey;
    s->m_ctxDraw = s->m_savedDraw;
}

double LoRaDistributionSampler::KeyedUniform01()
{
    if (m_source != nullptr && !m_inContext)
    {
        // A submodule's copy draws in its engine sampler's current link context.
        return m_source->KeyedUniform01();
    }
    uint64_t h;
    if (m_inContext)
    {
        h = Mix(m_ctxKey, m_ctxDraw++);
    }
    else
    {
        ++g_unkeyedDraws;
        h = Mix(Mix(Mix(g_keyedRunSeed, m_salt), 0xfa11bac4ULL), m_fallbackCounter++);
    }
    // 53 random bits, shifted half a step so the result is never 0 or 1.
    return (static_cast<double>(h >> 11) + 0.5) * (1.0 / 9007199254740992.0);
}

double LoRaDistributionSampler::KeyedStdNormal()
{
    // Box-Muller, one value per pair, so the draw count per value is fixed (2).
    const double u1 = KeyedUniform01();
    const double u2 = KeyedUniform01();
    return std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * M_PI * u2);
}

double LoRaDistributionSampler::KeyedGamma(double shape)
{
    // Marsaglia & Tsang (2000); shape < 1 by the u^(1/shape) boost.
    if (shape < 1.0)
    {
        const double u = KeyedUniform01();
        return KeyedGamma(shape + 1.0) * std::pow(u, 1.0 / shape);
    }
    const double d = shape - 1.0 / 3.0;
    const double c = 1.0 / std::sqrt(9.0 * d);
    for (;;)
    {
        double x;
        double v;
        do
        {
            x = KeyedStdNormal();
            v = 1.0 + c * x;
        } while (v <= 0.0);
        v = v * v * v;
        const double u = KeyedUniform01();
        if (u < 1.0 - 0.0331 * x * x * x * x)
        {
            return d * v;
        }
        if (std::log(u) < 0.5 * x * x + d * (1.0 - v + std::log(v)))
        {
            return d * v;
        }
    }
}

LoRaDistributionSampler::LoRaDistributionSampler(uint32_t seed) {
    m_uniform = CreateObject<UniformRandomVariable>();
    m_normal = CreateObject<NormalRandomVariable>();
    m_exponential = CreateObject<ExponentialRandomVariable>();
    m_lognormalUnderlying = CreateObject<NormalRandomVariable>();
    m_gamma = CreateObject<GammaRandomVariable>();
    
    // In ns-3, stream allocation is normally done by assigning streams to 
    // variables using AssignStreams or letting them use the default stream manager.
    // The underlying ns-3 RNG will respect --RngSeed and --RngRun automatically.
}

double LoRaDistributionSampler::Sample(const DistributionParam& param) {
    switch (param.type) {
        case DistributionParam::DETERMINISTIC:
            return param.mean;
        case DistributionParam::UNIFORM:
            return SampleUniform(param.min, param.max);
        case DistributionParam::NORMAL:
            return SampleNormal(param.mean, param.stddev, param.min, param.max);
        case DistributionParam::LOGNORMAL:
            return SampleLogNormal(param.mean, param.stddev, param.min, param.max);
        case DistributionParam::EXPONENTIAL:
            return SampleExponential(param.lambda, param.min, param.max);
        case DistributionParam::NAKAGAMI:
            return SampleNakagami(param.shape, param.mean, param.min, param.max);
    }
    return param.mean; // Fallback
}

double LoRaDistributionSampler::SamplePersistent(const DistributionParam& param,
                                                 uint64_t nodeId,
                                                 const std::string& paramName) {
    auto key = std::make_pair(nodeId, paramName);
    auto it = m_persistentCache.find(key);
    if (it != m_persistentCache.end()) {
        return it->second;
    }
    
    double val;
    if (g_keyed)
    {
        // Keyed by (node, parameter name), not by when it was first asked for.
        const bool saved = m_inContext;
        const uint64_t savedKey = m_ctxKey;
        const uint64_t savedDraw = m_ctxDraw;
        m_ctxKey = Mix(Mix(Mix(g_keyedRunSeed, m_salt), nodeId), Fnv1a(paramName));
        m_ctxDraw = 0;
        m_inContext = true;
        val = Sample(param);
        m_inContext = saved;
        m_ctxKey = savedKey;
        m_ctxDraw = savedDraw;
    }
    else
    {
        val = Sample(param);
    }
    m_persistentCache[key] = val;
    return val;
}

void LoRaDistributionSampler::ClearNodeCache(uint64_t nodeId) {
    for (auto it = m_persistentCache.begin(); it != m_persistentCache.end();) {
        if (it->first.first == nodeId) {
            it = m_persistentCache.erase(it);
        } else {
            ++it;
        }
    }
}

void LoRaDistributionSampler::ClearAllCaches() {
    m_persistentCache.clear();
}

double LoRaDistributionSampler::SampleUniform(double min, double max) {
    if (g_keyed)
    {
        return min + (max - min) * KeyedUniform01();
    }
    m_uniform->SetAttribute("Min", DoubleValue(min));
    m_uniform->SetAttribute("Max", DoubleValue(max));
    return m_uniform->GetValue();
}

double LoRaDistributionSampler::SampleNormal(double mean, double stddev, double min, double max) {
    if (g_keyed)
    {
        return Clamp(mean + stddev * KeyedStdNormal(), min, max);
    }
    m_normal->SetAttribute("Mean", DoubleValue(mean));
    // Variance is stddev^2 in ns-3 NormalRandomVariable
    m_normal->SetAttribute("Variance", DoubleValue(stddev * stddev));
    return Clamp(m_normal->GetValue(), min, max);
}

double LoRaDistributionSampler::SampleLogNormal(double mean, double stddev, double min, double max) {
    if (g_keyed)
    {
        return Clamp(std::exp(mean + stddev * KeyedStdNormal()), min, max);
    }
    m_lognormalUnderlying->SetAttribute("Mean", DoubleValue(mean));
    m_lognormalUnderlying->SetAttribute("Variance", DoubleValue(stddev * stddev));
    double val = m_lognormalUnderlying->GetValue();
    return Clamp(std::exp(val), min, max);
}

double LoRaDistributionSampler::SampleExponential(double lambda, double min, double max) {
    // Mean is 1/lambda in ns-3 ExponentialRandomVariable
    if (lambda <= 0.0) return Clamp(0.0, min, max);
    if (g_keyed)
    {
        return Clamp(-std::log(KeyedUniform01()) / lambda, min, max);
    }
    m_exponential->SetAttribute("Mean", DoubleValue(1.0 / lambda));
    return Clamp(m_exponential->GetValue(), min, max);
}

double LoRaDistributionSampler::SampleNakagami(double shape, double mean, double min, double max) {
    // A Nakagami-m random variable with shape m and spread Omega can be generated
    // by taking the square root of a Gamma variable with shape m and scale Omega/m
    if (shape <= 0.0 || mean <= 0.0) return Clamp(0.0, min, max);
    if (g_keyed)
    {
        return Clamp(std::sqrt(KeyedGamma(shape) * (mean / shape)), min, max);
    }
    m_gamma->SetAttribute("Alpha", DoubleValue(shape));
    m_gamma->SetAttribute("Beta", DoubleValue(mean / shape));
    double y = m_gamma->GetValue();
    return Clamp(std::sqrt(y), min, max);
}

double LoRaDistributionSampler::Clamp(double value, double min, double max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

} // namespace ns3
