/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Steps 1-2 of the backpack SHS pipeline: Iluz walk/step detection followed by
 * Ho et al. valley-peak-valley per-step window extraction.
 */

#ifndef LORA_STEP_DETECTOR_SUBMODULE_H
#define LORA_STEP_DETECTOR_SUBMODULE_H

#include "lora-shs-parameters.h"

#include <deque>
#include <vector>

namespace ns3
{
namespace lorawan
{

/**
 * @ingroup lorawan
 *
 * One accepted step, carrying exactly the features Weinberg's step-length
 * formula consumes.
 *
 * @see StepDetectorSubmodule
 * @see StepLengthSubmodule
 */
struct StepEvent
{
    double timestamp = 0.0; //!< Simulation time of the detected step, s
    double aMax = 0.0;      //!< Peak of the valley-peak-valley cycle, m/s^2 (Ho et al. 2016)
    double aMin = 0.0;      //!< Valley bounding the cycle, m/s^2 (Ho et al. 2016)
};

/**
 * @ingroup lorawan
 *
 * Simple one-pole IIR building block, cascaded as high-pass then low-pass to
 * approximate the 0.5-3 Hz band-pass.
 *
 * IMPLEMENTATION SIMPLIFICATION: the design specifies "band-pass 0.5-3 Hz"
 * (Iluz et al., 2014) without prescribing a filter order or topology. A
 * second-order Butterworth biquad would be a closer match to typical published
 * implementations; this one-pole cascade is a documented simplification and is
 * the first thing to refine if detector accuracy against synthetic data proves
 * insufficient.
 */
class OnePoleFilter
{
  public:
    /**
     * Configure this instance as a one-pole low-pass filter.
     *
     * @param cutoffHz Cutoff frequency, Hz.
     * @param sampleRateHz Sample rate, Hz.
     */
    void ConfigureLowPass(double cutoffHz, double sampleRateHz);

    /**
     * Configure this instance as a one-pole high-pass filter.
     *
     * @param cutoffHz Cutoff frequency, Hz.
     * @param sampleRateHz Sample rate, Hz.
     */
    void ConfigureHighPass(double cutoffHz, double sampleRateHz);

    /**
     * Filter one input sample.
     *
     * @param x Input sample.
     * @return The filtered output sample.
     */
    double Process(double x);

  private:
    /// Filter mode selected by the Configure* methods.
    enum Mode
    {
        LOWPASS, //!< One-pole low-pass
        HIGHPASS //!< One-pole high-pass
    };

    Mode m_mode = LOWPASS;   //!< Current filter mode
    double m_alpha = 0.0;    //!< Smoothing coefficient derived from cutoff and sample rate
    double m_yPrev = 0.0;    //!< Previous output sample
    double m_xPrev = 0.0;    //!< Previous input sample
    bool m_initialized = false; //!< True once the first sample has primed the filter state
};

/**
 * @ingroup lorawan
 *
 * Full Step 1 (Iluz walk/step detection) plus Step 2 (Ho et al. per-step window
 * extraction) pipeline.
 *
 * Step 1, in order: band-pass the vertical and anterior-posterior acceleration
 * to 0.5-3 Hz, slide a 5 s window, convolve with a 2 Hz sinusoidal template,
 * take local maxima of the convolution as candidate steps, and accept the
 * window as gait only if it contains between 2 and 15 steps.
 *
 * Step 2 then defines one step as one valley-peak-valley cycle of the raw
 * vertical acceleration and extracts the (a_max, a_min) pair from it.
 *
 * @see StepDetectorParams
 * @see StepEvent
 */
class StepDetectorSubmodule
{
  public:
    /**
     * Constructor.
     *
     * @param params Step detector parameters.
     * @param sampleRateHz Sample rate of the incoming acceleration stream, Hz.
     */
    StepDetectorSubmodule(const StepDetectorParams& params, double sampleRateHz);

    /**
     * Feed one sample tick.
     *
     * @param t Simulation time of this sample, s.
     * @param vertAccel Vertical acceleration component, m/s^2.
     * @param apAccel Anterior-posterior acceleration component, m/s^2.
     * @return Any step events newly accepted as a result of this sample.
     *         Usually empty, since windows are only evaluated once per second.
     */
    std::vector<StepEvent> Step(double t, double vertAccel, double apAccel);

    /**
     * Report whether the most recently evaluated window looked like walking.
     *
     * Driven by the combined vertical + anterior-posterior energy gate. False
     * before the first full window has been evaluated, and false whenever the
     * wearer is stationary -- which is what makes it usable as the stillness
     * trigger for zero-angular-rate gyro bias estimation.
     *
     * @return True if the last evaluated window passed the walking-energy gate.
     */
    bool IsWalking() const
    {
        return m_isWalking;
    }

  private:
    /**
     * Evaluate the current trailing window: convolve against the template,
     * apply the gait classification rule, and extract accepted steps.
     *
     * @return The step events accepted from this window.
     */
    std::vector<StepEvent> ProcessWindow();

    /**
     * Find prominent, adequately separated local maxima of a signal.
     *
     * @param signal Signal to scan.
     * @param minSeparationSamples Minimum spacing between accepted maxima, samples.
     * @return Indices of the accepted local maxima, ascending.
     */
    std::vector<double> FindLocalMaxima(const std::vector<double>& signal,
                                        double minSeparationSamples) const;

    /**
     * Extract the valley-peak-valley features around a detected peak, working
     * on the raw (unfiltered) vertical signal because Weinberg's formula wants
     * real acceleration magnitude, not the band-passed detection signal.
     *
     * @param peakIndexInBuf Index of the detected peak within the rolling buffer.
     * @param [out] aMax Peak value of the cycle, m/s^2.
     * @param [out] aMin Bounding valley value of the cycle, m/s^2.
     * @return True if a usable cycle was found, false otherwise.
     */
    bool ExtractValleyPeakValley(size_t peakIndexInBuf, double& aMax, double& aMin) const;

    StepDetectorParams m_params; //!< Step detector parameters
    double m_sampleRateHz;       //!< Sample rate, Hz
    double m_dt;                 //!< Sample period, s

    OnePoleFilter m_hpVert; //!< High-pass stage, vertical channel
    OnePoleFilter m_lpVert; //!< Low-pass stage, vertical channel
    OnePoleFilter m_hpAp;   //!< High-pass stage, anterior-posterior channel
    OnePoleFilter m_lpAp;   //!< Low-pass stage, anterior-posterior channel

    std::deque<double> m_rawVertBuf;  //!< Rolling raw vertical accel, for valley/peak extraction
    std::deque<double> m_filtVertBuf; //!< Rolling filtered vertical accel, for the Iluz convolution
    std::deque<double> m_filtApBuf;   //!< Rolling filtered anterior-posterior accel, for the gate
    std::deque<double> m_timeBuf;     //!< Rolling sample timestamps, s

    bool m_isWalking = false; //!< Whether the last evaluated window passed the energy gate

    double m_lastWindowProcessTime = -1e9; //!< Time of the last window evaluation, s
    double m_lastEmittedStepTime = -1e9;   //!< Time of the last emitted step, s

    /// Physical floor on inter-step time, i.e. a ceiling of ~4 steps/s.
    static constexpr double kMinInterStepS = 0.25;
};

} // namespace lorawan
} // namespace ns3

#endif // LORA_STEP_DETECTOR_SUBMODULE_H
