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

#include "appfont.h"

#include <QApplication>
#include <QFont>
#include <QSettings>

#include <algorithm>

namespace appfont
{

namespace
{
constexpr auto kKey = "ui/fontSize";

// The font of the platform as it was when the program started, so "default" stays reachable later.
const QFont &platformFont()
{
    static const QFont f = QApplication::font();
    return f;
}
}  // namespace

int clampSize(int points)
{
    return std::clamp(points, kMinSize, kMaxSize);
}

int systemSize()
{
    const double pt = platformFont().pointSizeF();
    return clampSize(pt > 0.0 ? static_cast<int>(pt + 0.5) : 10);  // pixel-sized fonts report -1
}

bool hasCustomSize()
{
    return QSettings().contains(QLatin1String(kKey));
}

int storedSize()
{
    QSettings s;
    if (!s.contains(QLatin1String(kKey))) return systemSize();
    bool ok = false;
    const int v = s.value(QLatin1String(kKey)).toInt(&ok);
    return ok ? clampSize(v) : systemSize();  // a damaged value falls back to the default
}

void storeSize(int points)
{
    QSettings s;
    const int v = clampSize(points);
    if (v == systemSize()) s.remove(QLatin1String(kKey));
    else s.setValue(QLatin1String(kKey), v);
}

int apply(QApplication &app, int points)
{
    (void)platformFont();  // remember the platform font before the first change
    const int v = clampSize(points);
    QFont f = platformFont();
    f.setPointSizeF(v);
    app.setFont(f);
    return v;
}

}  // namespace appfont
