#include "curvechart.h"

#include <QAreaSeries>
#include <QChart>
#include <QEvent>
#include <QGuiApplication>
#include <QLegendMarker>
#include <QLineSeries>
#include <QLocale>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QGraphicsSimpleTextItem>
#include <QValueAxis>

#include <algorithm>
#include <cmath>

namespace
{

constexpr QColor kPressure{0xe0, 0x6a, 0x6a};
constexpr QColor kVelocity{0x7a, 0x86, 0xe8};
constexpr QColor kEnergy{0x3f, 0xc1, 0xb0};
constexpr int kTicks = 6;  // identical tick counts keep the three value axes' grid lines aligned

double niceCeil(double v)
{
    if (v <= 0.0) return 1.0;
    const double mag = std::pow(10.0, std::floor(std::log10(v)));
    for (const double m : {1.0, 1.2, 1.5, 2.0, 2.5, 3.0, 4.0, 5.0, 6.0, 8.0, 10.0})
        if (v <= m * mag) return m * mag;
    return 10.0 * mag;
}

QPen pen(QColor c, qreal w, Qt::PenStyle s = Qt::SolidLine, int alpha = 255)
{
    c.setAlpha(alpha);
    return QPen(c, w, s);
}

}  // namespace

CurveChart::CurveChart(QWidget *parent)
    : QChartView(parent)
    , m_pressureUnit(&grt::units::unitsOf(grt::units::Quantity::Pressure).front())
    , m_energyUnit(&grt::units::unitsOf(grt::units::Quantity::Energy).front())
    , m_velocityUnit(&grt::units::unitsOf(grt::units::Quantity::Velocity).front())
    , m_lengthUnit(&grt::units::unitsOf(grt::units::Quantity::Length).front())
    , m_pressure(new QLineSeries)
    , m_velocity(new QLineSeries)
    , m_energy(new QLineSeries)
    , m_refPressure(new QLineSeries)
    , m_refVelocity(new QLineSeries)
    , m_refEnergy(new QLineSeries)
    , m_permittedLine(new QLineSeries)
    , m_axisX(new QValueAxis)
    , m_axisP(new QValueAxis)
    , m_axisE(new QValueAxis)
    , m_axisV(new QValueAxis)
{
    viewport()->setMouseTracking(true);  // the cursor follows the mouse without a pressed button
    auto *chart = new QChart;
    chart->legend()->setAlignment(Qt::AlignBottom);
    chart->setMargins({4, 4, 4, 4});

    m_pressure->setName(tr("Pressure"));
    m_energy->setName(tr("Energy"));
    m_velocity->setName(tr("Velocity"));
    m_refPressure->setName(tr("Pressure (original)"));
    m_refEnergy->setName(tr("Energy (original)"));
    m_refVelocity->setName(tr("Velocity (original)"));
    m_pressure->setPen(pen(kPressure, 2));
    m_velocity->setPen(pen(kVelocity, 2));
    m_energy->setPen(pen(kEnergy, 2, Qt::DotLine));
    m_refPressure->setPen(pen(kPressure, 1.4, Qt::DashLine, 170));
    m_refVelocity->setPen(pen(kVelocity, 1.4, Qt::DashLine, 170));
    m_refEnergy->setPen(pen(kEnergy, 1.4, Qt::DashLine, 170));
    m_permittedLine->setName(tr("Pmax permitted"));
    m_permittedLine->setPen(pen(QColor(0xc0, 0x60, 0xb0), 1, Qt::DashDotLine));

    m_axisX->setLabelFormat("%.2f");
    m_axisX->setTickCount(7);
    for (QValueAxis *a : {m_axisP, m_axisE, m_axisV})
    {
        a->setLabelFormat("%.0f");
        a->setTickCount(kTicks);
    }
    m_axisP->setLabelsColor(kPressure);
    m_axisE->setLabelsColor(kEnergy);
    m_axisV->setLabelsColor(kVelocity);

    chart->addAxis(m_axisX, Qt::AlignBottom);
    chart->addAxis(m_axisP, Qt::AlignLeft);
    chart->addAxis(m_axisE, Qt::AlignRight);
    chart->addAxis(m_axisV, Qt::AlignRight);

    // warning bands first (drawn underneath the curves)
    m_redLo = new QLineSeries;
    m_redHi = new QLineSeries;
    m_blueLo = new QLineSeries;
    m_blueHi = new QLineSeries;
    m_bandRed = makeBand(QColor(0xa0, 0x30, 0x30), m_redLo, m_redHi);
    m_bandBlue = makeBand(QColor(0x40, 0x70, 0xb0), m_blueLo, m_blueHi);
    for (QAreaSeries *b : {m_bandRed, m_bandBlue})
    {
        chart->addSeries(b);
        b->attachAxis(m_axisX);
        b->attachAxis(m_axisP);
    }

    for (QLineSeries *s : {m_pressure, m_refPressure, m_permittedLine})
    {
        chart->addSeries(s);
        s->attachAxis(m_axisX);
        s->attachAxis(m_axisP);
    }
    for (QLineSeries *s : {m_energy, m_refEnergy})
    {
        chart->addSeries(s);
        s->attachAxis(m_axisX);
        s->attachAxis(m_axisE);
    }
    for (QLineSeries *s : {m_velocity, m_refVelocity})
    {
        chart->addSeries(s);
        s->attachAxis(m_axisX);
        s->attachAxis(m_axisV);
    }

    static const QString names[] = {QStringLiteral("z1"), QStringLiteral("Pmax"), QStringLiteral("z2"), QStringLiteral("95%"),
                                    QStringLiteral("Burnout")};
    for (const QString &name : names)
    {
        Marker m{new QLineSeries, new QGraphicsSimpleTextItem(name, chart), 0.0, false};
        m.line->setPen(pen(QColor(0x90, 0xa0, 0xb0), 1, Qt::DashLine, 160));
        m.label->setZValue(20);
        chart->addSeries(m.line);
        m.line->attachAxis(m_axisX);
        m.line->attachAxis(m_axisP);
        m_markers.push_back(m);
    }
    connect(chart, &QChart::plotAreaChanged, this, [this] { updateMarkerLabels(); });
    // decoration series stay out of the legend
    for (QAbstractSeries *s : {static_cast<QAbstractSeries *>(m_bandRed), static_cast<QAbstractSeries *>(m_bandBlue)})
        for (QLegendMarker *lm : chart->legend()->markers(s)) lm->setVisible(false);
    for (const Marker &m : m_markers)
        for (QLegendMarker *lm : chart->legend()->markers(m.line)) lm->setVisible(false);

    setChart(chart);
    setRenderHint(QPainter::Antialiasing);
    setMinimumHeight(300);
    applyTheme();
    applyFonts();
    rebuild();
}

QAreaSeries *CurveChart::makeBand(const QColor &color, QLineSeries *lo, QLineSeries *hi)
{
    auto *area = new QAreaSeries(hi, lo);
    // a QAreaSeries does not own its boundary lines: the series owns them, so they go with it
    lo->setParent(area);
    hi->setParent(area);
    QColor fill = color;
    fill.setAlpha(120);
    area->setBrush(fill);
    area->setPen(Qt::NoPen);
    return area;
}

// The chart's texts follow the application font (which the user can change in the settings).
void CurveChart::applyFonts()
{
    QFont base = font();
    QFont small = base;
    small.setPointSizeF(base.pointSizeF() * 0.9);
    chart()->setTitleFont(base);
    chart()->legend()->setFont(small);
    for (QValueAxis *a : {m_axisX, m_axisP, m_axisE, m_axisV})
    {
        a->setLabelsFont(small);
        a->setTitleFont(base);
    }
    for (const Marker &m : m_markers) m.label->setFont(small);
    updateMarkerLabels();
}

void CurveChart::changeEvent(QEvent *event)
{
    QChartView::changeEvent(event);
    if (event->type() == QEvent::PaletteChange)
    {
        applyTheme();
        applyFonts();
    }
    else if (event->type() == QEvent::FontChange || event->type() == QEvent::ApplicationFontChange)
    {
        applyFonts();
    }
}

void CurveChart::mouseMoveEvent(QMouseEvent *event)
{
    QChartView::mouseMoveEvent(event);
    const QPointF pos = chart()->mapFromScene(mapToScene(event->position().toPoint()));
    std::optional<double> x;
    if (chart()->plotArea().contains(pos)) x = chart()->mapToValue(pos, m_pressure).x();
    if (x && !pointAt(*x)) x.reset();
    if (x == m_cursorX) return;
    m_cursorX = x;
    viewport()->update();
}

void CurveChart::leaveEvent(QEvent *event)
{
    QChartView::leaveEvent(event);
    m_cursorX.reset();
    viewport()->update();
}

std::optional<grt::SimPoint> CurveChart::pointAt(double x) const
{
    const auto &pts = m_result.points;
    if (!m_result.ok() || pts.empty() || x < xOf(pts.front()) || x > xOf(pts.back())) return std::nullopt;
    // x grows along the curve (time and bullet position)
    const auto it = std::ranges::lower_bound(pts, x, {}, [&](const grt::SimPoint &p) { return xOf(p); });
    if (it == pts.begin()) return pts.front();
    const grt::SimPoint &a = *std::prev(it), &b = *it;
    const double span = xOf(b) - xOf(a), f = span > 0.0 ? (x - xOf(a)) / span : 0.0;
    const auto lerp = [f](double u, double v) { return u + f * (v - u); };
    return grt::SimPoint{.x_mm = lerp(a.x_mm, b.x_mm),
                         .z = lerp(a.z, b.z),
                         .p_bar = lerp(a.p_bar, b.p_bar),
                         .v_ms = lerp(a.v_ms, b.v_ms),
                         .e_J = lerp(a.e_J, b.e_J),
                         .t_ms = lerp(a.t_ms, b.t_ms)};
}

// The cursor: a vertical line at the mouse, a dot on each curve and a box with the values.
void CurveChart::drawForeground(QPainter *painter, const QRectF &rect)
{
    QChartView::drawForeground(painter, rect);
    if (!m_cursorX) return;
    const std::optional<grt::SimPoint> pt = pointAt(*m_cursorX);
    if (!pt) return;
    using grt::units::format;
    const QRectF plot = chart()->mapRectToScene(chart()->plotArea());
    const auto scene = [&](double y, QLineSeries *s) { return chart()->mapToScene(chart()->mapToPosition({*m_cursorX, y}, s)); };
    const double x = scene(0.0, m_pressure).x();
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    QColor lineColor = palette().color(QPalette::Text);
    lineColor.setAlpha(140);
    painter->setPen(QPen(lineColor, 1.0, Qt::DashLine));
    painter->drawLine(QPointF(x, plot.top()), QPointF(x, plot.bottom()));
    const struct
    {
        QLineSeries *series;
        double value;
        QColor color;
    } dots[] = {{m_pressure, grt::units::toDisplay(pt->p_bar, *m_pressureUnit), kPressure},
                {m_energy, grt::units::toDisplay(pt->e_J, *m_energyUnit), kEnergy},
                {m_velocity, grt::units::toDisplay(pt->v_ms, *m_velocityUnit), kVelocity}};
    for (const auto &d : dots)
    {
        painter->setPen(QPen(palette().color(QPalette::Base), 1.5));
        painter->setBrush(d.color);
        painter->drawEllipse(scene(d.value, d.series), 4.0, 4.0);
    }

    const auto unit = [](const grt::units::Unit &u) { return QString::fromUtf8(u.symbol); };
    const QString lines[] = {
        tr("Time: %1 ms").arg(QLocale().toString(pt->t_ms, 'f', 3)),
        tr("Bullet base: %1 %2").arg(format(pt->x_mm, *m_lengthUnit), unit(*m_lengthUnit)),
        tr("Pressure: %1 %2").arg(format(pt->p_bar, *m_pressureUnit), unit(*m_pressureUnit)),
        tr("Velocity: %1 %2").arg(format(pt->v_ms, *m_velocityUnit), unit(*m_velocityUnit)),
        tr("Energy: %1 %2").arg(format(pt->e_J, *m_energyUnit), unit(*m_energyUnit)),
        tr("Burnt propellant: %1 %").arg(QLocale().toString(100.0 * pt->z, 'f', 1)),
    };
    const QColor colors[] = {palette().color(QPalette::Text), palette().color(QPalette::Text), kPressure, kVelocity, kEnergy,
                             palette().color(QPalette::Text)};
    QFont f = font();
    f.setPointSizeF(f.pointSizeF() * 0.9);
    painter->setFont(f);
    const QFontMetricsF fm(f);
    double w = 0.0;
    for (const QString &l : lines) w = std::max(w, fm.horizontalAdvance(l));
    const double pad = 6.0, h = std::size(lines) * fm.height() + 2 * pad;
    w += 2 * pad;
    // right of the line, on the left near the right edge
    QRectF box(x + 10.0, plot.top() + 6.0, w, h);
    if (box.right() > plot.right()) box.moveRight(x - 10.0);
    QColor bg = palette().color(QPalette::Base);
    bg.setAlpha(230);
    painter->setPen(QPen(palette().color(QPalette::PlaceholderText), 1.0));
    painter->setBrush(bg);
    painter->drawRoundedRect(box, 4.0, 4.0);
    for (std::size_t i = 0; i < std::size(lines); ++i)
    {
        painter->setPen(colors[i]);
        painter->drawText(QPointF(box.left() + pad, box.top() + pad + fm.ascent() + i * fm.height()), lines[i]);
    }
    painter->restore();
}

void CurveChart::applyTheme()
{
    const bool dark = palette().color(QPalette::Window).lightness() < 128;
    chart()->setTheme(dark ? QChart::ChartThemeDark : QChart::ChartThemeLight);
    chart()->setBackgroundBrush(palette().brush(QPalette::Base));
    chart()->setPlotAreaBackgroundBrush(palette().brush(QPalette::Base));
    chart()->setPlotAreaBackgroundVisible(true);
    // the theme resets pens and label colors: set ours again
    m_pressure->setPen(pen(kPressure, 2));
    m_velocity->setPen(pen(kVelocity, 2));
    m_energy->setPen(pen(kEnergy, 2, Qt::DotLine));
    m_refPressure->setPen(pen(kPressure, 1.4, Qt::DashLine, 170));
    m_refVelocity->setPen(pen(kVelocity, 1.4, Qt::DashLine, 170));
    m_refEnergy->setPen(pen(kEnergy, 1.4, Qt::DashLine, 170));
    m_permittedLine->setPen(pen(QColor(0xc0, 0x60, 0xb0), 1, Qt::DashDotLine));
    for (const Marker &m : m_markers) m.line->setPen(pen(QColor(0x90, 0xa0, 0xb0), 1, Qt::DashLine, 160));
    m_axisP->setLabelsColor(kPressure);
    m_axisE->setLabelsColor(kEnergy);
    m_axisV->setLabelsColor(kVelocity);
    for (const Marker &m : m_markers) m.label->setBrush(palette().color(QPalette::Text));
    for (QAreaSeries *b : {m_bandRed, m_bandBlue})
    {
        QColor c = b == m_bandRed ? QColor(0xa0, 0x30, 0x30) : QColor(0x40, 0x70, 0xb0);
        c.setAlpha(dark ? 120 : 70);
        b->setBrush(c);
        b->setPen(Qt::NoPen);
    }
}

double CurveChart::xOf(const grt::SimPoint &p) const noexcept
{
    return m_xAxis == XAxis::Time ? p.t_ms : grt::units::toDisplay(p.x_mm, *m_lengthUnit);
}

void CurveChart::setUnits(const grt::units::Unit &pressure, const grt::units::Unit &energy, const grt::units::Unit &velocity,
                          const grt::units::Unit &length)
{
    m_pressureUnit = &pressure;
    m_energyUnit = &energy;
    m_velocityUnit = &velocity;
    m_lengthUnit = &length;
    rebuild();
}

void CurveChart::setResult(const grt::SimResult &result, double permittedPressure_bar)
{
    m_result = result;
    m_permitted = permittedPressure_bar;
    rebuild();
}

void CurveChart::clearResult()
{
    m_result = {};
    rebuild();
}

void CurveChart::setReference(std::span<const grt::SimPoint> points)
{
    m_reference.assign(points.begin(), points.end());
    rebuild();
}

void CurveChart::setLadder(std::vector<grt::SimResult> others)
{
    m_ladder = std::move(others);
    rebuild();
}

void CurveChart::setXAxis(XAxis axis)
{
    if (m_xAxis == axis) return;
    m_xAxis = axis;
    rebuild();
}

void CurveChart::placeMarker(Marker &m, bool visible, double x, double yTop, const QString &text)
{
    Q_UNUSED(text)
    m.visible = visible;
    m.x = x;
    if (visible) m.line->replace({{x, 0.0}, {x, yTop}});
    else m.line->clear();
}

void CurveChart::updateMarkerLabels()
{
    const QRectF plot = chart()->plotArea();
    QList<QRectF> placed;  // labels that would overlap are moved one row down, inside the plot
    for (Marker &m : m_markers)
    {
        m.label->setVisible(m.visible);
        if (!m.visible) continue;
        const QPointF pos = chart()->mapToPosition({m.x, m_axisP->max()}, m_pressure);
        const QRectF r = m.label->boundingRect();
        const double x = std::clamp(pos.x() - r.width() / 2, plot.left(), plot.right() - r.width());
        double y = plot.top() - r.height() - 1;
        for (int row = 0; row < 3; ++row)
        {
            const QRectF candidate(x, y, r.width(), r.height());
            if (std::ranges::none_of(placed, [&](const QRectF &q) { return q.adjusted(-2, 0, 2, 0).intersects(candidate); })) break;
            y += r.height() + 1;
        }
        m.label->setPos(x, y);
        placed.append(QRectF(x, y, r.width(), r.height()));
    }
}

void CurveChart::rebuild()
{
    viewport()->update();  // the cursor shows the new values
    const auto &pts = m_result.points;
    const grt::units::Unit &pu = *m_pressureUnit, &eu = *m_energyUnit, &vu = *m_velocityUnit;
    const auto P = [&](double bar) { return grt::units::toDisplay(bar, pu); };
    const auto E = [&](double joule) { return grt::units::toDisplay(joule, eu); };
    const auto V = [&](double ms) { return grt::units::toDisplay(ms, vu); };
    const auto fillWith = [&](QLineSeries *s, std::span<const grt::SimPoint> p, auto value)
    {
        QList<QPointF> out;
        out.reserve(static_cast<qsizetype>(p.size()));
        for (const grt::SimPoint &q : p) out.append({xOf(q), value(q)});
        s->replace(out);
    };
    fillWith(m_pressure, pts, [&](const grt::SimPoint &p) { return P(p.p_bar); });
    fillWith(m_velocity, pts, [&](const grt::SimPoint &p) { return V(p.v_ms); });
    fillWith(m_energy, pts, [&](const grt::SimPoint &p) { return E(p.e_J); });
    fillWith(m_refPressure, m_reference, [&](const grt::SimPoint &p) { return P(p.p_bar); });
    fillWith(m_refVelocity, m_reference, [&](const grt::SimPoint &p) { return V(p.v_ms); });
    fillWith(m_refEnergy, m_reference, [&](const grt::SimPoint &p) { return E(p.e_J); });
    // ladder: the other charges as faded curves
    for (QLineSeries *s : m_ladderSeries)
    {
        chart()->removeSeries(s);
        delete s;
    }
    m_ladderSeries.clear();
    for (const grt::SimResult &step : m_ladder)
    {
        if (!step.ok()) continue;
        const struct
        {
            QValueAxis *axis;
            QColor color;
            int what;
        } kinds[] = {{m_axisP, kPressure, 0}, {m_axisE, kEnergy, 1}, {m_axisV, kVelocity, 2}};
        for (const auto &k : kinds)
        {
            auto *s = new QLineSeries;
            s->setPen(pen(k.color, 1.2, Qt::SolidLine, 85));
            fillWith(s, step.points, [&](const grt::SimPoint &p) { return k.what == 0 ? P(p.p_bar) : k.what == 1 ? E(p.e_J) : V(p.v_ms); });
            chart()->addSeries(s);
            s->attachAxis(m_axisX);
            s->attachAxis(k.axis);
            for (QLegendMarker *lm : chart()->legend()->markers(s)) lm->setVisible(false);
            m_ladderSeries.push_back(s);
        }
    }
    const bool haveRef = !m_reference.empty();
    for (QLineSeries *s : {m_refPressure, m_refVelocity, m_refEnergy})
        for (QLegendMarker *lm : chart()->legend()->markers(s)) lm->setVisible(haveRef);

    // ranges
    double xMax = 0.0, pMax = 0.0, eMax = 0.0, vMax = 0.0, xMin = 0.0;
    bool first = true;
    const auto scan = [&](std::span<const grt::SimPoint> p)
    {
        for (const grt::SimPoint &q : p)
        {
            if (first)
            {
                xMin = xMax = xOf(q);
                first = false;
            }
            xMin = std::min(xMin, xOf(q));
            xMax = std::max(xMax, xOf(q));
            pMax = std::max(pMax, P(q.p_bar));
            eMax = std::max(eMax, E(q.e_J));
            vMax = std::max(vMax, V(q.v_ms));
        }
    };
    scan(pts);
    scan(m_reference);
    for (const grt::SimResult &step : m_ladder) scan(step.points);
    if (m_xAxis == XAxis::Time) xMin = 0.0;
    const double permitted = P(m_permitted);
    pMax = std::max(pMax, m_permitted > 0.0 ? permitted * 1.02 : 0.0);
    const double pTop = niceCeil(pMax * 1.04);
    m_axisX->setRange(m_xAxis == XAxis::Time ? 0.0 : std::floor(xMin / 10.0) * 10.0, std::max(xMax, xMin + 0.01));
    m_axisX->setTitleText(m_xAxis == XAxis::Time ? tr("ms") : tr("Bullet base position [%1]").arg(QString::fromUtf8(m_lengthUnit->symbol)));
    m_axisP->setTitleText(QString::fromUtf8(pu.symbol));
    m_axisE->setTitleText(QString::fromUtf8(eu.symbol));
    m_axisV->setTitleText(QString::fromUtf8(vu.symbol));
    m_axisP->setRange(0.0, pTop);
    m_axisE->setRange(0.0, niceCeil(eMax * 1.04));
    m_axisV->setRange(0.0, niceCeil(vMax * 1.04));

    // bands and permitted pressure
    const double x0 = m_axisX->min(), x1 = m_axisX->max();
    const auto hline = [&](QLineSeries *s, double y, bool on)
    {
        if (on) s->replace({{x0, y}, {x1, y}});
        else s->clear();
    };
    const bool bands = m_permitted > 0.0 && m_result.ok();
    hline(m_redLo, 0.85 * permitted, bands);
    hline(m_redHi, pTop, bands);
    hline(m_blueLo, 0.75 * permitted, bands);
    hline(m_blueHi, 0.85 * permitted, bands);
    hline(m_permittedLine, permitted, bands);
    for (QLegendMarker *lm : chart()->legend()->markers(m_permittedLine)) lm->setVisible(bands);

    // markers
    const bool ok = m_result.ok() && !pts.empty();
    const auto at = [&](std::size_t i) { return xOf(pts[std::min(i, pts.size() - 1)]); };
    placeMarker(m_markers[0], ok && m_result.indexZ1 > 0, ok ? at(m_result.indexZ1) : 0.0, pTop, {});
    placeMarker(m_markers[1], ok, ok ? at(m_result.indexPmax) : 0.0, pTop, {});
    placeMarker(m_markers[2], ok && m_result.indexZ2 > 0, ok ? at(m_result.indexZ2) : 0.0, pTop, {});
    placeMarker(m_markers[3], ok && m_result.indexEnergy95 > 0, ok ? at(m_result.indexEnergy95) : 0.0, pTop, {});
    placeMarker(m_markers[4], ok && m_result.burnoutInBarrel, ok ? at(m_result.indexBurnout) : 0.0, pTop, {});
    updateMarkerLabels();
}
