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

#include <QLatin1String>
#include <QMap>
#include <QPointF>
#include <QString>

#include <algorithm>
#include <vector>

// Helpers shared by the cartridge and chamber drawings.

// A dimension of the C.I.P. record by name; 0 = not applicable.
inline double dim(const QMap<QString, double> &d, const char *key)
{
    return d.value(QLatin1String(key), 0.0);
}

// Radius of a polyline of (radius, y) points at height y.
inline double radiusAt(const std::vector<QPointF> &pts, double y)
{
    for (std::size_t i = 1; i < pts.size(); ++i)
    {
        const QPointF &a = pts[i - 1], &b = pts[i];
        if (y >= a.y() && y <= b.y())
        {
            if (b.y() == a.y()) return std::max(a.x(), b.x());
            return a.x() + (b.x() - a.x()) * (y - a.y()) / (b.y() - a.y());
        }
    }
    return pts.empty() ? 0.0 : pts.back().x();
}
