#pragma once

#include "dimensionlabel.h"

#include <QColor>
#include <QFontMetricsF>
#include <QPainter>
#include <QPointF>
#include <QString>

#include <algorithm>
#include <functional>

// A dimension of the drawing that is shown while the mouse rests on its entry in the table: either a length
// along the axis (from y = a to y = b, mm) or a diameter (width b at height a).
struct HoverMeasure {
    bool diameter{false};
    double a{0.0};
    double b{0.0};
    QString text;  // "L3  57.00 mm"

    [[nodiscard]] static HoverMeasure axial(double from, double to, QString text) { return {false, from, to, std::move(text)}; }
    [[nodiscard]] static HoverMeasure across(double y, double width, QString text) { return {true, y, width, std::move(text)}; }
};

// Draws the measure in the accent colour on top of the drawing. Axial measures get a dimension line to the right
// of the drawing (at most `maxX`), with thin guide lines back to the part; diameters a horizontal line.
inline void drawHoverMeasure(QPainter &p, const HoverMeasure &m, const std::function<double(double)> &X,
                             const std::function<double(double)> &Y, double dimensionX, double maxX, const QFontMetricsF &fm,
                             const QColor &accent, const QColor &background)
{
    p.save();
    p.setBrush(Qt::NoBrush);
    QPen guide(accent, 1.0, Qt::DotLine);
    QPen line(accent, 1.8);
    if (!m.diameter) {
        const double x = std::min(dimensionX, maxX - fm.height() - 6.0);
        p.setPen(guide);
        for (double y : {m.a, m.b}) p.drawLine(QPointF(X(0.0), Y(y)), QPointF(x, Y(y)));
        p.setPen(line);
        p.drawLine(QPointF(x, Y(m.a)), QPointF(x, Y(m.b)));
        for (double y : {m.a, m.b}) p.drawLine(QPointF(x - 4, Y(y)), QPointF(x + 4, Y(y)));
        drawVerticalLabel(p, QPointF(x + 5.0, (Y(m.a) + Y(m.b)) / 2.0), m.text, fm, background, accent);
    } else {
        const double y = Y(m.a), x0 = X(-m.b / 2.0), x1 = X(m.b / 2.0);
        p.setPen(line);
        p.drawLine(QPointF(x0, y), QPointF(x1, y));
        for (double x : {x0, x1}) p.drawLine(QPointF(x, y - 4), QPointF(x, y + 4));
        const double w = fm.horizontalAdvance(m.text) + 8.0, h = fm.height() + 2.0;
        QRectF r(X(0.0) - w / 2.0, y - h - 4.0, w, h);
        if (r.right() > maxX) r.moveRight(maxX);
        if (r.left() < 2.0) r.moveLeft(2.0);
        QColor bg = background;
        bg.setAlpha(225);
        p.setPen(Qt::NoPen);
        p.setBrush(bg);
        p.drawRoundedRect(r, 3, 3);
        p.setPen(accent);
        p.drawText(r, Qt::AlignCenter, m.text);
    }
    p.restore();
}
