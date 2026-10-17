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

// Simulates loads made of records of the ORIGINAL program's database (caliber, projectile, propellant by id), with the solver profile
// of your choice (default: GRT Original). analysis/compare_grt_published.py uses it to compare the original's model with published data.
//
//   csr_dbeval --db GordonsReloadingTool.plain.db [--profile original|published] [--set name=value]... [--steps n] [--list-params] jobs.csv
//
// jobs.csv has no header; per line: row,caliberId,projectileId,propellantId,charge_g,barrel_mm,overallLength_mm,twist_mm (twist may be empty).
// Output, one line per job: row,status,muzzleVelocity_ms,maxPressure_bar (status ok / fail / seating / record).
// The project's own records (data/*/*.json) are not loaded, so only the original's records can be addressed.

#include "database.h"
#include "loadbuilder.h"
#include "simconfig.h"
#include "solver.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>

using namespace Qt::StringLiterals;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCommandLineParser cli;
    cli.addHelpOption();
    cli.addPositionalArgument(u"jobs"_s, u"CSV file without header, see the header of tools/csr_dbeval.cpp"_s);
    cli.addOptions({{u"db"_s, u"the original program's database (plain copy)"_s, u"file"_s},
                    {u"profile"_s, u"original (default) or published"_s, u"name"_s, u"original"_s},
                    {u"set"_s, u"calibration value name=value (repeatable)"_s, u"nv"_s},
                    {u"steps"_s, u"solver steps"_s, u"n"_s, u"2000"_s},
                    {u"list-params"_s, u"print the calibration names and values"_s}});
    cli.process(app);
    QTextStream out(stdout);

    csr::Calibration cal = csr::calibrationFor(cli.value(u"profile"_s) == u"published"_s ? csr::CalibrationProfile::Published
                                                                                         : csr::CalibrationProfile::Original);
    auto named = csr::parameters(cal);
    if (cli.isSet(u"list-params"_s))
    {
        for (const auto &p : named) out << QString::fromLatin1(p.name) << ' ' << *p.value << '\n';
        return 0;
    }
    for (const QString &nv : cli.values(u"set"_s))
    {
        const QString name = nv.section(u'=', 0, 0);
        const auto it = std::ranges::find_if(named, [&](const csr::NamedParameter &p) { return name == QLatin1String(p.name); });
        if (it == named.end())
        {
            qCritical("unknown calibration name %s", qPrintable(name));
            return 2;
        }
        *it->value = nv.section(u'=', 1).toDouble();
    }
    if (cli.positionalArguments().isEmpty() || !cli.isSet(u"db"_s)) return 2;

    QTemporaryDir tmp;  // an empty project data folder: only the original's records exist
    csr::Database::setProjectDataLocation(tmp.path());
    csr::Database db;
    if (!db.open(cli.value(u"db"_s), tmp.filePath(u"own.db"_s))) return 2;
    QStringList caliberColumns;
    for (const auto &[name, type] : db.schema(csr::Table::Caliber)) caliberColumns << name;

    QFile jobs(cli.positionalArguments().front());
    if (!jobs.open(QIODevice::ReadOnly | QIODevice::Text)) return 2;
    const int steps = cli.value(u"steps"_s).toInt();
    while (!jobs.atEnd())
    {
        const QStringList f = QString::fromUtf8(jobs.readLine()).trimmed().split(u',');
        if (f.size() < 8) continue;
        const auto caliber = db.record(csr::Table::Caliber, f[1].toInt());
        const auto projectile = db.record(csr::Table::Projectile, f[2].toInt());
        const auto propellant = db.record(csr::Table::Propellant, f[3].toInt());
        if (!caliber || !projectile || !propellant)
        {
            out << f[0] << ",record,,\n";
            continue;
        }
        csr::LoadParams p;
        p.charge_g = f[4].toDouble();
        p.barrelLength_mm = f[5].toDouble();
        p.seatingDepth_mm = caliber->value(u"L3"_s).toDouble() + projectile->value(u"glen"_s).toDouble() - f[6].toDouble();
        if (!f[7].isEmpty()) p.twistLength_mm = f[7].toDouble();
        if (p.seatingDepth_mm <= 1.0 || p.barrelLength_mm <= 0.0)
        {
            out << f[0] << ",seating,,\n";
            continue;
        }
        const csr::SimResult r =
            csr::simulate(csr::SimInput::fromLoad(csr::buildLoad(*caliber, *projectile, *propellant, p, caliberColumns)), cal, steps);
        out << f[0] << ',' << (r.status == csr::SimStatus::Ok ? "ok" : "fail") << ','
            << QString::asprintf("%.2f,%.1f", r.muzzleVelocity_ms, r.maxPressure_bar) << '\n';
    }
    return 0;
}
