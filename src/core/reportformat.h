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

#include <QString>

namespace csr::report
{

// Formats a number with a format string of the report language (see the original's "Format string" page):
//   #  digit if present        0  digit, or a zero            .  decimal point
//   ,  thousands separators    %  value times 100, shows "%"   (  )  literal parentheses
//   +  sign: "+" or "-"        -  "-" for negative numbers     E  scientific notation ("0.00E+00")
// Any other characters are output as they are, e.g. "#.000 mm²". Numbers always use '.' as the decimal
// point and ',' as the thousands separator, independent of the user's language (ISO style).
[[nodiscard]] QString formatNumber(double value, const QString &format);

}  // namespace csr::report
