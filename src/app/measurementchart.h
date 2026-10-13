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

#include "measurement.h"
#include "units.h"

#include <QChartView>

#include <optional>
#include <span>
#include <vector>

class QLineSeries;
class QScatterSeries;
class QValueAxis;

// Velocity and peak pressure over the charge: the measured shots, their mean with +- one standard deviation, and
// the simulated curves of the load for comparison. The mouse over the plot shows a cursor line with a box of the values.
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
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void drawForeground(QPainter *painter, const QRectF &rect) override;

private:
    void rebuild();
    void applyTheme();
    void applyFonts();

    std::optional<Prediction> predictionAt(double charge_g) const;

    csr::Measurement m_data;
    std::optional<double> m_cursorX;  // mouse cursor, in charge axis units
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
