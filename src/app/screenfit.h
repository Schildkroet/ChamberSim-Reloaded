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

#include <QGuiApplication>
#include <QRect>
#include <QScreen>
#include <QWidget>

#include <algorithm>

// Resizes a dialog to `size`, but never beyond the available area of the screen it will appear on (the parent's, else the primary one;
// 95 % leaves room for the window frame).
inline void resizeWithinScreen(QWidget *dialog, QSize size)
{
    const QScreen *screen =
        dialog->parentWidget() && dialog->parentWidget()->screen() ? dialog->parentWidget()->screen() : QGuiApplication::primaryScreen();
    if (screen)
    {
        const QRect area = screen->availableGeometry();
        size = size.boundedTo({static_cast<int>(area.width() * 0.95), static_cast<int>(area.height() * 0.95)});
    }
    dialog->resize(size);
}
