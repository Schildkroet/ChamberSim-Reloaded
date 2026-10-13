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

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QVariantMap>

#include <utility>
#include <vector>

// The project's own records (calibers, projectiles, propellants contributed to the repository): JSON files in
// data/<calibers|projectiles|propellants>/, each with one record (an object) or several (an array of objects; the
// projectiles have one file per manufacturer), the values in the units of the database columns (as in the
// original program's database: bullet mass and case capacity in grains, lengths in mm, pressures in bar, ...).
// The files are built into the program and loaded into a read-only part of the database at startup.
namespace csr::projectdata
{

// Column name -> SQL type of a table, in schema order (Database::schema).
using Schema = std::vector<std::pair<QString, QString>>;

// "calibers", "projectiles", "propellants".
[[nodiscard]] QString folderName(Table t);

struct Loaded
{
    std::vector<QVariantMap> records;  // sorted by file name, then in the order of the file; no "id"
    QStringList errors;                // "file: problem", one per problem
};

// Reads and checks every *.json file (one record or an array of them) of a table's folder below `dataDir` (a directory or a resource path like
// ":/data"). Files with problems are reported and skipped.
[[nodiscard]] Loaded load(const QString &dataDir, Table t, const Schema &schema);

// Checks one record (as read from a file): known columns, numbers where the column is numeric, a name, and
// plausible values for the model (e.g. a burn rate > 0). Returns the problems (empty: fine).
[[nodiscard]] QStringList check(Table t, const QVariantMap &record, const Schema &schema);

// A record as the JSON file to contribute: the columns in schema order without the id and the deleted flag.
[[nodiscard]] QByteArray toJson(const QVariantMap &record, const Schema &schema);

// "vihtavuori-n150-2021-08-29.json": a file name for a record (lower case, ASCII, from its names).
[[nodiscard]] QString fileName(Table t, const QVariantMap &record);

}  // namespace csr::projectdata
