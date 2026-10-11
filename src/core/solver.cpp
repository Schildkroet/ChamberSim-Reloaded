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

#include <algorithm>
#include <cmath>
#include <numbers>

namespace csr
{

namespace
{

constexpr double kBar = 1.0e5;              // Pa per bar
constexpr double kReferenceSteps = 2000.0;  // position steps of the original program

// The pressure part of the burn law for a pressure in bar: p * (p / 1000 bar)^exponent.
[[nodiscard]] double burnPressureTerm(double pBar, double exponent) noexcept
{
    return pBar * (exponent == 0.0 ? 1.0 : std::pow(std::max(pBar, 1.0) / 1000.0, exponent));
}

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

    [[nodiscard]] double pressureTerm(double pBar) const noexcept { return burnPressureTerm(pBar, burnExponent); }

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
// `Ba` is the vivacity at 21 degC to use (default: the load's, times burnRateScale).
[[nodiscard]] double burnCoefficient(const SimInput &in, const Calibration &cal, double Ba = 0.0) noexcept
{
    const double dT = in.temperature_C - 21.0;
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
    return pb ? FormFunction{.cal = &cal, .a0 = pb->a0, .z1 = pb->z1, .z2 = pb->z2}
              : FormFunction{.cal = &cal, .a0 = in.a0, .z1 = in.z1, .z2 = in.z2};
}

[[nodiscard]] double burnCoefficient(const SimInput &in, const Calibration &cal, const std::optional<PhysicalBurn> &pb) noexcept
{
    return pb ? burnCoefficient(in, cal, pb->Ba) : burnCoefficient(in, cal);
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
    constexpr double kFtPerM = 3.280839895;
    const double mc = in.charge_g * 1e-3, mp = in.bulletMass_g * 1e-3;
    res.burnoutPressure_bar = pts[res.indexBurnout].p_bar;
    res.propellantEnergy_J = in.heatQex_kJkg * 1e3 * mc * last.z;
    res.frictionForce_N = cal.frictionStress_MPa * std::numbers::pi * in.bulletDiameter_mm * in.bulletLength_mm;
    res.muzzleFrictionEnergy_J = res.frictionForce_N * (last.x_mm - pts.front().x_mm) * 1e-3;
    res.ipscFactor = in.bulletMass_g / kGrainToGram * last.v_ms * kFtPerM / 1000.0;
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
//   U     = impetusFactor * Qex * mc * z + Eprimer - 1/2 M v^2 - Q - W
//   Vgas  = V0 + A (x - x0) - mc (1 - z) / pc - eta * b * mc * z
//   M dv/dt = A p - F,   M = mp (1 + spin inertia) + sebert * mc
//   dQ/dt = h S (T - Tw),   dW/dt = F v
// The bullet stays at x0 until the gas pressure reaches the start pressure ps; until then the powder burns in the closed case, lit by
// the primer gas (primerEnergy_J in the powder space). The resistance F: engraving (engravingShare * ps * A once the bullet has
// crossed the free run gjump, decaying over the bearing length), friction of the bearing surface (slideStress * pi * D * bearing
// length), the user's extra friction gfriction [bar] * A (when unlocked) and the air pushed out of the bore (piston-driven shock).
// Heat transfer: Dittus-Boelter for turbulent pipe flow, h = factor * 0.023 * lambda * Pr^0.4 * mu^-0.8 * D^-0.2 * (rho u)^0.8 with
// the gas velocity u = v / 2 + heatVelocity_ms (the turbulence of the burning charge, also before the bullet moves), the gas temperature T = p Vgas / (m_gas R), the wetted surface = powder space (a cylinder as long
// as the case behind the bullet) + the bore behind the bullet. The reported pressure is the breech pressure of the Lagrange gradient.
// The friction outputs are the sliding friction (bearing surface + gfriction) and its work over the bore; the engraving and the air are
// part of the energy balance but not of the friction outputs.

using PhysState = std::array<double, 6>;  // x [m], v [m/s], z, heat loss Q [J], resistance work W [J], time since ignition [s]

struct PhysicalModel
{
    double A, D, mBullet, mMove, mc, V0, x0, solid, covolume, theta, eChem, ePrimer, mPrimerGas, startP, jump, bearing;
    double engraveForce, slideForce, extraForce, heatCoeff, heatVelocity, gasR, chamberSurface, burnBa, burnExponent, ignitionTime;
    FormFunction phi;

    [[nodiscard]] double gasVolume(double x, double z) const noexcept
    {
        return V0 + A * (x - x0) - mc * (1.0 - z) * solid - covolume * mc * z;
    }
    [[nodiscard]] double gasPressure(const PhysState &y) const noexcept
    {
        const double e = eChem * mc * y[2] + ePrimer - 0.5 * mMove * y[1] * y[1] - y[3] - y[4];
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
    [[nodiscard]] double resistance(double x, double v) const noexcept
    {
        double f = extraForce + airPressure(v) * A;
        const double s = x - x0 - jump;  // travel in the rifling
        // in the free run the case neck still holds the bullet: the release force of the start pressure, which the engraving continues at s = 0
        // (without it the bullet would run free at the start pressure: -15 % peak pressure for a jump of 0.25 mm in a 9 mm Luger)
        if (s >= 0.0) f += slideForce + engraveForce * std::exp(-s / bearing);
        else f += engraveForce;
        return std::max(f, 0.0);
    }
    [[nodiscard]] double heatRate(const PhysState &y, double p) const noexcept
    {
        const double vg = gasVolume(y[0], y[2]);
        const double u = 0.5 * y[1] + heatVelocity;
        if (heatCoeff <= 0.0 || vg <= 0.0 || u <= 0.0) return 0.0;
        const double mg = mc * y[2] + mPrimerGas;
        const double T = p * vg / (mg * gasR);
        const double h = heatCoeff * std::pow(mg / vg * u, 0.8);
        return std::max(h * (chamberSurface + std::numbers::pi * D * (y[0] - x0)) * (T - 293.0), 0.0);
    }
    struct Rates
    {
        PhysState dy;
        double p, a, force;
    };
    [[nodiscard]] Rates rates(const PhysState &y, bool released) const noexcept
    {
        const double p = gasPressure(y);
        const double pBar = p / kBar;
        const double term = burnPressureTerm(pBar, burnExponent);
        const double lit = ignitionTime > 0.0 ? 1.0 - std::exp(-y[5] / ignitionTime) : 1.0;
        const double dz = y[2] >= 1.0 ? 0.0 : burnBa * term * phi(y[2]) * lit;
        double a = 0.0, force = 0.0;
        if (released)
        {
            force = resistance(y[0], y[1]);
            a = (A * p - force) / mMove;
            if (y[1] <= 0.0 && a < 0.0) a = 0.0;  // the bullet waits until the gas overcomes the resistance
        }
        return {{y[1], a, dz, heatRate(y, p), force * std::max(y[1], 0.0), 1.0}, p, a, force};
    }
    // breech pressure (Lagrange): the bullet base needs (bullet mass * a + F) / A, the gas column adds mc * a / (2 A)
    [[nodiscard]] double breechPressure(const Rates &r, bool released) const noexcept
    {
        return released ? (mBullet * r.a + r.force) / A + mc * r.a / (2.0 * A) : r.p;
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
    // a moving powder charge without kinetic energy is not physical: Sebert 0 (loads built from the project's caliber records by earlier
    // versions, which have none) counts as 0.5 like the original's default; the Original model keeps 0 (the original program does)
    m.mMove = m.mBullet + cal.gasInertiaFactor * (in.sebert > 0.0 ? in.sebert : 0.5) * m.mc;
    m.x0 = in.startPosition_mm() * 1e-3;
    m.jump = in.bulletJump_mm * 1e-3;
    m.V0 = cal.capacityFactor * in.caseVolume_cm3 * 1e-6 - bulletIntrusion_m3(in) + m.A * m.jump;
    m.solid = 1.0 / in.density_kgm3;
    m.covolume = cal.gasCovolume_cm3g * 1e-3 * (in.covolumeEta > 0.0 ? in.covolumeEta : 1.0);
    m.theta = in.kappa - 1.0;
    m.eChem = cal.impetusFactor * in.heatQex_kJkg * 1e3;
    m.startP = in.startPressure_bar * kBar;
    m.gasR = cal.gasConstant_JkgK;
    const double vg0 = m.gasVolume(m.x0, 0.0);
    if (m.V0 <= 0.0 || vg0 <= 0.0 || m.x0 <= 0.0) return res;
    m.ePrimer = cal.primerEnergy_J;
    m.mPrimerGas = cal.primerEnergy_J * m.theta / (m.gasR * 2500.0);  // primer gas at about 2500 K
    // bearing surface: the bullet's length without the boat tail and about one calibre of nose (one calibre if the length is unknown)
    const double tail = in.tailType == 2 ? in.tailLength_mm * 1e-3 : 0.0;
    m.bearing = std::max(in.bulletLength_mm > 0.0 ? in.bulletLength_mm * 1e-3 - tail - bulletDia : bulletDia, 0.3 * bulletDia);
    // relative oversize of the bullet over the groove diameter in percent (0 when the caliber has no groove diameter)
    const double oversize = in.grooveDiameter_mm > 0.0 && in.bulletDiameter_mm > 0.0
                                ? 100.0 * (in.bulletDiameter_mm - in.grooveDiameter_mm) / in.grooveDiameter_mm
                                : 0.0;
    const auto fit = [oversize](double k) { return std::max(1.0 + k * oversize, 0.25); };
    m.engraveForce = cal.engravingShare * fit(cal.engravingInterference) * m.startP * m.A;
    m.slideForce = cal.slideStress_MPa * fit(cal.slideInterference) * 1e6 * n::pi * bulletDia * m.bearing;
    m.extraForce = in.extraFrictionUnlocked ? in.extraFriction_bar * kBar * m.A : 0.0;
    // gas: lambda 0.2 W/(m K), mu 8e-5 Pa s, Pr 0.7
    m.heatCoeff = cal.heatTransferFactor * 0.023 * 0.2 * std::pow(0.7, 0.4) * std::pow(8e-5, -0.8) * std::pow(m.D, -0.2);
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
    const double tol = cal.integrationTolerance > 0.0 ? cal.integrationTolerance : 1e-7;
    const auto grow = [](double err) { return std::clamp(0.9 * std::pow(std::max(err, 1e-12), -0.2), 0.2, 5.0); };
    PhysState y{m.x0, 0.0, 0.0, 0.0, 0.0, 0.0};
    PhysState yn{};
    const auto stuck = [&]
    {
        res.status = SimStatus::BulletStuck;
        return res;
    };

    // ---- the powder burns in the closed case until the pressure reaches the start pressure ----
    double h = 1e-7;
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
    h = 1e-8;
    sample(t, y);
    for (int guard = 0;; ++guard)
    {
        if (guard > 2000000 || t > 0.1) return stuck();
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
    SimInput used = in;  // the derived outputs (propellant impulse, recoil) with the Sebert factor of the dynamics
    if (used.sebert <= 0.0) used.sebert = 0.5;
    summarize(res, used, cal, freeVolume_cm3);
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
    if (in.handgun && !in.shotshell && in.barrelLength_mm < cal.handgunMaxBarrel_mm &&
        cal.handgunStartFraction * in.maxPressure_bar > in.startPressure_bar)
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
    // 1.10, rejected ones start at 1.29).
    const double freeVolume_cm3 = in.caseVolume_cm3 - bulletIntrusion_m3(in) * 1e6;
    if (freeVolume_cm3 <= 0.0 || in.charge_g / freeVolume_cm3 > 1.2 * in.bulkDensity_kgm3 * 1e-3)
    {
        res.status = SimStatus::ChargeTooHigh;
        return res;
    }

    // the project's own model (shotshells keep their model, fitted with the Original equations)
    if (cal.model == SolverModel::Physical && !in.shotshell) return simulatePhysical(in, cal, steps, freeVolume_cm3);

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
        // 2026-10-04: gasInertiaFactor 0.2, burnExponent -0.15 and ignitionTime 0.3 ms (analysis/scan_physical.py on the loads of all
        // makers, the trends of the error over loading density and bullet mass are gone or small), all powders refitted.
        // 2026-10-08: ignitionTime 0.1 ms, burnExponent -0.05, engravingShare 0.6 (scan_physical.py): published velocity rises with the charge
        // (start to maximum load) with an elasticity of 0.91, the model had 0.66; start/max velocity bias +1.7 / -0.8 % -> +0.7 / -0.15 %.
        cal.model = SolverModel::Physical;
        cal.handgunStartFraction =
            0.3;  // 2026-10-08: pistol and revolver cartridges start at 0.3 x the permitted pressure (>= the load's own value)
        cal.burnRateScale = 0.84;
        cal.burnSlopeOffset = 1.31191;
        cal.impetusFactor = 1.01833;
        cal.capacityFactor = 1.15;
        cal.gasCovolume_cm3g = 0.8;
        cal.primerEnergy_J = 10.0009;
        cal.ignitionTime_ms = 0.1;
        cal.engravingShare = 0.6;
        cal.slideStress_MPa = 0.700469;
        cal.slideInterference = 1.5;  // 2026-10-11: sliding stress +1.5 % per percent of bullet oversize over the groove (scan_physical.py)
        cal.heatTransferFactor = 0.0618079;
        cal.heatVelocity_ms = 169.199;
        cal.burnExponent = -0.05;
        cal.gasInertiaFactor = 0.2;
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
            {"handgunMaxBarrel_mm", &cal.handgunMaxBarrel_mm},
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
            {"ignitionTime_ms", &cal.ignitionTime_ms},
            {"engravingShare", &cal.engravingShare},
            {"slideStress_MPa", &cal.slideStress_MPa},
            {"engravingInterference", &cal.engravingInterference},
            {"slideInterference", &cal.slideInterference},
            {"heatTransferFactor", &cal.heatTransferFactor},
            {"heatVelocity_ms", &cal.heatVelocity_ms},
            {"bulletInertiaFactor", &cal.bulletInertiaFactor},
            {"gasInertiaFactor", &cal.gasInertiaFactor},
            {"gasConstant_JkgK", &cal.gasConstant_JkgK},
            {"integrationTolerance", &cal.integrationTolerance}};
}

}  // namespace csr
