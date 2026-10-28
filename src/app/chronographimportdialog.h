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

#include "chronograph.h"

#include <QDialog>

class QLabel;
class QTableWidget;

// One series of the file with the charge it was shot with.
struct ImportedCharge
{
    csr::ChronographSeries series;
    double charge_g{};
};

// "Import chronograph file": lists the series of a MagnetoSpeed / Labradar file with their statistics. The files
// do not know the powder charge, so it is entered here (default: the load's charge) and every ticked series
// becomes a charge with its shots in the Measurements tab.
class ChronographImportDialog : public QDialog
{
    Q_OBJECT
public:
    ChronographImportDialog(csr::ChronographFile file, double defaultCharge_g, QWidget *parent = nullptr);

    // The ticked series with their charges (an unreadable charge falls back to the default).
    [[nodiscard]] std::vector<ImportedCharge> selected() const;

    // Test access: tick state and charge text of a row.
    void setChecked(int row, bool on);

private:
    csr::ChronographFile m_file;
    double m_default;
    QTableWidget *m_table;
};
