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

// Renders the cartridge drawing (CartridgeView, as in the program) of every caliber record to a PNG, for a visual check of the dimensions:
//   QT_QPA_PLATFORM=offscreen csr_cartridges [--data dir] [--db original.db] [--only text] outDir
// Each caliber is loaded with a bullet of its diameter (the project's projectile closest in diameter, a plain cylinder when there is none)
// and the first propellant, with the default charge and seating depth.

#include "cartridgeview.h"
#include "database.h"
#include "loadbuilder.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QRegularExpression>
#include <QSqlQuery>

#include <algorithm>
#include <cmath>

using namespace Qt::StringLiterals;

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QCommandLineParser cli;
    cli.addPositionalArgument(u"outDir"_s, u"folder for the PNG files"_s);
    cli.addOption(
        {u"data"_s, u"project data folder (default: the source tree's data)"_s, u"dir"_s, QStringLiteral(CSR_SOURCE_DIR "/data")});
    cli.addOption({u"db"_s, u"the original's plain database (optional)"_s, u"file"_s});
    cli.addOption({u"only"_s, u"only calibers whose name contains this text"_s, u"text"_s});
    cli.addHelpOption();
    cli.process(app);
    if (cli.positionalArguments().isEmpty()) cli.showHelp(2);
    const QDir out(cli.positionalArguments().front());
    QDir().mkpath(out.path());

    csr::Database::setProjectDataLocation(cli.value(u"data"_s));
    csr::Database db;
    if (!db.open(cli.value(u"db"_s), QDir::temp().filePath(u"csr_cartridges_own.db"_s)))
    {
        qCritical("cannot open the database");
        return 2;
    }
    const QStringList caliberColumns = db.columns(csr::Table::Caliber);

    auto ids = [&](const char *view)
    {
        QList<int> v;
        QSqlQuery q(db.handle());
        q.exec(QStringLiteral("SELECT id FROM %1 WHERE geloescht = 0 ORDER BY id").arg(QLatin1String(view)));
        while (q.next()) v << q.value(0).toInt();
        return v;
    };
    const QList<int> caliberIds = ids("caliber");
    std::vector<QVariantMap> projectiles;
    for (int id : ids("projectile"))
        if (const auto r = db.record(csr::Table::Projectile, id)) projectiles.push_back(*r);
    const QList<int> propellantIds = ids("propellant");
    if (projectiles.empty() || propellantIds.isEmpty()) return 2;
    const auto propellant = db.record(csr::Table::Propellant, propellantIds.front());

    int written = 0, failed = 0;
    for (int id : caliberIds)
    {
        const auto caliber = db.record(csr::Table::Caliber, id);
        if (!caliber) continue;
        const QString name = caliber->value(u"cipname"_s).toString();
        if (cli.isSet(u"only"_s) && !name.contains(cli.value(u"only"_s), Qt::CaseInsensitive)) continue;

        const double dia = caliber->value(u"G1"_s).toDouble();
        // the bullet that sits in the neck up to the caliber's overall length: diameter first, then the length
        const double L3 = caliber->value(u"L3"_s).toDouble(), L6 = caliber->value(u"L6"_s).toDouble(),
                     L2 = caliber->value(u"L2"_s).toDouble();
        // seated about as deep as the neck is long, at most 1.2 calibers (straight cases have no L2: 40 % of the case length)
        const double wantLength = (L6 - L3) + std::min(L2 > 0 ? L3 - L2 : 0.4 * L3, 1.2 * dia);
        QVariantMap projectile = projectiles.front();
        double best = 1e9, bestScore = 1e9;
        for (const auto &p : projectiles)
        {
            const double d = std::abs(p.value(u"gdia"_s).toDouble() - dia);
            const double score = (d > 0.06 ? 1000.0 + d : 0.0) + std::abs(p.value(u"glen"_s).toDouble() - wantLength);
            if (score < bestScore) bestScore = score, best = d, projectile = p;
        }
        if (dia > 0 && (best > 0.06 || std::abs(projectile.value(u"glen"_s).toDouble() - wantLength) > 0.25 * wantLength))
        {  // no fitting bullet of this diameter: a plain cylinder of lead
            const double len = wantLength > 0 ? wantLength : 3.0 * dia;
            projectile.insert(u"gdia"_s, dia);
            projectile.insert(u"glen"_s, len);
            projectile.insert(u"gmass"_s, M_PI * dia * dia / 4.0 * len * 0.0108);
        }
        const csr::LoadFile load = csr::buildLoad(*caliber, projectile, *propellant, {}, caliberColumns);
        CartridgeView view;
        view.resize(420, 520);
        view.setLoad(load, csr::SimInput::fromLoad(load));
        QString file = name;
        file.replace(QRegularExpression(u"[^A-Za-z0-9.+-]+"_s), u"_"_s);
        const bool ok = view.geometry().valid || view.round().shotshell;
        if (!ok) ++failed;
        if (view.grab().save(out.filePath(QStringLiteral("%1%2_%3.png").arg(ok ? u""_s : u"INVALID_"_s).arg(id).arg(file)))) ++written;
    }
    qInfo("%d images written, %d without a valid geometry", written, failed);
    return 0;
}
