#pragma once

#include <numbers>

namespace csr
{

// Effective cross-section of a rifled barrel: the bore (lands diameter F) plus the grooves (N grooves of width b, cut down to the
// groove diameter Z), Q = pi/4 F^2 + N b (Z - F) / 2. Reproduces the Q of the C.I.P. sheets within 0.1 % (median of 396 calibers, at most 1.8 %).
// All lengths in mm, the result in mm^2. A smooth bore (no grooves, or Z <= F) gives the bore area.
[[nodiscard]] constexpr double boreCrossSection_mm2(double bore_mm, double groove_mm, double grooveCount, double grooveWidth_mm) noexcept
{
    const double lands = std::numbers::pi / 4.0 * bore_mm * bore_mm;
    const double depth = groove_mm > bore_mm ? (groove_mm - bore_mm) / 2.0 : 0.0;
    return lands + (grooveCount > 0.0 ? grooveCount : 0.0) * (grooveWidth_mm > 0.0 ? grooveWidth_mm : 0.0) * depth;
}

}  // namespace csr
