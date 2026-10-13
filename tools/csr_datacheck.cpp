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

// Checks the project's record files (data/<calibers|projectiles|propellants>/*.json) the way the program reads them:
// valid JSON, known columns, numbers where numbers belong, a name, plausible model values, and no two records with
// the same name. Prints every problem and exits with 1 if there is one (used by CI).
//
//   csr_datacheck [data directory]      (default: data/ of the source tree)

#include "database.h"
#include "dbfields.h"
#include "projectdata.h"

#include <QCoreApplication>
#include <QSet>
#include <QTemporaryDir>
#include <QTextStream>

using namespace Qt::StringLiterals;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    const QStringList args = app.arguments();
    const QString dataDir = args.size() > 1 ? args.at(1) : QStringLiteral(CSR_SOURCE_DIR "/data");
    QTextStream out(stdout);

    // a throw-away own database provides the schema
    QTemporaryDir tmp;
    csr::Database::setProjectDataLocation(dataDir);
    csr::Database db;
    QString error;
    if (!db.open({}, tmp.filePath(u"own.db"_s), &error))
    {
        out << "cannot open a database: " << error << '\n';
        return 2;
    }

    QStringList problems = db.projectErrors();
    for (const csr::Table t : csr::kRecordTables)
    {
        const csr::projectdata::Loaded loaded = csr::projectdata::load(dataDir, t, db.schema(t));
        QSet<QString> names;
        for (const QVariantMap &r : loaded.records)
        {
            const QString name = csr::dbfields::recordTitle(t, r).toLower();
            if (names.contains(name))
                problems << csr::projectdata::folderName(t) + u": two records named \""_s + csr::dbfields::recordTitle(t, r) + u'"';
            names.insert(name);
        }
        out << csr::projectdata::folderName(t) << ": " << db.projectCount(t) << " records\n";
    }
    for (const QString &p : problems) out << "ERROR " << p << '\n';
    out << (problems.isEmpty() ? "OK\n" : QStringLiteral("%1 problem(s)\n").arg(problems.size()));
    return problems.isEmpty() ? 0 : 1;
}
