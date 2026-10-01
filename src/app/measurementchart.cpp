#include "measurementchart.h"

#include <QChart>
#include <QEvent>
#include <QLegend>
#include <QLegendMarker>
#include <QLineSeries>
#include <QScatterSeries>
#include <QValueAxis>

#include <algorithm>
#include <limits>

namespace {

const QColor kVelocity(0x2f, 0x7f, 0xd0);
const QColor kPressure(0xd0, 0x4a, 0x3a);

struct Range {
    double lo{std::numeric_limits<double>::max()};
    double hi{std::numeric_limits<double>::lowest()};
    void add(double v) { lo = std::min(lo, v); hi = std::max(hi, v); }
    [[nodiscard]] bool valid() const { return lo <= hi; }
};

}  // namespace

MeasurementChart::MeasurementChart(QWidget *parent)
    : QChartView(parent)
    , m_chargeUnit(&grt::units::systemUnit(grt::units::Quantity::BulletMass, grt::units::System::Metric))
    , m_velocityUnit(&grt::units::systemUnit(grt::units::Quantity::Velocity, grt::units::System::Metric))
    , m_pressureUnit(&grt::units::systemUnit(grt::units::Quantity::Pressure, grt::units::System::Metric))
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
    for (QXYSeries *s : std::initializer_list<QXYSeries *>{m_simVelocity, m_shotVelocity, m_meanVelocity}) {
        chart->addSeries(s);
        s->attachAxis(m_axisX);
        s->attachAxis(m_axisV);
    }
    for (QXYSeries *s : std::initializer_list<QXYSeries *>{m_simPressure, m_shotPressure}) {
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
    setRenderHint(QPainter::Antialiasing);
    setMinimumSize(320, 220);
    applyTheme();
    applyFonts();
    rebuild();
}

void MeasurementChart::setData(const grt::Measurement &m, std::span<const Prediction> simulated)
{
    m_data = m;
    m_simulated.assign(simulated.begin(), simulated.end());
    rebuild();
}

void MeasurementChart::setUnits(const grt::units::Unit &charge, const grt::units::Unit &velocity, const grt::units::Unit &pressure)
{
    m_chargeUnit = &charge;
    m_velocityUnit = &velocity;
    m_pressureUnit = &pressure;
    rebuild();
}

void MeasurementChart::rebuild()
{
    using grt::units::toDisplay;
    const auto cx = [&](double g) { return toDisplay(g, *m_chargeUnit); };
    const auto cv = [&](double v) { return toDisplay(v, *m_velocityUnit); };
    const auto cp = [&](double p) { return toDisplay(p, *m_pressureUnit); };

    for (QLineSeries *s : m_errorBars) {
        chart()->removeSeries(s);
        delete s;
    }
    m_errorBars.clear();

    Range rx, rv, rp;
    QList<QPointF> simV, simP, shotV, meanV, shotP;
    for (const Prediction &p : m_simulated) {
        simV.append({cx(p.charge_g), cv(p.velocity_ms)});
        simP.append({cx(p.charge_g), cp(p.pressure_bar)});
        rx.add(cx(p.charge_g));
        rv.add(cv(p.velocity_ms));
        rp.add(cp(p.pressure_bar));
    }
    for (const grt::MeasuredCharge &c : m_data.charges) {
        const double x = cx(c.charge_g());
        bool any = false;
        for (const grt::Shot &s : c.shots) {
            if (s.velocity_ms > 0.0) { shotV.append({x, cv(s.velocity_ms)}); rv.add(cv(s.velocity_ms)); any = true; }
            if (s.pressure_bar > 0.0) { shotP.append({x, cp(s.pressure_bar)}); rp.add(cp(s.pressure_bar)); any = true; }
        }
        if (any) rx.add(x);
        const grt::ShotStatistics st = grt::velocityStatistics(c);
        if (st.count == 0) continue;
        meanV.append({x, cv(st.mean)});
        if (st.count > 1) {
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
    if (rx.hi - rx.lo < 1e-9) { rx.lo -= 0.5; rx.hi += 0.5; }
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
    for (QValueAxis *a : {m_axisX, m_axisV, m_axisP}) {
        a->setLabelsFont(small);
        a->setTitleFont(small);
    }
    chart()->legend()->setFont(small);
}

void MeasurementChart::changeEvent(QEvent *event)
{
    QChartView::changeEvent(event);
    if (event->type() == QEvent::PaletteChange) {
        applyTheme();
        applyFonts();
    } else if (event->type() == QEvent::FontChange || event->type() == QEvent::ApplicationFontChange) {
        applyFonts();
    }
}
