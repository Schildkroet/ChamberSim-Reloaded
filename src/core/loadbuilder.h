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
#include "numeric.h"

#include <QStringList>
#include <QVariantMap>

namespace csr
{

// What the user chooses besides the three database records. A value of 0 means "use the default".
struct LoadParams
{
    double charge_g{};         // default: 70 % of the bulk-density capacity of the case
    double barrelLength_mm{};  // default: depends on the cartridge type (pistol/rimfire/rifle)
    double seatingDepth_mm{};  // default: overall length = the cartridge's maximum length
    double twistLength_mm{240.0};
    double gunWeight_kg{};
    double powderTemperature_C{kPowderReferenceTemperature_C};
};

// Case capacity in cm^3 (the database stores grains of water).
[[nodiscard]] double caseVolume_cm3(const QVariantMap &caliber);
[[nodiscard]] double defaultBarrelLength_mm(const QVariantMap &caliber);
[[nodiscard]] double defaultSeatingDepth_mm(const QVariantMap &caliber, const QVariantMap &projectile);
[[nodiscard]] double defaultCharge_g(const QVariantMap &caliber, const QVariantMap &propellant);

// Fills missing tail dimensions of a boat-tail or hollow-base bullet with typical proportions (boat tail 0.53 D long, D to 0.835 D;
// hollow base 0.055 D deep, 0.86 D to 0.29 D). With `previousDiameter`, estimates for that diameter are redone. True if set.
bool completeTail(EntryList &projectile, double previousDiameter = 0.0);

// After a case length change: keeps the overall length and the powder space (seating depth and case volume follow, depth >= 0).
// True if anything changed.
bool followCaseLength(EntryList &caliber, EntryList &projectile, double previousLength);

// Adds the entries primer and psadd (primer.h) to a load without them: the caliber's usual primer and ps - 250 bar, so its start
// pressure stays the same. True if added.
bool ensurePrimer(LoadFile &load);
// Attaches the caliber record a load names to a load without a <caliberfile> block. True if attached.
bool attachCaliberRecord(LoadFile &load, const QVariantMap &record);
// Adds the bullet material entry gmaterial (empty: jacketed) to a load without it. True if added or completed.
bool ensureBulletMaterial(LoadFile &load);
// A Sebert factor outside 0..1 becomes 0.25. True if changed.
bool ensureSebert(LoadFile &load);
// Sets ps and gpressure from psadd; nothing for a load without psadd.
void updateStartPressure(LoadFile &load);
// The start pressure [bar] of a load's caliber section: 250 bar + psadd, or ps for a section without psadd.
[[nodiscard]] double loadStartPressure_bar(const EntryList &caliber);
// The physical model's addition to it: the primer type's start pressure - 250 bar (0 without psadd and for shotshells).
[[nodiscard]] double primerOffset_bar(const EntryList &caliber);

// Replaces one component of a load by a database record and keeps the user's other values; `resetDependent` resets the charge and
// seating depth that depend on it. Updates the title and the overall length.
void applyCaliber(LoadFile &load, const QVariantMap &caliber, const QStringList &caliberColumns = {}, bool resetDependent = false,
                  const QVariantMap *projectile = nullptr, const QVariantMap *propellant = nullptr);
void applyProjectile(LoadFile &load, const QVariantMap &projectile, bool resetDependent = false, const QVariantMap *caliber = nullptr);
void applyPropellant(LoadFile &load, const QVariantMap &propellant, bool resetDependent = false, const QVariantMap *caliber = nullptr);

// The burn and material values of the load's propellant that differ from `record`.
[[nodiscard]] QStringList propellantDifferences(const EntryList &propellant, const QVariantMap &record);

// Builds a load from one record of each table; `caliberColumns` orders the <caliberfile> block (else sorted).
[[nodiscard]] LoadFile buildLoad(const QVariantMap &caliber, const QVariantMap &projectile, const QVariantMap &propellant,
                                 LoadParams params = {}, const QStringList &caliberColumns = {});

}  // namespace csr
