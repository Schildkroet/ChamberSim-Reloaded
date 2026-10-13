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

#include <QDialog>
#include <QVariantMap>

// Read-only view of one caliber, projectile or propellant record: all columns in groups, numbers in the user's units
// (opened by a double click on a record). The buttons offer to edit the record (own records) or to copy it.
class RecordViewDialog : public QDialog
{
    Q_OBJECT
public:
    enum Result
    {
        Closed = QDialog::Rejected,
        EditRequested = 2,
        CopyRequested = 3
    };

    RecordViewDialog(const csr::Database &db, csr::Table table, int id, const QVariantMap &record, QWidget *parent = nullptr);

    // Number of fields shown, and the text shown for a column ("" without it).
    [[nodiscard]] int fieldCount() const noexcept { return m_fieldCount; }
    [[nodiscard]] QString valueText(const QString &column) const;

private:
    int m_fieldCount{0};
};
