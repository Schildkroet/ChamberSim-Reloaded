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
#include "physicalpowders.h"

#include <optional>

namespace csr
{

// All inputs of one interior-ballistics simulation. Units are those of the
// *.grtload file and are part of the member names.
struct SimInput
{
    // caliber / cartridge
    double caseLength_mm{};
    double caseVolume_cm3{};
    double boreArea_mm2{};
    double grooveDiameter_mm{};    // 0: unknown
    double caliberBurnScale{1.0};  // factor on the physical burn rate for this caliber (data/physical/calibers.json)
    double sebert{0.5};
    double startPressure_bar{250.0};
    double primerOffset_bar{};  // start pressure of the load's primer type - 250 bar: added by the physical model only (primer.h)
    double maxPressure_bar{};   // permitted maximum (CIP), informative only
    double shotCapacityScale{
        1.0};  // factors of the shell, wad and primer named in the caliber on the shotshell model (shotparts.h; 1: none chosen)
    double shotEnergyScale{1.0};
    bool handgun{};    // pistol or revolver cartridge (C.I.P. table IV)
    bool shotshell{};  // shotgun cartridge (caliber flag): wad and shot column instead of a bullet

    // gun
    double barrelLength_mm{};
    double twistLength_mm{};
    double gunWeight_kg{};
    // free run of the bullet before it reaches the rifling [mm]; the original uses it also when the field is locked
    double bulletJump_mm{};

    // projectile
    double bulletDiameter_mm{};
    double bulletMass_g{};
    double bulletLength_mm{};
    double seatingDepth_mm{};
    double tailLength_mm{};
    double tailDiameterA_mm{};
    double tailDiameterB_mm{};
    int tailType{};  // 0 flat base, 1 hollow base, 2 boat tail
    double extraFriction_bar{};

    // propellant
    double vivacityBa{};
    double heatQex_kJkg{};
    double kappa{1.22};  // ratio of specific heats k
    double a0{};
    double a1{};
    double z1{};
    double z2{};
    double covolumeEta{1.0};
    double density_kgm3{1600.0};
    double bulkDensity_kgm3{900.0};
    double temperature_C{kPowderReferenceTemperature_C};
    double coldCoeff{};
    double hotCoeff{};
    double charge_g{};
    int ladderCount{1};     // load development ladder: number of curves (below 2: no ladder)
    double ladderStep_g{};  // charge difference between two ladder steps
    // burn values of the physical model (load entries physBa ...; only while the load's Ba, a0, z1, z2 are the ones they belong to)
    std::optional<PhysicalBurn> physicalBurn;

    // Reads the numeric inputs out of a load file (missing values keep their defaults).
    [[nodiscard]] static SimInput fromLoad(const LoadFile &lf);

    // Start position of the bullet base measured from the case head [mm].
    [[nodiscard]] double startPosition_mm() const noexcept { return caseLength_mm - seatingDepth_mm; }
    [[nodiscard]] double effectiveMass_g() const noexcept { return bulletMass_g + sebert * charge_g; }
};

}  // namespace csr
