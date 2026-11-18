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

// Primer of a metallic cartridge load. Start pressure ps = 250 bar + psadd (additional start pressure); the physical model adds
// (primer type's start pressure - 250 bar), fitted in data/physical/primers.json. Shotshells: shotparts.h.
#pragma once

#include <QString>
#include <QVariantMap>

#include <array>

namespace csr
{

struct PrimerType
{
    const char *code;  // stored in the load and in the caliber record
    const char *name;  // English name (translated with the context "csr::primer")
};

inline constexpr std::array<PrimerType, 9> kPrimerTypes{{{"SP", "Small Pistol"},
                                                         {"SPM", "Small Pistol Magnum"},
                                                         {"LP", "Large Pistol"},
                                                         {"LPM", "Large Pistol Magnum"},
                                                         {"SR", "Small Rifle"},
                                                         {"SRM", "Small Rifle Magnum"},
                                                         {"LR", "Large Rifle"},
                                                         {"LRM", "Large Rifle Magnum"},
                                                         {"BMG", ".50 BMG"}}};

inline constexpr double kReferenceStartPressure_bar = 250.0;  // psadd and gpressure are relative to it

[[nodiscard]] bool isPrimerCode(const QString &code);
// The translated name of a type ("Small Rifle Magnum"); an unknown code is returned as it is.
[[nodiscard]] QString primerName(const QString &code);
// The usual primer of a caliber record: its column primer, else from the case (size from P1, magnum case types, BMG); rimfire and
// shotshells: empty.
[[nodiscard]] QString primerOfCaliber(const QVariantMap &caliber);
// The start pressure [bar] of a primer type in a case of the given volume [cm3]; an empty or unknown code: the reference 250 bar.
[[nodiscard]] double primerStartPressure_bar(const QString &code, double caseVolume_cm3);

// The file of the start pressures (default ":/data/physical/primers.json", read on first use).
void setPrimerFile(const QString &path);

}  // namespace csr
