/*
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "lora-position-update-submodule.h"

#include <cmath>

namespace ns3
{
namespace lorawan
{

void
PositionUpdateSubmodule::ApplyStep(double stepLength, double theta)
{
    m_x += stepLength * std::cos(theta);
    m_y += stepLength * std::sin(theta);
}

} // namespace lorawan
} // namespace ns3
