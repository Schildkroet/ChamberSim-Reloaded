#include "cartridgeview.h"

#include "loadbuilder.h"
#include "numeric.h"
#include "dimensionlabel.h"
#include "hovermeasure.h"
#include "unitprefs.h"

#include <QLinearGradient>
#include <QLocale>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace
{

double dim(const QMap<QString, double> &d, const char *key)
{
    return d.value(QLatin1String(key), 0.0);
}

// Outer silhouette of the case (right half), from the CIP dimensions. Zero values mean "not applicable".
std::vector<QPointF> outerProfile(const QMap<QString, double> &d)
{
    const double R1 = dim(d, "R1"), E = dim(d, "E"), E1 = dim(d, "E1"), emin = dim(d, "Emin");
    const double P1 = dim(d, "P1"), P2 = dim(d, "P2"), H1 = dim(d, "H1"), H2 = dim(d, "H2");
    const double L1 = dim(d, "L1"), L2 = dim(d, "L2"), L3 = dim(d, "L3");
    std::vector<QPointF> pts;
    if (L3 <= 0.0) return pts;
    const double head = R1 > 0.0 ? R1 / 2.0 : std::max(P1, H2) / 2.0;
    const double rim = emin > 0.0 ? std::max(emin, 0.8) : 1.0;

    pts.push_back({0.0, 0.0});
    pts.push_back({head, 0.0});
    pts.push_back({head, rim});
    double bodyStartY = rim;
    if (E1 > 0.0 && E1 < R1 && E > rim)
    {  // extractor groove
        pts.push_back({E1 / 2.0, rim});
        pts.push_back({E1 / 2.0, E});
        bodyStartY = E;
    }
    const double bodyStartR = (P1 > 0.0 ? P1 : std::max(H2, R1)) / 2.0;
    pts.push_back({bodyStartR, bodyStartY});
    if (L1 > bodyStartY && P2 > 0.0 && H1 > 0.0 && L2 > L1)
    {  // bottleneck: body, shoulder, neck
        pts.push_back({P2 / 2.0, L1});
        pts.push_back({H1 / 2.0, L2});
        pts.push_back({(H2 > 0.0 ? H2 : H1) / 2.0, L3});
    }
    else
    {  // straight wall
        pts.push_back({(H2 > 0.0 ? H2 : bodyStartR * 2.0) / 2.0, L3});
    }
    return pts;
}

double radiusAt(const std::vector<QPointF> &outer, double y)
{
    for (std::size_t i = 1; i < outer.size(); ++i)
    {
        const QPointF &a = outer[i - 1], &b = outer[i];
        if (y >= a.y() && y <= b.y())
        {
            if (b.y() == a.y()) return std::max(a.x(), b.x());
            return a.x() + (b.x() - a.x()) * (y - a.y()) / (b.y() - a.y());
        }
    }
    return outer.empty() ? 0.0 : outer.back().x();
}

// Interior volume [mm3] between y = wall and `yTop` for a given wall thickness.
double cavityVolume(const std::vector<QPointF> &outer, double wall, double yTop)
{
    constexpr int N = 400;
    const double y0 = wall;
    if (yTop <= y0) return 0.0;
    const double dy = (yTop - y0) / N;
    double v = 0.0;
    for (int i = 0; i < N; ++i)
    {
        const double y = y0 + (i + 0.5) * dy;
        const double r = std::max(radiusAt(outer, y) - wall, 0.0);
        v += std::numbers::pi * r * r * dy;
    }
    return v;
}

QString fmtNumber(double v, int decimals)
{
    return QLocale().toString(v, 'f', decimals);
}

}  // namespace

CartridgeView::CartridgeView(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(300, 340);
    setMouseTracking(true);
}

void CartridgeView::mouseMoveEvent(QMouseEvent *event)
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

void CartridgeView::leaveEvent(QEvent *)
{
    if (!m_hoverKey.isEmpty())
    {
        m_hoverKey.clear();
        update();
    }
}

std::optional<HoverMeasure> CartridgeView::hoverMeasure(const QString &key, const QString &label, const QString &valueText) const
{
    const auto d = [&](const char *k) { return m_round.dim(k); };
    const QString text = label + QStringLiteral("  ") + valueText;
    const double L1 = d("L1"), L2 = d("L2"), L3 = d("L3");
    const double rim = d("Emin") > 0.0 ? d("Emin") : 1.0;
    // lengths along the axis, measured from the case head
    if (key == u"Emin") return HoverMeasure::axial(0.0, d("Emin"), text);
    if (key == u"E") return HoverMeasure::axial(0.0, d("E"), text);
    if (key == u"L1") return HoverMeasure::axial(0.0, L1, text);
    if (key == u"L2") return HoverMeasure::axial(0.0, L2, text);
    if (key == u"L3") return HoverMeasure::axial(0.0, L3, text);
    if (key == u"L6") return HoverMeasure::axial(0.0, d("L6"), text);
    // diameters, at the height where they occur
    if (key == u"R1") return HoverMeasure::across(rim / 2.0, d("R1"), text);
    if (key == u"E1") return HoverMeasure::across((rim + std::max(d("E"), rim)) / 2.0, d("E1"), text);
    if (key == u"P1") return HoverMeasure::across(std::max(d("E"), rim) + 1.0, d("P1"), text);
    if (key == u"P2") return HoverMeasure::across(L1, d("P2"), text);
    if (key == u"H1") return HoverMeasure::across(L2, d("H1"), text);
    if (key == u"H2") return HoverMeasure::across(L3, d("H2"), text);
    if (key == u"G1") return HoverMeasure::across(m_round.bulletBase() + m_round.bulletLength / 2.0, d("G1"), text);
    return std::nullopt;
}

CartridgeView::Geometry CartridgeView::buildGeometry(const QMap<QString, double> &dims, double caseVolume_cm3, double powderBulkVolume_cm3)
{
    Geometry g;
    g.outer = outerProfile(dims);
    if (g.outer.size() < 3) return g;
    const double L3 = dim(dims, "L3");
    const double target = caseVolume_cm3 * 1e3;  // mm3
    // wall thickness such that the interior up to the mouth has the case capacity
    double lo = 0.05, hi = 3.0;
    if (target > 0.0)
    {
        if (cavityVolume(g.outer, lo, L3) < target) hi = lo;       // capacity larger than the outline: thinnest wall
        else if (cavityVolume(g.outer, hi, L3) > target) lo = hi;  // thicker walls would be needed than we draw
        else
            for (int i = 0; i < 50; ++i)
            {
                const double mid = 0.5 * (lo + hi);
                (cavityVolume(g.outer, mid, L3) > target ? lo : hi) = mid;
            }
    }
    g.wall = 0.5 * (lo + hi);
    g.cavityVolume_mm3 = cavityVolume(g.outer, g.wall, L3);

    // powder fill: bulk volume stacked from the bottom
    const double powder = powderBulkVolume_cm3 * 1e3;
    double acc = 0.0;
    g.powderHeight_mm = L3;
    constexpr int N = 800;
    const double dy = (L3 - g.wall) / N;
    for (int i = 0; i < N; ++i)
    {
        const double y = g.wall + (i + 0.5) * dy;
        const double r = std::max(radiusAt(g.outer, y) - g.wall, 0.0);
        acc += std::numbers::pi * r * r * dy;
        if (acc >= powder)
        {
            g.powderHeight_mm = g.wall + (i + 1) * dy;
            break;
        }
    }
    g.valid = true;
    return g;
}

CartridgeView::RoundShape CartridgeView::makeRound(const csr::LoadFile &file, const csr::SimInput &input)
{
    RoundShape r;
    for (const csr::Entry &e : file.caliberFile.entries())
    {
        if (const auto v = csr::parseFinite(e.value)) r.dims.insert(e.name, *v);
    }
    // the load's own values win where the caliber record has none
    if (r.dims.value(QStringLiteral("L3")) <= 0.0) r.dims.insert(QStringLiteral("L3"), input.caseLength_mm);
    r.bulletDiameter = input.bulletDiameter_mm;
    r.bulletLength = input.bulletLength_mm;
    r.depth = input.seatingDepth_mm;
    r.tailLength = input.tailLength_mm;
    r.tailDiameter = input.tailType == 2 ? input.tailDiameterB_mm : input.bulletDiameter_mm;
    const double powderCm3 = input.bulkDensity_kgm3 > 0.0 ? input.charge_g / (input.bulkDensity_kgm3 * 1e-3) : 0.0;
    if (input.shotshell)
    {
        // typical proportions of a shotshell hull (12 gauge: 20.3 mm hull, 22.9 mm rim, 16 mm brass head), scaled by the bore
        const double bore = r.bulletDiameter > 0.0 ? r.bulletDiameter : 18.2;
        r.shotshell = true;
        r.hullDiameter = bore + 2.1;
        r.rimDiameter = r.hullDiameter + 2.6;
        r.headHeight = std::clamp(0.88 * bore, 9.0, 19.0);
        r.floorHeight = 0.7 * r.headHeight;  // the top of the base wad inside the head: the powder lies on it
        r.wadHeight = std::min(25.0, std::max(input.bulletLength_mm - 6.0, 10.0));
        const double inner = r.hullDiameter / 2.0 - 0.5;
        r.powderTop = r.floorHeight + (inner > 0.0 ? powderCm3 * 1e3 / (std::numbers::pi * inner * inner) : 0.0);
        r.geometry.wall = 0.5;
        r.geometry.valid = true;
        return r;
    }
    r.geometry = buildGeometry(r.dims, input.caseVolume_cm3, powderCm3);
    return r;
}

void CartridgeView::setLoad(const csr::LoadFile &file, const csr::SimInput &input)
{
    m_input = input;
    m_round = makeRound(file, input);
    m_hasLoad = true;
    update();
}

void CartridgeView::setTableVisible(bool visible)
{
    m_showTable = visible;
    update();
}

void CartridgeView::clear()
{
    m_hasLoad = false;
    m_round = {};
    update();
}

void CartridgeView::paintShotshell(QPainter &p, const RoundShape &round, const std::function<double(double)> &X,
                                   const std::function<double(double)> &Y, bool dark)
{
    const double L = round.caseLength();
    const double rh = round.hullDiameter / 2.0, rr = round.rimDiameter / 2.0, wall = 0.55;
    const double yBase = round.bulletBase(), yTip = std::min(round.bulletTip(), L);
    const double crimp = std::min(7.0, 0.1 * L);
    const QColor edge = dark ? QColor(0xc8, 0xcc, 0xd2) : QColor(0x30, 0x34, 0x3a);
    p.setPen(Qt::NoPen);

    // ---- hull: translucent plastic tube, closed by a fold crimp at the mouth ----
    QPainterPath hull;
    hull.moveTo(X(-rh), Y(round.headHeight));
    hull.lineTo(X(-rh), Y(L - crimp));
    hull.lineTo(X(-rh * 0.78), Y(L - crimp * 0.35));
    hull.lineTo(X(-rh * 0.2), Y(L));
    hull.lineTo(X(rh * 0.2), Y(L));
    hull.lineTo(X(rh * 0.78), Y(L - crimp * 0.35));
    hull.lineTo(X(rh), Y(L - crimp));
    hull.lineTo(X(rh), Y(round.headHeight));
    hull.closeSubpath();
    QLinearGradient plastic(X(-rh), 0, X(rh), 0);
    plastic.setColorAt(0.0, QColor(0xa8, 0x22, 0x22));
    plastic.setColorAt(0.35, QColor(0xe0, 0x4a, 0x40));
    plastic.setColorAt(1.0, QColor(0x90, 0x1c, 0x1c));
    p.setBrush(plastic);
    p.drawPath(hull);
    // the inside: the wall thickness cut out (the crimp folds close the mouth)
    QPainterPath inside;
    inside.moveTo(X(-rh + wall), Y(round.headHeight));
    inside.lineTo(X(-rh + wall), Y(L - crimp));
    inside.lineTo(X(-rh * 0.7), Y(L - crimp * 0.4));
    inside.lineTo(X(-rh * 0.12), Y(L - 0.25 * wall));
    inside.lineTo(X(rh * 0.12), Y(L - 0.25 * wall));
    inside.lineTo(X(rh * 0.7), Y(L - crimp * 0.4));
    inside.lineTo(X(rh - wall), Y(L - crimp));
    inside.lineTo(X(rh - wall), Y(round.headHeight));
    inside.closeSubpath();
    p.setBrush(dark ? QColor(0x18, 0x1c, 0x22) : QColor(0xf4, 0xf0, 0xe6));
    p.drawPath(inside);

    // ---- brass head with the rim, the inner base wad and the primer ----
    QPainterPath head;
    head.moveTo(X(-rr), Y(0));
    head.lineTo(X(-rr), Y(1.8));
    head.lineTo(X(-rh - 0.2), Y(1.8));
    head.lineTo(X(-rh - 0.2), Y(round.headHeight));
    head.lineTo(X(rh + 0.2), Y(round.headHeight));
    head.lineTo(X(rh + 0.2), Y(1.8));
    head.lineTo(X(rr), Y(1.8));
    head.lineTo(X(rr), Y(0));
    head.closeSubpath();
    QLinearGradient brass(X(-rr), 0, X(rr), 0);
    brass.setColorAt(0.0, QColor(0x8a, 0x68, 0x28));
    brass.setColorAt(0.35, QColor(0xe6, 0xc2, 0x62));
    brass.setColorAt(1.0, QColor(0x7a, 0x5a, 0x22));
    p.setPen(QPen(dark ? QColor(0xd8, 0xb0, 0x58) : QColor(0x70, 0x52, 0x1c), 1.0));
    p.setBrush(brass);
    p.drawPath(head);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0xd8, 0xd2, 0xc4));  // the base wad inside the head
    p.drawRect(QRectF(QPointF(X(-rh + wall), Y(round.floorHeight)), QPointF(X(rh - wall), Y(2.2))));
    p.setBrush(QColor(0x9a, 0x9d, 0xa4));  // the primer
    p.drawRoundedRect(QRectF(QPointF(X(-4.9), Y(0.6)), QPointF(X(4.9), Y(3.4))), 1.0, 1.0);

    // ---- powder ----
    p.setBrush(QBrush(QColor(0x8c, 0x8f, 0x5a, 200), Qt::Dense4Pattern));
    p.drawRect(QRectF(QPointF(X(-rh + wall), Y(std::min(round.powderTop, yBase))), QPointF(X(rh - wall), Y(round.floorHeight))));
    p.setBrush(QColor(0x7c, 0x80, 0x4c, 120));
    p.drawRect(QRectF(QPointF(X(-rh + wall), Y(std::min(round.powderTop, yBase))), QPointF(X(rh - wall), Y(round.floorHeight))));

    // ---- wad: the cushion (a zigzag column) below, the cup with the petals around the shot above ----
    const double rw = rh - wall - 0.15;
    const double cushion = round.wadHeight * 0.45, cup = round.wadHeight - cushion;
    p.setBrush(dark ? QColor(0xe8, 0xe4, 0xd0) : QColor(0xf2, 0xe9, 0xc8));
    p.setPen(QPen(edge, 0.8));
    p.drawRect(QRectF(QPointF(X(-rw), Y(yBase + 0.8)), QPointF(X(rw), Y(yBase))));  // the wad's base
    QPainterPath zigzag;
    constexpr int folds = 4;
    for (int i = 0; i <= folds * 2; ++i)
    {
        const double y = yBase + 0.8 + cushion * i / (folds * 2);
        const double r = (i % 2 == 0) ? rw : rw * 0.55;
        if (i == 0) zigzag.moveTo(X(-r), Y(y));
        else zigzag.lineTo(X(-r), Y(y));
    }
    for (int i = folds * 2; i >= 0; --i) zigzag.lineTo(X((i % 2 == 0) ? rw : rw * 0.55), Y(yBase + 0.8 + cushion * i / (folds * 2)));
    zigzag.closeSubpath();
    p.drawPath(zigzag);
    const double yCupFloor = yBase + 0.8 + cushion;
    p.drawRect(QRectF(QPointF(X(-rw), Y(yCupFloor + 0.8)), QPointF(X(rw), Y(yCupFloor))));  // the cup floor

    // ---- shot column: pellets from the cup floor to the top of the load ----
    const double yShot0 = yCupFloor + 0.8, yShot1 = std::max(yTip, yShot0);
    p.setPen(Qt::NoPen);
    p.setBrush(dark ? QColor(0x8a, 0x90, 0x9a) : QColor(0x70, 0x76, 0x80));
    p.drawRect(QRectF(QPointF(X(-rw + 0.3), Y(yShot1)), QPointF(X(rw - 0.3), Y(yShot0))));
    p.setPen(QPen(dark ? QColor(0xc4, 0xca, 0xd2) : QColor(0xa8, 0xae, 0xb8), 0.9));
    p.setBrush(Qt::NoBrush);
    const double pellet = 2.4, step = pellet * 0.9;  // #7.5 - 8 lead shot
    for (int row = 0; yShot0 + row * step * 0.87 + pellet / 2.0 <= yShot1; ++row)
    {
        const double y = yShot0 + pellet / 2.0 + row * step * 0.87;
        const double offset = (row % 2) ? step / 2.0 : 0.0;
        for (double x = -rw + pellet / 2.0 + 0.3 + offset; x <= rw - pellet / 2.0 - 0.3; x += step)
            p.drawEllipse(QRectF(QPointF(X(x - pellet / 2.0), Y(y + pellet / 2.0)), QPointF(X(x + pellet / 2.0), Y(y - pellet / 2.0))));
    }
    // the petals of the cup around the shot (to the top of the cup)
    p.setPen(QPen(edge, 0.8));
    p.setBrush(dark ? QColor(0xe8, 0xe4, 0xd0, 180) : QColor(0xf2, 0xe9, 0xc8, 200));
    const double cupTop = std::min(yCupFloor + cup, yShot1);
    p.drawRect(QRectF(QPointF(X(-rw), Y(cupTop)), QPointF(X(-rw + 0.9), Y(yCupFloor))));
    p.drawRect(QRectF(QPointF(X(rw - 0.9), Y(cupTop)), QPointF(X(rw), Y(yCupFloor))));
    // outline of the hull on top of everything
    p.setPen(QPen(dark ? QColor(0xe0, 0x6a, 0x60) : QColor(0x70, 0x14, 0x14), 1.0));
    p.setBrush(Qt::NoBrush);
    p.drawPath(hull);
}

void CartridgeView::paintRound(QPainter &p, const RoundShape &round, const std::function<double(double)> &X,
                               const std::function<double(double)> &Y, bool dark)
{
    if (round.shotshell) return paintShotshell(p, round, X, Y, dark);
    const double L3 = round.caseLength();
    const double yBase = round.bulletBase();
    const double yTip = round.bulletTip();
    // ---- case: outer silhouette filled with brass, interior cut out ----
    QPainterPath outline;
    outline.moveTo(X(0), Y(0));
    for (const QPointF &q : round.geometry.outer) outline.lineTo(X(q.x()), Y(q.y()));
    for (auto it = round.geometry.outer.rbegin(); it != round.geometry.outer.rend(); ++it) outline.lineTo(X(-it->x()), Y(it->y()));
    outline.closeSubpath();

    QPainterPath cavity;
    const double w = round.geometry.wall;
    cavity.moveTo(X(0), Y(w));
    const auto inner = [&](double y) { return std::max(radiusAt(round.geometry.outer, y) - w, 0.0); };
    constexpr int N = 120;
    for (int i = 0; i <= N; ++i)
    {
        const double y = w + (L3 - w) * i / N;
        cavity.lineTo(X(inner(y)), Y(y));
    }
    for (int i = N; i >= 0; --i)
    {
        const double y = w + (L3 - w) * i / N;
        cavity.lineTo(X(-inner(y)), Y(y));
    }
    cavity.closeSubpath();

    QLinearGradient brass(X(-round.dim("R1") / 2), 0, X(round.dim("R1") / 2), 0);
    brass.setColorAt(0.0, QColor(0x8a, 0x68, 0x28));
    brass.setColorAt(0.35, QColor(0xe6, 0xc2, 0x62));
    brass.setColorAt(1.0, QColor(0x7a, 0x5a, 0x22));
    p.setPen(QPen(dark ? QColor(0xd8, 0xb0, 0x58) : QColor(0x70, 0x52, 0x1c), 1.0));
    p.setBrush(brass);
    p.drawPath(outline);
    p.setBrush(dark ? QColor(0x18, 0x1c, 0x22) : QColor(0xf4, 0xf0, 0xe6));
    p.setPen(Qt::NoPen);
    p.drawPath(cavity);

    // ---- powder fill ----
    const double fillTop = std::min(round.geometry.powderHeight_mm, yBase);
    QPainterPath powder;
    powder.moveTo(X(0), Y(w));
    for (int i = 0; i <= N; ++i)
    {
        const double y = w + (fillTop - w) * i / N;
        powder.lineTo(X(inner(y)), Y(y));
    }
    for (int i = N; i >= 0; --i)
    {
        const double y = w + (fillTop - w) * i / N;
        powder.lineTo(X(-inner(y)), Y(y));
    }
    powder.closeSubpath();
    p.setBrush(QBrush(QColor(0x8c, 0x8f, 0x5a, 200), Qt::Dense4Pattern));
    p.drawPath(powder);
    p.setBrush(QColor(0x7c, 0x80, 0x4c, 120));
    p.drawPath(powder);

    // ---- bullet: boat tail, bearing cylinder, ogive ----
    const double rb = round.bulletDiameter / 2.0, rt = std::min(round.tailDiameter, round.bulletDiameter) / 2.0;
    const double tailH = std::min(round.tailLength, round.bulletLength * 0.4);
    const double ogiveLen = std::clamp((round.bulletLength - tailH) * 0.55, round.bulletDiameter, 3.2 * round.bulletDiameter);
    const double yOgive = yTip - ogiveLen;
    QPainterPath bullet;
    bullet.moveTo(X(-rt), Y(yBase));
    bullet.lineTo(X(rt), Y(yBase));
    bullet.lineTo(X(rb), Y(yBase + tailH));
    bullet.lineTo(X(rb), Y(yOgive));
    bullet.cubicTo(X(rb), Y(yOgive + ogiveLen * 0.55), X(rb * 0.45), Y(yTip - ogiveLen * 0.12), X(0.12), Y(yTip));
    bullet.lineTo(X(-0.12), Y(yTip));
    bullet.cubicTo(X(-rb * 0.45), Y(yTip - ogiveLen * 0.12), X(-rb), Y(yOgive + ogiveLen * 0.55), X(-rb), Y(yOgive));
    bullet.lineTo(X(-rb), Y(yBase + tailH));
    bullet.closeSubpath();
    QLinearGradient copper(X(-rb), 0, X(rb), 0);
    copper.setColorAt(0.0, QColor(0x7a, 0x3c, 0x16));
    copper.setColorAt(0.4, QColor(0xd2, 0x86, 0x4a));
    copper.setColorAt(1.0, QColor(0x6a, 0x32, 0x12));
    p.setPen(QPen(QColor(0x50, 0x26, 0x0e), 1.0));
    p.setBrush(copper);
    p.drawPath(bullet);
}

void CartridgeView::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), palette().base());
    if (!m_hasLoad || !m_round.geometry.valid)
    {
        p.setPen(palette().color(QPalette::PlaceholderText));
        p.drawText(rect(), Qt::AlignCenter, tr("No cartridge data"));
        return;
    }
    const bool dark = palette().color(QPalette::Window).lightness() < 128;
    const double L3 = m_round.caseLength();
    const double top = std::max({m_round.bulletTip(), dim(m_round.dims, "L6"), L3});

    // drawing area on the left, table on the right
    const QRectF all = QRectF(rect()).adjusted(8, 8, -8, -8);
    const double fontScale = m_showTable ? 1.0 : 1.7;  // report images are scaled down: bigger text
    QFont legendFont = font();
    legendFont.setPointSizeF(std::max(8.0, font().pointSizeF()) * fontScale);
    const double legendHeight =
        m_showTable ? 0.0 : 3.0 * (QFontMetricsF(legendFont).height() + 1.0) + 12.0;  // measures go below the drawing
    const QRectF drawArea(all.left(), all.top(), m_showTable ? all.width() * 0.58 : all.width(), all.height() - legendHeight);
    const QRectF tableArea(drawArea.right() + 6, all.top(), all.width() - drawArea.width() - 6, all.height());
    const double scale =
        std::min((drawArea.height() - 30.0) / (top + 2.0), (drawArea.width() - 80.0) / std::max(dim(m_round.dims, "R1"), 14.0));
    const double cx = drawArea.center().x() + (m_showTable ? 30 : 40);
    const double yPix0 = drawArea.bottom() - 10;  // y = 0 (case head)
    const auto X = [&](double r) { return cx + r * scale; };
    const auto Y = [&](double y) { return yPix0 - y * scale; };

    paintRound(p, m_round, X, Y, dark);
    const double rb = m_round.bulletDiameter / 2.0;
    const double yBase = m_round.bulletBase(), yTip = m_round.bulletTip();

    // ---- dimension lines: a horizontal letter at the top of each line, the values are listed in the table ----
    const QColor dimColor = palette().color(QPalette::Text);
    const QPen dimPen(dimColor, 1.0);
    QFont small = font();
    small.setPointSizeF(std::max(8.0, font().pointSizeF()) * fontScale);
    small.setBold(true);
    p.setFont(small);
    const QFontMetricsF dfm(small);
    const auto lengthText = [&](double mm)
    {
        const csr::units::Unit &u = UnitPreferences::instance().unitFor(csr::units::Quantity::Length);
        return csr::units::format(mm, u) + u' ' + QString::fromUtf8(u.symbol);
    };
    // A dimension line with a vertical label "A 79.50 mm" beside it, centred on the line. The label has a soft
    // background so it stays readable where it crosses the drawing.
    const double dimGap = dfm.height() + 10.0;  // distance between neighbouring dimension lines
    const auto vdim = [&](double xPix, double y0, double y1, const QString &letter, double mm)
    {
        p.setPen(dimPen);
        p.drawLine(QPointF(xPix, Y(y0)), QPointF(xPix, Y(y1)));
        for (double yy : {y0, y1}) p.drawLine(QPointF(xPix - 3, Y(yy)), QPointF(xPix + 3, Y(yy)));
        drawVerticalLabel(p, QPointF(xPix + 4.0, (Y(y0) + Y(y1)) / 2.0), letter + QStringLiteral("  ") + lengthText(mm), dfm,
                          palette().color(QPalette::Base), dimColor);
    };
    const double xDim = drawArea.left() + 12;
    vdim(xDim, 0, yTip, QStringLiteral("A"), yTip);
    vdim(xDim + dimGap, 0, L3, QStringLiteral("B"), L3);
    vdim(xDim + 2 * dimGap, yBase, L3, QStringLiteral("C"), m_round.depth);

    if (m_showTable)
    {
        drawTable(p, tableArea);
        if (!m_hoverKey.isEmpty())
        {
            for (const RowHit &h : m_rowHits)
            {
                if (h.key != m_hoverKey) continue;
                if (const auto m = hoverMeasure(h.key, h.label, h.value))
                    drawHoverMeasure(p, *m, X, Y, X(std::max(m_round.dim("R1"), m_round.dim("P1")) / 2.0) + 14.0, drawArea.right(), dfm,
                                     palette().color(QPalette::Highlight), palette().color(QPalette::Base));
            }
        }
    }
    else
    {
        drawMeasures(p, QRectF(all.left(), drawArea.bottom() + 8, all.width(), legendHeight), legendFont);
    }
}

double CartridgeView::drawMeasures(QPainter &p, const QRectF &area, const QFont &f) const
{
    using csr::units::Quantity;
    const QFontMetricsF fm(f);
    const double lineH = fm.height() + 1.0;
    double y = area.top();
    const auto lengthText = [&](double mm)
    {
        const csr::units::Unit &u = UnitPreferences::instance().unitFor(Quantity::Length);
        return csr::units::format(mm, u) + u' ' + QString::fromUtf8(u.symbol);
    };
    const struct
    {
        const char *letter;
        QString label;
        double value;
    } measures[] = {{"A", tr("Overall length"), m_round.bulletTip()},
                    {"B", tr("Case length"), m_round.caseLength()},
                    {"C", tr("Seating depth"), m_round.depth}};
    QFont bold = f;
    bold.setBold(true);
    for (const auto &m : measures)
    {
        p.setFont(bold);
        p.setPen(palette().color(QPalette::Text));
        p.drawText(QPointF(area.left(), y + fm.ascent()), QString::fromLatin1(m.letter));
        p.setFont(f);
        const double textLeft = area.left() + fm.horizontalAdvance(u'A') + 6;
        p.drawText(QRectF(textLeft, y, area.width(), lineH), Qt::AlignLeft | Qt::AlignVCenter,
                   fm.elidedText(QStringLiteral("%1 %2").arg(m.label, lengthText(m.value)), Qt::ElideRight,
                                 static_cast<int>(area.right() - textLeft)));
        y += lineH;
    }
    return y - area.top();
}

void CartridgeView::drawTable(QPainter &p, const QRectF &area) const
{
    using csr::units::Quantity;
    struct Row
    {
        const char *key;
        const char *label;
        Quantity quantity;
        const char *suffix;  // for angles / special units
    };
    static const Row rows[] = {
        {"Pmax", "Pmax", Quantity::Pressure, ""}, {"PK", "PK", Quantity::Pressure, ""}, {"PE", "PE", Quantity::Pressure, ""},
        {"EE", "EE", Quantity::Pressure, ""},     {"c_Q", "Q", Quantity::Area, ""},     {"V", "V", Quantity::Volume, ""},
        {"E", "E", Quantity::Length, ""},         {"E1", "E1", Quantity::Length, ""},   {"Emin", "emin", Quantity::Length, ""},
        {"G1", "G1", Quantity::Length, ""},       {"H1", "H1", Quantity::Length, ""},   {"H2", "H2", Quantity::Length, ""},
        {"L1", "L1", Quantity::Length, ""},       {"L2", "L2", Quantity::Length, ""},   {"L3", "L3", Quantity::Length, ""},
        {"L6", "L6", Quantity::Length, ""},       {"M", "M", Quantity::Length, ""},     {"P1", "P1", Quantity::Length, ""},
        {"P2", "P2", Quantity::Length, ""},       {"R", "R", Quantity::Length, ""},     {"R1", "R1", Quantity::Length, ""},
        {"r1min", "r1min", Quantity::Length, ""}, {"r2", "r2", Quantity::Length, ""},   {"S", "S", Quantity::Length, ""},
        {"alpha", "α", Quantity::None, "°"},      {"beta", "β", Quantity::None, "°"},   {"delta", "δ", Quantity::None, "°"},
    };
    QFont f = font();
    f.setPointSizeF(std::max(7.0, font().pointSizeF() * 0.82));
    p.setFont(f);
    const QFontMetricsF fm(f);
    const double lineH = fm.height() + 1.0;
    p.setPen(palette().color(QPalette::Text));
    double y = area.top();
    m_rowHits.clear();
    y += drawMeasures(p, area, f);
    p.setPen(palette().color(QPalette::PlaceholderText));
    p.drawLine(QPointF(area.left(), y), QPointF(area.right(), y));
    y += 4;
    p.setPen(palette().color(QPalette::Text));
    p.drawText(QPointF(area.left(), y + fm.ascent()), tr("CIP dimensions"));
    y += lineH + 2;
    for (const Row &r : rows)
    {
        if (y + lineH > area.bottom()) break;
        if (!m_round.dims.contains(QLatin1String(r.key)) || m_round.dims.value(QLatin1String(r.key)) == 0.0) continue;
        const double v = m_round.dims.value(QLatin1String(r.key));
        QString value;
        if (r.quantity == Quantity::None)
        {
            value = fmtNumber(v, 2) + QString::fromUtf8(r.suffix);
        }
        else
        {
            // the database stores the case capacity in grains of water
            const double base = r.quantity == Quantity::Volume ? v * csr::kGrainToGram : v;
            const csr::units::Unit &u = UnitPreferences::instance().unitFor(r.quantity);
            value = csr::units::format(base, u) + u' ' + QString::fromUtf8(u.symbol);
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
        const double valueLeft = area.left() + std::max(34.0, fm.horizontalAdvance(QStringLiteral("emin ")));
        p.drawText(QRectF(valueLeft, y, area.right() - valueLeft, lineH), Qt::AlignLeft | Qt::AlignVCenter,
                   fm.elidedText(value, Qt::ElideRight, static_cast<int>(area.right() - valueLeft)));
        y += lineH;
    }
}
