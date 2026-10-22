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

class QApplication;

// Light / dark appearance and a few named color schemes. "System" keeps the platform's palette.
namespace theme
{

enum class Mode
{
    System,
    Light,
    Dark,
    GruvboxLight,
    GruvboxDark,
    Nord,
    Dracula,
    CatppuccinLatte,
    CatppuccinFrappe
};

// Stable name of a mode (settings value, --theme), "system" for an unknown one
[[nodiscard]] QString modeKey(Mode mode);
[[nodiscard]] Mode modeFromKey(const QString &key);

[[nodiscard]] Mode storedMode();
void storeMode(Mode mode);

// Applies the mode to the whole application (palette + Fusion style for the dark variant).
void apply(QApplication &app, Mode mode);

}  // namespace theme
