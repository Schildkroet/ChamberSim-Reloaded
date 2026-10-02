#pragma once

#include "loadfile.h"

namespace grt
{

// All inputs of one interior-ballistics simulation. Units are those of the
// *.grtload file and are part of the member names.
struct SimInput
{
    // caliber / cartridge
    double caseLength_mm{};
    double caseVolume_cm3{};
    double boreArea_mm2{};
    double sebert{0.5};
    double startPressure_bar{250.0};
    double maxPressure_bar{};  // permitted maximum (CIP), informative only

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
    bool extraFrictionUnlocked{};

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
    double temperature_C{21.0};
    double coldCoeff{};
    double hotCoeff{};
    double charge_g{};
    int ladderCount{1};     // load development ladder: number of curves (below 2: no ladder)
    double ladderStep_g{};  // charge difference between two ladder steps

    // Reads the numeric inputs out of a load file (missing values keep their defaults).
    [[nodiscard]] static SimInput fromLoad(const LoadFile &lf);

    // Start position of the bullet base measured from the case head [mm].
    [[nodiscard]] double startPosition_mm() const noexcept { return caseLength_mm - seatingDepth_mm; }
    [[nodiscard]] double effectiveMass_g() const noexcept { return bulletMass_g + sebert * charge_g; }
};

}  // namespace grt
