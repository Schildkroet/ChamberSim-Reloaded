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

#include <QColor>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QRect>

// The frame around the main window (when it draws its own) and around dialogs: Fusion draws none, and the window manager's frame or
// shadow is hardly visible on a dark window. 2 px in the theme's accent color, so it follows the light/dark theme and separates a
// dialog from the window behind it.
inline constexpr int kWindowBorderWidth = 2;

// Corner radius of the dialogs' own frame (DialogFrame); the main window has square corners.
inline constexpr int kDialogCornerRadius = 8;

// The border of `rect`; with a radius the corners are rounded (the area outside is left alone).
inline void paintWindowBorder(QPainter &painter, const QRect &rect, const QPalette &palette, int radius = 0)
{
    const QColor color = palette.color(QPalette::Highlight);
    const int w = kWindowBorderWidth;
    if (radius > 0)
    {
        QPainterPath ring;
        ring.addRoundedRect(QRectF(rect), radius, radius);
        ring.addRoundedRect(QRectF(rect).adjusted(w, w, -w, -w), radius - w, radius - w);  // even-odd: the area between the two
        ring.setFillRule(Qt::OddEvenFill);
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing);
        painter.fillPath(ring, color);
        painter.restore();
        return;
    }
    painter.fillRect(QRect(rect.left(), rect.top(), rect.width(), w), color);
    painter.fillRect(QRect(rect.left(), rect.bottom() - w + 1, rect.width(), w), color);
    painter.fillRect(QRect(rect.left(), rect.top() + w, w, rect.height() - 2 * w), color);
    painter.fillRect(QRect(rect.right() - w + 1, rect.top() + w, w, rect.height() - 2 * w), color);
}
