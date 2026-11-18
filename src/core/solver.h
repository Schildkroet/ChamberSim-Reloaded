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

#include "simconfig.h"

#include <array>
#include <cstddef>
#include <limits>
#include <vector>

namespace csr
{

// One point of the curves, at the end of a position step.
struct SimPoint
{
    double x_mm{};   // bullet base position measured from the case head
    double z{};      // burned fraction of the propellant, 0..1
    double p_bar{};  // pressure
    double v_ms{};   // bullet velocity
    double e_J{};    // bullet kinetic energy
    double t_ms{};   // time since the bullet started to move
};

enum class SimStatus
{
    Ok,
    InvalidInput,
    ChargeTooHigh,  // charge exceeds the bulk-density capacity of the case
    BulletStuck,    // physical model: the gas cannot push the bullet out of the barrel (or not even start it)
};

// Pressure before the bullet moves (physical model; t <= 0, the bullet starts at t = 0).
struct RestPoint
{
    double t_ms{};
    double p_bar{};
};

// Curves and summary values of one simulation. Only meaningful when ok().
struct SimResult
{
    SimStatus status{SimStatus::InvalidInput};
    std::vector<SimPoint> points;
    std::vector<RestPoint> restPhase;  // physical model: pressure rise before the bullet moves

    double maxPressure_bar{};
    double muzzlePressure_bar{};
    double muzzleVelocity_ms{};
    double muzzleEnergy_J{};
    double muzzleTime_ms{};
    double pmax10Time_ms{};          // time at which the pressure first reaches 10 % of the maximum
    double optimalBarrelTime_ms{};   // OBT: time of the nearest node (see obt.h), 0 without one
    double optimalBarrelNode{-1.0};  // its number (0, 0.5, ... 24.5), -1 without one
    double bulletLeadTime_ms{};      // BLT: muzzle time minus that (the original's BulletLeadTime10Pmax)
    double burntFraction{};          // z at the muzzle
    double efficiency{};             // muzzle energy / total chemical energy of the charge
    std::size_t indexPmax{};
    std::size_t indexBurnout{};  // first point with z == 1, or last point if not burnt out
    bool burnoutInBarrel{};
    double burnoutPressure_bar{};
    std::size_t indexZ1{};  // first point with z >= z1 (progressive -> degressive), 0 if never
    std::size_t indexZ2{};
    std::size_t indexEnergy95{};  // first point where the bullet has 95 % of its muzzle energy

    double effectiveCaseVolume_cm3{};  // case volume minus the part of the bullet inside the case
    double loadRatio{};                // charge / (bulk density * effective case volume)
    double muzzleRpm{};                // rotation speed of the bullet at the muzzle (needs the twist length)

    // derived quantities (formulas identified from the original's results)
    double propellantEnergy_J{};      // chemical energy released up to the muzzle: Qex * mc * z
    double frictionForce_N{};         // built-in bullet friction, constant along the barrel
    double muzzleFrictionEnergy_J{};  // work of that force over the bore (without engraving and air)
    double ipscFactor{};              // bullet grains * velocity [ft/s] / 1000
    double mipFactor{};
    double projectileImpulse_Ns{};
    double propellantImpulse_Ns{};  // Sebert-weighted charge moving with the bullet
    double gasImpulse_Ns{};         // outflow of the powder gases after the muzzle
    double recoilImpulse_Ns{};
    double meanRecoilForce_N{};  // impulse / bullet time
    double recoilVelocity_ms{};  // needs the gun weight
    double recoilEnergy_J{};
    double recoilAcceleration_ms2{};
    double heatLoss_J{};  // physical model: heat lost to the walls up to the muzzle

    [[nodiscard]] bool ok() const noexcept { return status == SimStatus::Ok; }
};

// One curve of the load ladder: the selected charge is the maximum, each further step has `step` less powder.
struct LadderStep
{
    double charge_g{};
    SimResult result;
};

inline constexpr int kMaxLadderSteps = 10;

// Which equations the solver uses.
//   Original:  identified from the original program's results, artifacts included (position steps, ignition ramp).
//   Physical:  the project's own model (solver.cpp): integrated in time, resistance and heat loss in the energy balance; fitted to
//              published load data only.
enum class SolverModel
{
    Original,
    Physical
};

// A ...Handgun constant with this value takes the rifle value.
inline constexpr double kSameAsRifle = std::numeric_limits<double>::quiet_NaN();

// Tunable constants. The member defaults are the Original model's fit to the original program (analysis/calibrate.py,
// docs/ACCURACY.md); calibrationFor() sets the other profiles. A unit suffix gives the unit; the others are factors.
struct Calibration
{
    // ---- burn rate ----
    double burnRateScale{1.0};  // scales the powder's Ba
    // ---- gas and energy ----
    double impetusFactor{1.01455};  // scales f = (k-1)*Qex
    double kineticFactor{1.00787};  // scales the kinetic-energy term in the energy balance
    // covolume of the gas = covolume_cm3g + covolumeDensityFactor / pc (the original ignores eta); unburnt powder: solidDensityFactor / pc
    double covolume_cm3g{0.0905643};
    double solidDensityFactor{0.955343};
    double covolumeDensityFactor{1.06641};
    // ---- chamber volume (chamberVolume_m3) ----
    double intrusionFactor{1.13549};           // fraction of the bullet's volume inside the case that reduces the chamber
    double chamberOffset_cm3{0.0};             // empirical chamber-volume offset
    double capacityFactor{1.10205};            // scales the case capacity (database value) in the chamber volume
    double chamberOffsetLength_mm{-0.139628};  // extra chamber length behind the bullet base, times the bore area
    // ---- bullet friction set by the user (gfriction, only when unlocked) ----
    double manualFrictionFactor{3.49e-4};  // fraction of the pressure force lost per bar of the gfriction input
    // ---- powder temperature (burnCoefficient) ----
    double hotCoeffDefault{0.000962335};   // relative burn-rate change per K above 21 degC (used if tch == 0)
    double coldCoeffDefault{0.000677371};  // relative burn-rate change per K below 21 degC (used if tcc == 0)
    double explicitColdScale{0.025};       // explicit tcc: relative burn-rate change = scale * tcc * (21 - T)
    double explicitHotScale{0.0272};       // explicit tch: relative burn-rate change = scale * tch * (T - 21)

    // ---- shotshells (caliber flag "shotshell"; Original equations) ----
    // The projectile is the wad with the shot; ps is the crimp release (about 20 bar). Fitted to 64 published shotgun loads
    // (analysis/fit_shotgun.py, input convention in docs/ACCURACY.md); not matched to the original program.
    double shotCapacityFactor{1.863};  // powder space x factor: the wad cushion compresses, the plastic hull expands
    double shotFrictionShare{0.0317};  // share of the pressure force lost to the wad and shot sliding in the bore
    double shotEnergyFactor{1.0};      // gas energy x factor (gas and heat losses; not above 1)

    // ---- derived outputs (do not affect the curves) ----
    double frictionStress_MPa{0.907};  // built-in friction force = stress * pi * D * bullet length
    double gasImpulseFactor{1.1622};   // gas impulse after the muzzle = factor * mc * v
    double mipFactor{1.975};           // MIP = factor * IPSC

    // ---- physical model (model == SolverModel::Physical) ----
    // Also uses burnRateScale, impetusFactor, capacityFactor, the burn law and the form function. Background: docs/SOLVER.md.
    SolverModel model{SolverModel::Original};
    double gasCovolume_cm3g{1.0};  // covolume of the gas (x the powder's eta); the solid powder takes 1 / pc
    double primerEnergy_J{50.0};   // energy of the primer gas
    double primerTime_ms{0.05};    // time constant of its release (0: at once)
    double ignitionTime_ms{0.0};   // flame spread: lit share of the grains 1 - exp(-t / this) (0: all at once)
    // fill = charge / (bulk density x powder space); 0: off. Tested, not used (docs/SOLVER.md, "Charge and loading density").
    double ignitionFillExponent{0.0};  // flame spread time x (fill / 0.85)^this
    double heatFillExponent{0.0};      // heat loss x (fill / 0.85)^-this
    double slideFillExponent{0.0};     // sliding friction x (fill / 0.85)^-this
    // start resistance: share x start pressure x bore area (fitted, not a physical engraving force), decaying over engravingLength_mm
    double engravingShare{0.5};
    double engravingPressure_bar{0.0};  // > 0: absolute engraving pressure instead of the share
    double engravingLength_mm{0.0};     // 0: the bearing length
    double slideStress_MPa{3.0};        // friction stress on the bearing surface
    double neckStress_MPa{0.0};         // case-neck friction over the seated length; 0: a constant release force during the free run
    // oversize e = (bullet - groove diameter) / groove diameter [%] scales engraving and sliding by max(1 + k e, 0.25)
    double engravingInterference{0.0};
    double slideInterference{0.0};
    double leadSlideFactor{1.0};     // sliding stress of jacketless lead bullets
    double caliberBurnScaling{1.0};  // 1: physical burn rates x the caliber's factor (physicalCaliberScale)
    double heatTransferFactor{0.5};  // x the Dittus-Boelter coefficient
    double heatVelocity_ms{0.0};     // gas velocity of the heat transfer on top of v / 2 (turbulence)
    // moving share of the charge in momentum and kinetic energy (Lagrange: 1/3); the Original model uses the load's Sebert factor
    double gasMassShare{0.25};
    double gasShareRatioExponent{0.0};  // share x ((charge / bullet) / 0.3)^-this
    // share x (1 - c (V0 / V)^3), c = this x (1 - (A / Ac)^2): the gas in a chamber wider than the bore moves slower (0: off, 1: full)
    double chambrage{0.0};
    double burnAtBreech{1.0};           // 1: the powder burns at the breech pressure, 0: at the space-mean pressure
    double noseFromMass{0.0};           // 1: nose length from the bullet's mass (noseLength_m), 0: one calibre
    double bulletInertiaFactor{0.45};   // moment of inertia = factor x mass x radius^2
    double gasConstant_JkgK{340.0};     // specific gas constant of the powder gas
    double integrationTolerance{1e-7};  // relative error per step
    // Pistol and revolver cartridges (C.I.P. table IV): own values of these constants, applied by handgunCalibration().
    double slideStressHandgun_MPa{kSameAsRifle};
    double engravingShareHandgun{kSameAsRifle};
    double engravingLengthHandgun_mm{kSameAsRifle};
    double leadSlideFactorHandgun{kSameAsRifle};
    double burnExponentHandgun{kSameAsRifle};
    double gasMassShareHandgun{kSameAsRifle};
    double gasShareRatioExponentHandgun{kSameAsRifle};
    double chambrageHandgun{kSameAsRifle};
    double gasCovolumeHandgun_cm3g{kSameAsRifle};
    double capacityFactorHandgun{kSameAsRifle};
    double impetusFactorHandgun{kSameAsRifle};
    double slideInterferenceHandgun{kSameAsRifle};
    double engravingInterferenceHandgun{kSameAsRifle};
    double neckStressHandgun_MPa{kSameAsRifle};
    double heatTransferFactorHandgun{kSameAsRifle};
    double heatVelocityHandgun_ms{kSameAsRifle};
    double gasConstantHandgun_JkgK{kSameAsRifle};
    double primerEnergyHandgun_J{kSameAsRifle};
    double primerTimeHandgun_ms{kSameAsRifle};
    double ignitionTimeHandgun_ms{kSameAsRifle};
    double bulletInertiaFactorHandgun{kSameAsRifle};

    // ---- burn law dz/dt = Ba * p[bar] * (p / 1000 bar)^burnExponent * phi(z) ----
    //   phase 1 (z < z1):   phi = burnIntercept + (burnSlopeA0 * a0 + burnSlopeOffset) * z
    //   phase 2 (z1..z2):   phi = phi(z1) * poly2(s),  s = (z - z1) / (z2 - z1),  poly2(0) = 1
    //   phase 3 (z > z2):   phi = phi(z1) * poly3(t),  t = (z - z2) / (1 - z2),   poly3(0) = poly2(1)
    // Cubics, coefficients from the highest power down without the constant term (phi is continuous at z1 and z2).
    double burnIntercept{1.14476};
    double burnSlopeA0{0.380336};
    double burnSlopeOffset{-0.237782};
    double burnExponent{-0.00120751};
    // ---- start of the shot ----
    double initialGasFactor{0.536312};  // weight of the initial-gas energy offset (1 = reproduces ps exactly)
    double handgunStartFraction{0.0};   // handguns: start pressure >= this x permitted pressure (0: the load's own)
    double handgunStartGain{0.0};       // changes of the start pressure against the neutral load added on top of that floor x this

    double burnZ0Factor{0.0};  // the form function sees z + burnZ0Factor * z0 (initial gas counts as burnt)
    // the burnt powder's energy reaches the gas by 1 - exp(-ignitionRate * (step + ignitionLead)), steps of a 2000 point grid
    double ignitionRate{0.132};
    double ignitionLead{7.17663};  // steps the ramp is ahead (the gas has energy before the first step)
    // shape of the form function in phases 2 and 3 (see above); held at t = phase3Limit beyond it
    std::array<double, 3> phase2{-0.478611, 0.25453, -0.536076};
    double phase3Limit{1.0};  // the phase-3 shape is held constant beyond this t = (z - z2)/(1 - z2) (1: not at all)
    std::array<double, 3> phase3{0.115942, -0.394165, 0.683927};
};

// Which constants (and model) the solver uses.
//   Original:  fitted to runs of the original program (analysis/calibrate.py).
//   Published: the Original equations fitted to published load data (analysis/calibrate_published.py).
//   Physical:  SolverModel::Physical fitted to published load data, with the powders' physical burn values (physicalpowders.h).
enum class CalibrationProfile
{
    Original,
    Published,
    Physical
};
[[nodiscard]] Calibration calibrationFor(CalibrationProfile profile);
// The profile of simulations without an explicit Calibration (default Original). Not thread-safe against running simulations.
void setCalibrationProfile(CalibrationProfile profile);
[[nodiscard]] CalibrationProfile calibrationProfile() noexcept;
[[nodiscard]] const Calibration &activeCalibration() noexcept;

// bulletIntrusion_m3() with the seated part of the nose as a tangent ogive (physical model).
[[nodiscard]] double bulletIntrusionPhysical_m3(const SimInput &in, const Calibration &cal = activeCalibration());

// Length of the bullet's bearing surface in the physical model [m]: the bullet without its boat tail and nose, at least 0.3 calibres.
[[nodiscard]] double bearingLength_m(const SimInput &in, const Calibration &cal = activeCalibration());

// Named access to every tunable constant (for calibration tools and tests).
struct NamedParameter
{
    const char *name;
    double *value;
};
[[nodiscard]] std::vector<NamedParameter> parameters(Calibration &cal);
// The constants of a pistol or revolver cartridge: each ...Handgun member that is not NaN replaces its rifle value.
[[nodiscard]] Calibration handgunCalibration(const Calibration &cal);

// Runs the simulation; the curves have `steps` equal position steps from the start position to the muzzle. Not ok() for invalid
// input or a charge above 1.2 x bulk density in the volume left by the bullet (the original's limit).
[[nodiscard]] SimResult simulate(const SimInput &in, const Calibration &cal = activeCalibration(), int steps = 2000);

// Burn-rate coefficient Ba * phi(z) for n values of z in [0, 1] (the "vivacity" curve).
[[nodiscard]] std::vector<double> vivacityCurve(const SimInput &in, const Calibration &cal = activeCalibration(), int n = 101);

// Volume of the bullet's part inside the case [m^3] (cylinder minus the boat-tail frustum).
[[nodiscard]] double bulletIntrusion_m3(const SimInput &in);

// Simulates the whole ladder. Without a ladder (count below 2) the result has one step, the selected
// charge. Steps that would have no powder left are omitted. Index 0 is the maximum charge.
[[nodiscard]] std::vector<LadderStep> simulateLadder(const SimInput &in, const Calibration &cal = activeCalibration(), int steps = 2000);

// Chamber volume at the start (case volume minus the part of the bullet inside the case) [m^3].
[[nodiscard]] double chamberVolume_m3(const SimInput &in, const Calibration &cal = activeCalibration());

}  // namespace csr
