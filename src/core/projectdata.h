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

// The project's records: JSON files in data/<calibers|projectiles|propellants>/ (one record or an array), in the units of the
// database columns; built into the program and loaded read-only at startup.
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

// Reads and checks every *.json file of a table's folder below `dataDir` (directory or ":/data"); files with problems are skipped.
[[nodiscard]] Loaded load(const QString &dataDir, Table t, const Schema &schema);

// The problems of one record: unknown columns, non-numbers, no name, implausible model values.
[[nodiscard]] QStringList check(Table t, const QVariantMap &record, const Schema &schema);

// A record as the JSON file to contribute: the columns in schema order without the id and the deleted flag.
[[nodiscard]] QByteArray toJson(const QVariantMap &record, const Schema &schema);

// "vihtavuori-n150-2021-08-29.json": a file name for a record (lower case, ASCII, from its names).
[[nodiscard]] QString fileName(Table t, const QVariantMap &record);

// A manufacturer's bullet file (docs/BULLET_FILE.md): {"format": "ChamberSimReloaded bullets", "version": 1, "manufacturer", "source",
// "date", "bullets": [projectile records]}; a plain record or array is read as well.
inline constexpr const char *kBulletFileFormat = "ChamberSimReloaded bullets";
inline constexpr int kBulletFileVersion = 1;
inline constexpr const char *kBulletFileSuffix = "csrbullets";

struct BulletFile
{
    QString manufacturer;
    QString source;  // where the data come from (web address, data sheet); stored in each record's "origin"
    QString date;
    std::vector<QVariantMap> bullets;  // the usable records; "gValid" is dropped (only the user validates)
    QStringList errors;                // "bullet 3 (name): problem", or a problem of the file as a whole (then no bullets)
};

// Reads a bullet file; records with problems are reported in `errors` and left out.
[[nodiscard]] BulletFile readBulletFile(const QByteArray &json, const Schema &projectileSchema);

// The text of a bullet file (columns in schema order, as toJson).
[[nodiscard]] QByteArray toBulletFile(const BulletFile &file, const Schema &projectileSchema);

}  // namespace csr::projectdata
