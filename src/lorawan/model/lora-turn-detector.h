/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Yaw-based turn detection for the backpack SHS pipeline.
 */

#ifndef LORA_TURN_DETECTOR_H
#define LORA_TURN_DETECTOR_H

#include "lora-shs-parameters.h"

#include <cstdint>
#include <deque>

namespace ns3
{
namespace lorawan
{

/**
 * @ingroup lorawan
 *
 * Detects turns from the integrated heading, so the rest of the pipeline can
 * know when the wearer is turning.
 *
 * Method and thresholds from Bertoli et al., PMC5591331, which validated step
 * detection at a lower-back mount across straight AND turning walking and
 * defined a turn as "a change of the yaw angle (i.e., around the vertical axis)
 * with a magnitude >90 degrees and a duration between 0.5 and 10 s". Same sensor
 * location class as this project, which is why its rule is used in preference to
 * the foot-mounted turn literature.
 *
 * WHAT THIS IS FOR. The engine previously had no turn awareness whatsoever: the
 * ground-truth generator knew when a turn was happening, the estimator never
 * did. Turn state is exposed here for diagnostics and as the prerequisite for
 * any turn-adaptive processing. It deliberately does NOT alter step detection
 * yet -- Bertoli et al. adapt by switching wavelet kernel (gaus2 during turns,
 * db2 otherwise), which has no direct equivalent in this project's matched-filter
 * design, and inventing an adaptation would be worse than leaving the hook
 * unused. See the design doc's turning flag.
 *
 * @see TurnDetectorParams
 */
class TurnDetector
{
  public:
    /**
     * Constructor.
     *
     * @param params Turn detector parameters.
     */
    explicit TurnDetector(const TurnDetectorParams& params);

    /**
     * Feed the current heading estimate.
     *
     * @param t Simulation time, s.
     * @param theta Current integrated heading, rad.
     */
    void Update(double t, double theta);

    /**
     * Report whether the wearer is currently turning.
     *
     * @return True if a qualifying yaw change is in progress.
     */
    bool IsTurning() const
    {
        return m_isTurning;
    }

    /**
     * Get the number of distinct turns detected so far.
     *
     * @return The completed-turn count.
     */
    uint32_t GetTurnCount() const
    {
        return m_turnCount;
    }

    /**
     * Get the total time spent turning, s.
     *
     * @return Accumulated turning duration.
     */
    double GetTurningTimeS() const
    {
        return m_turningTimeS;
    }

  private:
    /// One decimated heading sample.
    struct Sample
    {
        double t;     //!< Timestamp, s
        double theta; //!< Heading, rad
    };

    TurnDetectorParams m_params;   //!< Detector parameters
    std::deque<Sample> m_history;  //!< Decimated heading history over the max window
    double m_lastSampleT = -1e9;   //!< Time of the last stored history sample, s
    double m_lastUpdateT = -1e9;   //!< Time of the previous Update call, s
    bool m_isTurning = false;      //!< Current turn state
    uint32_t m_turnCount = 0;      //!< Completed turns
    double m_turningTimeS = 0.0;   //!< Accumulated turning time, s
};

} // namespace lorawan
} // namespace ns3

#endif // LORA_TURN_DETECTOR_H
