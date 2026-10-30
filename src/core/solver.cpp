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

// Interior ballistics solver: a lumped-parameter (Resal / Lagrange) model of the gun, integrated over equal steps
// of the bullet's position.
//
// The model
// ---------
// State per step: bullet position x, velocity v, burnt fraction of the powder z (0..1). Units inside: SI (m, kg,
// Pa, s); the burn law and the results use bar.
//
//  * Burn law:        dz/dt = Ba * p[bar] * (p / 1000 bar)^n * phi(z)         (FormFunction, Model::burnRate)
//    Ba is the powder's vivacity (corrected for the powder temperature, burnCoefficient), phi(z) the three-phase form
//    function of the powder (a0, z1, z2) and n = Calibration::burnExponent (about 0: the rate is linear in p).
//  * Energy balance (Resal):  p * Vgas = f * mc * (z + z0) - (k - 1) * kin * 1/2 * mEff * v^2   (thermoPressure)
//    with the impetus f = impetusFactor * (k - 1) * Qex, the effective moving mass mEff = mp + sebert * mc (the
//    Sebert / Lagrange correction for the moving gas and powder), and z0 an initial gas energy that reproduces the
//    start pressure ps at z = 0, v = 0.
//  * Gas volume:      Vgas = V0 + A * (x - x0) - mc * (1 - z) * solid - mc * z * covolume              (gasVolume)
//    V0 = chamber volume behind the bullet (chamberVolume_m3), A = bore area, solid = specific volume of the unburnt
//    powder, covolume = covolume of the gas; both depend on the material density pc of the powder.
//  * Momentum:        mEff * dv/dt = A * p   ->   over a step dx:  p = mEff * (v1^2 - v0^2) / (2 * A * dx)
//    (the pressure shown in the results is this "momentum pressure" at the bullet base).
//  * Ignition ramp:   only the fraction 1 - exp(-ignitionRate * (step + ignitionLead)) of the burnt powder's energy
//    reaches the gas that pushes the bullet; the powder itself burns at the full gas pressure. This was measured on
//    the original with fast powders and limits their peak pressure.
//
// One step (simulate): the velocity v1 at the end of the step is the unknown. For a trial v1 the momentum equation
// gives the pressure the bullet needs (pMom), the burn law (trapezoid rule, iterated) gives the new z, and the
// energy balance gives the pressure the gas has (pTh). pMom grows with v1 and pTh falls with it, so v1 is found by
// bracketing and bisection where both agree. The time step follows from dt = 2 dx / (v0 + v1).
//
// Where it comes from: the structure follows the original program's documentation and what its results show; the
// numbers in Calibration are least-squares fits against reference runs of the original (analysis/calibrate.py,
// tools/grt_compare.cpp). It is NOT the original's algorithm. Known gaps are listed in docs/ACCURACY.md.

#include "solver.h"

#include "numeric.h"
#include "obt.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace csr
{

namespace
{

constexpr double kBar = kPaPerBar;
constexpr double kReferenceSteps = 2000.0;  // position steps of the original program

// Physical model
constexpr double kMinBearingDiameters = 0.3;  // shortest bearing surface of a bullet, in calibers
constexpr double kPrimerGasTemperature_K = 2500.0;
constexpr double kMinFitFactor = 0.25;    // lowest factor an interference oversize may give the engraving and sliding force
constexpr double kGasConductivity = 0.2;  // W/(m K)
constexpr double kGasViscosity = 8e-5;    // Pa s
constexpr double kGasPrandtl = 0.7;
constexpr double kDittusBoelter = 0.023;     // Nusselt prefactor of turbulent pipe flow
constexpr double kInitialStep_s = 1e-7;      // first integration step in the closed case
constexpr double kBoreInitialStep_s = 1e-8;  // first integration step in the bore
constexpr int kMaxSteps = 2000000;           // integration guard
constexpr double kMaxTime_s = 0.1;           // a bullet that has not left the barrel by then is stuck
constexpr double kDefaultTolerance = 1e-7;   // error per step of the integrator
constexpr double kMinStepError = 1e-12;
constexpr double kStepSafety = 0.9, kStepShrink = 0.2, kStepGrowth = 5.0;  // step size controller (order 5: exponent -1/5)

// The pressure part of the burn law for a pressure in bar: p * (p / 1000 bar)^exponent.
[[nodiscard]] double burnPressureTerm(double pBar, double exponent) noexcept
{
    return pBar * (exponent == 0.0 ? 1.0 : std::pow(std::max(pBar, 1.0) / 1000.0, exponent));
}

// Geometric burning surface of a grain, normalized to its initial value and scaled by grainAmplitude. Ball: a sphere burning
// uniformly, phi = (1 - z)^(2/3). Cylinder: a single-perforated cylinder burning on both ends; the burnt fraction z is the
// volume fraction, so the surface S(z)/S(0) is tabulated and interpolated in z.
struct GrainFormFunction
{
    static constexpr std::size_t kTable = 257;

    GrainShape shape{GrainShape::Fitted};
    double amplitude{1.0};
    std::array<double, kTable> zTable{};
    std::array<double, kTable> phiTable{};

    void buildCylinder(double ld, double hole) noexcept
    {
        const double r0 = std::clamp(hole, 0.0, 0.95);
        const double length = std::max(ld, 0.05) * 2.0;  // outer diameter = 2
        const double e = (1.0 - r0) / 2.0;               // half the web
        const double wMax = std::min(e, 0.5 * length);
        const double v0 = std::numbers::pi * (1.0 - r0 * r0) * length;
        const double s0 = 2.0 * std::numbers::pi * (1.0 + r0) * length + 2.0 * std::numbers::pi * (1.0 - r0 * r0);
        if (v0 <= 0.0 || s0 <= 0.0) return;
        for (std::size_t i = 0; i < kTable; ++i)
        {
            const double w = wMax * static_cast<double>(i) / static_cast<double>(kTable - 1);
            const double outer = 1.0 - w;
            const double inner = r0 + w;
            const double v = std::numbers::pi * (outer * outer - inner * inner) * (length - 2.0 * w);
            const double s =
                2.0 * std::numbers::pi * (outer + inner) * (length - 2.0 * w) + 2.0 * std::numbers::pi * (outer * outer - inner * inner);
            zTable[i] = std::clamp(1.0 - std::max(v, 0.0) / v0, 0.0, 1.0);
            phiTable[i] = amplitude * std::max(s, 0.0) / s0;
        }
        zTable[0] = 0.0;
        zTable[kTable - 1] = 1.0;
    }

    [[nodiscard]] double operator()(double z) const noexcept
    {
        if (shape == GrainShape::Ball) return amplitude * std::pow(std::max(1.0 - z, 0.0), 2.0 / 3.0);
        if (shape != GrainShape::Cylinder) return 0.0;
        const auto it = std::lower_bound(zTable.begin(), zTable.end(), z);
        if (it == zTable.begin()) return phiTable.front();
        if (it == zTable.end()) return phiTable.back();
        const std::size_t j = static_cast<std::size_t>(it - zTable.begin());
        const std::size_t i = j - 1;
        const double dz = zTable[j] - zTable[i];
        return dz > 0.0 ? phiTable[i] + (phiTable[j] - phiTable[i]) * (z - zTable[i]) / dz : phiTable[j];
    }
};

// Three-phase form function phi(z) of the burn law dz/dt = Ba * p[bar] * phi(z): progressive up to z1 (a straight
// line whose slope follows the powder's a0), primary degressive from z1 to z2, secondary degressive from z2 to 1. The
// shapes of phases 2 and 3 are cubics fitted to the original's results (Calibration::phase2 / phase3); the phases
// join without jumps. When a grain geometry is set, its normalized surface replaces the cubic instead.
struct FormFunction
{
    const Calibration *cal;
    double a0, z1, z2;
    GrainFormFunction grain;
    bool useGrain{};

    // cubic with the given constant term c0 (the coefficients c are the cubic, quadratic and linear ones)
    [[nodiscard]] static double poly(const std::array<double, 3> &c, double c0, double x) noexcept
    {
        return ((c[0] * x + c[1]) * x + c[2]) * x + c0;
    }

    // phase 1: phi(0) = burnIntercept, rising (a0 > 0: progressive powder) or falling with z
    [[nodiscard]] double phase1(double z) const noexcept { return cal->burnIntercept + (cal->burnSlopeA0 * a0 + cal->burnSlopeOffset) * z; }

    [[nodiscard]] double operator()(double z) const noexcept
    {
        if (useGrain) return std::max(grain(z), 0.0);
        constexpr double kFloor = 0.02;  // the burn rate never reverses
        if (z < z1) return std::max(phase1(z), kFloor);
        const double phi1 = std::max(phase1(z1), kFloor);
        if (z < z2) return std::max(phi1 * poly(cal->phase2, 1.0, (z - z1) / std::max(z2 - z1, 1e-9)), kFloor);
        const double atZ2 = poly(cal->phase2, 1.0, 1.0);  // phase 3 starts where phase 2 ends
        return std::max(phi1 * poly(cal->phase3, atZ2, std::min((z - z2) / std::max(1.0 - z2, 1e-9), cal->phase3Limit)), kFloor);
    }
};

// The constants of one simulation (from the load and the calibration) and the equations of the model.
struct Model
{
    double A_m2;           // bore area (effective cross section)
    double mEff_kg;        // moving mass: bullet + Sebert share of the charge
    double mc_kg;          // powder charge
    double V0_m3;          // chamber volume behind the bullet at the start
    double x0_m;           // start position of the bullet base (from the case head)
    double solid_m3kg;     // specific volume of the unburnt powder
    double covolume_m3kg;  // covolume of the powder gas
    double fEnergy;        // impetus-like constant: factor * (k-1) * Qex [J/kg]
    double theta;          // k - 1
    double kinetic;        // kinetic factor
    double z0;             // initial gas "burn fraction" reproducing the start pressure
    double burnBa;         // vivacity Ba at the powder temperature
    double burnExponent;   // pressure exponent of the burn law on top of the linear p
    double burnZ0Factor;   // share of the initial gas the form function counts as burnt
    FormFunction phi;

    // Free gas volume with the bullet base at x and the fraction z burnt: chamber + bore behind the bullet, minus the
    // unburnt powder and the covolume of the gas.
    [[nodiscard]] double gasVolume(double z, double x_m) const noexcept
    {
        return V0_m3 + A_m2 * (x_m - x0_m) - mc_kg * (1.0 - z) * solid_m3kg - covolume_m3kg * mc_kg * z;
    }

    // Gas pressure from the energy balance (Resal): energy of the gas (burnt powder + initial gas) minus the kinetic
    // energy of the moving mass, divided by the free volume. `ignited` is the fraction of the burnt powder whose energy
    // has reached the gas (see Calibration::ignitionRate).
    [[nodiscard]] double thermoPressure(double z, double v, double x_m, double ignited = 1.0) const noexcept
    {
        const double e = fEnergy * mc_kg * (z * ignited + z0) - theta * kinetic * 0.5 * mEff_kg * v * v;
        const double vg = gasVolume(z, x_m);
        return vg > 0.0 ? std::max(e, 0.0) / vg : 0.0;
    }

    [[nodiscard]] double pressureTerm(double pBar) const noexcept { return burnPressureTerm(pBar, burnExponent); }

    // dz/dt at the burnt fraction z for the given pressure term; 0 once everything has burnt.
    [[nodiscard]] double burnRateAt(double z, double pressureTermValue) const noexcept
    {
        return z >= 1.0 ? 0.0 : burnBa * pressureTermValue * phi(z + burnZ0Factor * z0);
    }

    // dz/dt at the burnt fraction z and the pressure p [Pa]; 0 once everything has burnt.
    [[nodiscard]] double burnRate(double z, double p_Pa) const noexcept { return burnRateAt(z, pressureTerm(p_Pa / kBar)); }

    // z after a time step dt: trapezoid rule between the rate r0 at the start (zPrev) and at the end (z, pNew), iterated
    // because the end rate depends on the new z. r0 does not change between the trials of a step. Clamped at 1.
    [[nodiscard]] double advanceBurnFrom(double zPrev, double r0, double pNew, double dt) const noexcept
    {
        const double term = pressureTerm(pNew / kBar);  // the same in all iterations: the pressure is given
        double z = zPrev;
        for (int k = 0; k < 3; ++k) z = std::min(1.0, zPrev + 0.5 * (r0 + burnRateAt(z, term)) * dt);
        return z;
    }
};

// Vivacity Ba at the powder temperature (the powder data refers to 21 degC).
// `Ba` is the vivacity at 21 degC to use (default: the load's, times burnRateScale).
[[nodiscard]] double burnCoefficient(const SimInput &in, const Calibration &cal, double Ba = 0.0) noexcept
{
    const double dT = in.temperature_C - kPowderReferenceTemperature_C;
    if (Ba <= 0.0) Ba = in.vivacityBa * cal.burnRateScale;
    if (dT == 0.0) return Ba;
    // Measured on the original: with both coefficients 0 the built-in defaults apply. Otherwise each side uses its
    // own coefficient, and a side whose coefficient is 0 has NO effect. An explicit coefficient acts on the burn
    // rate linearly in coefficient * |dT| with the sign of the coefficient (a positive tcc raises the rate when
    // cold), and far weaker than the documented Ba(21) - Ba(T) per K would give.
    if (in.coldCoeff == 0.0 && in.hotCoeff == 0.0) return Ba * (1.0 + (dT > 0.0 ? cal.hotCoeffDefault : cal.coldCoeffDefault) * dT);
    return Ba * (1.0 + (dT > 0.0 ? cal.explicitHotScale * in.hotCoeff * dT : cal.explicitColdScale * in.coldCoeff * -dT));
}

// The form function and the vivacity a simulation burns with: the powder's physical burn values when `pb` is set, else the load's
// own values (Ba scaled with burnRateScale).
[[nodiscard]] FormFunction formFunction(const SimInput &in, const Calibration &cal, const std::optional<PhysicalBurn> &pb) noexcept
{
    FormFunction f = pb ? FormFunction{.cal = &cal, .a0 = pb->a0, .z1 = pb->z1, .z2 = pb->z2, .grain = {}}
                        : FormFunction{.cal = &cal, .a0 = in.a0, .z1 = in.z1, .z2 = in.z2, .grain = {}};
    if (pb && pb->grain != GrainShape::Fitted && pb->grainAmplitude > 0.0)
    {
        f.useGrain = true;
        f.grain.shape = pb->grain;
        f.grain.amplitude = pb->grainAmplitude;
        if (pb->grain == GrainShape::Cylinder) f.grain.buildCylinder(pb->grainLd, pb->grainHole);
    }
    return f;
}

[[nodiscard]] double burnCoefficient(const SimInput &in, const Calibration &cal, const std::optional<PhysicalBurn> &pb) noexcept
{
    // the caliber's correction of the physical burn rates (pistol and revolver cartridges, data/physical/calibers.json)
    return pb ? burnCoefficient(in, cal, pb->Ba * (cal.caliberBurnScaling ? in.caliberBurnScale : 1.0)) : burnCoefficient(in, cal);
}

}  // namespace

double bulletIntrusion_m3(const SimInput &in)
{
    namespace n = std::numbers;
    const double D = in.bulletDiameter_mm * 1e-3;
    const double depth = in.seatingDepth_mm * 1e-3;
    const double h = in.tailLength_mm * 1e-3;
    const double areaBullet = n::pi / 4.0 * D * D;
    double inside = areaBullet * depth;
    if ((in.tailType == 1 || in.tailType == 2) && h > 0.0 && depth >= h)
    {
        // the tail is a cone between the diameters A and B over the length h (the order does not matter)
        const double R = (in.tailDiameterA_mm > 0.0 ? in.tailDiameterA_mm * 1e-3 : D) / 2.0;
        const double r = in.tailDiameterB_mm * 1e-3 / 2.0;
        const double cone = n::pi * h / 3.0 * (R * R + R * r + r * r);
        if (in.tailType == 2) inside -= areaBullet * h - cone;  // boat tail: the cone is missing from the cylinder
        else inside -= cone;                                    // hollow base: the cavity adds combustion volume
    }
    return inside;
}

// The bullet's part inside the case for the physical model: the same cylinder as bulletIntrusion_m3, but the part of the ogive
// nose that is seated in the case is corrected with the exact volume of a tangent ogive. bulletIntrusion_m3() counts the whole
// seated length as a cylinder of bullet diameter; that overstates the powder-space loss of deeply seated bullets.
double bulletIntrusionPhysical_m3(const SimInput &in)
{
    const double inside = bulletIntrusion_m3(in);
    const double D = in.bulletDiameter_mm * 1e-3;
    const double L = in.bulletLength_mm * 1e-3;
    const double depth = in.seatingDepth_mm * 1e-3;
    if (D <= 0.0 || L <= 0.0 || depth <= 0.0) return inside;

    const double tail = in.tailType == 2 ? in.tailLength_mm * 1e-3 : 0.0;
    const double bearing = std::max(L - tail - D, kMinBearingDiameters * D);  // the same bearing length as the physical model uses
    const double nose = std::max(L - tail - bearing, 0.0);
    const double noseStart = tail + bearing;
    if (nose <= 1e-12 || depth <= noseStart) return inside;

    const double sMax = std::min({depth, L, noseStart + nose}) - noseStart;
    if (sMax <= 0.0) return inside;

    const double R = D / 2.0;
    const double rho = (R * R + nose * nose) / (2.0 * R);  // tangent ogive of the nose length
    const auto integral = [&](double s)
    {
        const double x = std::clamp(s / rho, -1.0, 1.0);
        const double root = std::sqrt(std::max(rho * rho - s * s, 0.0));
        return (rho * rho + (R - rho) * (R - rho)) * s - s * s * s / 3.0 +
               2.0 * (R - rho) * (0.5 * s * root + 0.5 * rho * rho * std::asin(x));
    };
    const double s = std::min(sMax, nose);
    const double noseVolume = std::numbers::pi * integral(s);
    const double cylinderVolume = std::numbers::pi * R * R * s;
    return inside - std::max(cylinderVolume - noseVolume, 0.0);
}

namespace
{

CalibrationProfile g_profile = CalibrationProfile::Original;
Calibration g_active;  // the constants of g_profile

// The shotshell model was fitted with the original's constants (published shotgun loads, not rifle loads): it keeps them whatever
// the profile is, also with the physical model's constants given explicitly (they belong to other equations). Other explicit
// calibrations (tools) are used as given.
const Calibration &effectiveCalibration(const SimInput &in, const Calibration &cal)
{
    static const Calibration original;
    return in.shotshell && (&cal == &g_active || cal.model == SolverModel::Physical) ? original : cal;
}

// Summary values and derived quantities of a finished curve (both models); sets the status to Ok.
void summarize(SimResult &res, const SimInput &in, const Calibration &cal, double freeVolume_cm3)
{
    const auto &pts = res.points;
    const auto pmax = std::ranges::max_element(pts, {}, &SimPoint::p_bar);
    res.indexPmax = static_cast<std::size_t>(pmax - pts.begin());
    res.maxPressure_bar = pmax->p_bar;
    const SimPoint &last = pts.back();
    res.muzzlePressure_bar = last.p_bar;
    res.muzzleVelocity_ms = last.v_ms;
    res.muzzleEnergy_J = last.e_J;
    res.muzzleTime_ms = last.t_ms;
    // first rise through 10 % of the maximum pressure (linear between the two points around it)
    const double p10 = 0.1 * res.maxPressure_bar;
    const auto rise = std::ranges::find_if(pts, [p10](const SimPoint &q) { return q.p_bar >= p10; });
    if (rise != pts.end())
    {
        res.pmax10Time_ms = rise->t_ms;
        if (rise == pts.begin() && !res.restPhase.empty())
        {
            // already above 10 % when the bullet starts to move: the crossing lies in the time before
            const auto before = std::ranges::find_if(res.restPhase, [p10](const RestPoint &q) { return q.p_bar >= p10; });
            if (before != res.restPhase.end())
            {
                res.pmax10Time_ms = before->t_ms;
                if (before != res.restPhase.begin() && before->p_bar > (before - 1)->p_bar)
                {
                    const RestPoint &lower = *(before - 1);
                    res.pmax10Time_ms = lower.t_ms + (p10 - lower.p_bar) / (before->p_bar - lower.p_bar) * (before->t_ms - lower.t_ms);
                }
            }
        }
        else if (rise != pts.begin() && rise->p_bar > (rise - 1)->p_bar)
        {
            const SimPoint &before = *(rise - 1);
            res.pmax10Time_ms = before.t_ms + (p10 - before.p_bar) / (rise->p_bar - before.p_bar) * (rise->t_ms - before.t_ms);
        }
    }
    res.bulletLeadTime_ms = res.muzzleTime_ms - res.pmax10Time_ms;
    if (const auto obt = nearestObtNode(res.bulletLeadTime_ms, in.barrelLength_mm))
    {
        res.optimalBarrelTime_ms = obt->time_ms;
        res.optimalBarrelNode = obt->node;
    }
    res.burntFraction = last.z;
    res.efficiency = last.e_J / (in.charge_g * 1e-3 * in.heatQex_kJkg * 1e3);
    const auto burnout = std::ranges::find_if(pts, [](const SimPoint &q) { return q.z >= 1.0; });
    res.burnoutInBarrel = burnout != pts.end();
    res.indexBurnout = static_cast<std::size_t>((res.burnoutInBarrel ? burnout : pts.end() - 1) - pts.begin());

    // markers and ratios
    const auto firstAtLeast = [&](auto value, double limit)
    {
        const auto it = std::ranges::find_if(pts, [&](const SimPoint &q) { return value(q) >= limit; });
        return it == pts.end() ? std::size_t{0} : static_cast<std::size_t>(it - pts.begin());
    };
    const bool physicalBurn = cal.model == SolverModel::Physical && in.physicalBurn;  // the phase limits the physical model used
    res.indexZ1 = firstAtLeast([](const SimPoint &q) { return q.z; }, physicalBurn ? in.physicalBurn->z1 : in.z1);
    res.indexZ2 = firstAtLeast([](const SimPoint &q) { return q.z; }, physicalBurn ? in.physicalBurn->z2 : in.z2);
    res.indexEnergy95 = firstAtLeast([](const SimPoint &q) { return q.e_J; }, 0.95 * last.e_J);
    res.effectiveCaseVolume_cm3 = freeVolume_cm3;
    res.loadRatio = in.charge_g / (in.bulkDensity_kgm3 * 1e-3 * freeVolume_cm3);
    if (in.twistLength_mm > 0.0) res.muzzleRpm = last.v_ms / (in.twistLength_mm * 1e-3) * 60.0;

    // ---- derived quantities (formulas identified from the original's results) ----
    const double mc = in.charge_g * 1e-3, mp = in.bulletMass_g * 1e-3;
    res.burnoutPressure_bar = pts[res.indexBurnout].p_bar;
    res.propellantEnergy_J = in.heatQex_kJkg * 1e3 * mc * last.z;
    res.frictionForce_N = cal.frictionStress_MPa * std::numbers::pi * in.bulletDiameter_mm * in.bulletLength_mm;
    res.muzzleFrictionEnergy_J = res.frictionForce_N * (last.x_mm - pts.front().x_mm) * 1e-3;
    res.ipscFactor = in.bulletMass_g / kGrainToGram * last.v_ms * kFeetPerMeter / 1000.0;
    res.mipFactor = cal.mipFactor * res.ipscFactor;
    res.projectileImpulse_Ns = mp * last.v_ms;
    res.propellantImpulse_Ns = in.sebert * mc * last.v_ms;
    res.gasImpulse_Ns = cal.gasImpulseFactor * mc * last.v_ms;
    res.recoilImpulse_Ns = res.projectileImpulse_Ns + res.propellantImpulse_Ns + res.gasImpulse_Ns;
    if (last.t_ms > 0.0) res.meanRecoilForce_N = res.recoilImpulse_Ns / (last.t_ms * 1e-3);
    if (in.gunWeight_kg > 0.0)
    {
        res.recoilVelocity_ms = res.recoilImpulse_Ns / in.gunWeight_kg;
        res.recoilEnergy_J = 0.5 * in.gunWeight_kg * res.recoilVelocity_ms * res.recoilVelocity_ms;
        res.recoilAcceleration_ms2 = res.meanRecoilForce_N / in.gunWeight_kg;
    }
    res.status = SimStatus::Ok;
}

// ---- the physical model (Calibration::model == SolverModel::Physical) ----
//
// State: bullet position x, velocity v, burnt fraction z, heat lost to the walls Q, work done against the resistance W; integrated in
// time (Dormand-Prince 5(4) with error control, so the result does not depend on the output grid):
//   dz/dt = Ba * p[bar] * (p / 1000 bar)^n * phi(z) * g   burn law and form function as in the Original model; g = 1 - exp(-t / tau)
//                                                       is the lit share of the grains (flame spread from the primer, tau =
//                                                       ignitionTime_ms, t counted from ignition)
//   p     = (k - 1) * U / Vgas                         Resal: the space-mean pressure of the gas
//   U     = impetusFactor * Qex * mc * z + Eprimer * (1 - exp(-t / primerTime)) - 1/2 M v^2 - Q - W
//   Vgas  = V0 + A (x - x0) - mc (1 - z) / pc - eta * b * mc * z
//   M dv/dt = A p - F,   M = mp (1 + spin inertia) + gasMassShare * mc
//   p_burn = p + (1/2 - gasMassShare) * mc * a / A      the breech pressure (Lagrange gradient, linear profile)
//   dQ/dt = h S (T - Tw),   dW/dt = F v
// For a linear gas gradient the volume-average pressure drives the momentum with gasMassShare = 1/4 and the breech pressure is
// p + mc a / (4 A). (The gas kinetic energy of a linear velocity profile is a share of 1/3; the energy balance uses the same 1/4
// anyway: a separate 1/3 changes no fit result.) The powder burns at the breech pressure. The Original model keeps the load's Sebert factor.
// The bullet stays at x0 until the gas pressure reaches the start pressure ps; until then the powder burns in the closed case, lit by
// the primer gas (primerEnergy_J, released over primerTime_ms). The resistance F: start resistance (engravingShare * ps * A, or an
// absolute engravingPressure_bar * A when that is set; once the bullet has crossed the free run gjump, decaying over
// engravingLength_mm, else over the bearing length), case-neck friction while the bullet slides out of the neck (neckStress_MPa x
// pi x D x the remaining seating depth; only when set), friction of the bearing surface (slideStress * pi * D * bearing length), the
// user's extra friction gfriction [bar] * A (when unlocked) and the air pushed out of the bore (piston-driven shock).
// Heat transfer: Dittus-Boelter for turbulent pipe flow, h = factor * 0.023 * lambda * Pr^0.4 * mu^-0.8 * D^-0.2 * (rho u)^0.8 with
// the gas velocity u = v / 2 + heatVelocity_ms (the turbulence of the burning charge, also before the bullet moves), the gas temperature T = p Vgas / (m_gas R), the wetted surface = powder space (a cylinder as long
// as the case behind the bullet) + the bore behind the bullet, and the wall at the load temperature. The reported pressure is the breech pressure of the Lagrange gradient.
// The friction outputs are the sliding friction (bearing surface + gfriction) and its work over the bore; the engraving and the air are
// part of the energy balance but not of the friction outputs.

using PhysState = std::array<double, 6>;  // x [m], v [m/s], z, heat loss Q [J], resistance work W [J], time since ignition [s]

struct PhysicalModel
{
    double A, D, mBullet, mMove, mc, V0, x0, solid, covolume, theta, eChem, ePrimer, mPrimerGas, startP, jump, bearing;
    double engraveForce, slideForce, extraForce, heatCoeff, heatVelocity, gasR, chamberSurface, burnBa, burnExponent, ignitionTime;
    double gasMassShare, primerTime, wallTemperature;
    double neckLength, neckStress, engravingLength;
    FormFunction phi;

    [[nodiscard]] double gasVolume(double x, double z) const noexcept
    {
        return V0 + A * (x - x0) - mc * (1.0 - z) * solid - covolume * mc * z;
    }
    [[nodiscard]] double primerFraction(double t) const noexcept { return primerTime > 0.0 ? 1.0 - std::exp(-t / primerTime) : 1.0; }
    [[nodiscard]] double gasPressure(const PhysState &y) const noexcept
    {
        const double e = eChem * mc * y[2] + ePrimer * primerFraction(y[5]) - 0.5 * mMove * y[1] * y[1] - y[3] - y[4];
        const double vg = gasVolume(y[0], y[2]);
        return vg > 0.0 ? theta * std::max(e, 0.0) / vg : 0.0;
    }
    // pressure of the air in front of the bullet [Pa]: a shock ahead of a piston moving at v into air at rest
    [[nodiscard]] static double airPressure(double v) noexcept
    {
        constexpr double p1 = 101325.0, a1 = 343.0, g = 1.4, c = (g + 1.0) / 4.0;
        const double M = v / a1;
        return p1 * (1.0 + g * c * M * M + g * M * std::sqrt(1.0 + c * c * M * M));
    }
    // Case-neck friction: the contact length falls while the bullet slides out of the neck.
    [[nodiscard]] double neckForce(double travel) const noexcept
    {
        if (neckStress <= 0.0 || neckLength <= 0.0 || travel >= neckLength) return 0.0;
        return neckStress * std::numbers::pi * D * (neckLength - travel);
    }
    [[nodiscard]] double resistance(double x, double v) const noexcept
    {
        double f = extraForce + airPressure(v) * A;
        const double travel = std::max(x - x0, 0.0);
        const double s = x - x0 - jump;  // travel after the bullet reached the rifling
        // In the free run the case neck still holds the bullet: the release force of the start pressure, which the engraving continues at
        // s = 0. Without neck friction (fitted value) it is a constant force (otherwise the bullet would run free at the start pressure:
        // -15 % peak pressure for a jump of 0.25 mm in a 9 mm Luger).
        if (s < 0.0)
        {
            f += neckStress > 0.0 ? neckForce(travel) : engraveForce;
            return std::max(f, 0.0);
        }
        f += neckForce(travel);
        f += slideForce + engraveForce * std::exp(-s / (engravingLength > 0.0 ? engravingLength : bearing));
        return std::max(f, 0.0);
    }
    [[nodiscard]] double heatRate(const PhysState &y, double p) const noexcept
    {
        const double vg = gasVolume(y[0], y[2]);
        const double u = 0.5 * y[1] + heatVelocity;
        if (heatCoeff <= 0.0 || vg <= 0.0 || u <= 0.0) return 0.0;
        const double mg = mc * y[2] + mPrimerGas * primerFraction(y[5]);
        if (mg <= 0.0) return 0.0;
        const double T = p * vg / (mg * gasR);
        const double h = heatCoeff * std::pow(mg / vg * u, 0.8);
        return std::max(h * (chamberSurface + std::numbers::pi * D * (y[0] - x0)) * (T - wallTemperature), 0.0);
    }
    struct Rates
    {
        PhysState dy;
        double p, a, force;
    };
    [[nodiscard]] Rates rates(const PhysState &y, bool released) const noexcept
    {
        const double p = gasPressure(y);
        double a = 0.0, force = 0.0, pBurn = p;
        if (released)
        {
            force = resistance(y[0], y[1]);
            a = (A * p - force) / mMove;
            if (y[1] <= 0.0 && a < 0.0) a = 0.0;  // the bullet waits until the gas overcomes the resistance
            // The powder at the breech burns at the breech pressure; p is the volume-average pressure of the energy balance.
            pBurn = std::max(p + (0.5 - gasMassShare) * mc * a / A, 0.0);
        }
        const double term = burnPressureTerm(pBurn / kBar, burnExponent);
        const double lit = ignitionTime > 0.0 ? 1.0 - std::exp(-y[5] / ignitionTime) : 1.0;
        const double dz = y[2] >= 1.0 ? 0.0 : burnBa * term * phi(y[2]) * lit;
        return {{y[1], a, dz, heatRate(y, p), force * std::max(y[1], 0.0), 1.0}, p, a, force};
    }
    // Breech pressure of the Lagrange gradient: the volume-average pressure plus the gas-column term.
    [[nodiscard]] double breechPressure(const Rates &r, bool released) const noexcept
    {
        return released ? r.p + (0.5 - gasMassShare) * mc * r.a / A : r.p;
    }
};

// One Dormand-Prince 5(4) step of size h from y: the new state, and the error norm (<= 1: within the tolerance).
double dopriStep(const PhysicalModel &m, bool released, const PhysState &y, double h, double tol, PhysState &out)
{
    static constexpr double a21 = 1.0 / 5, a31 = 3.0 / 40, a32 = 9.0 / 40, a41 = 44.0 / 45, a42 = -56.0 / 15, a43 = 32.0 / 9,
                            a51 = 19372.0 / 6561, a52 = -25360.0 / 2187, a53 = 64448.0 / 6561, a54 = -212.0 / 729, a61 = 9017.0 / 3168,
                            a62 = -355.0 / 33, a63 = 46732.0 / 5247, a64 = 49.0 / 176, a65 = -5103.0 / 18656, b1 = 35.0 / 384,
                            b3 = 500.0 / 1113, b4 = 125.0 / 192, b5 = -2187.0 / 6784, b6 = 11.0 / 84, e1 = 71.0 / 57600, e3 = -71.0 / 16695,
                            e4 = 71.0 / 1920, e5 = -17253.0 / 339200, e6 = 22.0 / 525, e7 = -1.0 / 40;
    static constexpr PhysState atol{1e-9, 1e-6, 1e-9, 1e-4, 1e-4, 1e-12};
    const auto f = [&](const PhysState &s) { return m.rates(s, released).dy; };
    const auto at = [&](std::initializer_list<std::pair<double, const PhysState *>> terms)
    {
        PhysState s = y;
        for (const auto &[c, k] : terms)
            for (std::size_t i = 0; i < s.size(); ++i) s[i] += h * c * (*k)[i];
        return s;
    };
    const PhysState k1 = f(y);
    const PhysState k2 = f(at({{a21, &k1}}));
    const PhysState k3 = f(at({{a31, &k1}, {a32, &k2}}));
    const PhysState k4 = f(at({{a41, &k1}, {a42, &k2}, {a43, &k3}}));
    const PhysState k5 = f(at({{a51, &k1}, {a52, &k2}, {a53, &k3}, {a54, &k4}}));
    const PhysState k6 = f(at({{a61, &k1}, {a62, &k2}, {a63, &k3}, {a64, &k4}, {a65, &k5}}));
    out = at({{b1, &k1}, {b3, &k3}, {b4, &k4}, {b5, &k5}, {b6, &k6}});
    out[2] = std::min(out[2], 1.0);
    out[1] = std::max(out[1], 0.0);
    const PhysState k7 = f(out);
    double err = 0.0;
    for (std::size_t i = 0; i < y.size(); ++i)
    {
        const double e = h * (e1 * k1[i] + e3 * k3[i] + e4 * k4[i] + e5 * k5[i] + e6 * k6[i] + e7 * k7[i]);
        err = std::max(err, std::abs(e) / (atol[i] + tol * std::max(std::abs(y[i]), std::abs(out[i]))));
    }
    return std::isfinite(err) ? err : 1e10;
}

// The step size in (0, h] at which `reached` starts to hold for the state after the step (bisection); `out` is that state.
template <class Pred>
double stepUntil(const PhysicalModel &m, bool released, const PhysState &y, double h, double tol, PhysState &out, Pred reached)
{
    double lo = 0.0, hi = h;
    for (int k = 0; k < 60; ++k)
    {
        const double mid = 0.5 * (lo + hi);
        (void)dopriStep(m, released, y, mid, tol, out);
        (reached(out) ? hi : lo) = mid;
    }
    (void)dopriStep(m, released, y, hi, tol, out);
    return hi;
}

SimResult simulatePhysical(const SimInput &in, const Calibration &cal, int steps, double freeVolume_cm3)
{
    namespace n = std::numbers;
    SimResult res;
    PhysicalModel m{};
    m.A = in.boreArea_mm2 * 1e-6;
    m.D = std::sqrt(4.0 * m.A / n::pi);
    const double mp = in.bulletMass_g * 1e-3;
    const double bulletDia = (in.bulletDiameter_mm > 0.0 ? in.bulletDiameter_mm * 1e-3 : m.D);
    const double spin = in.twistLength_mm > 0.0 ? cal.bulletInertiaFactor * 0.25 * bulletDia * bulletDia *
                                                      std::pow(2.0 * n::pi / (in.twistLength_mm * 1e-3), 2.0)
                                                : 0.0;
    m.mBullet = mp * (1.0 + spin);
    m.mc = in.charge_g * 1e-3;
    // the moving share of the charge is the calibration's (the Lagrange value), whatever Sebert factor the load holds: the powder burn
    // values are fitted with it; the Original model uses the load's factor (0 included, as the original program does)
    m.mMove = m.mBullet + cal.gasMassShare * m.mc;
    m.gasMassShare = cal.gasMassShare;
    m.x0 = in.startPosition_mm() * 1e-3;
    m.jump = in.bulletJump_mm * 1e-3;
    m.neckLength = std::max(in.seatingDepth_mm * 1e-3, 0.0);
    m.neckStress = std::max(cal.neckStress_MPa, 0.0) * 1e6;
    const auto perClass = [&](double rifle, double handgun) { return in.handgun && handgun >= 0.0 ? handgun : rifle; };
    m.engravingLength = std::max(perClass(cal.engravingLength_mm, cal.engravingLengthHandgun_mm) * 1e-3, 0.0);
    m.primerTime = std::max(cal.primerTime_ms * 1e-3, 0.0);
    m.wallTemperature = in.temperature_C + kKelvinOffset;
    // The free run is not part of the gas volume at rest: the bullet base starts at x0 and gasVolume() adds A (x - x0) while it
    // travels the jump. Adding A * jump here counted the freebore twice.
    m.V0 = cal.capacityFactor * in.caseVolume_cm3 * 1e-6 - bulletIntrusionPhysical_m3(in);
    m.solid = 1.0 / in.density_kgm3;
    m.covolume = cal.gasCovolume_cm3g * 1e-3 * (in.covolumeEta > 0.0 ? in.covolumeEta : 1.0);
    m.theta = in.kappa - 1.0;
    m.eChem = cal.impetusFactor * in.heatQex_kJkg * 1e3;
    m.startP = in.startPressure_bar * kBar;
    m.gasR = cal.gasConstant_JkgK;
    const double vg0 = m.gasVolume(m.x0, 0.0);
    if (m.V0 <= 0.0 || vg0 <= 0.0 || m.x0 <= 0.0) return res;
    m.ePrimer = cal.primerEnergy_J;
    m.mPrimerGas = cal.primerEnergy_J * m.theta / (m.gasR * kPrimerGasTemperature_K);  // primer gas at about 2500 K
    // bearing surface: the bullet's length without the boat tail and about one calibre of nose (one calibre if the length is unknown)
    const double tail = in.tailType == 2 ? in.tailLength_mm * 1e-3 : 0.0;
    m.bearing =
        std::max(in.bulletLength_mm > 0.0 ? in.bulletLength_mm * 1e-3 - tail - bulletDia : bulletDia, kMinBearingDiameters * bulletDia);
    // relative oversize of the bullet over the groove diameter in percent (0 when the caliber has no groove diameter)
    const double oversize = in.grooveDiameter_mm > 0.0 && in.bulletDiameter_mm > 0.0
                                ? 100.0 * (in.bulletDiameter_mm - in.grooveDiameter_mm) / in.grooveDiameter_mm
                                : 0.0;
    const auto fit = [oversize](double k) { return std::max(1.0 + k * oversize, kMinFitFactor); };
    // absolute engraving pressure of the bullet in the rifling, else the empirical share of the start pressure
    const double engravePressure = cal.engravingPressure_bar > 0.0 ? cal.engravingPressure_bar * kBar
                                                                   : perClass(cal.engravingShare, cal.engravingShareHandgun) * m.startP;
    m.engraveForce = fit(cal.engravingInterference) * engravePressure * m.A;
    m.slideForce = cal.slideStress_MPa * fit(cal.slideInterference) * 1e6 * n::pi * bulletDia * m.bearing;
    m.extraForce = in.extraFriction_bar * kBar * m.A;
    m.heatCoeff = cal.heatTransferFactor * kDittusBoelter * kGasConductivity * std::pow(kGasPrandtl, 0.4) * std::pow(kGasViscosity, -0.8) *
                  std::pow(m.D, -0.2);
    const double rc = std::sqrt(m.V0 / (n::pi * m.x0));
    m.chamberSurface = 2.0 * n::pi * rc * m.x0 + 2.0 * n::pi * rc * rc;
    // the powder's physical burn values when the load has them, else the load's (original model's) values with the scaled burn rate
    const auto &pb = in.physicalBurn;
    m.burnBa = burnCoefficient(in, cal, pb);
    m.burnExponent = cal.burnExponent;
    m.ignitionTime = cal.ignitionTime_ms * 1e-3;
    m.heatVelocity = cal.heatVelocity_ms;
    m.phi = formFunction(in, cal, pb);

    const double xe = in.barrelLength_mm * 1e-3;
    const double tol = cal.integrationTolerance > 0.0 ? cal.integrationTolerance : kDefaultTolerance;
    const auto grow = [](double err)
    { return std::clamp(kStepSafety * std::pow(std::max(err, kMinStepError), -0.2), kStepShrink, kStepGrowth); };
    PhysState y{m.x0, 0.0, 0.0, 0.0, 0.0, 0.0};
    PhysState yn{};
    const auto stuck = [&]
    {
        res.status = SimStatus::BulletStuck;
        return res;
    };

    // ---- the powder burns in the closed case until the pressure reaches the start pressure ----
    double h = kInitialStep_s;
    bool released = m.gasPressure(y) >= m.startP;
    for (int guard = 0; !released; ++guard)
    {
        if (guard > 200000) return stuck();
        const double err = dopriStep(m, false, y, h, tol, yn);
        if (err > 1.0)
        {
            h *= grow(err);
            continue;
        }
        if (m.gasPressure(yn) >= m.startP)
        {
            // the moment the pressure reaches ps
            (void)stepUntil(m, false, y, h, tol, yn, [&](const PhysState &s) { return m.gasPressure(s) >= m.startP; });
            y = yn;
            released = true;
            break;
        }
        if (yn[2] >= 1.0) return stuck();  // everything burnt below the start pressure
        y = yn;
        h *= grow(err);
    }

    // ---- the bullet moves: accepted steps, resampled to the output grid afterwards ----
    struct Sample
    {
        double t, x, v, z, p;
    };
    std::vector<Sample> samples;
    const auto sample = [&](double t, const PhysState &s)
    { samples.push_back({t, s[0], s[1], s[2], m.breechPressure(m.rates(s, true), true)}); };
    const double dxOut = (xe - m.x0) / (steps - 1);
    double t = 0.0;
    h = kBoreInitialStep_s;
    sample(t, y);
    for (int guard = 0;; ++guard)
    {
        if (guard > kMaxSteps || t > kMaxTime_s) return stuck();
        h = std::min(h, dxOut / std::max(y[1], 1.0));  // at least one sample per output step
        const double err = dopriStep(m, true, y, h, tol, yn);
        if (err > 1.0)
        {
            h *= grow(err);
            continue;
        }
        if (yn[0] >= xe)
        {
            // the step that ends at the muzzle
            t += stepUntil(m, true, y, h, tol, yn, [xe](const PhysState &s) { return s[0] >= xe; });
            y = yn;
            sample(t, y);
            break;
        }
        t += h;
        y = yn;
        sample(t, y);
        if (y[1] <= 1e-6 && y[2] >= 1.0 && m.rates(y, true).a <= 0.0) return stuck();  // burnt out and stopped in the bore
        h *= grow(err);
    }

    // The bullet waits at the start while the resistance (engraving, friction) is above the pressure force: the curves and the barrel
    // time begin when it moves (the last sample at rest).
    std::size_t first = 0;
    while (first + 1 < samples.size() && samples[first + 1].v <= 0.0) ++first;
    // the pressure rise before that (time before the motion is negative): the bullet lead time starts at 10 % of the maximum pressure
    for (std::size_t i = 0; i <= first; ++i)
        res.restPhase.push_back({.t_ms = (samples[i].t - samples[first].t) * 1e3, .p_bar = samples[i].p / kBar});
    if (first > 0)
    {
        samples.erase(samples.begin(), samples.begin() + static_cast<std::ptrdiff_t>(first));
        const double t0 = samples.front().t;
        for (Sample &s : samples) s.t -= t0;
    }

    // ---- equal position steps from the start position to the muzzle ----
    res.points.reserve(static_cast<std::size_t>(steps));
    std::size_t j = 0;
    for (int i = 0; i < steps; ++i)
    {
        const double x = i + 1 == steps ? samples.back().x : m.x0 + i * dxOut;
        while (j + 2 < samples.size() && samples[j + 1].x < x) ++j;
        const Sample &a = samples[j];
        const Sample &b = samples[std::min(j + 1, samples.size() - 1)];
        const double w = b.x > a.x ? std::clamp((x - a.x) / (b.x - a.x), 0.0, 1.0) : 1.0;
        const auto lerp = [w](double p, double q) { return p + w * (q - p); };
        const double v = lerp(a.v, b.v);
        res.points.push_back({.x_mm = x * 1e3,
                              .z = lerp(a.z, b.z),
                              .p_bar = lerp(a.p, b.p) / kBar,
                              .v_ms = v,
                              .e_J = 0.5 * mp * v * v,
                              .t_ms = lerp(a.t, b.t) * 1e3});
    }
    summarize(res, in, cal, freeVolume_cm3);  // propellant impulse and recoil with the load's Sebert factor (0..1, 0 included)
    res.frictionForce_N = m.slideForce + m.extraForce;
    // The work of that force alone: y[4] also holds the work against the engraving and the air, and the sliding friction only acts once
    // the bullet has crossed the free run, which the reported travel still includes.
    res.muzzleFrictionEnergy_J = res.frictionForce_N * std::max(0.0, y[0] - (m.x0 + m.jump));
    res.heatLoss_J = y[3];
    return res;
}

}  // namespace

// Chamber volume behind the bullet at the start: the case capacity (scaled: the database value is a water capacity,
// the effective volume is larger), minus the bullet inside the case, plus empirical offsets (one of them a length
// of bore behind the bullet base). A bullet jump adds the bore volume of the free run: measured on the original
// (batch 10), this reproduces its lower pressure and velocity for jumps of 0.25..4 mm within about 1.5 % of the
// pressure (the original shows no effect for jumps up to 0.5 mm in a 9 mm Luger, the model does).
double chamberVolume_m3(const SimInput &in, const Calibration &calibration)
{
    const Calibration &cal = effectiveCalibration(in, calibration);
    return cal.capacityFactor * in.caseVolume_cm3 * 1e-6 - cal.intrusionFactor * bulletIntrusion_m3(in) + cal.chamberOffset_cm3 * 1e-6 +
           in.boreArea_mm2 * (cal.chamberOffsetLength_mm + in.bulletJump_mm) * 1e-9;
}

SimResult simulate(const SimInput &in, const Calibration &calibration, int steps)
{
    const Calibration &cal = effectiveCalibration(in, calibration);
    // the primer's start pressure belongs to the physical model (fitted with it); the other models keep the load's start pressure
    if (in.primerOffset_bar != 0.0)
    {
        SimInput adjusted = in;
        adjusted.primerOffset_bar = 0.0;
        if (cal.model == SolverModel::Physical && !in.shotshell) adjusted.startPressure_bar += in.primerOffset_bar;
        return simulate(adjusted, calibration, steps);
    }
    if (in.handgun && !in.shotshell && cal.handgunStartFraction * in.maxPressure_bar > in.startPressure_bar)
    {
        SimInput adjusted = in;
        adjusted.startPressure_bar = cal.handgunStartFraction * in.maxPressure_bar;
        return simulate(adjusted, calibration, steps);
    }
    SimResult res;
    const double x0 = in.startPosition_mm() * 1e-3;
    const double xe = in.barrelLength_mm * 1e-3;

    // NaN or infinity anywhere would pass the "> 0" checks below (or skip them) and give NaN results
    const double all[] = {in.caseLength_mm,
                          in.caseVolume_cm3,
                          in.boreArea_mm2,
                          in.sebert,
                          in.startPressure_bar,
                          in.barrelLength_mm,
                          in.twistLength_mm,
                          in.gunWeight_kg,
                          in.bulletJump_mm,
                          in.bulletDiameter_mm,
                          in.bulletMass_g,
                          in.bulletLength_mm,
                          in.seatingDepth_mm,
                          in.tailLength_mm,
                          in.tailDiameterA_mm,
                          in.tailDiameterB_mm,
                          in.extraFriction_bar,
                          in.vivacityBa,
                          in.heatQex_kJkg,
                          in.kappa,
                          in.a0,
                          in.a1,
                          in.z1,
                          in.z2,
                          in.covolumeEta,
                          in.density_kgm3,
                          in.bulkDensity_kgm3,
                          in.temperature_C,
                          in.coldCoeff,
                          in.hotCoeff,
                          in.shotCapacityScale,
                          in.shotEnergyScale,
                          in.charge_g};
    // the burn values of the physical model are separate inputs (they replace the load's Ba, a0, z1, z2 in that model)
    const bool burnOk = !in.physicalBurn || (std::isfinite(in.physicalBurn->Ba) && std::isfinite(in.physicalBurn->a0) &&
                                             std::isfinite(in.physicalBurn->z1) && std::isfinite(in.physicalBurn->z2));
    const bool inputOk = std::ranges::all_of(all, [](double v) { return std::isfinite(v); }) && burnOk && steps > 1 &&
                         in.boreArea_mm2 > 0.0 && in.bulletMass_g > 0.0 && in.charge_g > 0.0 && in.density_kgm3 > 0.0 &&
                         in.vivacityBa > 0.0 && in.heatQex_kJkg > 0.0 && in.kappa > 1.0 && in.caseVolume_cm3 > 0.0 && xe > x0;
    if (!inputOk) return res;

    // The original rejects charges that do not fit: measured on its accepted/rejected loads, the limit is
    // 1.2 x bulk density in the volume left after the bullet intrudes into the case (accepted loads reach
    // 1.10, rejected ones start at 1.29). The physical model uses the ogive-corrected intrusion.
    const bool physical = cal.model == SolverModel::Physical && !in.shotshell;
    const double freeVolume_cm3 = in.caseVolume_cm3 - (physical ? bulletIntrusionPhysical_m3(in) : bulletIntrusion_m3(in)) * 1e6;
    if (freeVolume_cm3 <= 0.0 || in.charge_g / freeVolume_cm3 > 1.2 * in.bulkDensity_kgm3 * 1e-3)
    {
        res.status = SimStatus::ChargeTooHigh;
        return res;
    }

    // the project's own model (shotshells keep their model, fitted with the Original equations)
    if (physical) return simulatePhysical(in, cal, steps, freeVolume_cm3);

    // ---- the constants of this load ----
    Model m{};
    m.A_m2 = in.boreArea_mm2 * 1e-6;
    m.mEff_kg = in.effectiveMass_g() * 1e-3;
    m.mc_kg = in.charge_g * 1e-3;
    m.V0_m3 = chamberVolume_m3(in, cal) * (in.shotshell ? cal.shotCapacityFactor * in.shotCapacityScale : 1.0);
    m.x0_m = x0;
    m.solid_m3kg = cal.solidDensityFactor / in.density_kgm3;
    // measured: the original ignores eta (a load with eta 1.2 gives the identical curve)
    m.covolume_m3kg = cal.covolume_cm3g * 1e-3 + cal.covolumeDensityFactor / in.density_kgm3;
    m.theta = in.kappa - 1.0;
    m.fEnergy = cal.impetusFactor * m.theta * in.heatQex_kJkg * 1e3 * (in.shotshell ? cal.shotEnergyFactor * in.shotEnergyScale : 1.0);
    m.kinetic = cal.kineticFactor;
    m.burnBa = burnCoefficient(in, cal);
    m.burnExponent = cal.burnExponent;
    m.burnZ0Factor = cal.burnZ0Factor;
    m.phi = formFunction(in, cal, std::nullopt);
    if (m.V0_m3 <= 0.0) return res;

    // shares of the pressure force lost to friction: "gfriction" of the original (always applied here), and for a
    // shotshell the wad and shot sliding in the bore
    // a negative gfriction (low-friction bullets, the original's database goes to -100 bar) adds force; limited so a typing error stays harmless
    const double gfrictionShare = std::clamp(cal.manualFrictionFactor * in.extraFriction_bar, -0.5, 0.9);
    const double shotShare = in.shotshell ? std::clamp(cal.shotFrictionShare, 0.0, 0.9) : 0.0;
    const double manualFriction = 1.0 - (1.0 - gfrictionShare) * (1.0 - shotShare);
    const double p0 = in.startPressure_bar * kBar;
    m.z0 = cal.initialGasFactor * p0 * m.gasVolume(0.0, x0) /
           (m.fEnergy * m.mc_kg);  // initial gas energy reproduces the start pressure p(z=0,v=0)=ps

    // ---- integration over equal steps of the bullet's position (the original uses 2000) ----
    const double dx = (xe - x0) / steps;
    res.points.reserve(static_cast<std::size_t>(steps));
    res.points.push_back({.x_mm = x0 * 1e3, .z = 0.0, .p_bar = in.startPressure_bar});

    // state at the start of a step: burnt fraction, velocity, bullet (momentum) pressure, gas pressure the powder
    // burns at, time
    double z = 0.0, v = 0.0, p = p0, pBurn = p0, t = 0.0;
    for (int i = 1; i < steps; ++i)
    {
        const double x1 = x0 + i * dx;
        // the ramp was measured in steps of the original's 2000 point grid: it stays the same distance for other `steps`
        const double ignited =
            cal.ignitionRate > 0.0 ? 1.0 - std::exp(-cal.ignitionRate * (i * (kReferenceSteps / steps) + cal.ignitionLead)) : 1.0;

        // What a trial end velocity v1 implies: the pressure the bullet needs for it (pMom, momentum equation), the
        // pressure the gas has (pTh, energy balance with the powder burnt over the step), the new z and the gas
        // pressure the powder burnt at.
        struct Trial
        {
            double pMom, pTh, z, pGas;
        };
        const double rate = m.burnRate(z, p);  // burn rates at the start of the step, the same for every trial
        const double rateRamp = m.burnRate(z, pBurn);
        const auto trial = [&](double v1) -> Trial
        {
            const double pMom = m.mEff_kg * (v1 * v1 - v * v) / (2.0 * m.A_m2 * dx * (1.0 - manualFriction));
            const double dt = 2.0 * dx / (v + v1);
            if (ignited >= 1.0)
            {
                const double zNew = m.advanceBurnFrom(z, rate, pMom, dt);
                return {pMom, m.thermoPressure(zNew, v1, x1), zNew, pMom};
            }
            // the powder burns at the full gas pressure, the bullet only feels the ignited part of it
            double zNew = m.advanceBurnFrom(z, rateRamp, pBurn, dt), pGas = pBurn;
            for (int k = 0; k < 4; ++k)
            {
                pGas = m.thermoPressure(zNew, v1, x1);
                zNew = m.advanceBurnFrom(z, rateRamp, pGas, dt);
            }
            return {pMom, m.thermoPressure(zNew, v1, x1, ignited), zNew, pGas};
        };

        // Solve pMom(v1) = pTh(v1): pMom rises with v1, pTh falls (more expansion, more kinetic energy). Bracket the
        // root by doubling, then bisect. If even the smallest step forward needs more pressure than the gas has, the
        // bullet does not accelerate in this step.
        double lo = std::max(v, 1e-9);
        double hi = std::max(2.0 * v, 1.0);
        const auto gap = [&](double v1)
        {
            const Trial b = trial(v1);
            return b.pMom - b.pTh;  // negative below the root, positive above
        };
        double flo = gap(lo);
        if (flo >= 0.0)
        {
            hi = lo;  // stalled: nothing to solve
        }
        else
        {
            double fhi = 0.0;
            for (int g = 0; g < 60; ++g)
            {
                fhi = gap(hi);
                if (fhi >= 0.0) break;
                lo = hi;
                flo = fhi;
                hi *= 2.0;
            }
            // Illinois (regula falsi with halving of the stale end): the root to the precision of a double in a few
            // evaluations instead of 70 bisections; bisection when the secant step leaves the bracket
            int side = 0;
            for (int it = 0; it < 100 && hi - lo > 1e-14 * hi && fhi > 0.0; ++it)
            {
                double mid = hi - fhi * (hi - lo) / (fhi - flo);
                if (!(mid > lo && mid < hi)) mid = 0.5 * (lo + hi);
                const double fm = gap(mid);
                if (fm < 0.0)
                {
                    lo = mid;
                    flo = fm;
                    if (side == -1) fhi *= 0.5;
                    side = -1;
                }
                else
                {
                    hi = mid;
                    fhi = fm;
                    if (side == 1) flo *= 0.5;
                    side = 1;
                }
            }
        }
        const double v1 = 0.5 * (lo + hi);
        const Trial fin = trial(v1);
        t += 2.0 * dx / std::max(v + v1, 1e-12);
        z = fin.z;
        p = fin.pMom;
        pBurn = fin.pGas;
        v = v1;
        res.points.push_back(
            {.x_mm = x1 * 1e3, .z = z, .p_bar = p / kBar, .v_ms = v, .e_J = 0.5 * in.bulletMass_g * 1e-3 * v * v, .t_ms = t * 1e3});
    }

    summarize(res, in, cal, freeVolume_cm3);
    return res;
}

std::vector<LadderStep> simulateLadder(const SimInput &in, const Calibration &cal, int steps)
{
    std::vector<LadderStep> out;
    const int count = in.ladderCount < 2 || in.ladderStep_g <= 0.0 ? 1 : std::min(in.ladderCount, kMaxLadderSteps);
    for (int i = 0; i < count; ++i)
    {
        SimInput step = in;
        step.charge_g = in.charge_g - i * in.ladderStep_g;
        if (step.charge_g <= 0.0) break;
        out.push_back({.charge_g = step.charge_g, .result = simulate(step, cal, steps)});
    }
    if (out.empty()) out.push_back({.charge_g = in.charge_g, .result = simulate(in, cal, steps)});
    return out;
}

std::vector<double> vivacityCurve(const SimInput &in, const Calibration &calibration, int n)
{
    const Calibration &cal = effectiveCalibration(in, calibration);  // a shotshell: the constants its model uses
    std::vector<double> out;
    if (n < 2) return out;
    out.reserve(static_cast<std::size_t>(n));
    // the physical model with the powder's physical burn values: those, else the load's
    const auto &pb = cal.model == SolverModel::Physical && !in.shotshell ? in.physicalBurn : std::nullopt;
    const FormFunction phi = formFunction(in, cal, pb);
    const double ba = burnCoefficient(in, cal, pb);
    for (int i = 0; i < n; ++i) out.push_back(ba * phi(static_cast<double>(i) / (n - 1)));
    return out;
}

Calibration calibrationFor(CalibrationProfile profile)
{
    Calibration cal;  // the member defaults are the original's calibration
    // The burn-law constants of the Original profile were refitted on 2026-10-27 (burnout batch, z weight 2); the other profiles keep the
    // earlier ones, which their own fits are based on.
    if (profile != CalibrationProfile::Original)
    {
        cal.burnIntercept = 1.09171;
        cal.burnSlopeA0 = 0.434982;
        cal.burnSlopeOffset = 0.00638254;
        cal.burnExponent = -0.00810765;
        cal.phase2 = {-0.0867208, -0.0659705, -0.461006};
        cal.phase3 = {-0.103524, -0.0695024, 0.00958064};
    }
    if (profile == CalibrationProfile::Published)
    {
        // fitted to the published rifle and handgun loads of Hodgdon and Western Powders (analysis/calibrate_published.py, burn rates of the
        // records kept): trained on 22 powders, 3/4 of the cartridges; on the held-out cartridges and on other powders velocity 3.4-3.5 %
        // and peak pressure 10 % rms off (the original's constants: 4.5 % and 16 %). Some constants sit at the limits of the fit.
        cal.burnRateScale = 1.13597;
        cal.impetusFactor = 1.02996;
        cal.kineticFactor = 1.5;
        cal.covolume_cm3g = 0.000348165;
        cal.covolumeDensityFactor = 0.0545449;
        cal.solidDensityFactor = 9.93145e-05;
        cal.intrusionFactor = 0.3;
        cal.capacityFactor = 0.740647;
        cal.chamberOffsetLength_mm = 0.552238;
        cal.burnIntercept = 1.28131;
        cal.burnExponent = -0.00676818;
        cal.initialGasFactor = 1.19872;
    }
    else if (profile == CalibrationProfile::Physical)
    {
        // the project's own physical model (simulatePhysical), constants first fitted with analysis/calibrate_published.py to the velocities
        // and measured pressures of 22 rifle powders of the Hodgdon and Western Powders manuals (3/4 of the cartridges), alternating with
        // the powders' physical burn values (analysis/fit_physical_powders.py, data/physical/propellants.json). burnRateScale and
        // burnSlopeOffset matter only for powders without physical burn values (burnRateScale: 0.84 balances velocity and pressure when
        // all powders are simulated without their values). At a limit of the fit: capacityFactor, engravingShare (upper), gasCovolume_cm3g,
        // primerEnergy_J (lower).
        // 2026-10-04: gas mass share 0.2, burnExponent -0.15 and ignitionTime 0.3 ms (analysis/scan_physical.py on the loads of all
        // makers, the trends of the error over loading density and bullet mass are gone or small), all powders refitted.
        // 2026-10-08: ignitionTime 0.1 ms, burnExponent -0.05, engravingShare 0.6 (scan_physical.py): published velocity rises with the charge
        // (start to maximum load) with an elasticity of 0.91, the model had 0.66; start/max velocity bias +1.7 / -0.8 % -> +0.7 / -0.15 %.
        cal.model = SolverModel::Physical;
        cal.handgunStartFraction =
            0.3;  // 2026-10-08: pistol and revolver cartridges start at 0.3 x the permitted pressure (>= the load's own value)
        // 2026-10-17: the physical model uses the classical Lagrange pair (gasMassShare 1/4, breech burn pressure), the
        // ogive-corrected bullet intrusion and no double-counted jump volume. The per-powder Ba were refitted to the published
        // samples with these equations; the global constants keep the 2026-10-11 values (a joint fit on a subset improved the
        // pressure but lost the start-to-max velocity gain, 0.55 instead of 0.96 of the published).
        cal.burnRateScale = 0.84;
        cal.burnSlopeOffset = 1.31191;
        cal.impetusFactor = 1.01833;
        cal.capacityFactor = 1.15;
        cal.gasCovolume_cm3g = 0.8;
        cal.primerEnergy_J = 10.0009;
        cal.primerTime_ms = 0.05;  // the primer gas reaches the powder over this time constant
        cal.ignitionTime_ms = 0.1;
        // 2026-10-18: separate start resistance per class (share x start pressure): rifles 1.2 decaying over 2 mm, handguns (start
        // pressure 0.3 x pMax) 0.6 over the bearing length; docs/SOLVER.md
        cal.engravingShare = 1.2;
        cal.engravingLength_mm = 2.0;
        cal.engravingShareHandgun = 0.6;
        cal.engravingLengthHandgun_mm = 0.0;
        cal.slideStress_MPa = 0.700469;
        cal.slideInterference = 1.5;  // 2026-10-11: sliding stress +1.5 % per percent of bullet oversize over the groove (scan_physical.py)
        cal.heatTransferFactor = 0.0618079;
        cal.heatVelocity_ms = 169.199;
        cal.burnExponent = -0.05;
        cal.gasMassShare = 0.25;  // classical Lagrange value for the mean-pressure momentum and energy equations
    }
    return cal;
}

void setCalibrationProfile(CalibrationProfile profile)
{
    g_profile = profile;
    g_active = calibrationFor(profile);
}

CalibrationProfile calibrationProfile() noexcept
{
    return g_profile;
}

const Calibration &activeCalibration() noexcept
{
    return g_active;
}

std::vector<NamedParameter> parameters(Calibration &cal)
{
    return {{"burnRateScale", &cal.burnRateScale},
            {"impetusFactor", &cal.impetusFactor},
            {"kineticFactor", &cal.kineticFactor},
            {"covolume_cm3g", &cal.covolume_cm3g},
            {"covolumeDensityFactor", &cal.covolumeDensityFactor},
            {"solidDensityFactor", &cal.solidDensityFactor},
            {"intrusionFactor", &cal.intrusionFactor},
            {"chamberOffset_cm3", &cal.chamberOffset_cm3},
            {"capacityFactor", &cal.capacityFactor},
            {"chamberOffsetLength_mm", &cal.chamberOffsetLength_mm},
            {"frictionStress_MPa", &cal.frictionStress_MPa},
            {"gasImpulseFactor", &cal.gasImpulseFactor},
            {"mipFactor", &cal.mipFactor},
            {"manualFrictionFactor", &cal.manualFrictionFactor},
            {"hotCoeffDefault", &cal.hotCoeffDefault},
            {"coldCoeffDefault", &cal.coldCoeffDefault},
            {"explicitColdScale", &cal.explicitColdScale},
            {"explicitHotScale", &cal.explicitHotScale},
            {"burnIntercept", &cal.burnIntercept},
            {"burnSlopeA0", &cal.burnSlopeA0},
            {"burnSlopeOffset", &cal.burnSlopeOffset},
            {"burnExponent", &cal.burnExponent},
            {"burnZ0Factor", &cal.burnZ0Factor},
            {"initialGasFactor", &cal.initialGasFactor},
            {"handgunStartFraction", &cal.handgunStartFraction},
            {"shotCapacityFactor", &cal.shotCapacityFactor},
            {"shotFrictionShare", &cal.shotFrictionShare},
            {"shotEnergyFactor", &cal.shotEnergyFactor},
            {"ignitionRate", &cal.ignitionRate},
            {"ignitionLead", &cal.ignitionLead},
            {"phase2_3", &cal.phase2[0]},
            {"phase2_2", &cal.phase2[1]},
            {"phase2_1", &cal.phase2[2]},
            {"phase3Limit", &cal.phase3Limit},
            {"phase3_3", &cal.phase3[0]},
            {"phase3_2", &cal.phase3[1]},
            {"phase3_1", &cal.phase3[2]},
            {"gasCovolume_cm3g", &cal.gasCovolume_cm3g},
            {"primerEnergy_J", &cal.primerEnergy_J},
            {"primerTime_ms", &cal.primerTime_ms},
            {"ignitionTime_ms", &cal.ignitionTime_ms},
            {"engravingShare", &cal.engravingShare},
            {"engravingLength_mm", &cal.engravingLength_mm},
            {"engravingPressure_bar", &cal.engravingPressure_bar},
            {"engravingShareHandgun", &cal.engravingShareHandgun},
            {"engravingLengthHandgun_mm", &cal.engravingLengthHandgun_mm},
            {"neckStress_MPa", &cal.neckStress_MPa},
            {"slideStress_MPa", &cal.slideStress_MPa},
            {"engravingInterference", &cal.engravingInterference},
            {"slideInterference", &cal.slideInterference},
            {"caliberBurnScaling", &cal.caliberBurnScaling},
            {"heatTransferFactor", &cal.heatTransferFactor},
            {"heatVelocity_ms", &cal.heatVelocity_ms},
            {"bulletInertiaFactor", &cal.bulletInertiaFactor},
            {"gasMassShare", &cal.gasMassShare},
            {"gasConstant_JkgK", &cal.gasConstant_JkgK},
            {"integrationTolerance", &cal.integrationTolerance}};
}

}  // namespace csr
