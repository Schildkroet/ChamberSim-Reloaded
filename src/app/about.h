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

#include <QString>

#include <optional>

class QWidget;

namespace about
{

// The records of one part of the database.
struct Counts
{
    int calibers{0};
    int projectiles{0};
    int propellants{0};
};

// The three parts of the database (Database: project records, own records, the original program's database).
struct DatabaseInfo
{
    Counts project;
    Counts own;
    Counts original;
    QString originalPath;  // empty: no original database chosen
};

// The text of the About box as rich text (version, purpose, features, accuracy, safety notice, versions of the
// libraries in use, and the parts of the database if one is open).
[[nodiscard]] QString html(const std::optional<DatabaseInfo> &database = std::nullopt);

void show(QWidget *parent, const std::optional<DatabaseInfo> &database = std::nullopt);

}  // namespace about
