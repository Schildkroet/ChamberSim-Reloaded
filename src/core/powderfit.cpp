#include "powderfit.h"
#include "log.h"

#include "measurement.h"
#include "solver.h"

#include <QDate>

#include <algorithm>
#include <array>
#include <cmath>
#include <set>

using namespace Qt::StringLiterals;

namespace grt::powderfit
{

namespace
{

constexpr double kGrainToGram = 0.06479891;  // also cm3 of water per grain H2O

double &field(SimInput &in, Param p)
{
    switch (p)
    {
    case Param::Ba: return in.vivacityBa;
    case Param::A0: return in.a0;
    case Param::Z1: return in.z1;
    case Param::Z2: return in.z2;
    }
    return in.vivacityBa;
}

// keeps the model valid: Ba > 0, 0 < z1 < z2 < 1, a0 >= 0
void clamp(std::vector<double> &x, const std::vector<Param> &params)
{
    double z1 = -1.0;
    for (std::size_t i = 0; i < params.size(); ++i)
    {
        switch (params[i])
        {
        case Param::Ba: x[i] = std::clamp(x[i], 0.01, 50.0); break;
        case Param::A0: x[i] = std::clamp(x[i], 0.0, 20.0); break;
        case Param::Z1:
            x[i] = std::clamp(x[i], 0.02, 0.95);
            z1 = x[i];
            break;
        case Param::Z2: x[i] = std::clamp(x[i], 0.05, 0.99); break;
        }
    }
    for (std::size_t i = 0; i < params.size(); ++i)
        if (params[i] == Param::Z2 && z1 > 0.0) x[i] = std::max(x[i], z1 + 0.02);
}

struct Simulated
{
    bool ok{};
    double velocity{}, pressure{};
};

Simulated simulateWith(const Sample &s, const std::vector<Param> &params, const std::vector<double> &x)
{
    SimInput in = s.input;
    in.charge_g = s.charge_g;
    in.ladderCount = 1;
    for (std::size_t i = 0; i < params.size(); ++i) field(in, params[i]) = x[i];
    const SimResult r = simulate(in);
    return {r.ok(), r.muzzleVelocity_ms, r.maxPressure_bar};
}

// relative errors, weighted with the square root of the number of shots (a mean of more shots counts more)
std::vector<double> residuals(const std::vector<Sample> &samples, const std::vector<Param> &params, const std::vector<double> &x,
                              double pressureWeight, std::vector<Simulated> *sims = nullptr)
{
    std::vector<double> r;
    for (const Sample &s : samples)
    {
        const Simulated sim = simulateWith(s, params, x);
        if (sims) sims->push_back(sim);
        const double wv = std::sqrt(static_cast<double>(s.shots));
        r.push_back(sim.ok ? wv * (sim.velocity / s.velocity_ms - 1.0) : wv);  // a charge that cannot be simulated: 100 %
        if (s.pressureShots > 0 && pressureWeight > 0.0)
        {
            const double wp = pressureWeight * std::sqrt(static_cast<double>(s.pressureShots));
            r.push_back(sim.ok ? wp * (sim.pressure / s.pressure_bar - 1.0) : wp);
        }
    }
    return r;
}

double sumSquares(const std::vector<double> &r)
{
    double s = 0.0;
    for (double v : r) s += v * v;
    return s;
}

// solves A x = b for a small dense system (Gaussian elimination with pivoting); false if singular
bool solve(std::vector<std::vector<double>> a, std::vector<double> b, std::vector<double> &x)
{
    const std::size_t n = b.size();
    for (std::size_t c = 0; c < n; ++c)
    {
        std::size_t piv = c;
        for (std::size_t r = c + 1; r < n; ++r)
            if (std::abs(a[r][c]) > std::abs(a[piv][c])) piv = r;
        if (std::abs(a[piv][c]) < 1e-300) return false;
        std::swap(a[piv], a[c]);
        std::swap(b[piv], b[c]);
        for (std::size_t r = c + 1; r < n; ++r)
        {
            const double f = a[r][c] / a[c][c];
            for (std::size_t k = c; k < n; ++k) a[r][k] -= f * a[c][k];
            b[r] -= f * b[c];
        }
    }
    x.assign(n, 0.0);
    for (std::size_t i = n; i-- > 0;)
    {
        double s = b[i];
        for (std::size_t k = i + 1; k < n; ++k) s -= a[i][k] * x[k];
        x[i] = s / a[i][i];
    }
    return true;
}

void rms(const std::vector<Sample> &samples, const std::vector<Simulated> &sims, double &velocity, double &pressure)
{
    double sv = 0.0, sp = 0.0, nv = 0.0, np = 0.0;
    for (std::size_t i = 0; i < samples.size(); ++i)
    {
        const double ev = sims[i].ok ? sims[i].velocity / samples[i].velocity_ms - 1.0 : 1.0;
        sv += samples[i].shots * ev * ev;
        nv += samples[i].shots;
        if (samples[i].pressureShots > 0)
        {
            const double ep = sims[i].ok ? sims[i].pressure / samples[i].pressure_bar - 1.0 : 1.0;
            sp += samples[i].pressureShots * ep * ep;
            np += samples[i].pressureShots;
        }
    }
    velocity = nv > 0 ? std::sqrt(sv / nv) : 0.0;
    pressure = np > 0 ? std::sqrt(sp / np) : 0.0;
}

}  // namespace

QString name(Param p)
{
    switch (p)
    {
    case Param::Ba: return u"Ba"_s;
    case Param::A0: return u"a0"_s;
    case Param::Z1: return u"z1"_s;
    case Param::Z2: return u"z2"_s;
    }
    return {};
}

std::vector<Param> parse(const QString &list, QString *error)
{
    std::vector<Param> out;
    for (const QString &t : list.split(u',', Qt::SkipEmptyParts))
    {
        const QString n = t.trimmed();
        bool found = false;
        for (const Param p : {Param::Ba, Param::A0, Param::Z1, Param::Z2})
            if (n.compare(name(p), Qt::CaseInsensitive) == 0)
            {
                if (std::ranges::find(out, p) == out.end()) out.push_back(p);
                found = true;
            }
        if (!found)
        {
            if (error) *error = u"unknown parameter \"%1\" (Ba, a0, z1, z2)"_s.arg(n);
            return {};
        }
    }
    return out;
}

std::vector<Sample> samples(const LoadFile &load, const QString &source, QStringList *warnings)
{
    std::vector<Sample> out;
    const auto series = readMeasurements(load.appendixXml);
    if (series.empty())
    {
        if (warnings) *warnings << u"%1: no Measurements tab"_s.arg(source);
        return out;
    }
    const SimInput in = SimInput::fromLoad(load);
    for (const Measurement &m : series)
        for (const MeasuredCharge &c : m.charges)
        {
            const ShotStatistics v = velocityStatistics(c);
            if (v.count == 0) continue;
            const ShotStatistics p = pressureStatistics(c);
            out.push_back({.source = source,
                           .input = in,
                           .charge_g = c.charge_g(),
                           .shots = v.count,
                           .velocity_ms = v.mean,
                           .pressureShots = p.count,
                           .pressure_bar = p.mean});
        }
    if (out.empty() && warnings) *warnings << u"%1: no charge with velocity shots"_s.arg(source);
    if (warnings && !out.empty())
    {
        // the CIP capacity of the caliber record vs. the case volume of the load
        const double cip = load.caliberFile.number(u"V") * kGrainToGram;
        if (cip > 0.0 && std::abs(in.caseVolume_cm3 / cip - 1.0) < 0.005)
            *warnings << u"%1: the case volume is the database value (%2 cm3); a measured FIRED case (water) gives a "
                         "better model"_s.arg(source, QString::number(in.caseVolume_cm3, 'f', 3));
    }
    return out;
}

std::vector<Param> defaultParams(const std::vector<Sample> &samples)
{
    std::set<QString> files;
    for (const Sample &s : samples) files.insert(s.source);
    if (samples.size() >= 6 && files.size() >= 2) return {Param::Ba, Param::A0, Param::Z1, Param::Z2};
    if (samples.size() >= 3) return {Param::Ba, Param::A0};
    return {Param::Ba};
}

Result fit(const std::vector<Sample> &samples, const Options &options)
{
    Result res;
    res.params = options.params;
    if (samples.empty() || options.params.empty()) return res;
    const auto &params = options.params;
    const std::size_t n = params.size();
    LOG_Debug() << "Powder fit:" << samples.size() << "samples," << n << "parameters, at most" << options.maxIterations << "iterations";

    std::vector<double> x;
    SimInput start = samples.front().input;
    for (const Param p : params) x.push_back(field(start, p));
    clamp(x, params);
    res.before = x;

    std::vector<Simulated> simsBefore;
    std::vector<double> r = residuals(samples, params, x, options.pressureWeight, &simsBefore);
    double cost = sumSquares(r);
    const double startCost = cost;
    double lambda = 1e-2;

    for (int it = 0; it < options.maxIterations; ++it)
    {
        res.iterations = it + 1;
        // numerical Jacobian (forward differences)
        std::vector<std::vector<double>> jac(r.size(), std::vector<double>(n));
        for (std::size_t j = 0; j < n; ++j)
        {
            std::vector<double> xh = x;
            const double h = 1e-4 * std::max(std::abs(x[j]), 0.05);
            xh[j] += h;
            const std::vector<double> rh = residuals(samples, params, xh, options.pressureWeight);
            for (std::size_t i = 0; i < r.size(); ++i) jac[i][j] = (rh[i] - r[i]) / h;
        }
        std::vector<std::vector<double>> jtj(n, std::vector<double>(n, 0.0));
        std::vector<double> jtr(n, 0.0);
        for (std::size_t i = 0; i < r.size(); ++i)
            for (std::size_t a = 0; a < n; ++a)
            {
                jtr[a] -= jac[i][a] * r[i];
                for (std::size_t b = 0; b < n; ++b) jtj[a][b] += jac[i][a] * jac[i][b];
            }
        // Levenberg-Marquardt: try steps with growing damping until one lowers the cost
        bool accepted = false;
        for (int tries = 0; tries < 12 && !accepted; ++tries)
        {
            auto damped = jtj;
            for (std::size_t a = 0; a < n; ++a) damped[a][a] += lambda * std::max(jtj[a][a], 1e-12);
            std::vector<double> step;
            if (!solve(damped, jtr, step))
            {
                lambda *= 10.0;
                continue;
            }
            std::vector<double> xn = x;
            for (std::size_t a = 0; a < n; ++a) xn[a] += step[a];
            clamp(xn, params);
            const std::vector<double> rn = residuals(samples, params, xn, options.pressureWeight);
            const double cn = sumSquares(rn);
            if (cn < cost)
            {
                const double gain = cost - cn;
                x = xn;
                r = rn;
                cost = cn;
                lambda = std::max(lambda / 3.0, 1e-7);
                accepted = true;
                if (gain < 1e-10 * std::max(cost, 1e-12)) it = options.maxIterations;  // converged
            }
            else
            {
                lambda *= 10.0;
            }
        }
        if (!accepted) break;
    }

    std::vector<Simulated> simsAfter;
    residuals(samples, params, x, options.pressureWeight, &simsAfter);
    res.after = x;
    res.improved = cost < startCost;
    for (std::size_t i = 0; i < samples.size(); ++i)
        res.samples.push_back({.velocityBefore = simsBefore[i].ok ? simsBefore[i].velocity : 0.0,
                               .velocityAfter = simsAfter[i].ok ? simsAfter[i].velocity : 0.0,
                               .pressureBefore = simsBefore[i].ok ? simsBefore[i].pressure : 0.0,
                               .pressureAfter = simsAfter[i].ok ? simsAfter[i].pressure : 0.0});
    rms(samples, simsBefore, res.velocityRmsBefore, res.pressureRmsBefore);
    rms(samples, simsAfter, res.velocityRmsAfter, res.pressureRmsAfter);
    LOG_Info() << "Powder fit finished after" << res.iterations << "iterations, improved" << res.improved << "; velocity rms"
               << res.velocityRmsBefore << "->" << res.velocityRmsAfter << ", pressure rms" << res.pressureRmsBefore << "->"
               << res.pressureRmsAfter;
    return res;
}

void apply(EntryList &propellant, const Result &result)
{
    for (std::size_t i = 0; i < result.params.size() && i < result.after.size(); ++i)
        propellant.set(name(result.params[i]), result.after[i]);
}

QVariantMap toRecord(const EntryList &propellant, const Result &result, const projectdata::Schema &schema, const QString &descr)
{
    EntryList fitted = propellant;
    apply(fitted, result);
    QVariantMap record;
    for (const auto &[column, type] : schema)
    {
        const Entry *e = fitted.find(column);
        if (!e || column == u"id" || column == u"geloescht")
            continue;  // the photos stay (saving the record copies them into the own database)
        record.insert(column, type == u"FLOAT" || type == u"INTEGER" ? QVariant(fitted.number(column)) : QVariant(e->value));
    }
    record.insert(u"pt"_s, 21.0);  // the fitted Ba refers to 21 degC (the loads' powder temperatures are converted)
    record.insert(u"origin"_s, u"own measurements"_s);
    record.insert(u"cdate"_s, QDate::currentDate().toString(Qt::ISODate));
    record.insert(u"descr"_s, descr);
    return record;
}

QString describe(const Result &result, const std::vector<Sample> &samples)
{
    QStringList names, files;
    for (const Param p : result.params) names << name(p);
    int shots = 0;
    for (const Sample &s : samples)
    {
        shots += s.shots;
        if (!files.contains(s.source)) files << s.source;
    }
    return u"Fitted with GRT Reloaded (%1) to %2 charges / %3 shots in %4 load(s): %5. Velocity error %6 %."_s.arg(names.join(u", "_s))
        .arg(samples.size())
        .arg(shots)
        .arg(files.size())
        .arg(files.join(u", "_s))
        .arg(100 * result.velocityRmsAfter, 0, 'f', 2);
}

}  // namespace grt::powderfit
