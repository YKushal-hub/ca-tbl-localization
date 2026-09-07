/*
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "lora-turn-detector.h"

#include <algorithm>
#include <cmath>

namespace ns3
{
namespace lorawan
{

TurnDetector::TurnDetector(const TurnDetectorParams& params)
    : m_params(params)
{
}

void
TurnDetector::Update(double t, double theta)
{
    // History is decimated rather than stored per tick. At 100 Hz a 10 s window
    // would be 1000 samples rescanned every tick; decimating to the configured
    // rate keeps the scan bounded without affecting the decision, since a turn
    // qualifying under this rule lasts at least 0.5 s.
    if (t - m_lastSampleT >= m_params.historySamplePeriodS)
    {
        m_history.push_back(Sample{t, theta});
        m_lastSampleT = t;
    }
    while (!m_history.empty() && (t - m_history.front().t) > m_params.maxTurnDurationS)
    {
        m_history.pop_front();
    }

    // A turn is a yaw change exceeding the magnitude threshold, accumulated over
    // an interval within [minTurnDurationS, maxTurnDurationS]. Scan the eligible
    // history for the largest such change.
    double maxYawChange = 0.0;
    for (const auto& s : m_history)
    {
        double elapsed = t - s.t;
        if (elapsed < m_params.minTurnDurationS || elapsed > m_params.maxTurnDurationS)
        {
            continue;
        }
        maxYawChange = std::max(maxYawChange, std::fabs(theta - s.theta));
    }

    // Hysteresis. A bare threshold chatters when the real turn magnitude sits
    // near it, which is exactly the case for 90 degree turns against a 90 degree
    // threshold: the state flips repeatedly through one physical turn and each
    // re-entry counts as a new turn. Enter on the threshold, leave only once the
    // change has fallen well below it. See TurnDetectorParams::turnExitFraction.
    bool turning;
    if (m_isTurning)
    {
        turning = maxYawChange > (m_params.turnExitFraction * m_params.turnMagnitudeRad);
    }
    else
    {
        turning = maxYawChange > m_params.turnMagnitudeRad;
    }

    if (turning && !m_isTurning)
    {
        ++m_turnCount;
    }
    if (turning && m_lastUpdateT > -1e8)
    {
        m_turningTimeS += (t - m_lastUpdateT);
    }
    m_isTurning = turning;
    m_lastUpdateT = t;
}

} // namespace lorawan
} // namespace ns3
