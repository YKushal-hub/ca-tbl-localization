/*
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "lora-step-detector-submodule.h"

#include <algorithm>
#include <cmath>

namespace ns3
{
namespace lorawan
{

// ---------------------------------------------------------------------------
// OnePoleFilter
// ---------------------------------------------------------------------------

void
OnePoleFilter::ConfigureLowPass(double cutoffHz, double sampleRateHz)
{
    double dt = 1.0 / sampleRateHz;
    double rc = 1.0 / (2.0 * M_PI * cutoffHz);
    m_alpha = dt / (rc + dt);
    m_mode = LOWPASS;
    m_initialized = false;
}

void
OnePoleFilter::ConfigureHighPass(double cutoffHz, double sampleRateHz)
{
    double dt = 1.0 / sampleRateHz;
    double rc = 1.0 / (2.0 * M_PI * cutoffHz);
    m_alpha = rc / (rc + dt);
    m_mode = HIGHPASS;
    m_initialized = false;
}

double
OnePoleFilter::Process(double x)
{
    if (!m_initialized)
    {
        m_yPrev = (m_mode == LOWPASS) ? x : 0.0;
        m_xPrev = x;
        m_initialized = true;
        return m_yPrev;
    }
    double y;
    if (m_mode == LOWPASS)
    {
        y = m_yPrev + m_alpha * (x - m_yPrev);
    }
    else
    {
        y = m_alpha * (m_yPrev + x - m_xPrev);
    }
    m_yPrev = y;
    m_xPrev = x;
    return y;
}

// ---------------------------------------------------------------------------
// StepDetectorSubmodule
// ---------------------------------------------------------------------------

StepDetectorSubmodule::StepDetectorSubmodule(const StepDetectorParams& params, double sampleRateHz)
    : m_params(params),
      m_sampleRateHz(sampleRateHz),
      m_dt(1.0 / sampleRateHz)
{
    // Cascade: high-pass at bandpassLowHz, then low-pass at bandpassHighHz,
    // approximating the 0.5-3 Hz band-pass. See the OnePoleFilter header
    // comment for the documented simplification.
    m_hpVert.ConfigureHighPass(m_params.bandpassLowHz, sampleRateHz);
    m_lpVert.ConfigureLowPass(m_params.bandpassHighHz, sampleRateHz);
    m_hpAp.ConfigureHighPass(m_params.bandpassLowHz, sampleRateHz);
    m_lpAp.ConfigureLowPass(m_params.bandpassHighHz, sampleRateHz);
}

std::vector<StepEvent>
StepDetectorSubmodule::Step(double t, double vertAccel, double apAccel)
{
    double filtVert = m_lpVert.Process(m_hpVert.Process(vertAccel));
    double filtAp = m_lpAp.Process(m_hpAp.Process(apAccel));

    m_rawVertBuf.push_back(vertAccel);
    m_filtVertBuf.push_back(filtVert);
    m_filtApBuf.push_back(filtAp);
    m_timeBuf.push_back(t);

    size_t windowSamples = static_cast<size_t>(m_params.windowLengthS * m_sampleRateHz);
    while (m_rawVertBuf.size() > windowSamples)
    {
        m_rawVertBuf.pop_front();
        m_filtVertBuf.pop_front();
        m_filtApBuf.pop_front();
        m_timeBuf.pop_front();
    }

    std::vector<StepEvent> emitted;
    if (m_rawVertBuf.size() < windowSamples)
    {
        return emitted; // not enough data yet to evaluate a full window
    }

    // Evaluate a trailing window once per second (hop), not every sample -- an
    // implementation choice to bound compute cost, not individually cited.
    if (t - m_lastWindowProcessTime >= 1.0)
    {
        m_lastWindowProcessTime = t;
        emitted = ProcessWindow();
    }
    return emitted;
}

std::vector<StepEvent>
StepDetectorSubmodule::ProcessWindow()
{
    std::vector<StepEvent> result;
    size_t n = m_filtVertBuf.size();

    // Walking-energy gate. The matched filter and the 2-15 steps-per-window
    // rule both constrain how MANY peaks a window has, never how BIG they are,
    // so neither can tell walking from the sensor noise floor -- correlating
    // noise against the template still produces peaks, at roughly the rhythm
    // the separation floor allows. Reject quiet windows outright before any
    // peak-finding runs. See StepDetectorParams::walkingRmsThresholdMps2.
    //
    // The gate uses the combined VERTICAL + ANTERIOR-POSTERIOR energy, which is
    // where Iluz's use of both channels enters this implementation. The primary
    // paper is paywalled and the algorithm here was reconstructed from
    // secondary sources, so the split of responsibility is an explicit
    // interpretation: both channels decide WHETHER this window is walking,
    // while the vertical channel alone decides WHERE the steps are. Step
    // instants must come from the vertical channel regardless, because the
    // Weinberg handoff needs the vertical valley-peak-valley pair (Ho et al.
    // 2016), and driving peak-finding off a combined signal risks
    // double-counting when the two channels run out of phase.
    double sumSqVert = 0.0;
    double sumSqAp = 0.0;
    for (size_t i = 0; i < n; ++i)
    {
        sumSqVert += m_filtVertBuf[i] * m_filtVertBuf[i];
        sumSqAp += m_filtApBuf[i] * m_filtApBuf[i];
    }
    double windowRms = std::sqrt((sumSqVert + sumSqAp) / static_cast<double>(n));
    m_isWalking = (windowRms >= m_params.walkingRmsThresholdMps2);
    if (!m_isWalking)
    {
        return result; // not walking
    }

    // Build a 2 Hz sinusoidal template the same length as the window.
    std::vector<double> templateSig(n);
    for (size_t i = 0; i < n; ++i)
    {
        templateSig[i] = std::sin(2.0 * M_PI * m_params.templateFreqHz * (i * m_dt));
    }

    // Correlation of the filtered signal against the template, evaluated at
    // each sample offset over a local window (a simplified matched-filter
    // response, not a full linear convolution -- sufficient for finding
    // rhythmic local maxima, which is what Iluz's method actually requires).
    std::vector<double> filtVec(m_filtVertBuf.begin(), m_filtVertBuf.end());
    std::vector<double> corrOut(n, 0.0);
    size_t halfCycleSamples = static_cast<size_t>(m_sampleRateHz / (2.0 * m_params.templateFreqHz));
    for (size_t i = halfCycleSamples; i + halfCycleSamples < n; ++i)
    {
        double acc = 0.0;
        for (size_t k = 0; k < 2 * halfCycleSamples; ++k)
        {
            acc += filtVec[i - halfCycleSamples + k] * templateSig[k];
        }
        corrOut[i] = acc;
    }

    double minSeparationSamples = kMinInterStepS * m_sampleRateHz;
    std::vector<double> peakIndices = FindLocalMaxima(corrOut, minSeparationSamples);

    uint32_t stepCount = static_cast<uint32_t>(peakIndices.size());
    bool windowIsGait =
        (stepCount >= m_params.minStepsPerWindow) && (stepCount <= m_params.maxStepsPerWindow);
    if (!windowIsGait)
    {
        return result;
    }

    for (double idxD : peakIndices)
    {
        size_t idx = static_cast<size_t>(idxD);
        double stepTime = m_timeBuf[idx];
        if (stepTime - m_lastEmittedStepTime < kMinInterStepS)
        {
            continue; // already emitted from a previous overlapping window
        }
        double aMax;
        double aMin;
        if (ExtractValleyPeakValley(idx, aMax, aMin))
        {
            result.push_back(StepEvent{stepTime, aMax, aMin});
            m_lastEmittedStepTime = stepTime;
        }
    }
    return result;
}

// Two passes, following PMC5591331: collect every adequately separated local
// maximum, then keep only those exceeding a fraction of the MEAN candidate
// magnitude. A relative test is what lets one threshold work across slow and
// fast walking; the absolute compare this replaces could not, and in practice
// rejected nothing at all.
std::vector<double>
StepDetectorSubmodule::FindLocalMaxima(const std::vector<double>& signal,
                                       double minSeparationSamples) const
{
    std::vector<double> candidates;
    double lastPeakIdx = -1e9;
    double magnitudeSum = 0.0;
    for (size_t i = 1; i + 1 < signal.size(); ++i)
    {
        bool isLocalMax = signal[i] > signal[i - 1] && signal[i] >= signal[i + 1];
        bool positive = signal[i] > 0.0;
        bool separated = (static_cast<double>(i) - lastPeakIdx) >= minSeparationSamples;
        if (isLocalMax && positive && separated)
        {
            candidates.push_back(static_cast<double>(i));
            magnitudeSum += signal[i];
            lastPeakIdx = static_cast<double>(i);
        }
    }

    if (candidates.empty())
    {
        return candidates;
    }

    double threshold =
        m_params.peakRelativeThreshold * (magnitudeSum / static_cast<double>(candidates.size()));

    std::vector<double> peaks;
    peaks.reserve(candidates.size());
    for (double idxD : candidates)
    {
        if (signal[static_cast<size_t>(idxD)] > threshold)
        {
            peaks.push_back(idxD);
        }
    }
    return peaks;
}

// Ho et al. (2016): one step = one valley->peak->valley cycle of vertical
// acceleration. Search outward from the detected peak index, in the RAW
// (unfiltered) vertical signal, for the nearest local minima on each side.
bool
StepDetectorSubmodule::ExtractValleyPeakValley(size_t peakIndexInBuf,
                                               double& aMax,
                                               double& aMin) const
{
    if (peakIndexInBuf == 0 || peakIndexInBuf + 1 >= m_rawVertBuf.size())
    {
        return false;
    }
    std::vector<double> raw(m_rawVertBuf.begin(), m_rawVertBuf.end());

    // Refine the peak to the actual local maximum of the raw signal nearby.
    size_t searchRadius = static_cast<size_t>(0.3 * m_sampleRateHz); // +/-0.3 s search
    size_t loSearch = (peakIndexInBuf > searchRadius) ? peakIndexInBuf - searchRadius : 0;
    size_t hiSearch = std::min(peakIndexInBuf + searchRadius, raw.size() - 1);
    size_t peakIdx = loSearch;
    for (size_t i = loSearch; i <= hiSearch; ++i)
    {
        if (raw[i] > raw[peakIdx])
        {
            peakIdx = i;
        }
    }

    // Walk left from the peak to the preceding valley (local minimum): keep
    // moving while the signal is still descending toward it.
    size_t leftValley = peakIdx;
    while (leftValley > 0 && raw[leftValley - 1] < raw[leftValley])
    {
        --leftValley;
    }
    // Walk right from the peak to the following valley, same logic.
    size_t rightValley = peakIdx;
    while (rightValley + 1 < raw.size() && raw[rightValley + 1] < raw[rightValley])
    {
        ++rightValley;
    }

    aMax = raw[peakIdx];
    aMin = std::min(raw[leftValley], raw[rightValley]);
    return true;
}

} // namespace lorawan
} // namespace ns3
