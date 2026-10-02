// Developer tool: runs the solver on every reference load that has a result from the
// original program and reports how far apart they are.
//
//   grt_compare [--jobs DIR] [--results DIR] [--include RE] [--exclude RE]
//               [--set name=value]... [--steps n] [--list-params] [--quiet]
//
// Output: one line per run, then "OBJECTIVE <score>" (smaller is better): the rms over
// all runs of sqrt(err_pmax^2 + err_vmuz^2 + err_curve^2), all relative.
#include "loadfile.h"
#include "reference.h"
#include "solver.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>

#include <algorithm>
#include <cmath>

using namespace Qt::StringLiterals;

namespace
{

struct RunError
{
    double pmax{};
    double vmuz{};
    double curve{};    // rms of the relative pressure error along the barrel (where p > 5 % of pmax)
    double zMuzzle{};  // absolute error of the burnt fraction at the muzzle
};

RunError compare(const grt::SimResult &sim, const grt::ReferenceRun &ref)
{
    RunError e;
    const double refP = ref.number(u"MaxPressure"_s);
    const double refV = ref.number(u"MuzzleVelocity"_s);
    e.pmax = sim.maxPressure_bar / refP - 1.0;
    // the original reports 0 when the bullet does not leave the barrel: nothing to compare then
    e.vmuz = refV > 0.0 ? sim.muzzleVelocity_ms / refV - 1.0 : 0.0;
    // the solver's pressure at the positions of the reference points (linear interpolation: --steps changes the grid)
    const auto pressureAt = [&](double x_mm)
    {
        const auto &pts = sim.points;
        const auto it = std::ranges::lower_bound(pts, x_mm, {}, &grt::SimPoint::x_mm);
        if (it == pts.begin()) return pts.front().p_bar;
        if (it == pts.end()) return pts.back().p_bar;
        const auto &a = *(it - 1);
        const auto &b = *it;
        const double f = b.x_mm > a.x_mm ? (x_mm - a.x_mm) / (b.x_mm - a.x_mm) : 0.0;
        return a.p_bar + f * (b.p_bar - a.p_bar);
    };
    double sum = 0.0;
    std::size_t cnt = 0;
    for (std::size_t i = 1; i < ref.points.size(); ++i)
    {
        const double rp = ref.points[i].p_bar;
        if (rp < 0.05 * refP) continue;
        const double d = pressureAt(ref.points[i].x_mm) / rp - 1.0;
        sum += d * d;
        ++cnt;
    }
    e.curve = cnt ? std::sqrt(sum / static_cast<double>(cnt)) : 0.0;
    e.zMuzzle = sim.burntFraction - ref.points.back().z;
    return e;
}

}  // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCommandLineParser cli;
    cli.addHelpOption();
    const QString root = QDir::cleanPath(QCoreApplication::applicationDirPath() + u"/.."_s);
    cli.addOptions({
        {u"jobs"_s, u"directory with exp_*.grtload"_s, u"dir"_s, QStringLiteral(GRT_SOURCE_DIR "/tools/GRTRef/jobs_all")},
        {u"results"_s, u"directory with exp_*.json"_s, u"dir"_s, QStringLiteral(GRT_SOURCE_DIR "/reference/results")},
        {u"include"_s, u"only runs whose name matches this regex"_s, u"re"_s},
        {u"exclude"_s,
         u"skip runs whose name matches this regex (default: stale runs, and extreme probes of batch 7 whose reference is an artifact of the original's unclamped first step)"_s,
         u"re"_s,
         u"^(casevol_x1\\.25|cv_mp200_casevol1\\.5|x7_fa_Ba16.*|x7_fa_Ba32|x7_fc8_ps1000|x8_Ba16|f(b3|b6)_z0\\.(05_0\\.12|3_0\\.45)|sa_.*|sb_12-70_shot.*|sh_.*_on)$"_s},
        {u"set"_s, u"override a calibration constant (name=value)"_s, u"kv"_s},
        {u"loads"_s, u"only simulate every exp_*.grtload of this directory and print one line each (name, ok, pmax, v0, load ratio)"_s,
         u"dir"_s},
        {u"steps"_s, u"number of position steps of the solver (default 2000, like the original)"_s, u"n"_s, u"2000"_s},
        {u"list-params"_s, u"print the calibration constants and exit"_s},
        {u"quiet"_s, u"print only the OBJECTIVE line"_s},
        {u"zweight"_s, u"weight of the burnt-fraction error at the muzzle in the objective"_s, u"w"_s, u"0.5"_s},
        {u"dump"_s, u"print the curves (solver vs original) of the first run matching --include"_s},
        {u"stride"_s, u"with --dump: print every n-th point (default: about 40 points)"_s, u"n"_s, u"0"_s},
        {u"residuals"_s, u"print one line \"RES ...\" with the signed errors of all runs (for optimizers)"_s},
    });
    cli.process(app);

    grt::Calibration cal;
    auto params = grt::parameters(cal);
    QTextStream out(stdout);
    if (cli.isSet(u"list-params"_s))
    {
        for (const auto &p : params) out << p.name << " = " << *p.value << "\n";
        return 0;
    }
    for (const QString &kv : cli.values(u"set"_s))
    {
        const QString key = kv.section(u'=', 0, 0);
        const auto it = std::ranges::find_if(params, [&](const grt::NamedParameter &p) { return key == QLatin1String(p.name); });
        if (it == params.end())
        {
            out << "unknown parameter: " << key << "\n";
            return 2;
        }
        *it->value = kv.section(u'=', 1).toDouble();
    }

    const int steps = cli.value(u"steps"_s).toInt();
    if (cli.isSet(u"loads"_s))
    {  // a check of generated jobs: does the solver accept them, and what do they look like
        for (const QFileInfo &fi : QDir(cli.value(u"loads"_s)).entryInfoList({u"exp_*.grtload"_s}, QDir::Files, QDir::Name))
        {
            const auto lf = grt::LoadFile::read(fi.absoluteFilePath());
            if (!lf)
            {
                out << QString::asprintf("%-32s UNREADABLE\n", qPrintable(fi.completeBaseName()));
                continue;
            }
            const grt::SimResult r = grt::simulate(grt::SimInput::fromLoad(*lf), cal, steps);
            out << QString::asprintf("%-32s %s pmax %8.1f  v0 %7.1f  burnt %.2f  ratio %.3f\n", qPrintable(fi.completeBaseName()),
                                     r.ok() ? "ok " : "BAD", r.maxPressure_bar, r.muzzleVelocity_ms, r.burntFraction, r.loadRatio);
        }
        return 0;
    }

    const QRegularExpression include(cli.value(u"include"_s));
    const QRegularExpression exclude(cli.value(u"exclude"_s));
    const QDir resDir(cli.value(u"results"_s));
    const QDir jobDir(cli.value(u"jobs"_s));
    const double zWeight = cli.value(u"zweight"_s).toDouble();
    const bool quiet = cli.isSet(u"quiet"_s) || cli.isSet(u"residuals"_s);
    QStringList residuals;

    double sumSq = 0.0, sumP = 0.0, sumV = 0.0, sumC = 0.0;
    int n = 0, failed = 0, rejectedRefs = 0, rejectMismatch = 0, acceptMismatch = 0;
    for (const QFileInfo &fi : resDir.entryInfoList({u"exp_*.json"_s}, QDir::Files, QDir::Name))
    {
        const QString name = fi.completeBaseName().mid(4);
        if (!cli.value(u"include"_s).isEmpty() && !include.match(name).hasMatch()) continue;
        if (!cli.value(u"exclude"_s).isEmpty() && exclude.match(name).hasMatch()) continue;
        const auto ref = grt::readReferenceRun(fi.absoluteFilePath());
        const auto lf = grt::LoadFile::read(jobDir.filePath(u"exp_%1.grtload"_s.arg(name)));
        if (!ref || !lf || ref->stale) continue;
        if (ref->points.empty())
        {  // the original rejected the load: the solver should reject it too
            const bool tooHigh = ref->summary.value(u"PropellantChargeTooHigh"_s).toString() == u"true"_s;
            if (!tooHigh) continue;  // e.g. the first job of a batch, which was read before the app finished
            ++rejectedRefs;
            if (grt::simulate(grt::SimInput::fromLoad(*lf), cal, steps).ok())
            {
                ++rejectMismatch;
                if (!quiet) out << QString::asprintf("%-26s original rejected the charge, solver accepted it\n", qPrintable(name));
            }
            continue;
        }

        const grt::SimResult sim = grt::simulate(grt::SimInput::fromLoad(*lf), cal, steps);
        if (!sim.ok())
        {
            ++failed;
            ++acceptMismatch;
            if (!quiet) out << QString::asprintf("%-26s SIMULATION FAILED\n", qPrintable(name));
            sumSq += 1.0;
            ++n;
            residuals << u"1"_s << u"1"_s << u"1"_s << u"1"_s;
            continue;
        }
        if (cli.isSet(u"dump"_s))
        {  // x, then time / pressure / burnt fraction / velocity of the original and of the solver
            const std::size_t m = std::min(sim.points.size(), ref->points.size());
            out << "# x_mm  t_ref t_sim  p_ref p_sim  z_ref z_sim  v_ref v_sim\n";
            for (std::size_t i = 0; i < m; i += cli.value(u"stride"_s).toInt() > 0
                                                    ? static_cast<std::size_t>(cli.value(u"stride"_s).toInt())
                                                    : std::max<std::size_t>(1, m / 40))
            {
                const auto &a = ref->points[i];
                const auto &b = sim.points[i];
                out << QString::asprintf("%8.2f  %7.4f %7.4f  %8.1f %8.1f  %6.4f %6.4f  %7.1f %7.1f\n", a.x_mm, a.t_ms, b.t_ms, a.p_bar,
                                         b.p_bar, a.z, b.z, a.v_ms, b.v_ms);
            }
            return 0;
        }
        const RunError e = compare(sim, *ref);
        sumSq += e.pmax * e.pmax + e.vmuz * e.vmuz + e.curve * e.curve + zWeight * zWeight * e.zMuzzle * e.zMuzzle;
        sumP += std::abs(e.pmax);
        sumV += std::abs(e.vmuz);
        sumC += e.curve;
        ++n;
        residuals << QString::number(e.pmax, 'g', 8) << QString::number(e.vmuz, 'g', 8) << QString::number(e.curve, 'g', 8)
                  << QString::number(zWeight * e.zMuzzle, 'g', 8);
        if (!quiet)
            out << QString::asprintf("%-26s pmax %+7.2f %%   vmuz %+6.2f %%   curve rms %6.2f %%   z(muzzle) %+6.3f\n", qPrintable(name),
                                     100 * e.pmax, 100 * e.vmuz, 100 * e.curve, e.zMuzzle);
    }
    if (n == 0)
    {
        out << "no runs compared\n";
        return 1;
    }
    if (!quiet)
        out << QString::asprintf(
            "\ncharge rule: %d rejected by the original, %d of them accepted by the solver; %d accepted loads rejected by the solver\n",
            rejectedRefs, rejectMismatch, acceptMismatch);
    if (!quiet)
        out << QString::asprintf("\n%d runs (%d failed): mean |pmax| %.2f %%, mean |vmuz| %.2f %%, mean curve rms %.2f %%\n", n, failed,
                                 100 * sumP / n, 100 * sumV / n, 100 * sumC / n);
    if (cli.isSet(u"residuals"_s)) out << "RES " << residuals.join(u' ') << "\n";
    out << QString::asprintf("OBJECTIVE %.6f\n", std::sqrt(sumSq / n));
    return 0;
}
