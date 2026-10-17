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

// Burn values of a powder for the physical model (CalibrationProfile::Physical): the burn rate Ba and either the form function
// (a0, z1, z2) or an effective grain geometry, fitted with that model to the powder's published loads (velocities and measured
// pressures; analysis/fit_physical_powders.py, analysis/fit_grain_geometry.py). The project's records carry the values of the
// original program's model; the physical ones are kept apart in data/physical/propellants.json, together with the record values
// they were fitted for.
// Shape of the burning surface used by the physical model. Fitted keeps the project's three-phase form function (a0, z1, z2);
// Ball and Cylinder derive phi(z) from a grain geometry instead. Cylinder is a single-perforated cylinder burning on both ends;
// grainLd is its length / diameter and grainHole its perforation radius / outer radius (0..1). grainAmplitude scales the
// normalized geometric surface to the powder's fitted form function (it plays the role of the initial surface ratio).
enum class GrainShape
{
    Fitted,
    Ball,
    Cylinder
};

[[nodiscard]] const char *grainShapeName(GrainShape shape) noexcept;
[[nodiscard]] GrainShape grainShapeFromName(QStringView name) noexcept;

struct PhysicalBurn
{
    double Ba{};
    double a0{};
    double z1{};
    double z2{};
    GrainShape grain{GrainShape::Fitted};
    double grainLd{};
    double grainHole{};
    double grainAmplitude{1.0};
};

// The file the values are read from (default ":/data/physical/propellants.json", read once on first use); tools and tests set the
// source directory's file. Setting it discards what was read.
void setPhysicalBurnFile(const QString &path);

// The physical values of a propellant record (mname, pname, Ba, a0, z1, z2): only when the file has the powder and the record's
// values are the ones the physical values were fitted for (a changed or other record of the same powder gets none).
[[nodiscard]] std::optional<PhysicalBurn> physicalBurnOf(const QVariantMap &propellant);

// The physical values of a powder by its name, whatever the burn values of the record or load are (they may be the original's, or
// edited): what a user can be offered for a load that has none that apply.
[[nodiscard]] std::optional<PhysicalBurn> findPhysicalBurn(const QString &maker, const QString &name);

// The factor on the physical burn rates for a pistol or revolver cartridge (data/physical/calibers.json, next to the powder file): the
// published loads of an auto-loading cartridge (small rim, about 1.1) and of a revolver cartridge (rimmed, about 0.95) are not met by
// the same burn rate of a powder, a caliber with enough published loads has its own factor. `rimRatio` is the rim over the head diameter
// (R1 / P1, 0 = unknown), which decides the class of a caliber without an entry. 1 for anything else (the file is missing, ...).
[[nodiscard]] double physicalCaliberScale(const QString &cipname, double rimRatio);

// In a load file the values are the propellant entries physBa, physA0, physZ1, physZ2, physGrain, physGrainLd, physGrainHole,
// physGrainAmp and physRef (the record's "Ba a0 z1 z2" they belong to). write() sets them, or removes them for std::nullopt; read()
// returns them only while the load's Ba, a0, z1 and z2 still equal physRef (after an edit of those values the physical ones no longer
// apply). A load saved before the grain entries existed keeps the cubic form function.
void writePhysicalBurn(EntryList &propellant, const std::optional<PhysicalBurn> &burn);
[[nodiscard]] std::optional<PhysicalBurn> readPhysicalBurn(const EntryList &propellant);

// Adds the physical values to a load that has none that apply (saved before they existed) when its powder is a project powder with
// unchanged values. Returns whether it added them.
bool attachPhysicalBurn(EntryList &propellant);

}  // namespace csr
