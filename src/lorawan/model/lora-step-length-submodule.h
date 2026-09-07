/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Step 3 of the backpack SHS pipeline: Weinberg step-length model with online
 * k_w calibration against trusted radio fixes.
 */

#ifndef LORA_STEP_LENGTH_SUBMODULE_H
#define LORA_STEP_LENGTH_SUBMODULE_H

#include "lora-shs-parameters.h"
#include "lora-step-detector-submodule.h"

#include <vector>

namespace ns3
{
namespace lorawan
{

/**
 * @ingroup lorawan
 *
 * A trusted radio fix, as consumed by the online k_w calibration (Step 3) and
 * by the heading submodule's theta_0 and drift-correction logic (Step 4).
 *
 * The fields are deliberately minimal; extend this to match the real RF
 * pipeline's fix message when wiring the engine into it.
 *
 * @see StepLengthSubmodule
 * @see HeadingSubmodule
 */
struct TrustedRadioFix
{
    double timestamp = 0.0; //!< Time the fix refers to, s
    double x = 0.0;         //!< Fix position x, same frame as the SHS position output, m
    double y = 0.0;         //!< Fix position y, same frame as the SHS position output, m
    bool isClean = true;    //!< CLEAN vs SUSPECT, per the Switching-Point Integrity Check
};

/**
 * @ingroup lorawan
 *
 * Weinberg step-length model (Step 3).
 *
 * Computes s_w = k_w * (a_max - a_min)^(1/4) per Weinberg, Analog Devices
 * AN-602, 2002. k_w is not a literature constant: it is calibrated online by
 * back-solving against the true distance between two consecutive trusted radio
 * fixes and smoothing the result with an exponential moving average. Until the
 * first calibration event fires, the provisional StepLengthParams::kwFallback
 * is used.
 *
 * @see StepLengthParams
 * @see TrustedRadioFix
 */
class StepLengthSubmodule
{
  public:
    /**
     * Constructor.
     *
     * @param params Step-length model parameters.
     */
    explicit StepLengthSubmodule(const StepLengthParams& params);

    /**
     * Estimate the step length for one detected step using the current k_w.
     *
     * @param step The detected step.
     * @return The estimated step length, m.
     */
    double EstimateStepLength(const StepEvent& step) const;

    /**
     * Buffer a newly detected step so a future calibration event can bracket
     * it. Call this for every StepEvent before calling OnTrustedFix with a
     * later fix, so the step is in the buffer when calibration runs.
     *
     * @param step The detected step.
     * @param theta Heading in effect when this step was applied to the
     *              position, rad. Required because calibration sums the steps
     *              as VECTORS: the chord between two fixes only equals the sum
     *              of step lengths when every step pointed the same way.
     */
    void BufferStep(const StepEvent& step, double theta);

    /**
     * Handle a new trusted radio fix.
     *
     * If a previous trusted fix exists, this brackets all buffered steps
     * between the two fixes and updates k_w. The step buffer is cleared
     * afterwards regardless of whether calibration fired, since steps taken
     * before the first-ever trusted fix cannot be calibrated against anything.
     *
     * @param fix The incoming fix.
     */
    void OnTrustedFix(const TrustedRadioFix& fix);

    /**
     * Get the k_w currently in use.
     *
     * @return The current Weinberg constant.
     */
    double GetCurrentKw() const
    {
        return m_kw;
    }

    /**
     * Report whether at least one online calibration event has fired.
     *
     * @return True if k_w has been calibrated, false if it is still the fallback.
     */
    bool HasBeenCalibrated() const
    {
        return m_hasCalibrated;
    }

  private:
    /// One buffered step, paired with the heading it was walked along.
    struct BufferedStep
    {
        StepEvent step; //!< The detected step
        double theta;   //!< Heading in effect when the step was applied, rad
    };

    StepLengthParams m_params;    //!< Step-length model parameters
    double m_kw;                  //!< Current Weinberg constant
    bool m_hasCalibrated = false; //!< True once the first calibration event has fired

    bool m_havePrevFix = false;               //!< True once a previous trusted fix is stored
    TrustedRadioFix m_prevFix;                //!< Previous trusted fix, start of the bracket
    std::vector<BufferedStep> m_bufferedSteps; //!< Steps awaiting a bracketing fix pair
};

} // namespace lorawan
} // namespace ns3

#endif // LORA_STEP_LENGTH_SUBMODULE_H
