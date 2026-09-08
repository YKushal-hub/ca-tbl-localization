/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Step 5 of the backpack SHS pipeline: per-step position update.
 */

#ifndef LORA_POSITION_UPDATE_SUBMODULE_H
#define LORA_POSITION_UPDATE_SUBMODULE_H

namespace ns3
{
namespace lorawan
{

/**
 * @ingroup lorawan
 *
 * Per-step position update (Step 5).
 *
 * Applies p_new = p_old + s_w * [cos(theta), sin(theta)] once per detected
 * step, under the standard mathematical angle convention (theta = 0 along +x,
 * positive counterclockwise). See doc/imu-shs-design.md Section 4: this
 * convention is consistent with the gyro sign convention in HeadingSubmodule
 * and with the EKF state definition, so no conversion is required at either
 * interface. There is no parameters struct, since this is a pure function of
 * its inputs.
 *
 * @see HeadingSubmodule
 * @see StepLengthSubmodule
 */
class PositionUpdateSubmodule
{
  public:
    PositionUpdateSubmodule()
        : m_x(0.0),
          m_y(0.0)
    {
    }

    /**
     * Set the position explicitly, e.g. from a trusted radio fix.
     *
     * @param x Position x, m.
     * @param y Position y, m.
     */
    void SetPosition(double x, double y)
    {
        m_x = x;
        m_y = y;
    }

    /**
     * Advance the position by one step.
     *
     * @param stepLength Step length s_w, m.
     * @param theta Heading at the time of the step, rad.
     */
    void ApplyStep(double stepLength, double theta);

    /**
     * Get the current position x.
     *
     * @return Position x, m.
     */
    double GetX() const
    {
        return m_x;
    }

    /**
     * Get the current position y.
     *
     * @return Position y, m.
     */
    double GetY() const
    {
        return m_y;
    }

  private:
    double m_x; //!< Current position x, m
    double m_y; //!< Current position y, m
};

} // namespace lorawan
} // namespace ns3

#endif // LORA_POSITION_UPDATE_SUBMODULE_H
