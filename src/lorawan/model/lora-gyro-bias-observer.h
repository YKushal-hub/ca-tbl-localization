/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Zero Angular Rate Update (ZARU) gyroscope bias observer for the backpack SHS
 * pipeline.
 */

#ifndef LORA_GYRO_BIAS_OBSERVER_H
#define LORA_GYRO_BIAS_OBSERVER_H

#include "lora-shs-parameters.h"

namespace ns3
{
namespace lorawan
{

/**
 * @ingroup lorawan
 *
 * Estimates the gyroscope Z-axis bias from stationary periods (ZARU).
 *
 * When the wearer is not moving, the true yaw rate is zero, so whatever the
 * gyroscope reports is its bias plus white noise. Averaging that reading over a
 * stationary period yields a bias estimate that can be subtracted during
 * heading integration. The technique is standard in pedestrian navigation,
 * where it is normally driven off foot-mounted stance detection.
 *
 * MOUNTING NOTE: at a backpack there is no zero-velocity instant during
 * walking, which is exactly why this pipeline uses SHS rather than strapdown
 * plus ZUPT. ZARU is therefore NOT applied per stride here -- it is applied
 * during genuine rest periods, when the wearer has stopped. The walking-energy
 * gate in StepDetectorSubmodule already distinguishes those, so this observer
 * consumes StepDetectorSubmodule::IsWalking rather than duplicating the test.
 *
 * A second gate on the instantaneous rate is still required: "not walking" is
 * not the same as "not rotating", since a stationary wearer can turn on the
 * spot to look around, and averaging that in would corrupt the estimate.
 *
 * @see GyroBiasObserverParams
 */
class GyroBiasObserver
{
  public:
    /**
     * Constructor.
     *
     * @param params Observer parameters.
     */
    explicit GyroBiasObserver(const GyroBiasObserverParams& params);

    /**
     * Advance the observer by one sample tick.
     *
     * @param dt Tick duration, s.
     * @param gzMeasured Raw gyro Z-axis reading, rad/s.
     * @param isWalking Whether the step detector currently classifies the
     *                  wearer as walking. Accumulation only runs when false.
     */
    void Update(double dt, double gzMeasured, bool isWalking);

    /**
     * Get the current bias estimate, for subtraction during heading
     * integration.
     *
     * @return The estimated gyro Z bias, rad/s. Exactly 0.0 until the first
     *         estimate has been adopted, so an unaided pipeline is unaffected.
     */
    double GetBiasEstimate() const
    {
        return m_bias;
    }

    /**
     * Report whether any stationary period has yet produced an estimate.
     *
     * @return True once at least one estimate has been adopted.
     */
    bool HasEstimate() const
    {
        return m_hasEstimate;
    }

    /**
     * Get the number of estimates adopted so far, i.e. how many qualifying
     * stationary dwells have completed.
     *
     * @return The adopted-estimate count.
     */
    uint32_t GetUpdateCount() const
    {
        return m_updateCount;
    }

  private:
    /// Fold the accumulated stationary mean into the running estimate.
    void AdoptAccumulatedEstimate();

    GyroBiasObserverParams m_params; //!< Observer parameters

    double m_bias = 0.0;       //!< Current bias estimate, rad/s
    bool m_hasEstimate = false; //!< True once an estimate has been adopted
    uint32_t m_updateCount = 0; //!< Number of adopted estimates

    double m_dwellS = 0.0;   //!< Stationary time accumulated toward the next estimate, s
    double m_sum = 0.0;      //!< Running sum of gyro readings over the current dwell
    uint32_t m_count = 0;    //!< Sample count over the current dwell
};

} // namespace lorawan
} // namespace ns3

#endif // LORA_GYRO_BIAS_OBSERVER_H
