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

#include "loadfile.h"

#include <QString>
#include <QVariantMap>

#include <optional>

namespace csr
{

// Burning surface in the physical model: Fitted = the form function (a0, z1, z2); Ball, Cylinder = from a grain geometry.
enum class GrainShape
{
    Fitted,
    Ball,
    Cylinder
};

[[nodiscard]] const char *grainShapeName(GrainShape shape) noexcept;
[[nodiscard]] GrainShape grainShapeFromName(QStringView name) noexcept;

// Burn values of a powder for the physical model, fitted to its published loads (data/physical/propellants.json, kept apart from the
// records together with the record values they belong to).
struct PhysicalBurn
{
    double Ba{};
    double a0{};
    double z1{};
    double z2{};
    GrainShape grain{GrainShape::Fitted};
    double grainLd{};            // cylinder: length / diameter
    double grainHole{};          // cylinder: perforation / outer radius
    double grainAmplitude{1.0};  // scales the relative geometric surface
};

// The file of the values (default ":/data/physical/propellants.json", read on first use); setting it discards what was read.
void setPhysicalBurnFile(const QString &path);

// The physical values of a propellant record, only if its Ba, a0, z1, z2 are the ones they were fitted for.
[[nodiscard]] std::optional<PhysicalBurn> physicalBurnOf(const QVariantMap &propellant);

// The physical values of a powder by name, whatever the record's values are.
[[nodiscard]] std::optional<PhysicalBurn> findPhysicalBurn(const QString &maker, const QString &name);

// Factor on the physical burn rates of a caliber (data/physical/calibers.json next to the powder file); without an entry the class
// factor by `rimRatio` = R1 / P1 (auto-loading or rimmed; 0: unknown), else 1.
[[nodiscard]] double physicalCaliberScale(const QString &cipname, double rimRatio);

// The propellant entries physBa ... physGrainAmp and physRef (the "Ba a0 z1 z2" they belong to). write() removes them for nullopt;
// read() returns them only while the load's Ba, a0, z1, z2 equal physRef.
void writePhysicalBurn(EntryList &propellant, const std::optional<PhysicalBurn> &burn);
[[nodiscard]] std::optional<PhysicalBurn> readPhysicalBurn(const EntryList &propellant);

// Adds the physical values to a load without applicable ones if its powder is an unchanged project powder; true if added.
bool attachPhysicalBurn(EntryList &propellant);

}  // namespace csr
