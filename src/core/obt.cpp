/*
 * This file is part of ChamberSim Reloaded.
 *
 * Copyright (C) 2026 Schildkroet
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "obt.h"

#include "numeric.h"

#include <cmath>

namespace csr
{

namespace
{

struct Constants
{
    double a, b, c, d;
};
// Long's master model: odd and even nodes
constexpr Constants kOdd{4.42642857E-03, 2.84942857E-02, -3.18785714E-03, 2.91180952E-02};
constexpr Constants kEven{4.40803571E-03, 2.68380952E-02, -2.40148810E-03, 4.39015873E-02};
}  // namespace

double obtTime_ms(double node, double barrelLength_mm)
{
    const Constants &k = static_cast<long long>(std::floor(node)) % 2 == 0 ? kEven : kOdd;
    return (k.a * node + k.b) * (barrelLength_mm / kMmPerInch) + k.c * node + k.d;
}

std::optional<ObtNode> nearestObtNode(double bulletLeadTime_ms, double barrelLength_mm, bool halfNodes)
{
    if (!(barrelLength_mm > 0.0) || !(bulletLeadTime_ms > 0.0)) return std::nullopt;
    const double step = halfNodes ? 0.5 : 1.0;
    ObtNode best{0.0, obtTime_ms(0.0, barrelLength_mm)};
    for (double node = step; node <= kObtMaxNode; node += step)
    {
        const double t = obtTime_ms(node, barrelLength_mm);
        if (std::abs(t - bulletLeadTime_ms) < std::abs(best.time_ms - bulletLeadTime_ms)) best = {node, t};
    }
    return best;
}

}  // namespace csr
