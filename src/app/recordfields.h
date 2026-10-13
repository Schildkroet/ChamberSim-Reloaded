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

#include "dbfields.h"

#include <QString>

namespace recordfields
{

// The unit a field is shown in (null: none), from the user's unit settings.
[[nodiscard]] const csr::units::Unit *unitOf(const csr::dbfields::Field &f);

// A stored number as shown to the user: the unit's usual decimals, more only where the value has them; no thousands
// separators (used in the input fields of the record dialog and in the record viewer).
[[nodiscard]] QString shownNumber(const csr::dbfields::Field &f, double stored);

}  // namespace recordfields
