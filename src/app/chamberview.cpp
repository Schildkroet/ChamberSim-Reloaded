#include "chamberview.h"

#include "dimensionlabel.h"
#include "hovermeasure.h"
#include "numeric.h"
#include "unitprefs.h"

using namespace Qt::StringLiterals;

#include <QLinearGradient>
#include <QLocale>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

#include <algorithm>
#include <cmath>

namespace
{

double dim(const QMap<QString, double> &d, const char *key)
{
    return d.value(QLatin1String(key), 0.0);
}

// Radius of a polyline of (radius, y) points at height y.
double radiusAt(const std::vector<QPointF> &pts, double y)
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

}  // namespace

ChamberView::ChamberView(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(300, 340);
    setMouseTracking(true);
}

void ChamberView::mouseMoveEvent(QMouseEvent *event)
{
    QString key;
    for (const RowHit &h : m_rowHits)
        if (h.rect.contains(event->position())) key = h.key;
    if (key != m_hoverKey)
    {
        m_hoverKey = key;
        update();
    }
}

void ChamberView::leaveEvent(QEvent *)
{
    if (!m_hoverKey.isEmpty())
    {
        m_hoverKey.clear();
        update();
    }
}

std::optional<HoverMeasure> ChamberView::hoverMeasure(const QString &key, const QString &label, const QString &valueText) const
{
    const auto d = [&](const char *k) { return dim(m_dims, k); };
    const QString text = label + QStringLiteral("  ") + valueText;
    const double L3 = d("c_L3"), h = std::max(d("c_h_"), 0.0);
    // lengths along the axis, from the breech face
    if (key == u"c_L1") return HoverMeasure::axial(0.0, d("c_L1"), text);
    if (key == u"c_L2") return HoverMeasure::axial(0.0, d("c_L2"), text);
    if (key == u"c_L3") return HoverMeasure::axial(0.0, L3, text);
    if (key == u"c_R") return HoverMeasure::axial(0.0, d("c_R"), text);
    if (key == u"c_E") return HoverMeasure::axial(0.0, d("c_E"), text);
    if (key == u"c_h_") return HoverMeasure::axial(L3, L3 + h, text);
    if (key == u"c_G") return HoverMeasure::axial(L3 + h, L3 + h + d("c_G"), text);
    // diameters
    if (key == u"c_R1") return HoverMeasure::across(std::max(d("c_R"), 0.0) / 2.0, d("c_R1"), text);
    if (key == u"c_P1") return HoverMeasure::across(std::max(d("c_E"), d("c_R")) + 1.0, d("c_P1"), text);
    if (key == u"c_P2") return HoverMeasure::across(d("c_L1"), d("c_P2"), text);
    if (key == u"c_H1") return HoverMeasure::across(d("c_L2"), d("c_H1"), text);
    if (key == u"c_H2") return HoverMeasure::across(L3, d("c_H2"), text);
    if (key == u"c_G1") return HoverMeasure::across(L3 + h, d("c_G1"), text);
    if (key == u"c_F") return HoverMeasure::across(L3 + h + d("c_G") + 1.0, d("c_F"), text);
    if (key == u"c_Z") return HoverMeasure::across(L3 + h + d("c_G") + 3.0, d("c_Z"), text);
    return std::nullopt;
}

ChamberView::Geometry ChamberView::buildGeometry(const QMap<QString, double> &d, double topY)
{
    Geometry g;
    const double L1 = dim(d, "c_L1"), L2 = dim(d, "c_L2"), L3 = dim(d, "c_L3");
    const double R = dim(d, "c_R"), R1 = dim(d, "c_R1"), E = dim(d, "c_E");
    const double P1 = dim(d, "c_P1"), P2 = dim(d, "c_P2"), H1 = dim(d, "c_H1"), H2 = dim(d, "c_H2");
    const double G1 = dim(d, "c_G1"), Gdist = dim(d, "c_G"), h = dim(d, "c_h_");
    g.lands = dim(d, "c_F");
    g.grooves = dim(d, "c_Z");
    if (L3 <= 0.0 || (P1 <= 0.0 && H2 <= 0.0)) return g;

    const double bodyStart = P1 > 0.0 ? P1 : H2;
    const double head = R1 > 0.0 ? R1 : bodyStart;
    auto &c = g.cavity;
    c.push_back({0.0, 0.0});
    c.push_back({head / 2.0, 0.0});  // breech recess for the case head
    if (R > 0.0) c.push_back({head / 2.0, R});
    c.push_back({bodyStart / 2.0, std::max(R, 0.0)});
    c.push_back({bodyStart / 2.0, std::max(E, R)});
    if (L1 > E && P2 > 0.0 && H1 > 0.0 && L2 > L1)
    {  // bottleneck chamber
        c.push_back({P2 / 2.0, L1});
        c.push_back({H1 / 2.0, L2});
        c.push_back({(H2 > 0.0 ? H2 : H1) / 2.0, L3});
    }
    else
    {  // straight chamber
        c.push_back({(H2 > 0.0 ? H2 : bodyStart) / 2.0, L3});
    }
    g.throatY = L3;

    // throat: a step (or short cone) down to G1, then the leade cone to the lands diameter
    const double bore = g.lands > 0.0 ? g.lands : (G1 > 0.0 ? G1 : g.grooves);
    const double throatR = (G1 > 0.0 ? G1 : bore) / 2.0;
    c.push_back({throatR, L3 + std::max(h, 0.0)});
    if (Gdist > 0.0 && bore > 0.0) c.push_back({bore / 2.0, L3 + std::max(h, 0.0) + Gdist});
    else if (bore > 0.0) c.push_back({bore / 2.0, L3 + std::max(h, 0.0) + 1.0});

    // cut at the top of the drawing
    if (topY > c.back().y()) c.push_back({c.back().x(), topY});
    else
    {
        const double r = radiusAt(c, topY);
        while (c.size() > 1 && c.back().y() > topY) c.pop_back();
        c.push_back({r, topY});
    }
    g.valid = true;
    return g;
}

void ChamberView::setLoad(const grt::LoadFile &file, const grt::SimInput &input)
{
    m_dims.clear();
    for (const grt::Entry &e : file.caliberFile.entries())
    {
        if (const auto v = grt::parseFinite(e.value)) m_dims.insert(e.name, *v);
    }
    m_round = CartridgeView::makeRound(file, input);
    m_topY = std::max({m_round.bulletTip() + 4.0, dim(m_dims, "c_L3") + 12.0, dim(m_dims, "L6")});
    m_geometry = buildGeometry(m_dims, m_topY);
    m_hasLoad = true;
    update();
}

void ChamberView::setTableVisible(bool visible)
{
    m_showTable = visible;
    update();
}

void ChamberView::clear()
{
    m_hasLoad = false;
    m_geometry = {};
    update();
}

void ChamberView::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), palette().base());
    if (!m_hasLoad || !m_geometry.valid)
    {
        p.setPen(palette().color(QPalette::PlaceholderText));
        p.drawText(rect(), Qt::AlignCenter, tr("No chamber data"));
        return;
    }
    const bool dark = palette().color(QPalette::Window).lightness() < 128;
    const double L3 = dim(m_dims, "c_L3");
    const double halfWidth = std::max({dim(m_dims, "c_R1"), dim(m_dims, "c_P1"), 12.0}) / 2.0 + 7.0;

    const QRectF all = QRectF(rect()).adjusted(8, 8, -8, -8);
    const double fontScale = m_showTable ? 1.0 : 1.7;
    QFont legendFont = font();
    legendFont.setPointSizeF(std::max(8.0, font().pointSizeF()) * fontScale);
    const double legendHeight = m_showTable ? 0.0 : QFontMetricsF(legendFont).height() + 12.0;
    const QRectF drawArea(all.left(), all.top(), m_showTable ? all.width() * 0.58 : all.width(), all.height() - legendHeight);
    const QRectF tableArea(drawArea.right() + 6, all.top(), all.width() - drawArea.width() - 6, all.height());
    const double scale = std::min((drawArea.height() - 30.0) / (m_topY + 6.0), (drawArea.width() - 40.0) / (2.0 * halfWidth));
    const double cx = drawArea.center().x() + 14;
    const double yPix0 = drawArea.bottom() - 14 - 4.0 * scale;  // y = 0 (breech face)
    const auto X = [&](double r) { return cx + r * scale; };
    const auto Y = [&](double y) { return yPix0 - y * scale; };

    // steel block with the cavity cut out, and the bolt behind the breech face
    const QColor steelLight = dark ? QColor(0x6c, 0x76, 0x84) : QColor(0xb4, 0xbc, 0xc6);
    const QColor steelDark = dark ? QColor(0x3a, 0x42, 0x4e) : QColor(0x8c, 0x96, 0xa4);
    QPainterPath block;
    block.addRect(QRectF(X(-halfWidth), Y(m_topY), 2 * halfWidth * scale, m_topY * scale));
    QPainterPath cavity;
    cavity.moveTo(X(0), Y(0));
    for (const QPointF &q : m_geometry.cavity) cavity.lineTo(X(q.x()), Y(q.y()));
    for (auto it = m_geometry.cavity.rbegin(); it != m_geometry.cavity.rend(); ++it) cavity.lineTo(X(-it->x()), Y(it->y()));
    cavity.closeSubpath();
    QLinearGradient steel(X(-halfWidth), 0, X(halfWidth), 0);
    steel.setColorAt(0.0, steelDark);
    steel.setColorAt(0.5, steelLight);
    steel.setColorAt(1.0, steelDark);
    p.setPen(QPen(palette().color(QPalette::Text), 0.8));
    p.setBrush(steel);
    p.drawPath(block.subtracted(cavity));
    p.setBrush(dark ? QColor(0x4a, 0x52, 0x5e) : QColor(0x9a, 0xa4, 0xb2));
    p.drawRect(QRectF(X(-halfWidth), Y(0), 2 * halfWidth * scale, 4.0 * scale));  // bolt
    p.setBrush(dark ? QColor(0x14, 0x18, 0x1e) : QColor(0xfa, 0xf8, 0xf2));
    p.setPen(Qt::NoPen);
    p.drawPath(cavity);

    // the loaded round, seated in the chamber
    CartridgeView::paintRound(p, m_round, X, Y, dark);

    // grooves: the barrel is wider over the grooves than over the lands
    if (m_geometry.grooves > m_geometry.lands && m_geometry.lands > 0.0)
    {
        p.setPen(QPen(palette().color(QPalette::Text), 0.8, Qt::DashLine));
        const double y0 = m_geometry.throatY + 1.0;
        for (double sign : {-1.0, 1.0})
            p.drawLine(QPointF(X(sign * m_geometry.grooves / 2.0), Y(y0)), QPointF(X(sign * m_geometry.grooves / 2.0), Y(m_topY)));
    }

    // dimension line with a horizontal label beside its top end (soft background, kept inside the widget)
    const QPen dimPen(palette().color(QPalette::Text), 1.0);
    QFont small = font();
    small.setPointSizeF(std::max(8.0, font().pointSizeF()) * fontScale);
    small.setBold(true);
    p.setFont(small);
    const QFontMetricsF dfm(small);
    const double xDim = drawArea.left() + 12;
    p.setPen(dimPen);
    p.drawLine(QPointF(xDim, Y(0)), QPointF(xDim, Y(L3)));
    for (double yy : {0.0, L3}) p.drawLine(QPointF(xDim - 3, Y(yy)), QPointF(xDim + 3, Y(yy)));
    {
        const grt::units::Unit &u = UnitPreferences::instance().unitFor(grt::units::Quantity::Length);
        const QString text = u"A  "_s + grt::units::format(L3, u) + u' ' + QString::fromUtf8(u.symbol);
        drawVerticalLabel(p, QPointF(xDim + 4.0, (Y(0) + Y(L3)) / 2.0), text, dfm, palette().color(QPalette::Base),
                          palette().color(QPalette::Text));
    }

    if (m_showTable)
    {
        drawTable(p, tableArea);
        for (const RowHit &hit : m_rowHits)
        {
            if (hit.key != m_hoverKey) continue;
            if (const auto m = hoverMeasure(hit.key, hit.label, hit.value))
                drawHoverMeasure(p, *m, X, Y, X(std::max(dim(m_dims, "c_R1"), dim(m_dims, "c_P1")) / 2.0) + 14.0, drawArea.right(), dfm,
                                 palette().color(QPalette::Highlight), palette().color(QPalette::Base));
        }
    }
    else
    {
        const grt::units::Unit &u = UnitPreferences::instance().unitFor(grt::units::Quantity::Length);
        const QFont f = legendFont;
        QFont bold = f;
        bold.setBold(true);
        p.setFont(bold);
        p.setPen(palette().color(QPalette::Text));
        const QRectF legend(all.left(), drawArea.bottom() + 8, all.width(), legendHeight);
        p.drawText(legend.topLeft() + QPointF(0, QFontMetricsF(bold).ascent()), u"A"_s);
        p.setFont(f);
        p.drawText(QRectF(legend.left() + 14, legend.top(), legend.width() - 14, legend.height()), Qt::AlignLeft | Qt::AlignTop,
                   tr("Chamber length %1").arg(grt::units::format(m_dims.value(u"c_L3"_s), u) + u' ' + QString::fromUtf8(u.symbol)));
    }
}

void ChamberView::drawTable(QPainter &p, const QRectF &area) const
{
    using grt::units::Quantity;
    struct Row
    {
        const char *key;
        const char *label;
        Quantity quantity;
        const char *suffix;
    };
    static const Row rows[] = {
        {"c_F", "F", Quantity::Length, ""},         {"c_Z", "Z", Quantity::Length, ""},    {"c_L1", "L1", Quantity::Length, ""},
        {"c_L2", "L2", Quantity::Length, ""},       {"c_L3", "L3", Quantity::Length, ""},  {"c_R", "R", Quantity::Length, ""},
        {"c_R1", "R1", Quantity::Length, ""},       {"c_R2", "R2", Quantity::Length, ""},  {"c_r_", "r", Quantity::Length, ""},
        {"c_R3", "R3", Quantity::Length, ""},       {"c_E", "E", Quantity::Length, ""},    {"c_P1", "P1", Quantity::Length, ""},
        {"c_P2", "P2", Quantity::Length, ""},       {"c_alpha", "α", Quantity::None, "°"}, {"c_S", "S", Quantity::Length, ""},
        {"c_r1max", "r1max", Quantity::Length, ""}, {"c_r2_", "r2", Quantity::Length, ""}, {"c_H1", "H1", Quantity::Length, ""},
        {"c_H2", "H2", Quantity::Length, ""},       {"c_G1", "G1", Quantity::Length, ""},  {"c_G", "G", Quantity::Length, ""},
        {"c_alpha1", "α1", Quantity::None, "°"},    {"c_h_", "h", Quantity::Length, ""},   {"c_s_", "s", Quantity::Length, ""},
        {"c_i_", "i", Quantity::None, "°"},         {"c_b_", "b", Quantity::Length, ""},   {"c_N", "N", Quantity::None, ""},
        {"c_u_", "u", Quantity::Length, ""},        {"c_Q", "Q", Quantity::Area, ""},
    };
    QFont f = font();
    f.setPointSizeF(std::max(7.0, font().pointSizeF() * 0.82));
    p.setFont(f);
    const QFontMetricsF fm(f);
    const double lineH = fm.height() + 1.0;
    p.setPen(palette().color(QPalette::Text));
    double y = area.top();
    m_rowHits.clear();
    {
        const grt::units::Unit &u = UnitPreferences::instance().unitFor(Quantity::Length);
        QFont bold = f;
        bold.setBold(true);
        p.setFont(bold);
        p.drawText(QPointF(area.left(), y + fm.ascent()), u"A"_s);
        p.setFont(f);
        p.drawText(
            QRectF(area.left() + 14, y, area.width() - 14, lineH), Qt::AlignLeft | Qt::AlignVCenter,
            fm.elidedText(tr("Chamber length %1").arg(grt::units::format(m_dims.value(u"c_L3"_s), u) + u' ' + QString::fromUtf8(u.symbol)),
                          Qt::ElideRight, static_cast<int>(area.width() - 14)));
        y += lineH + 4;
        p.setPen(palette().color(QPalette::PlaceholderText));
        p.drawLine(QPointF(area.left(), y), QPointF(area.right(), y));
        y += 4;
        p.setPen(palette().color(QPalette::Text));
    }
    p.drawText(QPointF(area.left(), y + fm.ascent()), tr("Chamber dimensions"));
    y += lineH + 2;
    for (const Row &r : rows)
    {
        if (y + lineH > area.bottom()) break;
        const double v = m_dims.value(QLatin1String(r.key), 0.0);
        if (v == 0.0) continue;
        QString value;
        if (r.quantity == Quantity::None)
        {
            value = QLocale().toString(v, 'f', r.suffix[0] ? 2 : 0) + QString::fromUtf8(r.suffix);
        }
        else
        {
            const grt::units::Unit &u = UnitPreferences::instance().unitFor(r.quantity);
            value = grt::units::format(v, u) + u' ' + QString::fromUtf8(u.symbol);
        }
        const QRectF rowRect(area.left(), y, area.width(), lineH);
        m_rowHits.push_back({rowRect, QString::fromLatin1(r.key), QString::fromUtf8(r.label), value});
        if (m_hoverKey == QLatin1String(r.key))
        {
            QColor hl = palette().color(QPalette::Highlight);
            hl.setAlpha(60);
            p.fillRect(rowRect, hl);
        }
        p.setPen(palette().color(QPalette::PlaceholderText));
        p.drawText(QPointF(area.left(), y + fm.ascent()), QString::fromUtf8(r.label));
        p.setPen(palette().color(QPalette::Text));
        const double valueLeft = area.left() + std::max(34.0, fm.horizontalAdvance(QStringLiteral("r1max ")));
        p.drawText(QRectF(valueLeft, y, area.right() - valueLeft, lineH), Qt::AlignLeft | Qt::AlignVCenter,
                   fm.elidedText(value, Qt::ElideRight, static_cast<int>(area.right() - valueLeft)));
        y += lineH;
    }
}
