#pragma once

#include "simconfig.h"

#include <array>
#include <cstddef>
#include <vector>

namespace grt
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

// Curves and summary values of one simulation. Only meaningful when ok().
struct SimResult
{
    SimStatus status{SimStatus::InvalidInput};
    std::vector<SimPoint> points;

    double maxPressure_bar{};
    double muzzlePressure_bar{};
    double muzzleVelocity_ms{};
    double muzzleEnergy_J{};
    double muzzleTime_ms{};
    double burntFraction{};  // z at the muzzle
    double efficiency{};     // muzzle energy / total chemical energy of the charge
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
    double muzzleFrictionEnergy_J{};  // work of that force over the bore (the physical model's engraving and air work is not part of it)
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
    double heatLoss_J{};  // physical model: heat lost to the chamber and bore walls up to the muzzle (0 in the original's model)

    [[nodiscard]] bool ok() const noexcept { return status == SimStatus::Ok; }
};

// One curve of the load development ladder: the selected charge is the maximum, every further step
// has `step` less powder (this is what the manual describes; checked against the original's results).
struct LadderStep
{
    double charge_g{};
    SimResult result;
};

inline constexpr int kMaxLadderSteps = 10;

// Which equations the solver uses.
//   Original:  the model identified from the original program's results (ignition ramp in grid steps, covolume tied to the powder
//              density, friction only as an output, 2000 position steps); reproduces the original including its artifacts.
//   Physical:  the project's own model (simulate() describes it): integrated in time to convergence, the bullet starts at the start
//              pressure, resistance (engraving, bearing friction, air) and heat loss in the energy balance, spin inertia; its
//              constants are fitted to measured (published) load data only.
enum class SolverModel
{
    Original,
    Physical
};

// Tunable constants of the model (see the overview at the top of solver.cpp). The structure of the model (uniform
// position steps, implicit momentum balance, Sebert mass, initial gas energy, ignition ramp) was identified from
// reference runs of the original program; the values are least-squares calibrations against those runs
// (analysis/calibrate.py) and change when the model or the reference data change. The accuracy table in docs/ACCURACY.md shows
// how well the current values fit. Names with a unit suffix are in that unit; the others are factors.
struct Calibration
{
    // ---- burn rate ----
    double burnRateScale{1.0};  // scales the powder's Ba (the records' Ba values belong to one calibration; see CalibrationProfile)
    // ---- gas and energy ----
    double impetusFactor{1.01455};  // scales f = (k-1)*Qex
    double kineticFactor{1.00787};  // scales the kinetic-energy term in the energy balance
    // covolume b of the powder gas = covolume_cm3g + covolumeDensityFactor / material density pc: measured on the
    // original, powders with a low pc have a much larger covolume (b ~ 1.2 / pc); eta has no effect at all.
    // The unburnt powder takes solidDensityFactor / pc per kg.
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

    // ---- shotshells (caliber flag "shotshell") ----
    // The "projectile" is the wad with the shot column; the plastic wad seals the bore like a driving band, so the gas
    // keeps pushing after the crimp has opened (start pressure ps = crimp release, about 20 bar). What differs from a
    // bullet, fitted (analysis/fit_shotgun.py) to 64 published shotgun loads whose powders the database has: 50 of the
    // Vihtavuori Reloading Guide 2026 (12/76, velocity 2.5 m from the muzzle, CIP pressure as the upper limit of the
    // maximum loads), 2 of Reload Swiss (12/70, velocity at 3 m, measured pressure) and 12 of the Hodgdon Reloading
    // Data Center (12/70, Universal and W244, velocity at 3 ft, measured pressure). Downrange velocities are converted
    // to the muzzle with the air drag of the pellets. Muzzle velocity mean -4.4 %, mean |error| 6.7 %; pressure of the
    // 14 measured loads about right on average but -11 ... +64 % per powder (the powders' models, made for rifles and
    // handguns, differ most). The gas energy stays at 1 (fitted free it is 1.03 once measured pressures are included).
    // The fit assumes the input convention of docs/ACCURACY.md (shot + wad as the projectile, column inside the hull).
    // NOT matched to the original: its shotshell mode follows no physical mechanism we could identify.
    double shotCapacityFactor{1.863};  // powder space x factor: the wad cushion compresses, the plastic hull expands
    double shotFrictionShare{0.0317};  // share of the pressure force lost to the wad and shot sliding in the bore
    double shotEnergyFactor{1.0};      // gas energy x factor (gas and heat losses; not above 1)

    // ---- derived outputs (do not affect the curves) ----
    double frictionStress_MPa{0.907};  // built-in friction force = stress * pi * D * bullet length
    double gasImpulseFactor{1.1622};   // gas impulse after the muzzle = factor * mc * v
    double mipFactor{1.975};           // MIP = factor * IPSC

    // ---- physical model (model == SolverModel::Physical; the Original model ignores these) ----
    // It also uses burnRateScale, impetusFactor, capacityFactor, the burn law and the form function above; the other constants of the
    // Original model (ignition ramp, covolume/solid density factors, intrusion and chamber offsets, kinetic factor, initial gas) do not
    // apply to it.
    SolverModel model{SolverModel::Original};
    // covolume of the powder gas (times the powder's eta, 1 when unknown); the solid powder takes 1 / pc. 1.0 cm3/g is the median of
    // closed-vessel tests of 470 powders (pressure at loading density 0.2 g/cm3 with f = impetusFactor (k-1) Qex), independent of pc.
    double gasCovolume_cm3g{1.0};
    double primerEnergy_J{50.0};     // energy of the primer gas that lights the powder (20-30 mg of priming compound)
    double ignitionTime_ms{0.0};     // flame spread: the lit share of the grains is 1 - exp(-t / this) after ignition (0: all at once)
    double engravingShare{0.5};      // rifling engraving: resistance = share x start pressure x bore area, decaying over the bearing length
    double slideStress_MPa{3.0};     // friction stress on the bullet's bearing surface in the bore
    double heatTransferFactor{0.5};  // x the Dittus-Boelter coefficient of turbulent pipe flow (heat loss to the walls)
    double heatVelocity_ms{0.0};     // gas velocity of the heat transfer on top of v / 2: turbulence of the burning charge
    double gasInertiaFactor{1.0};    // x the load's Sebert factor: the share of the powder gas that moves with the bullet (Lagrange: 1/3)
    double bulletInertiaFactor{0.45};   // moment of inertia of the bullet = factor x mass x radius^2 (solid cylinder 0.5)
    double gasConstant_JkgK{340.0};     // specific gas constant of the powder gas (molar mass about 24.5 g/mol): gas temperature
    double integrationTolerance{1e-7};  // relative error per time step of the adaptive integration

    // ---- burn law dz/dt = Ba * p[bar] * (p / 1000 bar)^burnExponent * phi(z), identified from the powder sweeps ----
    //   phase 1 (z < z1):   phi = burnIntercept + (burnSlopeA0 * a0 + burnSlopeOffset) * z
    //   phase 2 (z1..z2):   phi = phi(z1) * poly2(s),  s = (z - z1) / (z2 - z1),  poly2(0) = 1
    //   phase 3 (z > z2):   phi = phi(z1) * poly3(t),  t = (z - z2) / (1 - z2),   poly3(0) = poly2(1)
    // The polynomials are cubic; the coefficients are listed from the highest power down WITHOUT the constant term,
    // which follows from the continuity of phi at z1 and z2 (the original's curve has no jumps).
    double burnIntercept{1.09171};
    double burnSlopeA0{0.434982};
    double burnSlopeOffset{0.00638254};
    double burnExponent{-0.00810765};  // about 0: the rate is linear in p (measured: 200..1500 bar within 2 %)
    // ---- start of the shot ----
    double initialGasFactor{0.536312};  // weight of the initial-gas energy offset (1 = reproduces ps exactly)
    // Pistol and revolver cartridges (C.I.P. table IV): the start pressure is at least this fraction of the permitted pressure (0: the load's
    // own value). The published handgun loads of high-pressure cartridges (9 mm Luger, .40 S&W, .357 Magnum) need about 600 bar, low-pressure ones less.
    double handgunStartFraction{0.0};
    double handgunMaxBarrel_mm{400.0};  // longer barrels (carbines, rifles chambered for handgun cartridges) keep the load's own value
    double burnZ0Factor{0.0};           // the form function sees z + burnZ0Factor * z0 (initial gas counts as burnt)
    // Measured on the original with fast powders: the energy of the burnt powder reaches the gas only by the fraction
    // 1 - exp(-ignitionRate * (step + ignitionLead)), counted in steps of the original's 2000 point position grid
    // (independent of the bullet mass and charge; simulate() converts other step counts). It limits the peak pressure of fast powders; slow ones have burnt little by then.
    double ignitionRate{0.132};
    double ignitionLead{7.17663};  // steps the ramp is ahead (the gas has energy before the first step)
    // shape of the form function in phases 2 and 3 (see above); held at t = phase3Limit beyond it
    std::array<double, 3> phase2{-0.0867208, -0.0659705, -0.461006};
    double phase3Limit{1.0};  // the phase-3 shape is held constant beyond this t = (z - z2)/(1 - z2) (1: not at all)
    std::array<double, 3> phase3{-0.103524, -0.0695024, 0.00958064};
};

// Which constants (and model) the solver uses.
//   Original:  fitted to runs of the original program (analysis/calibrate.py); reproduces its results (accuracy table in docs/ACCURACY.md).
//   Published: the Original model's equations, constants fitted to the manufacturers' published load data (analysis/calibrate_published.py:
//              velocities and measured pressures of the loads of Hodgdon, Western Powders, ...); closer to measured pressures, but no
//              longer to the original program. Empirical: some constants have no physical meaning.
//   Physical:  the project's own physical model (SolverModel::Physical), constants fitted to the same published data; uses the physical
//              burn values of the project's propellant records (physicalpowders.h) when the load holds such a record unchanged.
enum class CalibrationProfile
{
    Original,
    Published,
    Physical
};
[[nodiscard]] Calibration calibrationFor(CalibrationProfile profile);
// The profile every simulation without an explicit Calibration uses (Original unless a program sets another one at start-up
// or when the user changes the setting; not meant to be changed while simulations are running).
void setCalibrationProfile(CalibrationProfile profile);
[[nodiscard]] CalibrationProfile calibrationProfile() noexcept;
[[nodiscard]] const Calibration &activeCalibration() noexcept;

// Named access to every tunable constant (for calibration tools and tests).
struct NamedParameter
{
    const char *name;
    double *value;
};
[[nodiscard]] std::vector<NamedParameter> parameters(Calibration &cal);

// Runs the simulation. Original model: over `steps` equal position steps from the start position to the muzzle. Physical model:
// adaptive in time, the curves resampled to `steps` equal position steps from the start position to the muzzle (both included). The result is not
// ok() for invalid input (non-finite or non-positive values) or a charge that does not fit into the case (the
// original's limit: 1.2 x bulk density in the volume left by the bullet).
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

}  // namespace grt
