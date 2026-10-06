// Checks the project's record files (data/<calibers|projectiles|propellants>/*.json) the way the program reads them:
// valid JSON, known columns, numbers where numbers belong, a name, plausible model values, and no two records with
// the same name. Prints every problem and exits with 1 if there is one (used by CI).
//
//   grt_datacheck [data directory]      (default: data/ of the source tree)

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
    const QString dataDir = args.size() > 1 ? args.at(1) : QStringLiteral(GRT_SOURCE_DIR "/data");
    QTextStream out(stdout);

    // a throw-away own database provides the schema
    QTemporaryDir tmp;
    grt::Database::setProjectDataLocation(dataDir);
    grt::Database db;
    QString error;
    if (!db.open({}, tmp.filePath(u"own.db"_s), &error))
    {
        out << "cannot open a database: " << error << '\n';
        return 2;
    }

    QStringList problems = db.projectErrors();
    for (const grt::Table t : grt::kRecordTables)
    {
        const grt::projectdata::Loaded loaded = grt::projectdata::load(dataDir, t, db.schema(t));
        QSet<QString> names;
        for (const QVariantMap &r : loaded.records)
        {
            const QString name = grt::dbfields::recordTitle(t, r).toLower();
            if (names.contains(name))
                problems << grt::projectdata::folderName(t) + u": two records named \""_s + grt::dbfields::recordTitle(t, r) + u'"';
            names.insert(name);
        }
        out << grt::projectdata::folderName(t) << ": " << db.projectCount(t) << " records\n";
    }
    for (const QString &p : problems) out << "ERROR " << p << '\n';
    out << (problems.isEmpty() ? "OK\n" : QStringLiteral("%1 problem(s)\n").arg(problems.size()));
    return problems.isEmpty() ? 0 : 1;
}
