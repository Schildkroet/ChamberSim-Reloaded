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
#include "tablebrowser.h"

#include <QDialog>

class QCheckBox;
class QDialogButtonBox;
class QualityRow;

// Pick one record of a database table (caliber, projectile or propellant).
class SelectRecordDialog : public QDialog
{
    Q_OBJECT
public:
    SelectRecordDialog(const csr::Database &db, csr::Table table, const QString &title, QWidget *parent = nullptr);

    // Optional restriction, e.g. bullets that fit the caliber. The check box switches it on and off.
    void setRestriction(const QString &sqlCondition, const QString &checkBoxText);
    // Optional "also reset dependent values" choice shown below the table.
    void setResetOption(const QString &text, bool checked);

    bool select(int id);
    [[nodiscard]] int selectedId() const { return m_browser->currentId(); }
    [[nodiscard]] bool resetChecked() const;

private:
    void updateOk();

    TableBrowser *m_browser;
    QCheckBox *m_restrict{nullptr};
    QCheckBox *m_reset{nullptr};
    QDialogButtonBox *m_buttons;
    QString m_condition;
    QualityRow *m_qualityRow{nullptr};  // propellants: the fit quality of the selected record
};
