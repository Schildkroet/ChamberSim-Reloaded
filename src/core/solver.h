#pragma once

#include "simconfig.h"

#include <cstddef>
#include <vector>

namespace grt {

struct SimPoint {
    double x_mm{};   // bullet base position measured from the case head
    double z{};      // burned fraction of the propellant, 0..1
    double p_bar{};  // pressure
    double v_ms{};   // bullet velocity
    double e_J{};    // bullet kinetic energy
    double t_ms{};   // time since the bullet started to move
};

enum class SimStatus {
    Ok,
    InvalidInput,
    ChargeTooHigh,  // charge exceeds the bulk-density capacity of the case
};

struct SimResult {
    SimStatus status{SimStatus::InvalidInput};
    std::vector<SimPoint> points;

    double maxPressure_bar{};
    double muzzlePressure_bar{};
    double muzzleVelocity_ms{};
    double muzzleEnergy_J{};
    double muzzleTime_ms{};
    double burntFraction{};     // z at the muzzle
    double efficiency{};        // muzzle energy / total chemical energy of the charge
    std::size_t indexPmax{};
    std::size_t indexBurnout{}; // first point with z == 1, or last point if not burnt out
    bool burnoutInBarrel{};
    double burnoutPressure_bar{};
    std::size_t indexZ1{};            // first point with z >= z1 (progressive -> degressive), 0 if never
    std::size_t indexZ2{};
    std::size_t indexEnergy95{};      // first point where the bullet has 95 % of its muzzle energy

    double effectiveCaseVolume_cm3{}; // case volume minus the part of the bullet inside the case
    double loadRatio{};               // charge / (bulk density * effective case volume)
    double muzzleRpm{};               // rotation speed of the bullet at the muzzle (needs the twist length)

    // derived quantities (formulas identified from the original's results)
    double propellantEnergy_J{};      // chemical energy released up to the muzzle: Qex * mc * z
    double frictionForce_N{};         // built-in bullet friction, constant along the barrel
    double muzzleFrictionEnergy_J{};  // friction work over the whole travel
    double ipscFactor{};              // bullet grains * velocity [ft/s] / 1000
    double mipFactor{};
    double projectileImpulse_Ns{};
    double propellantImpulse_Ns{};    // Sebert-weighted charge moving with the bullet
    double gasImpulse_Ns{};           // outflow of the powder gases after the muzzle
    double recoilImpulse_Ns{};
    double meanRecoilForce_N{};       // impulse / bullet time
    double recoilVelocity_ms{};       // needs the gun weight
    double recoilEnergy_J{};
    double recoilAcceleration_ms2{};

    [[nodiscard]] bool ok() const noexcept { return status == SimStatus::Ok; }
};

// One curve of the load development ladder: the selected charge is the maximum, every further step
// has `step` less powder (this is what the manual describes; checked against the original's results).
struct LadderStep {
    double charge_g{};
    SimResult result;
};

inline constexpr int kMaxLadderSteps = 10;

// Tunable constants of the gas model. The structure of the model (uniform
// position steps, implicit momentum balance, Sebert mass, initial gas energy)
// was identified from reference runs of the original program; the values below
// are *provisional* least-squares calibrations and are expected to change.
struct Calibration {
    double impetusFactor{1.05076};        // scales f = (k-1)*Qex
    double kineticFactor{1.09578};        // scales the kinetic-energy term in the energy balance
    // covolume b of the powder gas = eta * (covolume_cm3g + covolumeDensityFactor / material density): measured on
    // the original, powders with a low material density pc have a much larger covolume (b ~ 1.08 / pc)
    double covolume_cm3g{0.0132223};
    double covolumeDensityFactor{1.34175};
    double intrusionFactor{1.32743};      // fraction of the bullet's volume inside the case that reduces the chamber
    double chamberOffset_cm3{0.0};     // empirical chamber-volume offset
    double capacityFactor{1.20396};        // scales the case capacity (database value) in the chamber volume
    double chamberOffsetLength_mm{0.248693}; // extra chamber length behind the bullet base, times the bore area
    double hotCoeffDefault{0.000962335};    // relative burn-rate change per K above 21 degC (used if tch == 0)
    double coldCoeffDefault{0.000677371};   // relative burn-rate change per K below 21 degC (used if tcc == 0)
    double explicitColdScale{0.025};        // explicit tcc: relative burn-rate change = scale * tcc * (21 - T)
    double explicitHotScale{0.0272};        // explicit tch: relative burn-rate change = scale * tch * (T - 21)

    // Burn law dz/dt = Ba * p[bar] * phi(z). Identified from the reference powder sweep:
    //   phase 1 (z < z1):   phi = burnIntercept + (burnSlopeA0 * a0 + burnSlopeOffset) * z
    //   phase 2 (z1..z2):   phi = phi(z1) * poly2(s),  s = (z - z1) / (z2 - z1)
    //   phase 3 (z > z2):   phi = phi(z1) * poly3(t),  t = (z - z2) / (1 - z2)
    // poly coefficients are listed from the highest power down.
    // Derived outputs.
    double frictionStress_MPa{0.907};     // built-in friction force = stress * pi * D * bullet length
    double gasImpulseFactor{1.1622};      // gas impulse after the muzzle = factor * mc * v
    double mipFactor{1.975};              // MIP = factor * IPSC
    double manualFrictionFactor{3.49e-4}; // fraction of the pressure force lost per bar of the gfriction input

    double burnIntercept{1.11851};
    double burnSlopeA0{0.379371};
    double burnSlopeOffset{-0.0252242};
    double initialGasFactor{0.546915};          // weight of the initial-gas energy offset (1 = reproduces ps exactly)
    double burnZ0Factor{0.0};              // the form function sees z + burnZ0Factor * z0 (initial gas counts as burnt)
    double burnExponent{-0.0240318};
    // Measured on the original with fast powders: the energy of the burnt powder reaches the gas only by the fraction
    // 1 - exp(-ignitionRate * (step + ignitionLead)), counted in position steps (independent of the step length,
    // bullet mass and charge). It limits the peak pressure of fast powders; slow ones have burnt little by then.
    double ignitionRate{0.157903};
    double ignitionLead{4.63226};          // dz/dt scales with (p/1000 bar)^burnExponent on top of the linear p
    double phase2[4]{0.316743, 0.138886, -0.968774, 1.09811};
    double phase3Limit{0.7};       // the phase-3 shape is held constant beyond this t = (z - z2)/(1 - z2)
    double phase3[4]{-3.52729, 4.41272, -1.31126, 0.380902};
};

// Named access to every tunable constant (for calibration tools and tests).
struct NamedParameter {
    const char *name;
    double *value;
};
[[nodiscard]] std::vector<NamedParameter> parameters(Calibration &cal);

// Runs the simulation over `steps` equal position steps from the start position to the muzzle.
[[nodiscard]] SimResult simulate(const SimInput &in, const Calibration &cal = {}, int steps = 2000);

// Burn-rate coefficient Ba * phi(z) for n values of z in [0, 1] (the "vivacity" curve).
[[nodiscard]] std::vector<double> vivacityCurve(const SimInput &in, const Calibration &cal = {}, int n = 101);

// Volume of the bullet's part inside the case [m^3] (cylinder minus the boat-tail frustum).
[[nodiscard]] double bulletIntrusion_m3(const SimInput &in);

// Simulates the whole ladder. Without a ladder (count below 2) the result has one step, the selected
// charge. Steps that would have no powder left are omitted. Index 0 is the maximum charge.
[[nodiscard]] std::vector<LadderStep> simulateLadder(const SimInput &in, const Calibration &cal = {}, int steps = 2000);

// Chamber volume at the start (case volume minus the part of the bullet inside the case) [m^3].
[[nodiscard]] double chamberVolume_m3(const SimInput &in, const Calibration &cal = {});

}  // namespace grt
