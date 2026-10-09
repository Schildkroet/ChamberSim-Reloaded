#pragma once

#include "measurement.h"
#include "units.h"

#include <QChartView>

#include <span>
#include <vector>

class QLineSeries;
class QScatterSeries;
class QValueAxis;

// Velocity and peak pressure over the charge: the measured shots, their mean with +- one standard deviation, and
// the simulated curves of the load for comparison.
class MeasurementChart : public QChartView
{
    Q_OBJECT
public:
    struct Prediction
    {
        double charge_g;
        double velocity_ms;
        double pressure_bar;
    };

    explicit MeasurementChart(QWidget *parent = nullptr);

    void setData(const csr::Measurement &m, std::span<const Prediction> simulated);
    void setUnits(const csr::units::Unit &charge, const csr::units::Unit &velocity, const csr::units::Unit &pressure);

protected:
    void changeEvent(QEvent *event) override;

private:
    void rebuild();
    void applyTheme();
    void applyFonts();

    csr::Measurement m_data;
    std::vector<Prediction> m_simulated;
    const csr::units::Unit *m_chargeUnit;
    const csr::units::Unit *m_velocityUnit;
    const csr::units::Unit *m_pressureUnit;

    QLineSeries *m_simVelocity;
    QLineSeries *m_simPressure;
    QScatterSeries *m_shotVelocity;
    QScatterSeries *m_meanVelocity;
    QScatterSeries *m_shotPressure;
    std::vector<QLineSeries *> m_errorBars;
    QValueAxis *m_axisX;
    QValueAxis *m_axisV;
    QValueAxis *m_axisP;
};
