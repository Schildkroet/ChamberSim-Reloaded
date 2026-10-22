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

#pragma once

#include <optional>

namespace csr
{

// Optimal Barrel Time (OBT) of Christopher Long (2003): the bullet lead times (10 % Pmax to the muzzle) at which the stress wave
// in the barrel leaves the muzzle diameter steady. Long's "master model" gives the time of node N (any integer, odd and even nodes
// have their own constants) for a barrel of L inches: OBT = (A*N + B)*L + C*N + D. The original program (GRT) adds half nodes
// (N + 1/2, the constants of the integer part) and offers nodes 0 .. 24.5. Reproduces the original's results (251 reference values).
struct ObtNode
{
    double node{};  // 0, 0.5, 1, ... 24.5
    double time_ms{};
};

constexpr double kObtMaxNode = 24.5;

// Time of node `node` (integer or half) for a barrel of `barrelLength_mm`.
[[nodiscard]] double obtTime_ms(double node, double barrelLength_mm);

// The node (0 .. 24.5, with or without the half nodes) whose time is nearest to the bullet lead time; none without a barrel or time.
[[nodiscard]] std::optional<ObtNode> nearestObtNode(double bulletLeadTime_ms, double barrelLength_mm, bool halfNodes = true);

}  // namespace csr
