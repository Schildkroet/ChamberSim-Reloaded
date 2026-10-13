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
    double powderTemperature_C{21.0};
};

// Case capacity in cm^3 (the database stores grains of water).
[[nodiscard]] double caseVolume_cm3(const QVariantMap &caliber);
[[nodiscard]] double defaultBarrelLength_mm(const QVariantMap &caliber);
[[nodiscard]] double defaultSeatingDepth_mm(const QVariantMap &caliber, const QVariantMap &projectile);
[[nodiscard]] double defaultCharge_g(const QVariantMap &caliber, const QVariantMap &propellant);

// Fills the tail dimensions of a boat-tail or hollow-base bullet that the projectile section lacks (the base type then has an effect):
// tail length 0, or for a boat tail a base diameter 0 (a tail ending in a point). Typical proportions of the bullets that have them
// (medians of 1312 boat tails and 217 hollow bases): boat tail length 0.53 D, diameters D and 0.835 D; hollow base cavity 0.055 D deep,
// 0.86 D wide at the base, 0.29 D at the top. With `previousDiameter` (the bullet diameter was changed) dimensions that are still the
// estimates for the previous diameter are estimated anew; entered ones stay. Returns whether a value was set.
bool completeTail(EntryList &projectile, double previousDiameter = 0.0);

// After the case length (caliber caselen) was changed from `previousLength`: the bullet stays where it is (the cartridge overall length
// is kept), so the seating depth changes by the same amount, and the case volume (to the mouth) by the bullet's cross-section times
// the change (the neck's inside); the powder space behind the bullet stays the same. The seating depth does not go below 0 (the
// overall length then follows). Returns whether anything changed.
bool followCaseLength(EntryList &caliber, EntryList &projectile, double previousLength);

// Replaces one component of an existing load by a database record. Everything else the user has set
// (gun data, seating depth, charge, temperature, ...) is kept unless `resetDependent` asks for the
// defaults that belong to the new record: the charge and seating depth for a caliber change, the
// seating depth for a bullet, the charge for a powder. The title and the overall length are updated.
void applyCaliber(LoadFile &load, const QVariantMap &caliber, const QStringList &caliberColumns = {}, bool resetDependent = false,
                  const QVariantMap *projectile = nullptr, const QVariantMap *propellant = nullptr);
void applyProjectile(LoadFile &load, const QVariantMap &projectile, bool resetDependent = false, const QVariantMap *caliber = nullptr);
void applyPropellant(LoadFile &load, const QVariantMap &propellant, bool resetDependent = false, const QVariantMap *caliber = nullptr);

// The burn and material values (Ba, Qex, k, a0, a1, z1, z2, eta, pc, pcd, tcc, tch) of the load's propellant that differ from `record`
// (an older state of the database record, or edited in the load); empty when they all agree.
[[nodiscard]] QStringList propellantDifferences(const EntryList &propellant, const QVariantMap &record);

// Builds a complete load from one record of each table. `caliberColumns` (schema order) keeps the
// <caliberfile> block in the order of the original; without it the keys are sorted.
[[nodiscard]] LoadFile buildLoad(const QVariantMap &caliber, const QVariantMap &projectile, const QVariantMap &propellant,
                                 LoadParams params = {}, const QStringList &caliberColumns = {});

}  // namespace csr
