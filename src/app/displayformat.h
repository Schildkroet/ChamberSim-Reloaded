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

// Placeholder of a table cell or label without a value.
inline QString noValueText()
{
    return QStringLiteral("–");
}

// "+1.2 %": how far `value` is from `reference`; the placeholder when either is not positive.
inline QString percentDifference(double value, double reference)
{
    if (value <= 0.0 || reference <= 0.0) return noValueText();
    return QStringLiteral("%1 %").arg(QLocale().toString(100.0 * (value / reference - 1.0), 'f', 1));
}
