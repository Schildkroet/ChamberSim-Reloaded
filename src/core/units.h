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
#include <QStringList>

#include <optional>
#include <span>

namespace csr::units
{

// The physical quantities of the program. Values in load files and in the database are stored in
// the *base unit* of their quantity (mm, g, kg, bar, m/s, mm2, cm3, kg/m3, Cel, J, Ns, N).
enum class Quantity
{
    None,         // no conversion (dimensionless, text, specialised units such as kJ/kg)
    Length,       // base mm
    BulletMass,   // base g
    GunMass,      // base kg
    Pressure,     // base bar
    Velocity,     // base m/s
    Area,         // base mm2
    Volume,       // base cm3
    Density,      // base kg/m3
    Temperature,  // base Cel
    Energy,       // base J
    Impulse,      // base Ns
    Force,        // base N
    Time,         // base ms
};

// One way of showing a quantity: shown = (base - offset) / factor  (offset only for temperatures).
struct Unit
{
    const char *id;      // stable key, used in the settings
    const char *symbol;  // shown to the user
    double factor;       // base units per shown unit (1 inch = 25.4 mm)
    double offset;       // base value at shown 0 (0 except for Fahrenheit)
    int decimals;        // default decimals
};

// Unit systems: a preset choice of one unit per quantity.
enum class System
{
    Metric,
    Imperial,
    Original
};

[[nodiscard]] std::span<const Unit> unitsOf(Quantity q);
[[nodiscard]] const Unit &systemUnit(Quantity q, System s);
[[nodiscard]] const Unit *findUnit(Quantity q, const QString &id);

// The quantity of a unit string as it appears in the files ("mm", "grain", "bar", ...).
[[nodiscard]] Quantity quantityOfFileUnit(const QString &unit);

[[nodiscard]] double toDisplay(double base, const Unit &u) noexcept;
[[nodiscard]] double fromDisplay(double shown, const Unit &u) noexcept;

// "8.20" style text with the unit's decimals, using the application locale.
[[nodiscard]] QString format(double base, const Unit &u, int decimals = -1);

// Parses a user-typed number (locale or '.'), returns the value in the *base* unit.
[[nodiscard]] std::optional<double> parseToBase(const QString &text, const Unit &u);

[[nodiscard]] QString systemName(System s);

}  // namespace csr::units
