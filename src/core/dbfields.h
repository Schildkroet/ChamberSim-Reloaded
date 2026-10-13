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
#include "units.h"

#include <QString>

#include <utility>
#include <vector>

namespace csr::dbfields
{

enum class Kind
{
    Text,
    Number,
    Date,
    LongText,
    BaseType,
    Choice,
    Hidden
};

// How a database column is shown and edited: a readable label, its group, the physical quantity and the factor from
// the value stored in the database to the quantity's base unit (bullet masses and case capacities are stored in
// grains). Columns that are not described get their name as the label and are shown as plain values.
struct Field
{
    QString column;
    QString label;
    QString group;
    units::Quantity quantity{units::Quantity::None};
    double toBase{1.0};
    Kind kind{Kind::Text};
    bool readOnly{false};    // set by the program (date of the last change, image ids)
    bool required{false};    // a record needs it (its name)
    bool alignRight{false};  // numbers, and the numbers that are stored as text (angles of the caliber table)
    QString unitText;        // shown for numbers without a quantity (e.g. "kJ/kg")
};

// Describes a column; `sqlType` (TEXT, FLOAT, INTEGER, DATE) decides the kind of undescribed columns.
[[nodiscard]] Field describe(Table t, const QString &column, const QString &sqlType);

// The groups of a table in display order.
[[nodiscard]] QStringList groups(Table t);

// Countries for "country of origin": ISO 3166 codes (as the original database uses them) with their English
// names, plus the historical codes found in it (SU, CS, RSA), sorted by code.
[[nodiscard]] std::vector<std::pair<QString, QString>> countries();

// The allowed values (stored value, shown text) of a column of kind Choice: country of origin, standard, case
// type. The stored values are those of the original database (the program reads them: "SAAMI", "4 pistol/revolver").
[[nodiscard]] std::vector<std::pair<QString, QString>> choices(Table t, const QString &column);

// "Manufacturer Name (lot)", "cipname" ... : how a record is named in messages.
[[nodiscard]] QString recordTitle(Table t, const QVariantMap &record);

}  // namespace csr::dbfields
