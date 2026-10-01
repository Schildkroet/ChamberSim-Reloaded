#include "flags.h"

#include <QPainter>
#include <QPixmap>

#include <array>
#include <functional>

namespace flags {

namespace {

using Painter = std::function<void(QPainter &, const QRectF &)>;

void vertical(QPainter &p, const QRectF &r, const std::array<QColor, 3> &c)
{
    const double w = r.width() / 3.0;
    for (int i = 0; i < 3; ++i) p.fillRect(QRectF(r.left() + i * w, r.top(), w, r.height()), c[static_cast<std::size_t>(i)]);
}

void horizontal(QPainter &p, const QRectF &r, const std::array<QColor, 3> &c)
{
    const double h = r.height() / 3.0;
    for (int i = 0; i < 3; ++i) p.fillRect(QRectF(r.left(), r.top() + i * h, r.width(), h), c[static_cast<std::size_t>(i)]);
}

// 13 stripes and the blue canton; the stars only where there is room for them
void usa(QPainter &p, const QRectF &r)
{
    const double h = r.height() / 13.0;
    for (int i = 0; i < 13; ++i) p.fillRect(QRectF(r.left(), r.top() + i * h, r.width(), h), i % 2 ? Qt::white : QColor(0xb2, 0x22, 0x34));
    const QRectF canton(r.left(), r.top(), r.width() * 0.4, h * 7);
    p.fillRect(canton, QColor(0x3c, 0x3b, 0x6e));
    if (r.height() < 20) return;
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::white);
    const double d = canton.height() / 10.0;
    for (int row = 0; row < 5; ++row)
        for (int col = 0; col < 6; ++col)
            p.drawEllipse(QPointF(canton.left() + canton.width() * (col + 0.5) / 6.0, canton.top() + canton.height() * (row + 0.5) / 5.0), d / 2, d / 2);
}

Painter painterFor(const QString &country)
{
    if (country == u"DE") return [](QPainter &p, const QRectF &r) { horizontal(p, r, {Qt::black, QColor(0xdd, 0x00, 0x00), QColor(0xff, 0xce, 0x00)}); };
    if (country == u"FR") return [](QPainter &p, const QRectF &r) { vertical(p, r, {QColor(0x00, 0x55, 0xa4), Qt::white, QColor(0xef, 0x41, 0x35)}); };
    if (country == u"IT") return [](QPainter &p, const QRectF &r) { vertical(p, r, {QColor(0x00, 0x92, 0x46), Qt::white, QColor(0xce, 0x2b, 0x37)}); };
    if (country == u"US") return usa;
    return {};
}

QPixmap render(const Painter &paint, int width)
{
    const int height = width * 2 / 3;
    QPixmap pm(width, width);  // square: menus reserve a square icon area; the flag sits in the middle
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    const QRectF flag(0, (width - height) / 2.0, width, height);
    p.fillRect(flag, Qt::white);  // an opaque base: no half-transparent seams between the antialiased bands
    paint(p, flag);
    p.setPen(QPen(QColor(0, 0, 0, 90), 1.0));  // a thin outline keeps white parts visible on light menus
    p.setBrush(Qt::NoBrush);
    p.drawRect(flag.adjusted(0.5, 0.5, -0.5, -0.5));
    return pm;
}

}  // namespace

QIcon forLanguage(const QString &language)
{
    const QString country = language.section(u'_', 1, 1).toUpper();
    const Painter paint = painterFor(country);
    if (!paint) return {};
    QIcon icon;
    for (const int size : {16, 24, 32, 48}) icon.addPixmap(render(paint, size));
    return icon;
}

}  // namespace flags
