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

#include <QChartView>

#include <vector>

class QLineSeries;
class QValueAxis;

// Small chart of the burn-rate coefficient Ba * phi(z) over the burnt fraction z, with the
// transition limits z1 and z2.
class VivacityChart : public QChartView
{
    Q_OBJECT
public:
    explicit VivacityChart(QWidget *parent = nullptr);

    void setCurve(const std::vector<double> &values, double z1, double z2);
    void clear();

protected:
    void changeEvent(QEvent *event) override;

private:
    void applyTheme();
    void applyFonts();

    QLineSeries *m_curve;
    QLineSeries *m_z1;
    QLineSeries *m_z2;
    QValueAxis *m_axisX;
    QValueAxis *m_axisY;
};
