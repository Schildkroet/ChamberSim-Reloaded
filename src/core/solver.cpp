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
// tools/grt_compare.cpp). It is NOT the original's algorithm. Known gaps are listed in the README ("Accuracy").

#include "solver.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace grt
{

namespace
{

constexpr double kBar = 1.0e5;              // Pa per bar
constexpr double kReferenceSteps = 2000.0;  // position steps of the original program

// Three-phase form function phi(z) of the burn law dz/dt = Ba * p[bar] * phi(z): progressive up to z1 (a straight
// line whose slope follows the powder's a0), primary degressive from z1 to z2, secondary degressive from z2 to 1. The
// shapes of phases 2 and 3 are cubics fitted to the original's results (Calibration::phase2 / phase3); the phases
// join without jumps.
struct FormFunction
{
    const Calibration *cal;
    double a0, z1, z2;

    // cubic with the given constant term c0 (the coefficients c are the cubic, quadratic and linear ones)
    [[nodiscard]] static double poly(const std::array<double, 3> &c, double c0, double x) noexcept
    {
        return ((c[0] * x + c[1]) * x + c[2]) * x + c0;
    }

    // phase 1: phi(0) = burnIntercept, rising (a0 > 0: progressive powder) or falling with z
    [[nodiscard]] double phase1(double z) const noexcept { return cal->burnIntercept + (cal->burnSlopeA0 * a0 + cal->burnSlopeOffset) * z; }

    [[nodiscard]] double operator()(double z) const noexcept
    {
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

    // The pressure part of the burn law for a pressure in bar: p * (p / 1000 bar)^burnExponent.
    [[nodiscard]] double pressureTerm(double pBar) const noexcept
    {
        return pBar * (burnExponent == 0.0 ? 1.0 : std::pow(std::max(pBar, 1.0) / 1000.0, burnExponent));
    }

    // dz/dt at the burnt fraction z for the given pressure term; 0 once everything has burnt.
    [[nodiscard]] double burnRateAt(double z, double pressureTermValue) const noexcept
    {
        return z >= 1.0 ? 0.0 : burnBa * pressureTermValue * phi(z + burnZ0Factor * z0);
    }

    // dz/dt at the burnt fraction z and the pressure p [Pa]; 0 once everything has burnt.
    [[nodiscard]] double burnRate(double z, double p_Pa) const noexcept { return burnRateAt(z, pressureTerm(p_Pa / kBar)); }

    // z after a time step dt: trapezoid rule between the rate at the start (zPrev, pPrev) and at the end (z, pNew),
    // iterated because the end rate depends on the new z. Clamped at 1.
    [[nodiscard]] double advanceBurn(double zPrev, double pPrev, double pNew, double dt) const noexcept
    {
        return advanceBurnFrom(zPrev, burnRate(zPrev, pPrev), pNew, dt);
    }

    // The same with the rate at the start of the step given (it does not change between the trials of a step).
    [[nodiscard]] double advanceBurnFrom(double zPrev, double r0, double pNew, double dt) const noexcept
    {
        const double term = pressureTerm(pNew / kBar);  // the same in all iterations: the pressure is given
        double z = zPrev;
        for (int k = 0; k < 3; ++k) z = std::min(1.0, zPrev + 0.5 * (r0 + burnRateAt(z, term)) * dt);
        return z;
    }
};

// Vivacity Ba at the powder temperature (the powder data refers to 21 degC).
[[nodiscard]] double burnCoefficient(const SimInput &in, const Calibration &cal) noexcept
{
    const double dT = in.temperature_C - 21.0;
    if (dT == 0.0) return in.vivacityBa;
    // Measured on the original: with both coefficients 0 the built-in defaults apply. Otherwise each side uses its
    // own coefficient, and a side whose coefficient is 0 has NO effect. An explicit coefficient acts on the burn
    // rate linearly in coefficient * |dT| with the sign of the coefficient (a positive tcc raises the rate when
    // cold), and far weaker than the documented Ba(21) - Ba(T) per K would give.
    if (in.coldCoeff == 0.0 && in.hotCoeff == 0.0)
        return in.vivacityBa * (1.0 + (dT > 0.0 ? cal.hotCoeffDefault : cal.coldCoeffDefault) * dT);
    return in.vivacityBa * (1.0 + (dT > 0.0 ? cal.explicitHotScale * in.hotCoeff * dT : cal.explicitColdScale * in.coldCoeff * -dT));
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

// Chamber volume behind the bullet at the start: the case capacity (scaled: the database value is a water capacity,
// the effective volume is larger), minus the bullet inside the case, plus empirical offsets (one of them a length
// of bore behind the bullet base). A bullet jump adds the bore volume of the free run: measured on the original
// (batch 10), this reproduces its lower pressure and velocity for jumps of 0.25..4 mm within about 1.5 % of the
// pressure (the original shows no effect for jumps up to 0.5 mm in a 9 mm Luger, the model does).
double chamberVolume_m3(const SimInput &in, const Calibration &cal)
{
    return cal.capacityFactor * in.caseVolume_cm3 * 1e-6 - cal.intrusionFactor * bulletIntrusion_m3(in) + cal.chamberOffset_cm3 * 1e-6 +
           in.boreArea_mm2 * (cal.chamberOffsetLength_mm + in.bulletJump_mm) * 1e-9;
}

SimResult simulate(const SimInput &in, const Calibration &cal, int steps)
{
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
                          in.density_kgm3,
                          in.bulkDensity_kgm3,
                          in.temperature_C,
                          in.coldCoeff,
                          in.hotCoeff,
                          in.charge_g};
    const bool inputOk = std::ranges::all_of(all, [](double v) { return std::isfinite(v); }) && steps > 1 && in.boreArea_mm2 > 0.0 &&
                         in.bulletMass_g > 0.0 && in.charge_g > 0.0 && in.density_kgm3 > 0.0 && in.vivacityBa > 0.0 &&
                         in.heatQex_kJkg > 0.0 && in.kappa > 1.0 && in.caseVolume_cm3 > 0.0 && xe > x0;
    if (!inputOk) return res;

    // The original rejects charges that do not fit: measured on its accepted/rejected loads, the limit is
    // 1.2 x bulk density in the volume left after the bullet intrudes into the case (accepted loads reach
    // 1.10, rejected ones start at 1.29).
    const double freeVolume_cm3 = in.caseVolume_cm3 - bulletIntrusion_m3(in) * 1e6;
    if (freeVolume_cm3 <= 0.0 || in.charge_g / freeVolume_cm3 > 1.2 * in.bulkDensity_kgm3 * 1e-3)
    {
        res.status = SimStatus::ChargeTooHigh;
        return res;
    }

    // ---- the constants of this load ----
    Model m{};
    m.A_m2 = in.boreArea_mm2 * 1e-6;
    m.mEff_kg = in.effectiveMass_g() * 1e-3;
    m.mc_kg = in.charge_g * 1e-3;
    m.V0_m3 = chamberVolume_m3(in, cal) * (in.shotshell ? cal.shotCapacityFactor : 1.0);
    m.x0_m = x0;
    m.solid_m3kg = cal.solidDensityFactor / in.density_kgm3;
    // measured: the original ignores eta (a load with eta 1.2 gives the identical curve)
    m.covolume_m3kg = cal.covolume_cm3g * 1e-3 + cal.covolumeDensityFactor / in.density_kgm3;
    m.theta = in.kappa - 1.0;
    m.fEnergy = cal.impetusFactor * m.theta * in.heatQex_kJkg * 1e3 * (in.shotshell ? cal.shotEnergyFactor : 1.0);
    m.kinetic = cal.kineticFactor;
    m.burnBa = burnCoefficient(in, cal);
    m.burnExponent = cal.burnExponent;
    m.burnZ0Factor = cal.burnZ0Factor;
    m.phi = {.cal = &cal, .a0 = in.a0, .z1 = in.z1, .z2 = in.z2};
    if (m.V0_m3 <= 0.0) return res;

    // "gfriction" of the original (only when unlocked): a share of the pressure force that is lost to friction
    // shares of the pressure force lost to friction: "gfriction" of the original (only when unlocked), and for a
    // shotshell the wad and shot sliding in the bore
    const double gfrictionShare = in.extraFrictionUnlocked ? std::min(cal.manualFrictionFactor * in.extraFriction_bar, 0.9) : 0.0;
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

    // ---- summary values of the curve ----
    const auto &pts = res.points;
    const auto pmax = std::ranges::max_element(pts, {}, &SimPoint::p_bar);
    res.indexPmax = static_cast<std::size_t>(pmax - pts.begin());
    res.maxPressure_bar = pmax->p_bar;
    const SimPoint &last = pts.back();
    res.muzzlePressure_bar = last.p_bar;
    res.muzzleVelocity_ms = last.v_ms;
    res.muzzleEnergy_J = last.e_J;
    res.muzzleTime_ms = last.t_ms;
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
    res.indexZ1 = firstAtLeast([](const SimPoint &q) { return q.z; }, in.z1);
    res.indexZ2 = firstAtLeast([](const SimPoint &q) { return q.z; }, in.z2);
    res.indexEnergy95 = firstAtLeast([](const SimPoint &q) { return q.e_J; }, 0.95 * last.e_J);
    res.effectiveCaseVolume_cm3 = freeVolume_cm3;
    res.loadRatio = in.charge_g / (in.bulkDensity_kgm3 * 1e-3 * freeVolume_cm3);
    if (in.twistLength_mm > 0.0) res.muzzleRpm = last.v_ms / (in.twistLength_mm * 1e-3) * 60.0;

    // ---- derived quantities (formulas identified from the original's results) ----
    constexpr double kFtPerM = 3.280839895;
    const double mc = in.charge_g * 1e-3, mp = in.bulletMass_g * 1e-3;
    res.burnoutPressure_bar = pts[res.indexBurnout].p_bar;
    res.propellantEnergy_J = in.heatQex_kJkg * 1e3 * mc * last.z;
    res.frictionForce_N = cal.frictionStress_MPa * std::numbers::pi * in.bulletDiameter_mm * in.bulletLength_mm;
    res.muzzleFrictionEnergy_J = res.frictionForce_N * (last.x_mm - pts.front().x_mm) * 1e-3;
    res.ipscFactor = in.bulletMass_g / 0.06479890999975 * last.v_ms * kFtPerM / 1000.0;
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

std::vector<double> vivacityCurve(const SimInput &in, const Calibration &cal, int n)
{
    std::vector<double> out;
    if (n < 2) return out;
    out.reserve(static_cast<std::size_t>(n));
    const FormFunction phi{.cal = &cal, .a0 = in.a0, .z1 = in.z1, .z2 = in.z2};
    const double ba = burnCoefficient(in, cal);
    for (int i = 0; i < n; ++i) out.push_back(ba * phi(static_cast<double>(i) / (n - 1)));
    return out;
}

std::vector<NamedParameter> parameters(Calibration &cal)
{
    return {{"impetusFactor", &cal.impetusFactor},
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
            {"phase3_1", &cal.phase3[2]}};
}

}  // namespace grt
