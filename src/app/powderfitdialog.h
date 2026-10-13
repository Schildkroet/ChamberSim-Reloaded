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

#include "powderfit.h"

#include <QDialog>

#include <functional>
#include <map>
#include <optional>

class QCheckBox;
class QLabel;
class QPushButton;
class QTableWidget;

// "Fit to measurements": changes the load's propellant values until the simulation agrees with the measured charges, for
// both models (the original's Ba, a0, z1, z2, also used by the Published profile, and the physical model's physBa ...), shows the
// difference per charge before and after, and applies the values to the load or saves the original's as an own propellant record.
class PowderFitDialog : public QDialog
{
    Q_OBJECT
public:
    PowderFitDialog(std::vector<csr::powderfit::Sample> samples, QStringList warnings, QWidget *parent = nullptr);

    // Called by "Save as own propellant…" with the original model's result (the dialog stays open); none: the button is hidden.
    void setSaveHandler(std::function<bool(const csr::powderfit::Result &)> handler);

    void setParams(const std::vector<csr::powderfit::Param> &params);
    void runFit();
    // one result per model (the original's first), empty before the fit
    [[nodiscard]] const std::vector<csr::powderfit::Result> &results() const noexcept { return m_results; }
    [[nodiscard]] bool improved() const;

private:
    void showResult();

    std::vector<csr::powderfit::Sample> m_samples;
    std::vector<csr::powderfit::Result> m_results;
    std::function<bool(const csr::powderfit::Result &)> m_save;
    std::map<csr::powderfit::Param, QCheckBox *> m_params;
    QTableWidget *m_table;
    QLabel *m_summary;
    QPushButton *m_fit;
    QPushButton *m_apply;
    QPushButton *m_saveButton;
};
