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
