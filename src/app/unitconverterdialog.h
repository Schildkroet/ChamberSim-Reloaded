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

#include "units.h"

#include <QDialog>

class QComboBox;
class QLineEdit;
class QTableWidget;

// Tools > Unit converter: a value in one unit of a quantity, shown in all units of that quantity (the units of the program, see units.h).
class UnitConverterDialog : public QDialog
{
    Q_OBJECT
public:
    explicit UnitConverterDialog(QWidget *parent = nullptr);

    void setQuantity(csr::units::Quantity quantity);
    void setFromUnit(const QString &unitId);
    void setInput(const QString &text);
    [[nodiscard]] csr::units::Quantity quantity() const;
    // The converted values as shown, one row per unit: "symbol" and "value".
    [[nodiscard]] int rowCount() const;
    [[nodiscard]] QString symbolAt(int row) const;
    [[nodiscard]] QString valueAt(int row) const;

private:
    void fillUnits();
    void convert();

    QComboBox *m_quantity;
    QLineEdit *m_input;
    QComboBox *m_from;
    QTableWidget *m_table;
};
