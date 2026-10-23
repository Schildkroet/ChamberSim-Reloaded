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

// The primer of a (metallic) cartridge load. The load names a primer type and has its own "additional start pressure" (psadd) for the
// bullet, crimp and case on top of the reference 250 bar:
//
//   ps = 250 bar + additional start pressure                                 (the load's start pressure, as in the original program)
//   physical model: ps + (start pressure of the primer type - 250 bar)
//
// Only the physical model uses the primer: its start pressures are fitted with that model to published loads
// (data/physical/primers.json, analysis/fit_primers.py); the Original and Published models are calibrated with the original's
// ps and get it unchanged. Without the file every type starts at the reference 250 bar. A bullet record's start pressure (gpressure)
// and the ps of an original *.grtload are relative to the same reference: a load gets ps - 250 bar as its additional start pressure.
// A caliber record names its usual primer (column primer); records without one get it from the case (primerOfCaliber).
// Shotshells have their own primer choice (shotparts.h).
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

inline constexpr double kReferenceStartPressure_bar = 250.0;  // the start pressure of a load without primer; gpressure is relative to it

[[nodiscard]] bool isPrimerCode(const QString &code);
// The translated name of a type ("Small Rifle Magnum"); an unknown code is returned as it is.
[[nodiscard]] QString primerName(const QString &code);
// The usual primer of a caliber record: its column primer when that is a known type, else from the case (base diameter P1 below 11.0 mm
// for pistol and revolver cartridges, 11.25 mm for rifles: small; case types "3 ..." magnum; .50 BMG and 14.5 mm: BMG). Rimfire and
// shotshell cartridges: none (empty).
[[nodiscard]] QString primerOfCaliber(const QVariantMap &caliber);
// The start pressure [bar] of a primer type in a case of the given volume [cm3]; an empty or unknown code: the reference 250 bar.
[[nodiscard]] double primerStartPressure_bar(const QString &code, double caseVolume_cm3);

// The file the start pressures are read from (default ":/data/physical/primers.json", read once on first use); tools and tests set another.
void setPrimerFile(const QString &path);

}  // namespace csr
