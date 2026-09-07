/*
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "lora-gyro-bias-observer.h"

#include <cmath>

namespace ns3
{
namespace lorawan
{

GyroBiasObserver::GyroBiasObserver(const GyroBiasObserverParams& params)
    : m_params(params)
{
}

void
GyroBiasObserver::AdoptAccumulatedEstimate()
{
    if (m_count == 0)
    {
        return;
    }
    double mean = m_sum / static_cast<double>(m_count);

    if (!m_hasEstimate)
    {
        m_bias = mean; // first estimate: adopt directly, no history to blend with
        m_hasEstimate = true;
    }
    else
    {
        m_bias = m_params.emaAlpha * mean + (1.0 - m_params.emaAlpha) * m_bias;
    }
    ++m_updateCount;

    m_dwellS = 0.0;
    m_sum = 0.0;
    m_count = 0;
}

void
GyroBiasObserver::Update(double dt, double gzMeasured, bool isWalking)
{
    // Walking: the true yaw rate is unknown, so nothing can be inferred about
    // the bias. Discard any partial dwell rather than carrying it across the
    // moving period, which would mix walking samples into the average.
    if (!m_params.enabled || isWalking)
    {
        m_dwellS = 0.0;
        m_sum = 0.0;
        m_count = 0;
        return;
    }

    // Stationary but rotating: the wearer is standing still and turning on the
    // spot. The zero-rate assumption does not hold, so this is not usable.
    if (std::fabs(gzMeasured) > m_params.stillnessRateGateRadS)
    {
        m_dwellS = 0.0;
        m_sum = 0.0;
        m_count = 0;
        return;
    }

    m_dwellS += dt;
    m_sum += gzMeasured;
    ++m_count;

    // A dwell must be long enough that averaging pulls the white noise below
    // the bias being measured, otherwise the "estimate" is mostly noise. At
    // 100 Hz the gyro's per-sample noise is ~1.7e-3 rad/s against a bias
    // instability of ~4.3e-5 rad/s, so a single dwell cannot resolve the bias
    // on its own -- the EMA across successive dwells is what converges it.
    if (m_dwellS >= m_params.dwellS)
    {
        AdoptAccumulatedEstimate();
    }
}

} // namespace lorawan
} // namespace ns3
