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

// Headless simulation for scripts and batch jobs.
//
//   csr_sim [--profile name] [--grt-original] [--charge g] [--ladder-count n --ladder-step g]
//           [--format table|csv|json] [--out file] [--curve file] [--steps n] load.csrload|load.grtload ...
//
// One result row per load and ladder step (index 0 = highest charge). Units are the base units of the
// model: g, mm, bar, m/s, J, ms. Exit code: 0 all simulations ok, 1 at least one failed, 2 usage or read error.
#include "loadfile.h"
#include "numeric.h"
#include "physicalpowders.h"
#include "shotparts.h"
#include "simconfig.h"
#include "solver.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>

using namespace Qt::StringLiterals;

namespace
{

QString statusName(csr::SimStatus s)
{
    switch (s)
    {
    case csr::SimStatus::Ok: return u"ok"_s;
    case csr::SimStatus::InvalidInput: return u"invalid_input"_s;
    case csr::SimStatus::ChargeTooHigh: return u"charge_too_high"_s;
    case csr::SimStatus::BulletStuck: return u"bullet_stuck"_s;
    }
    return u"unknown"_s;
}

struct Row
{
    QString file;
    int step{};
    double charge_g{};
    csr::SimResult r;
};

// column name and value of one result row, in output order
using Column = std::pair<QString, QVariant>;

std::vector<Column> columns(const Row &row)
{
    const auto &r = row.r;
    return {
        {u"file"_s, row.file},
        {u"step"_s, row.step},
        {u"status"_s, statusName(r.status)},
        {u"charge_g"_s, row.charge_g},
        {u"maxPressure_bar"_s, r.maxPressure_bar},
        {u"muzzlePressure_bar"_s, r.muzzlePressure_bar},
        {u"muzzleVelocity_ms"_s, r.muzzleVelocity_ms},
        {u"muzzleEnergy_J"_s, r.muzzleEnergy_J},
        {u"muzzleTime_ms"_s, r.muzzleTime_ms},
        {u"burntFraction"_s, r.burntFraction},
        {u"efficiency"_s, r.efficiency},
        {u"burnoutInBarrel"_s, r.burnoutInBarrel},
        {u"loadRatio"_s, r.loadRatio},
    };
}

QString text(const QVariant &v)
{
    if (v.typeId() == QMetaType::Double) return QString::number(v.toDouble(), 'g', 6);
    if (v.typeId() == QMetaType::Bool) return v.toBool() ? u"1"_s : u"0"_s;
    return v.toString();
}

QString csvField(const QString &s)
{
    return s.contains(u',') || s.contains(u'"') ? u"\""_s + QString(s).replace(u'"', u"\"\""_s) + u'"' : s;
}

QString formatRows(const std::vector<Row> &rows, const QString &format)
{
    QString out;
    QTextStream ts(&out);
    if (format == u"json"_s)
    {
        QJsonArray arr;
        for (const Row &row : rows)
        {
            QJsonObject o;
            for (const auto &[name, v] : columns(row)) o.insert(name, QJsonValue::fromVariant(v));
            arr.append(o);
        }
        ts << QJsonDocument(arr).toJson(QJsonDocument::Indented);
        return out;
    }
    const bool csv = format == u"csv"_s;
    bool header = true;
    for (const Row &row : rows)
    {
        const auto cols = columns(row);
        if (header)
        {
            QStringList names;
            for (const auto &c : cols) names << (csv ? c.first : c.first.leftJustified(c.first == u"file"_s ? 28 : 18));
            ts << names.join(csv ? u","_s : u" "_s).trimmed() << "\n";
            header = false;
        }
        QStringList fields;
        for (const auto &c : cols)
        {
            const QString t = text(c.second);
            fields << (csv ? csvField(t) : t.leftJustified(c.first == u"file"_s ? 28 : 18));
        }
        ts << fields.join(csv ? u","_s : u" "_s).trimmed() << "\n";
    }
    return out;
}

bool writeCurve(const QString &path, const csr::SimResult &r, QString *error)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        *error = f.errorString();
        return false;
    }
    QTextStream ts(&f);
    ts << "x_mm,t_ms,p_bar,v_ms,e_J,z\n";
    for (const auto &p : r.points) ts << p.x_mm << ',' << p.t_ms << ',' << p.p_bar << ',' << p.v_ms << ',' << p.e_J << ',' << p.z << '\n';
    return true;
}

}  // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCommandLineParser cli;
    cli.setApplicationDescription(
        u"Simulates loads without a GUI. Units: g, mm, bar, m/s, J, ms. Exit code 0 = all ok, 1 = a simulation failed, 2 = usage or read error."_s);
    cli.addHelpOption();
    cli.addPositionalArgument(u"load"_s, u"load file(s): .csrload or .grtload"_s, u"load..."_s);
    cli.addOptions({
        {u"profile"_s, u"model: original, published or physical (default)"_s, u"name"_s, u"physical"_s},
        {u"grt-original"_s, u"use the Original model for .grtload files (like the setting in the program)"_s},
        {u"charge"_s, u"override the charge of the load (g)"_s, u"g"_s},
        {u"ladder-count"_s, u"override the number of ladder steps"_s, u"n"_s},
        {u"ladder-step"_s, u"override the charge difference of the ladder (g)"_s, u"g"_s},
        {u"format"_s, u"output: table (default), csv or json"_s, u"fmt"_s, u"table"_s},
        {u"out"_s, u"write the results to this file instead of stdout"_s, u"file"_s},
        {u"curve"_s, u"write the pressure/velocity curve (CSV) of the focused ladder step; {name} is replaced by the load's base name"_s,
         u"file"_s},
        {u"steps"_s, u"position steps of the solver (default 2000)"_s, u"n"_s, u"2000"_s},
    });
    cli.process(app);

    QTextStream err(stderr);
    const QStringList files = cli.positionalArguments();
    if (files.isEmpty())
    {
        err << "no load file given (see --help)\n";
        return 2;
    }
    const QString format = cli.value(u"format"_s);
    if (format != u"table"_s && format != u"csv"_s && format != u"json"_s)
    {
        err << "unknown format: " << format << "\n";
        return 2;
    }
    const QString profile = cli.value(u"profile"_s);
    if (profile != u"original"_s && profile != u"published"_s && profile != u"physical"_s)
    {
        err << "unknown profile: " << profile << "\n";
        return 2;
    }
    const auto profileOf = [](const QString &name)
    {
        return name == u"original"_s    ? csr::CalibrationProfile::Original
               : name == u"published"_s ? csr::CalibrationProfile::Published
                                        : csr::CalibrationProfile::Physical;
    };
    const int steps = cli.value(u"steps"_s).toInt();
    if (steps < 10)
    {
        err << "invalid --steps\n";
        return 2;
    }
    if (cli.isSet(u"curve"_s) && files.size() > 1 && !cli.value(u"curve"_s).contains(u"{name}"_s))
    {
        err << "--curve with several loads needs {name} in the file name\n";
        return 2;
    }

    std::vector<Row> rows;
    bool failed = false;
    for (const QString &path : files)
    {
        QString error;
        auto lf = csr::LoadFile::read(path, &error);
        if (!lf)
        {
            err << path << ": " << error << "\n";
            return 2;
        }
        csr::attachPhysicalBurn(lf->propellant);
        csr::ensureShotParts(lf->caliber);

        csr::SimInput in = csr::SimInput::fromLoad(*lf);
        if (cli.isSet(u"charge"_s)) in.charge_g = cli.value(u"charge"_s).toDouble();
        if (cli.isSet(u"ladder-count"_s)) in.ladderCount = cli.value(u"ladder-count"_s).toInt();
        if (cli.isSet(u"ladder-step"_s)) in.ladderStep_g = cli.value(u"ladder-step"_s).toDouble();

        const bool grt = csr::formatForPath(path) == csr::FileFormat::Original;
        const csr::Calibration cal =
            csr::calibrationFor(grt && cli.isSet(u"grt-original"_s) ? csr::CalibrationProfile::Original : profileOf(profile));
        const auto ladder = csr::simulateLadder(in, cal, steps);

        const QString name = QFileInfo(path).completeBaseName();
        for (std::size_t i = 0; i < ladder.size(); ++i)
        {
            rows.push_back({name, static_cast<int>(i), ladder[i].charge_g, ladder[i].result});
            failed |= !ladder[i].result.ok();
        }
        if (cli.isSet(u"curve"_s) && !ladder.empty())
        {
            const int focus = csr::toIntClamped(lf->config.number(u"CurveFocusIdx", 0.0), 0, static_cast<int>(ladder.size()) - 1);
            QString curvePath = cli.value(u"curve"_s);
            curvePath.replace(u"{name}"_s, name);
            if (!writeCurve(curvePath, ladder[static_cast<std::size_t>(focus)].result, &error))
            {
                err << curvePath << ": " << error << "\n";
                return 2;
            }
        }
    }

    const QString result = formatRows(rows, format);
    if (cli.isSet(u"out"_s))
    {
        QFile f(cli.value(u"out"_s));
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            err << f.fileName() << ": " << f.errorString() << "\n";
            return 2;
        }
        f.write(result.toUtf8());
    }
    else QTextStream(stdout) << result;
    return failed ? 1 : 0;
}
