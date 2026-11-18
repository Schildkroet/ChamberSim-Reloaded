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
#include <QStringList>

#include <array>
#include <vector>

namespace csr
{

// Shell, wad and primer of a shotshell load (caliber entries shothull, shotwad, shotprimer). Each has a factor on the powder space and on
// the gas energy (analysis/fit_shot_factors.py); a load's factors multiply.
enum class ShotPart
{
    Shell,
    Wad,
    Primer
};
inline constexpr std::array<ShotPart, 3> kShotParts{ShotPart::Shell, ShotPart::Wad, ShotPart::Primer};

struct ShotPartInfo
{
    QString id;                 // stored in the load file, e.g. "rem-tgt12"
    QString name;               // shown to the user
    int gauge{};                // wads: the gauge they are made for (0: any)
    double capacityScale{1.0};  // x Calibration::shotCapacityFactor
    double energyScale{1.0};    // x Calibration::shotEnergyFactor
    int loads{};                // published loads the factors were fitted to
    QString groupId;            // parts: their group
    bool group{false};          // a group of similar parts instead of one part
    QString rank;               // groups: "low", "medium" or "high" pressure
    QStringList members;        // groups: the member names, most loads first
};

// "shothull", "shotwad" or "shotprimer".
[[nodiscard]] QString shotPartKey(ShotPart part);

// The file of the parts (default ":/data/shotshell/parts.json", read on first use).
void setShotPartsFile(const QString &path);

// All parts of a kind, sorted by gauge and name.
[[nodiscard]] const std::vector<ShotPartInfo> &shotParts(ShotPart part);

// Groups of parts with a similar effect ("group-low", "group-medium", "group-high"), for users who do not know the exact part.
[[nodiscard]] const std::vector<ShotPartInfo> &shotPartGroups(ShotPart part);

// The part or group with this id, or null (the model's average applies).
[[nodiscard]] const ShotPartInfo *findShotPart(ShotPart part, const QString &id);

// The factors of the parts a shotshell load names (1 for the others).
struct ShotFactors
{
    double capacity{1.0};
    double energy{1.0};
};
[[nodiscard]] ShotFactors shotFactorsOf(const EntryList &caliber);

// Adds missing part entries (empty) to a shotshell caliber and replaces an individual part by its group. True if changed.
bool ensureShotParts(EntryList &caliber);

}  // namespace csr
