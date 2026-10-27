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

#include "database.h"
#include "loadbuilder.h"
#include "loadfile.h"
#include "tablebrowser.h"

#include <QDialog>

class QCheckBox;
class QDialogButtonBox;
class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QStackedWidget;
class QualityRow;
class QTabWidget;

// Two steps: first pick a caliber, a bullet and a powder from the database; "Next" then shows the charge and gun data with the
// defaults of that selection, which can be changed freely (they are only filled in when the step is entered with another selection).
// The result is a ready-to-simulate load.
class NewLoadDialog : public QDialog
{
    Q_OBJECT
public:
    NewLoadDialog(const csr::Database &db, QWidget *parent = nullptr);

    [[nodiscard]] csr::LoadFile result() const;

    // Programmatic selection (used by tests); returns false if a record is not listed. Goes on to the second step.
    bool select(int caliberId, int projectileId, int propellantId);

private:
    void onSelectionChanged();
    void goNext();
    void goBack();
    void applyDefaults();
    void updateSummary();
    [[nodiscard]] QWidget *makeSummary(std::array<QLabel *, 3> &values);

    const csr::Database &m_db;
    std::array<TableBrowser *, 3> m_browsers{};
    QStackedWidget *m_pages;
    QTabWidget *m_tabs;
    QCheckBox *m_matchDiameter;
    QualityRow *m_quality{nullptr};  // third list: the fit quality of the selected powder
    QDoubleSpinBox *m_charge;
    QDoubleSpinBox *m_barrel;
    QDoubleSpinBox *m_depth;
    QDoubleSpinBox *m_twist;
    QDoubleSpinBox *m_gunWeight;
    QDoubleSpinBox *m_temperature;
    std::array<QLabel *, 3> m_summary{};    // second step: what was selected (caliber, projectile, propellant)
    std::array<QLabel *, 3> m_selection{};  // first step: the same, below the lists
    QLabel *m_chargeGrains;
    QDialogButtonBox *m_buttons;
    QPushButton *m_back;
    QPushButton *m_next;  // "Next", on the second step "Create load"
    std::optional<QVariantMap> m_caliber, m_projectile, m_propellant;
    bool m_temperatureSet{false};
    std::array<int, 3> m_defaultsFor{-1, -1, -1};  // the records the data fields were last filled for
};
