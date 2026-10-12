#include "measurementchart.h"

#include <QChart>
#include <QEvent>
#include <QLegend>
#include <QLegendMarker>
#include <QLineSeries>
#include <QLocale>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QScatterSeries>
#include <QValueAxis>

#include <algorithm>
#include <limits>

namespace
{

const QColor kVelocity(0x2f, 0x7f, 0xd0);
const QColor kPressure(0xd0, 0x4a, 0x3a);

struct Range
{
    double lo{std::numeric_limits<double>::max()};
    double hi{std::numeric_limits<double>::lowest()};
    void add(double v)
    {
        lo = std::min(lo, v);
        hi = std::max(hi, v);
    }
    [[nodiscard]] bool valid() const { return lo <= hi; }
};

}  // namespace

MeasurementChart::MeasurementChart(QWidget *parent)
    : QChartView(parent)
    , m_chargeUnit(&csr::units::systemUnit(csr::units::Quantity::BulletMass, csr::units::System::Metric))
    , m_velocityUnit(&csr::units::systemUnit(csr::units::Quantity::Velocity, csr::units::System::Metric))
    , m_pressureUnit(&csr::units::systemUnit(csr::units::Quantity::Pressure, csr::units::System::Metric))
    , m_simVelocity(new QLineSeries)
    , m_simPressure(new QLineSeries)
    , m_shotVelocity(new QScatterSeries)
    , m_meanVelocity(new QScatterSeries)
    , m_shotPressure(new QScatterSeries)
    , m_axisX(new QValueAxis)
    , m_axisV(new QValueAxis)
    , m_axisP(new QValueAxis)
{
    auto *chart = new QChart;
    chart->setMargins({4, 4, 4, 4});
    chart->legend()->setAlignment(Qt::AlignBottom);
    m_simVelocity->setName(tr("Velocity (sim.)"));
    m_shotVelocity->setName(tr("Velocity"));
    m_meanVelocity->setName(tr("Mean ± SD"));  // not in the legend (the squares with bars explain themselves)
    m_simPressure->setName(tr("Pressure (sim.)"));
    m_shotPressure->setName(tr("Pressure"));
    m_axisX->setTickCount(6);
    m_axisV->setLabelFormat("%.0f");
    m_axisP->setLabelFormat("%.0f");
    chart->legend()->setShowToolTips(true);
    m_axisV->setTickCount(6);
    m_axisP->setTickCount(6);
    chart->addAxis(m_axisX, Qt::AlignBottom);
    chart->addAxis(m_axisV, Qt::AlignLeft);
    chart->addAxis(m_axisP, Qt::AlignRight);
    for (QXYSeries *s : std::initializer_list<QXYSeries *>{m_simVelocity, m_shotVelocity, m_meanVelocity})
    {
        chart->addSeries(s);
        s->attachAxis(m_axisX);
        s->attachAxis(m_axisV);
    }
    for (QXYSeries *s : std::initializer_list<QXYSeries *>{m_simPressure, m_shotPressure})
    {
        chart->addSeries(s);
        s->attachAxis(m_axisX);
        s->attachAxis(m_axisP);
    }
    m_shotVelocity->setMarkerSize(7);
    m_meanVelocity->setMarkerSize(12);
    m_meanVelocity->setMarkerShape(QScatterSeries::MarkerShapeRectangle);
    m_shotPressure->setMarkerSize(7);
    for (QLegendMarker *mk : chart->legend()->markers(m_meanVelocity)) mk->setVisible(false);
    setChart(chart);
    viewport()->setMouseTracking(true);  // the cursor follows the mouse without a pressed button
    setRenderHint(QPainter::Antialiasing);
    setMinimumSize(320, 220);
    applyTheme();
    applyFonts();
    rebuild();
}

void MeasurementChart::setData(const csr::Measurement &m, std::span<const Prediction> simulated)
{
    m_data = m;
    m_simulated.assign(simulated.begin(), simulated.end());
    rebuild();
}

void MeasurementChart::setUnits(const csr::units::Unit &charge, const csr::units::Unit &velocity, const csr::units::Unit &pressure)
{
    m_chargeUnit = &charge;
    m_velocityUnit = &velocity;
    m_pressureUnit = &pressure;
    rebuild();
}

void MeasurementChart::rebuild()
{
    using csr::units::toDisplay;
    const auto cx = [&](double g) { return toDisplay(g, *m_chargeUnit); };
    const auto cv = [&](double v) { return toDisplay(v, *m_velocityUnit); };
    const auto cp = [&](double p) { return toDisplay(p, *m_pressureUnit); };

    for (QLineSeries *s : m_errorBars)
    {
        chart()->removeSeries(s);
        delete s;
    }
    m_errorBars.clear();

    Range rx, rv, rp;
    QList<QPointF> simV, simP, shotV, meanV, shotP;
    for (const Prediction &p : m_simulated)
    {
        simV.append({cx(p.charge_g), cv(p.velocity_ms)});
        simP.append({cx(p.charge_g), cp(p.pressure_bar)});
        rx.add(cx(p.charge_g));
        rv.add(cv(p.velocity_ms));
        rp.add(cp(p.pressure_bar));
    }
    for (const csr::MeasuredCharge &c : m_data.charges)
    {
        const double x = cx(c.charge_g());
        bool any = false;
        for (const csr::Shot &s : c.shots)
        {
            if (s.velocity_ms > 0.0)
            {
                shotV.append({x, cv(s.velocity_ms)});
                rv.add(cv(s.velocity_ms));
                any = true;
            }
            if (s.pressure_bar > 0.0)
            {
                shotP.append({x, cp(s.pressure_bar)});
                rp.add(cp(s.pressure_bar));
                any = true;
            }
        }
        if (any) rx.add(x);
        const csr::ShotStatistics st = csr::velocityStatistics(c);
        if (st.count == 0) continue;
        meanV.append({x, cv(st.mean)});
        if (st.count > 1)
        {
            auto *bar = new QLineSeries;
            bar->append(x, cv(st.mean - st.sd));
            bar->append(x, cv(st.mean + st.sd));
            bar->setPen(QPen(kVelocity, 2));
            chart()->addSeries(bar);
            bar->attachAxis(m_axisX);
            bar->attachAxis(m_axisV);
            for (QLegendMarker *mk : chart()->legend()->markers(bar)) mk->setVisible(false);
            m_errorBars.push_back(bar);
        }
    }
    m_simVelocity->replace(simV);
    m_simPressure->replace(simP);
    m_shotVelocity->replace(shotV);
    m_meanVelocity->replace(meanV);
    m_shotPressure->replace(shotP);
    // the pressure only appears in the legend when there is something to show
    for (QLegendMarker *mk : chart()->legend()->markers(m_shotPressure)) mk->setVisible(!shotP.isEmpty());

    if (!rx.valid()) rx = {.lo = 0.0, .hi = 1.0};
    if (rx.hi - rx.lo < 1e-9)
    {
        rx.lo -= 0.5;
        rx.hi += 0.5;
    }
    const double padX = 0.04 * (rx.hi - rx.lo);
    m_axisX->setRange(rx.lo - padX, rx.hi + padX);
    if (!rv.valid()) rv = {.lo = 0.0, .hi = 1000.0};
    const double padV = std::max(0.08 * (rv.hi - rv.lo), 0.01 * rv.hi + 1.0);
    m_axisV->setRange(rv.lo - padV, rv.hi + padV);
    m_axisP->setRange(0.0, rp.valid() ? rp.hi * 1.1 : 1.0);
    for (QValueAxis *a : {m_axisX, m_axisV, m_axisP}) a->applyNiceNumbers();

    m_axisX->setTitleText(tr("Charge [%1]").arg(QString::fromUtf8(m_chargeUnit->symbol)));
    m_axisV->setTitleText(tr("Velocity [%1]").arg(QString::fromUtf8(m_velocityUnit->symbol)));
    m_axisP->setTitleText(tr("Peak pressure [%1]").arg(QString::fromUtf8(m_pressureUnit->symbol)));
}

void MeasurementChart::applyTheme()
{
    const bool dark = palette().color(QPalette::Window).lightness() < 128;
    chart()->setTheme(dark ? QChart::ChartThemeDark : QChart::ChartThemeLight);
    chart()->setBackgroundBrush(palette().brush(QPalette::Base));
    m_simVelocity->setPen(QPen(kVelocity, 2));
    m_simPressure->setPen(QPen(kPressure, 2, Qt::DashLine));
    m_shotVelocity->setColor(kVelocity.lighter(130));
    m_shotVelocity->setBorderColor(kVelocity);
    m_meanVelocity->setColor(kVelocity.darker(130));
    m_meanVelocity->setBorderColor(palette().color(QPalette::Text));
    m_shotPressure->setColor(kPressure.lighter(120));
    m_shotPressure->setBorderColor(kPressure);
    for (QLineSeries *s : m_errorBars) s->setPen(QPen(kVelocity, 2));
}

void MeasurementChart::applyFonts()
{
    QFont small = font();
    small.setPointSizeF(font().pointSizeF() * 0.9);
    for (QValueAxis *a : {m_axisX, m_axisV, m_axisP})
    {
        a->setLabelsFont(small);
        a->setTitleFont(small);
    }
    chart()->legend()->setFont(small);
}

void MeasurementChart::changeEvent(QEvent *event)
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

void MeasurementChart::mouseMoveEvent(QMouseEvent *event)
{
    QChartView::mouseMoveEvent(event);
    const QPointF pos = chart()->mapFromScene(mapToScene(event->position().toPoint()));
    std::optional<double> x;
    if (chart()->plotArea().contains(pos)) x = chart()->mapToValue(pos, m_simVelocity).x();
    if (x == m_cursorX) return;
    m_cursorX = x;
    viewport()->update();
}

void MeasurementChart::leaveEvent(QEvent *event)
{
    QChartView::leaveEvent(event);
    m_cursorX.reset();
    viewport()->update();
}

// The simulated values at a charge (linear between the simulated charges); nothing outside their range.
std::optional<MeasurementChart::Prediction> MeasurementChart::predictionAt(double charge_g) const
{
    if (m_simulated.empty() || charge_g < m_simulated.front().charge_g || charge_g > m_simulated.back().charge_g) return std::nullopt;
    const auto it = std::ranges::lower_bound(m_simulated, charge_g, {}, &Prediction::charge_g);
    if (it == m_simulated.begin()) return *it;
    const Prediction &a = *std::prev(it), &b = *it;
    const double span = b.charge_g - a.charge_g, f = span > 0.0 ? (charge_g - a.charge_g) / span : 0.0;
    return Prediction{.charge_g = charge_g,
                      .velocity_ms = a.velocity_ms + f * (b.velocity_ms - a.velocity_ms),
                      .pressure_bar = a.pressure_bar + f * (b.pressure_bar - a.pressure_bar)};
}

// The cursor: a vertical line at the mouse and a box with the simulated values and, when the mouse is near a measured charge, the
// measurements of that charge.
void MeasurementChart::drawForeground(QPainter *painter, const QRectF &rect)
{
    QChartView::drawForeground(painter, rect);
    if (!m_cursorX) return;
    using csr::units::format;
    const QRectF plot = chart()->mapRectToScene(chart()->plotArea());
    const auto sceneX = [&](double x) { return chart()->mapToScene(chart()->mapToPosition({x, 0.0}, m_simVelocity)).x(); };

    // the measured charge under the mouse (within 12 px); the line snaps to it
    const csr::MeasuredCharge *near = nullptr;
    double nearDist = 12.0;
    for (const csr::MeasuredCharge &c : m_data.charges)
    {
        const double d = std::abs(sceneX(csr::units::toDisplay(c.charge_g(), *m_chargeUnit)) - sceneX(*m_cursorX));
        if (d <= nearDist) near = &c, nearDist = d;
    }
    const double charge_g = near ? near->charge_g() : csr::units::fromDisplay(*m_cursorX, *m_chargeUnit);
    const std::optional<Prediction> pred = predictionAt(charge_g);
    const double x = sceneX(csr::units::toDisplay(charge_g, *m_chargeUnit));

    const auto unit = [](const csr::units::Unit &u) { return QString::fromUtf8(u.symbol); };
    QList<QString> lines{tr("Charge: %1 %2").arg(format(charge_g, *m_chargeUnit), unit(*m_chargeUnit))};
    QList<QColor> colors{palette().color(QPalette::Text)};
    const auto add = [&](const QString &l, const QColor &c)
    {
        lines.append(l);
        colors.append(c);
    };
    if (pred)
    {
        add(tr("Velocity (sim.): %1 %2").arg(format(pred->velocity_ms, *m_velocityUnit), unit(*m_velocityUnit)), kVelocity);
        add(tr("Pressure (sim.): %1 %2").arg(format(pred->pressure_bar, *m_pressureUnit), unit(*m_pressureUnit)), kPressure);
    }
    if (near)
    {
        const csr::ShotStatistics st = csr::velocityStatistics(*near);
        if (st.count > 0)
        {
            add(tr("Velocity: %1 ± %2 %3 (%n shot(s))", nullptr, st.count)
                    .arg(format(st.mean, *m_velocityUnit), format(st.sd, *m_velocityUnit), unit(*m_velocityUnit)),
                kVelocity);
            add(tr("Extreme spread: %1 %2").arg(format(st.extremeSpread(), *m_velocityUnit), unit(*m_velocityUnit)), kVelocity);
            if (pred)
                add(tr("Deviation from sim.: %1 %").arg(QLocale().toString(100.0 * (st.mean / pred->velocity_ms - 1.0), 'f', 1)),
                    kVelocity);
        }
        double sum = 0.0;
        int n = 0;
        for (const csr::Shot &s : near->shots)
            if (s.pressure_bar > 0.0) sum += s.pressure_bar, ++n;
        if (n > 0)
        {
            add(tr("Pressure: %1 %2 (%n shot(s))", nullptr, n).arg(format(sum / n, *m_pressureUnit), unit(*m_pressureUnit)), kPressure);
            if (pred)
                add(tr("Deviation from sim.: %1 %").arg(QLocale().toString(100.0 * (sum / n / pred->pressure_bar - 1.0), 'f', 1)),
                    kPressure);
        }
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    QColor lineColor = palette().color(QPalette::Text);
    lineColor.setAlpha(140);
    painter->setPen(QPen(lineColor, 1.0, Qt::DashLine));
    painter->drawLine(QPointF(x, plot.top()), QPointF(x, plot.bottom()));
    if (pred)
    {
        const double cx = csr::units::toDisplay(charge_g, *m_chargeUnit);
        const struct
        {
            QLineSeries *series;
            double value;
            QColor color;
        } dots[] = {{m_simVelocity, csr::units::toDisplay(pred->velocity_ms, *m_velocityUnit), kVelocity},
                    {m_simPressure, csr::units::toDisplay(pred->pressure_bar, *m_pressureUnit), kPressure}};
        for (const auto &d : dots)
        {
            painter->setPen(QPen(palette().color(QPalette::Base), 1.5));
            painter->setBrush(d.color);
            painter->drawEllipse(chart()->mapToScene(chart()->mapToPosition({cx, d.value}, d.series)), 4.0, 4.0);
        }
    }
    QFont f = font();
    f.setPointSizeF(f.pointSizeF() * 0.9);
    painter->setFont(f);
    const QFontMetricsF fm(f);
    double w = 0.0;
    for (const QString &l : lines) w = std::max(w, fm.horizontalAdvance(l));
    const double pad = 6.0, h = lines.size() * fm.height() + 2 * pad;
    w += 2 * pad;
    QRectF box(x + 10.0, plot.top() + 6.0, w, h);  // right of the line, on the left near the right edge
    if (box.right() > plot.right()) box.moveRight(x - 10.0);
    QColor bg = palette().color(QPalette::Base);
    bg.setAlpha(230);
    painter->setPen(QPen(palette().color(QPalette::PlaceholderText), 1.0));
    painter->setBrush(bg);
    painter->drawRoundedRect(box, 4.0, 4.0);
    for (qsizetype i = 0; i < lines.size(); ++i)
    {
        painter->setPen(colors[i]);
        painter->drawText(QPointF(box.left() + pad, box.top() + pad + fm.ascent() + i * fm.height()), lines[i]);
    }
    painter->restore();
}
