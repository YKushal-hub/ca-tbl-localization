#include "ca-tbl-nlos-classifier.h"

#include "ns3/double.h"
#include "ns3/integer.h"
#include "ns3/object-factory.h"

#include <algorithm>
#include <cmath>
#include <limits>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace ns3
{

namespace
{

const double kNaN = std::numeric_limits<double>::quiet_NaN();

// IEEE P802.15-04-0662-02-004a, Appendix A MATLAB listing (uwb_sv_params_15_4a), cm_num
// 3-6, copied field for field. `mFirst` is the listing's m0_sp under sfading_mode = 2
// (the first ray of EVERY cluster gets m = 3, which the listing marks "assumption"); 0
// where sfading_mode = 0. Shadowing (std_shdw) and the frequency dependence (kappa) are
// not used: the engine already applied its own shadowing and path loss to this link.
const CaTblCirModel kCm3 = {"CM3 office LOS", 0.016, 5.4, 0.19, 2.97, 0.0184, 14.6, 6.4,
                            0.0, 3.0, 0, kNaN, kNaN, kNaN, 0.42, 0.31, 3.0};
const CaTblCirModel kCm4 = {"CM4 office NLOS", 0.19, 3.1, 0.11, 2.09, 0.0096, 19.8, 11.2,
                            0.0, 3.0, 2, 15.21, 11.84, 0.78, 0.5, 0.25, 0.0};
const CaTblCirModel kCm5 = {"CM5 outdoor LOS", 0.0448, 13.6, 0.13, 2.41, 0.0078, 31.7, 3.7,
                            0.0, 3.0, 0, kNaN, kNaN, kNaN, 0.77, 0.78, 3.0};
const CaTblCirModel kCm6 = {"CM6 outdoor NLOS", 0.0243, 10.5, 0.15, 1.13, 0.062, 104.7, 9.3,
                            0.0, 3.0, 1, kNaN, kNaN, kNaN, 0.56, 0.25, 0.0};

} // namespace

const CaTblCirModel&
CaTblCirModelFor(EnvironmentClass env, bool nlos)
{
    if (env == ENV_INDOOR_OFFICE)
    {
        return nlos ? kCm4 : kCm3;
    }
    return nlos ? kCm6 : kCm5;
}

CaTblCirConfig::CaTblCirConfig()
    : tapNs(1.0),
      preNs(16.0),
      windowNs(512.0),
      noiseK(3.0)
{
}

CaTblCirResult::CaTblCirResult()
    : detected(false),
      leTap(-1),
      peakTap(-1),
      leDelayNs(0.0),
      peakDelayNs(0.0),
      aLeadingEdge(0.0),
      aPeak(0.0),
      noiseSigma(0.0),
      directPower(0.0),
      score(-1.0),
      nClusters(0),
      nRays(0)
{
}

CaTblCirResult
CaTblLeadingEdge(const std::vector<double>& mag, double noiseSigma, double noiseK)
{
    CaTblCirResult r;
    r.noiseSigma = noiseSigma;
    if (mag.empty())
    {
        return r;
    }
    size_t peak = 0;
    for (size_t i = 1; i < mag.size(); ++i)
    {
        if (mag[i] > mag[peak])
        {
            peak = i;
        }
    }
    r.peakTap = static_cast<int32_t>(peak);
    r.aPeak = mag[peak];
    const double thr = std::max(0.0, noiseK * noiseSigma);
    // Strictly positive as well as over threshold: with no noise the pre-window taps are
    // exactly zero, and a zero threshold must not call one of them the leading edge.
    if (!(r.aPeak > 0.0) || r.aPeak < thr)
    {
        return r;
    }
    for (size_t i = 0; i <= peak; ++i)
    {
        if (mag[i] >= thr && mag[i] > 0.0)
        {
            r.detected = true;
            r.leTap = static_cast<int32_t>(i);
            r.aLeadingEdge = mag[i];
            r.score = 1.0 - mag[i] / r.aPeak;
            break;
        }
    }
    return r;
}

CaTblCirSynth::CaTblCirSynth(int64_t stream, const CaTblCirConfig& cfg)
    : m_cfg(cfg)
{
    m_u = CreateObjectWithAttributes<UniformRandomVariable>("Stream",
                                                             IntegerValue(stream),
                                                             "Min",
                                                             DoubleValue(0.0),
                                                             "Max",
                                                             DoubleValue(1.0));
}

double
CaTblCirSynth::Uniform01()
{
    // GetValue() is in [0, 1); 1 - u is in (0, 1], safe under a logarithm.
    return 1.0 - m_u->GetValue();
}

double
CaTblCirSynth::StdNormal()
{
    const double u1 = Uniform01();
    const double u2 = Uniform01();
    return std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * M_PI * u2);
}

double
CaTblCirSynth::Expo(double rate)
{
    return -std::log(Uniform01()) / rate;
}

double
CaTblCirSynth::GammaUnit(double shape)
{
    // Marsaglia & Tsang (2000). For shape < 1, Gamma(shape) = Gamma(shape + 1) * U^(1/shape).
    if (shape < 1.0)
    {
        const double g = GammaUnit(shape + 1.0);
        return g * std::pow(Uniform01(), 1.0 / shape);
    }
    const double d = shape - 1.0 / 3.0;
    const double c = 1.0 / std::sqrt(9.0 * d);
    for (;;)
    {
        double x;
        double v;
        do
        {
            x = StdNormal();
            v = 1.0 + c * x;
        } while (v <= 0.0);
        v = v * v * v;
        const double u = Uniform01();
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

uint32_t
CaTblCirSynth::Poisson(double mean)
{
    const double limit = std::exp(-mean);
    double p = 1.0;
    uint32_t k = 0;
    do
    {
        ++k;
        p *= Uniform01();
    } while (p > limit);
    return k - 1;
}

std::vector<double>
CaTblCirSynth::Synthesize(const CaTblCirModel& m,
                          double directPower,
                          double snrAccDb,
                          CaTblCirResult& info)
{
    const double ts = m_cfg.tapNs;
    const size_t nPre = static_cast<size_t>(std::lround(m_cfg.preNs / ts));
    const size_t nWin = static_cast<size_t>(std::lround(m_cfg.windowNs / ts));
    const size_t nTaps = nPre + nWin;
    std::vector<double> re(nTaps, 0.0);
    std::vector<double> im(nTaps, 0.0);

    // ---- diffuse part: one Saleh-Valenzuela realisation, the listing's loop ----------
    std::vector<double> rayT;
    std::vector<double> rayA;
    std::vector<double> rayPhi;
    double tc = (m.nlosMode == 1) ? Expo(m.clusterRate) : 0.0;
    const uint32_t nClusters = std::max<uint32_t>(1u, Poisson(m.meanClusters));
    const double rayNorm = m.rayMix * m.rayRate1 + (1.0 - m.rayMix) * m.rayRate2 + 1.0;
    for (uint32_t l = 0; l < nClusters; ++l)
    {
        const bool riseShape = (m.nlosMode == 2 && l == 0);
        const double gamma = riseShape ? m.gamma1Ns : (m.kGamma * tc + m.rayDecayNs);
        const double pCluster =
            std::exp(-tc / m.clusterDecayNs) * std::pow(10.0, m.clusterSigmaDb * StdNormal() / 10.0);
        const double trLen = 10.0 * gamma;
        double tr = 0.0;
        bool firstRay = true;
        while (tr < trLen)
        {
            double hMean;
            if (riseShape)
            {
                // eq. (22): the first component is attenuated by (1 - chi), and the PDP
                // rises with gamma_rise before decaying with gamma_1.
                hMean = pCluster * (1.0 - m.chi * std::exp(-tr / m.gammaRiseNs)) *
                        std::exp(-tr / m.gamma1Ns) * (gamma + m.gammaRiseNs) / gamma /
                        (gamma + m.gammaRiseNs * (1.0 - m.chi));
            }
            else
            {
                hMean = pCluster / gamma * std::exp(-tr / gamma) / rayNorm; // eq. (19)
            }
            double nakM = (firstRay && m.mFirst > 0.0) ? m.mFirst
                                                       : std::exp(m.m0 + m.sigmaM0 * StdNormal());
            // The listing lets a log-normal m fall below 0.5, where Nakagami is undefined;
            // floored at 0.5, the report's own rule for its sub-GHz model. Ours here.
            nakM = std::max(nakM, 0.5);
            const double power = std::max(hMean, 0.0) / nakM * GammaUnit(nakM);
            rayT.push_back(tc + tr);
            rayA.push_back(std::sqrt(power));
            rayPhi.push_back(2.0 * M_PI * Uniform01());
            firstRay = false;
            tr += (Uniform01() <= m.rayMix) ? Expo(m.rayRate1) : Expo(m.rayRate2);
        }
        tc += Expo(m.clusterRate);
    }
    double eDiffuse = 0.0;
    for (size_t k = 0; k < rayA.size(); ++k)
    {
        eDiffuse += rayA[k] * rayA[k];
    }
    const double scale = (eDiffuse > 0.0) ? 1.0 / std::sqrt(eDiffuse) : 0.0;
    for (size_t k = 0; k < rayA.size(); ++k)
    {
        const size_t idx = nPre + static_cast<size_t>(std::floor(rayT[k] / ts));
        if (idx >= nTaps)
        {
            continue; // beyond the window: dropped, stated in CaTblCirConfig::windowNs
        }
        re[idx] += scale * rayA[k] * std::cos(rayPhi[k]);
        im[idx] += scale * rayA[k] * std::sin(rayPhi[k]);
    }

    // ---- direct ray at the true time of flight -----------------------------------
    const double pd = std::max(directPower, 0.0);
    const double phiD = 2.0 * M_PI * Uniform01();
    re[nPre] += std::sqrt(pd) * std::cos(phiD);
    im[nPre] += std::sqrt(pd) * std::sin(phiD);

    // ---- accumulator noise ----------------------------------------------------------
    double sigma = 0.0;
    if (std::isfinite(snrAccDb))
    {
        sigma = std::sqrt((1.0 + pd) / std::pow(10.0, snrAccDb / 10.0));
    }
    std::vector<double> mag(nTaps, 0.0);
    for (size_t i = 0; i < nTaps; ++i)
    {
        if (sigma > 0.0)
        {
            re[i] += sigma / std::sqrt(2.0) * StdNormal();
            im[i] += sigma / std::sqrt(2.0) * StdNormal();
        }
        mag[i] = std::sqrt(re[i] * re[i] + im[i] * im[i]);
    }

    info.noiseSigma = sigma;
    info.directPower = pd;
    info.nClusters = nClusters;
    info.nRays = static_cast<uint32_t>(rayA.size());
    return mag;
}

CaTblCirResult
CaTblCirSynth::Classify(EnvironmentClass env,
                        bool nlosTruth,
                        double kFactor,
                        double nlosDb,
                        double snrAccDb)
{
    const CaTblCirModel& m = CaTblCirModelFor(env, nlosTruth);
    const double k = std::isfinite(kFactor) ? std::max(0.0, kFactor) : 0.0;
    const double obstruction = std::isfinite(nlosDb) ? std::max(0.0, nlosDb) : 0.0;
    CaTblCirResult info;
    const std::vector<double> mag =
        Synthesize(m, k * std::pow(10.0, -obstruction / 10.0), snrAccDb, info);
    CaTblCirResult r = CaTblLeadingEdge(mag, info.noiseSigma, m_cfg.noiseK);
    r.directPower = info.directPower;
    r.nClusters = info.nClusters;
    r.nRays = info.nRays;
    const double nPre = std::round(m_cfg.preNs / m_cfg.tapNs);
    r.peakDelayNs = (static_cast<double>(r.peakTap) - nPre) * m_cfg.tapNs;
    r.leDelayNs = r.detected ? (static_cast<double>(r.leTap) - nPre) * m_cfg.tapNs : 0.0;
    return r;
}

double
CaTblNominalPathLossDb(double distanceM, const PropagationParams& prop)
{
    const double d = std::max(distanceM, 1.0);
    const double c = 299792458.0;
    const double lambda = c / prop.frequencyHz;
    const double d0 = std::max(prop.referenceDistanceM, 1.0);
    const double fsplD0 = 20.0 * std::log10(4.0 * M_PI * d0 / lambda);
    const double n1 = prop.pathLossExponent.mean;
    if (prop.envClass == ENV_URBAN_DENSE || prop.envClass == ENV_URBAN_MODERATE)
    {
        const double fGHz = prop.frequencyHz / 1e9;
        const double dBreak = 1000.0;
        if (d <= dBreak)
        {
            return 28.0 + 22.0 * std::log10(d) + 20.0 * std::log10(fGHz) - 6.5;
        }
        const double plBreak = 28.0 + 22.0 * std::log10(dBreak) + 20.0 * std::log10(fGHz);
        return plBreak + 20.0 * std::log10(d / dBreak) - 6.5;
    }
    if (prop.enableDualSlope && d > prop.dualSlopeBreakpointM)
    {
        const double dBreak = prop.dualSlopeBreakpointM;
        const double plBreak = fsplD0 + 10.0 * n1 * std::log10(dBreak / d0);
        return plBreak + 10.0 * prop.pathLossExponent2.mean * std::log10(d / dBreak);
    }
    return fsplD0 + 10.0 * n1 * std::log10(d / d0);
}

double
CaTblNlosRamp(double z, double zLo, double zHi)
{
    if (!std::isfinite(z) || z <= zLo)
    {
        return 0.0;
    }
    if (z >= zHi || zHi <= zLo)
    {
        return 1.0;
    }
    return (z - zLo) / (zHi - zLo);
}

CaTblRobustRef::CaTblRobustRef(uint32_t pool, uint32_t minPool)
    : m_pool(std::max<uint32_t>(pool, 3u)),
      m_minPool(std::max<uint32_t>(std::min(minPool, pool), 3u))
{
}

bool
CaTblRobustRef::Score(double x, double& z, double& median, double& sigma) const
{
    if (m_vals.size() < m_minPool || !std::isfinite(x))
    {
        return false;
    }
    std::vector<double> v(m_vals.begin(), m_vals.end());
    const size_t mid = v.size() / 2;
    std::nth_element(v.begin(), v.begin() + mid, v.end());
    double med = v[mid];
    if (v.size() % 2 == 0)
    {
        med = 0.5 * (med + *std::max_element(v.begin(), v.begin() + mid));
    }
    for (size_t i = 0; i < v.size(); ++i)
    {
        v[i] = std::fabs(v[i] - med);
    }
    std::nth_element(v.begin(), v.begin() + mid, v.end());
    double mad = v[mid];
    if (v.size() % 2 == 0)
    {
        mad = 0.5 * (mad + *std::max_element(v.begin(), v.begin() + mid));
    }
    // 1.4826 = 1 / Phi^-1(3/4): the MAD of a Gaussian times this is its sigma. The 1e-6 dB
    // floor only keeps a pool of identical values from dividing by zero.
    median = med;
    sigma = std::max(1.4826 * mad, 1e-6);
    z = (x - med) / sigma;
    return true;
}

void
CaTblRobustRef::Add(double x)
{
    if (!std::isfinite(x))
    {
        return;
    }
    m_vals.push_back(x);
    while (m_vals.size() > m_pool)
    {
        m_vals.pop_front();
    }
}

CaTblTofRssiResidual::CaTblTofRssiResidual()
    : predRssiDbm(0.0),
      excessDb(0.0),
      valid(false)
{
}

CaTblTofRssiResidual
CaTblTofRssiExcess(double tofRangeM, double rssiDbm, double txPowerDbm, const PropagationParams& prop)
{
    CaTblTofRssiResidual s;
    if (!std::isfinite(tofRangeM) || !std::isfinite(rssiDbm) || tofRangeM <= 0.0)
    {
        return s; // nothing to cross-check
    }
    s.predRssiDbm = txPowerDbm - CaTblNominalPathLossDb(tofRangeM, prop);
    s.excessDb = s.predRssiDbm - rssiDbm;
    s.valid = true;
    return s;
}

CaTblLinkVariability::Result::Result()
    : n(0),
      sRssiDb(0.0),
      sSnrDb(0.0),
      valid(false)
{
}

CaTblLinkVariability::CaTblLinkVariability(uint32_t window, uint32_t minReadings)
    : m_window(std::max<uint32_t>(window, 2u)),
      m_minReadings(std::max<uint32_t>(minReadings, 2u))
{
}

CaTblLinkVariability::Result
CaTblLinkVariability::Push(uint32_t linkKey, double rssiDbm, double snrDb)
{
    std::deque<std::pair<double, double>>& h = m_hist[linkKey];
    h.push_back(std::make_pair(rssiDbm, snrDb));
    while (h.size() > m_window)
    {
        h.pop_front();
    }
    Result r;
    r.n = static_cast<uint32_t>(h.size());
    if (r.n < m_minReadings)
    {
        return r;
    }
    double sr = 0.0;
    double ss = 0.0;
    for (size_t i = 1; i < h.size(); ++i)
    {
        const double dr = h[i].first - h[i - 1].first;
        const double ds = h[i].second - h[i - 1].second;
        sr += dr * dr;
        ss += ds * ds;
    }
    const double denom = 2.0 * static_cast<double>(h.size() - 1);
    r.sRssiDb = std::sqrt(sr / denom);
    r.sSnrDb = std::sqrt(ss / denom);
    r.valid = std::isfinite(r.sRssiDb) && std::isfinite(r.sSnrDb);
    return r;
}

} // namespace ns3
