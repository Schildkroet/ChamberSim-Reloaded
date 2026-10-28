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

#include <QLocale>
#include <QString>

#include <cmath>
#include <optional>

namespace csr
{

// Grams per grain, the exact factor of the original program (also cm3 of water per grain H2O, the unit of case capacities).
inline constexpr double kGrainToGram = 0.06479890999975;

// Share of the powder charge that moves with the bullet (Sebert factor) of new loads: the Lagrange value for the
// space-averaged pressure of a linear gas gradient, the physical model's calibration.
inline constexpr double kDefaultSebert = 0.25;

inline constexpr double kMmPerInch = 25.4;
inline constexpr double kMetersPerFoot = 0.3048;
inline constexpr double kFeetPerMeter = 1.0 / kMetersPerFoot;
inline constexpr double kPaPerBar = 1.0e5;
inline constexpr double kKelvinOffset = 273.15;  // 0 degC in K
// The temperature the powder data (Ba, ...) and the loads' defaults refer to.
inline constexpr double kPowderReferenceTemperature_C = 21.0;

// A finite number from text, or nullopt. QString::toDouble also accepts "nan" and "inf": those are rejected,
// they would pass every "> 0" check of the model and end up in the results.
[[nodiscard]] inline std::optional<double> parseFinite(const QString &text)
{
    bool ok = false;
    const double v = text.toDouble(&ok);
    return ok && std::isfinite(v) ? std::optional(v) : std::nullopt;
}

// The same for text typed by the user: the user's locale first, then the C locale ("1.5" also works in German).
[[nodiscard]] inline std::optional<double> parseFiniteLocale(const QString &text)
{
    const QString t = text.trimmed();
    bool ok = false;
    double v = QLocale().toDouble(t, &ok);
    if (!ok) v = QLocale::c().toDouble(t, &ok);
    return ok && std::isfinite(v) ? std::optional(v) : std::nullopt;
}

// A double as int within [lo, hi]: a plain static_cast is undefined for NaN and values out of the int range.
[[nodiscard]] constexpr int toIntClamped(double v, int lo, int hi) noexcept
{
    if (!(v > lo)) return lo;  // also NaN
    if (v >= hi) return hi;
    return static_cast<int>(v);
}

}  // namespace csr
