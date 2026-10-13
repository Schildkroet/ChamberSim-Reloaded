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
#include <QFontMetricsF>
#include <QPainter>
#include <QPointF>
#include <QRectF>
#include <QString>

// Draws `text` rotated to read bottom-to-top, centred vertically on `anchor.y()` with its left edge at
// `anchor.x()`, on a soft background so it stays readable where it crosses a drawing.
inline void drawVerticalLabel(QPainter &p, QPointF anchor, const QString &text, const QFontMetricsF &fm, QColor background,
                              const QColor &foreground)
{
    const double w = fm.horizontalAdvance(text) + 8.0, h = fm.height() + 2.0;
    p.save();
    p.translate(anchor.x() + h / 2.0, anchor.y());
    p.rotate(-90.0);
    const QRectF r(-w / 2.0, -h / 2.0, w, h);
    background.setAlpha(225);
    p.setPen(Qt::NoPen);
    p.setBrush(background);
    p.drawRoundedRect(r, 3, 3);
    p.setPen(foreground);
    p.drawText(r, Qt::AlignCenter, text);
    p.restore();
}
