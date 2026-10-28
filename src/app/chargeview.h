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

#include "solver.h"
#include <QWidget>
#include <array>
#include <vector>

class QButtonGroup;
class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QSpinBox;
class QTableWidget;

// The "Charge & ladder" tab of the input panel: the propellant charge, the number of ladder steps and the charge step in large
// input fields (the same values as in the propellant table), and below them the charges the ladder runs.
class ChargeView : public QWidget
{
    Q_OBJECT
public:
    explicit ChargeView(QWidget *parent = nullptr);

    // The file's values (grams, steps); no signal is emitted.
    void setValues(double charge_g, int steps, double step_g);
    // The simulated ladder (one entry without a ladder) and the permitted maximum pressure [bar] (0: none).
    void setLadder(const std::vector<csr::LadderStep> &ladder, int focus, double maxPressure_bar);

signals:
    // A field was edited: the file's key (mc, laddercnt, laddermc) and the value in the file's unit.
    void edited(const QString &key, const QString &base);
    void stepSelected(int index);

private:
    void applyUnits();
    void chooseUnit(const QString &id);
    void showValues();

    QDoubleSpinBox *m_charge;
    QSpinBox *m_steps;
    QDoubleSpinBox *m_step;
    QButtonGroup *m_unitGroup;
    std::array<QPushButton *, 2> m_unitButtons{};  // grain, gram
    QLabel *m_hint;
    QTableWidget *m_table;
    double m_charge_g{};
    int m_stepCount{1};
    double m_step_g{};
};
