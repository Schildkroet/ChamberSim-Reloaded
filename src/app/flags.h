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

#include <QIcon>
#include <QString>

// Small national flags for the language menu (resources/flags: SVG sources and the PNG icons built from them).
namespace flags
{

// The flag of a language's country ("de_DE" -> Germany, "en_US" -> USA); a null icon if there is none.
[[nodiscard]] QIcon forLanguage(const QString &language);

}  // namespace flags
