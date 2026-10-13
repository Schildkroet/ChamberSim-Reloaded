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

class QButtonGroup;
class QCheckBox;
class QLabel;
class QLineEdit;
class SearchFilterProxy;
class QStandardItemModel;
class QTableView;

// The burn rate table: all propellants with their burn factors, sorted by progressivity (Bp), brisance (Br) or both (Brp).
// A rough overview of how the powders of the different manufacturers compare, not a substitute table.
class BurnRateDialog : public QDialog
{
    Q_OBJECT
public:
    explicit BurnRateDialog(const csr::Database &db, QWidget *parent = nullptr);
    ~BurnRateDialog() override;

    // Column of the factor the table is sorted by: Br, Bp or Brp.
    enum Factor
    {
        Brisance,
        Progressivity,
        Combined
    };
    void setFactor(Factor factor);
    [[nodiscard]] Factor factor() const noexcept { return m_factor; }
    [[nodiscard]] int rowCount() const;
    // Row `row` (in the order shown): "Manufacturer Name".
    [[nodiscard]] QString powderAt(int row) const;
    [[nodiscard]] double valueAt(int row, Factor factor) const;
    // Filters the table by manufacturer or name (empty: all).
    void setSearch(const QString &text);

private:
    void load(const csr::Database &db);
    void applyFilter();

    QStandardItemModel *m_model;
    SearchFilterProxy *m_proxy;
    QTableView *m_view;
    QLineEdit *m_search;
    QCheckBox *m_allLots;
    QButtonGroup *m_buttons;
    QLabel *m_count;
    const csr::Database &m_db;
    Factor m_factor{Progressivity};
};
