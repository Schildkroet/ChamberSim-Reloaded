#include "database.h"

#include <QDate>
#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStandardPaths>
#include <QUrl>
#include <QUuid>

#include <array>

using namespace Qt::StringLiterals;

namespace grt {

namespace {

// The tables of the original program's database (same columns), created in the own database.
struct TableSchema {
    const char *name;
    const char *columns;
};
constexpr std::array<TableSchema, 5> kSchema{{
    {"caliber",
     "id INTEGER PRIMARY KEY AUTOINCREMENT, cipname TEXT NOT NULL, altname TEXT NOT NULL, "
     "standard TEXT NOT NULL, ciporigin TEXT NOT NULL, ciptype TEXT NOT NULL, cipdate DATE NOT NULL, "
     "ciprevdate DATE NOT NULL, cippdf TEXT NOT NULL, L0 FLOAT NOT NULL, L1 FLOAT NOT NULL, "
     "L2 FLOAT NOT NULL, L3 FLOAT NOT NULL, L4 FLOAT NOT NULL, L5 FLOAT NOT NULL, L6 FLOAT NOT NULL, "
     "R FLOAT NOT NULL, R1 FLOAT NOT NULL, R3 FLOAT NOT NULL, E FLOAT NOT NULL, E1 FLOAT NOT NULL, "
     "Emin FLOAT NOT NULL, delta TEXT NOT NULL, FG FLOAT NOT NULL, beta TEXT NOT NULL, P0 FLOAT NOT NULL, "
     "P1 FLOAT NOT NULL, P2 FLOAT NOT NULL, alpha TEXT NOT NULL, S FLOAT NOT NULL, r1min FLOAT NOT NULL, "
     "r2 FLOAT NOT NULL, H1 FLOAT NOT NULL, H2 FLOAT NOT NULL, G1 FLOAT NOT NULL, G2 FLOAT NOT NULL, "
     "f FLOAT NOT NULL, L3G FLOAT NOT NULL, Pmax FLOAT NOT NULL, PK FLOAT NOT NULL, PE FLOAT NOT NULL, "
     "EE FLOAT NOT NULL, V FLOAT NOT NULL, M TEXT NOT NULL, c_L0 FLOAT NOT NULL, c_L1 FLOAT NOT NULL, "
     "c_L2 FLOAT NOT NULL, c_L3 FLOAT NOT NULL, c_R FLOAT NOT NULL, c_R1 FLOAT NOT NULL, "
     "c_R2 FLOAT NOT NULL, c_R3 FLOAT NOT NULL, c_r_ FLOAT NOT NULL, c_E FLOAT NOT NULL, "
     "c_P0 FLOAT NOT NULL, c_P1 FLOAT NOT NULL, c_P2 FLOAT NOT NULL, c_alpha FLOAT NOT NULL, "
     "c_S FLOAT NOT NULL, c_r1max FLOAT NOT NULL, c_r2_ FLOAT NOT NULL, c_H1 FLOAT NOT NULL, "
     "c_H2 FLOAT NOT NULL, c_G FLOAT NOT NULL, c_G1 FLOAT NOT NULL, c_alpha1 FLOAT NOT NULL, "
     "c_h_ FLOAT NOT NULL, c_s_ FLOAT NOT NULL, c_i_ FLOAT NOT NULL, c_w_ FLOAT NOT NULL, "
     "c_F FLOAT NOT NULL, c_Z FLOAT NOT NULL, c_b_ FLOAT NOT NULL, c_N FLOAT NOT NULL, "
     "c_u_ FLOAT NOT NULL, c_Q FLOAT NOT NULL, c_delta FLOAT NOT NULL, c_Fe FLOAT NOT NULL, "
     "c_Fe_proof FLOAT NOT NULL, c_Fe_auto FLOAT NOT NULL, sebert FLOAT NOT NULL, cdate DATE NOT NULL, "
     "cby TEXT NOT NULL, mdate DATE NOT NULL, mby TEXT NOT NULL, type TEXT NOT NULL, mode TEXT NOT NULL, "
     "status TEXT NOT NULL, origin TEXT NOT NULL, descr TEXT NOT NULL, geloescht INTEGER NOT NULL"},
    {"projectile",
     "id INTEGER PRIMARY KEY AUTOINCREMENT, mname TEXT COLLATE NOCASE NOT NULL, "
     "pname TEXT COLLATE NOCASE NOT NULL, lotid TEXT COLLATE NOCASE NOT NULL, "
     "caliber TEXT COLLATE NOCASE NOT NULL, gdia FLOAT NOT NULL, glen FLOAT NOT NULL, "
     "gmass FLOAT NOT NULL, gpressure FLOAT NOT NULL, gfriction FLOAT NOT NULL, gtaildiaA FLOAT NOT NULL, "
     "gtaildiaB FLOAT NOT NULL, gtailh FLOAT NOT NULL, gtailtype INTEGER NOT NULL, "
     "gdepthmax FLOAT NOT NULL, g1bc FLOAT NOT NULL, g7bc FLOAT NOT NULL, "
     "gUBCS TEXT COLLATE NOCASE NOT NULL, cdate DATE NOT NULL, cby TEXT COLLATE NOCASE NOT NULL, "
     "mdate DATE NOT NULL, mby TEXT COLLATE NOCASE NOT NULL, type TEXT COLLATE NOCASE NOT NULL, "
     "mode TEXT COLLATE NOCASE NOT NULL, status TEXT COLLATE NOCASE NOT NULL, "
     "origin TEXT COLLATE NOCASE NOT NULL, descr TEXT COLLATE NOCASE NOT NULL, imageUuid TEXT NOT NULL, "
     "geloescht INTEGER NOT NULL"},
    {"propellant",
     "id INTEGER PRIMARY KEY AUTOINCREMENT, mname TEXT COLLATE NOCASE NOT NULL, "
     "pname TEXT COLLATE NOCASE NOT NULL, lotid TEXT COLLATE NOCASE NOT NULL, Br FLOAT NOT NULL, "
     "Bp FLOAT NOT NULL, Brp FLOAT NOT NULL, Ba FLOAT NOT NULL, Qex FLOAT NOT NULL, k FLOAT NOT NULL, "
     "eta FLOAT NOT NULL, a0 FLOAT NOT NULL, a1 FLOAT NOT NULL, z1 FLOAT NOT NULL, z2 FLOAT NOT NULL, "
     "pc FLOAT NOT NULL, pcd FLOAT NOT NULL, pt FLOAT NOT NULL, tcc FLOAT NOT NULL, tch FLOAT NOT NULL, "
     "Lp FLOAT NOT NULL, Lr FLOAT NOT NULL, L01 FLOAT NOT NULL, L02 FLOAT NOT NULL, L03 FLOAT NOT NULL, "
     "L04 FLOAT NOT NULL, L05 FLOAT NOT NULL, L06 FLOAT NOT NULL, L07 FLOAT NOT NULL, L08 FLOAT NOT NULL, "
     "L09 FLOAT NOT NULL, Qlty FLOAT NOT NULL, cdate DATE NOT NULL, cby TEXT COLLATE NOCASE NOT NULL, "
     "mdate DATE NOT NULL, mby TEXT COLLATE NOCASE NOT NULL, type TEXT COLLATE NOCASE NOT NULL, "
     "mode TEXT COLLATE NOCASE NOT NULL, status TEXT COLLATE NOCASE NOT NULL, "
     "origin TEXT COLLATE NOCASE NOT NULL, descr TEXT COLLATE NOCASE NOT NULL, imageUuid TEXT NOT NULL, "
     "imageDetailUuid TEXT NOT NULL, geloescht INTEGER NOT NULL"},
    {"gun",
     "id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL, data TEXT NOT NULL, cdate DATE NOT NULL, "
     "cby TEXT NOT NULL, mdate DATE NOT NULL, mby TEXT NOT NULL, type TEXT NOT NULL, mode TEXT NOT NULL, "
     "status TEXT NOT NULL, descr TEXT NOT NULL, geloescht INTEGER NOT NULL"},
    {"image",
     "id INTEGER PRIMARY KEY AUTOINCREMENT, uuid TEXT NOT NULL, name TEXT NOT NULL, descr TEXT NOT NULL, "
     "scale FLOAT NOT NULL, scaleunit TEXT NOT NULL, data TEXT NOT NULL, cdate DATE NOT NULL, "
     "cby TEXT NOT NULL, type TEXT NOT NULL, mode TEXT NOT NULL, status TEXT NOT NULL, "
     "geloescht INTEGER NOT NULL"},
}};

QString lastError(const QSqlQuery &q)
{
    return q.lastError().text();
}

bool exec(QSqlDatabase &db, const QString &sql, QString *error)
{
    QSqlQuery q(db);
    if (q.exec(sql)) return true;
    if (error) *error = lastError(q) + u" ("_s + sql.left(120) + u')';
    return false;
}

// Column name -> declared type, in schema order.
std::vector<std::pair<QString, QString>> tableColumns(const QSqlDatabase &db, const QString &schema, const QString &table)
{
    std::vector<std::pair<QString, QString>> out;
    QSqlQuery q(db);
    if (q.exec(u"PRAGMA %1.table_info(%2)"_s.arg(schema, table)))
        while (q.next()) out.emplace_back(q.value(1).toString(), q.value(2).toString().toUpper());
    return out;
}

// The columns are NOT NULL: a null string is stored as an empty one.
QVariant notNull(const QVariant &v)
{
    return v.typeId() == QMetaType::QString && v.toString().isNull() ? QVariant(u""_s) : v;
}

}  // namespace

Database::~Database()
{
    close();
}

QString Database::defaultOwnPath()
{
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).filePath(u"GRTReloaded.db"_s);
}

bool Database::open(const QString &originalPath, const QString &ownPath, QString *error)
{
    close();
    const QString own = ownPath.isEmpty() ? defaultOwnPath() : ownPath;
    QDir().mkpath(QFileInfo(own).absolutePath());
    m_connection = u"grt-"_s + QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_db = QSqlDatabase::addDatabase(u"QSQLITE"_s, m_connection);
    m_db.setDatabaseName(own);
    m_db.setConnectOptions(u"QSQLITE_OPEN_URI"_s);  // for attaching the original read-only
    if (!m_db.open()) {
        if (error) *error = m_db.lastError().text();
        close();
        return false;
    }
    if (!createOwnTables(error)) {
        close();
        return false;
    }
    m_ownPath = own;

    bool ok = true;
    if (!originalPath.isEmpty()) {
        QSqlQuery q(m_db);
        q.prepare(u"ATTACH DATABASE ? AS grt"_s);
        q.addBindValue(QUrl::fromLocalFile(QFileInfo(originalPath).absoluteFilePath()).toString() + u"?mode=ro"_s);
        if (!QFileInfo::exists(originalPath)) {
            if (error) *error = QStringLiteral("%1 does not exist").arg(originalPath);
            ok = false;
        } else if (!q.exec()) {
            if (error) *error = lastError(q);
            ok = false;
        } else {
            // an encrypted original database fails here with "file is not a database"
            QSqlQuery check(m_db);
            if (!check.exec(u"SELECT count(*) FROM grt.caliber"_s) || !check.next()) {
                if (error) *error = lastError(check);
                QSqlQuery(m_db).exec(u"DETACH DATABASE grt"_s);
                ok = false;
            } else {
                m_originalPath = originalPath;
            }
        }
    }
    if (!createViews(error)) {
        close();
        return false;
    }
    return ok;
}

bool Database::createOwnTables(QString *error)
{
    for (const TableSchema &t : kSchema) {
        const QString name = QLatin1String(t.name);
        if (!exec(m_db, u"CREATE TABLE IF NOT EXISTS main.%1 (%2)"_s.arg(name, QLatin1String(t.columns)), error)) return false;
        // own ids start at kOwnIdBase (AUTOINCREMENT continues after the highest sequence value)
        if (!exec(m_db, u"INSERT INTO main.sqlite_sequence (name, seq) SELECT '%1', %2 WHERE NOT EXISTS "
                         "(SELECT 1 FROM main.sqlite_sequence WHERE name = '%1')"_s.arg(name).arg(kOwnIdBase - 1),
                  error))
            return false;
    }
    return true;
}

// temp.<table> = own rows + original rows; unqualified names resolve to the temp schema first.
bool Database::createViews(QString *error)
{
    for (const TableSchema &t : kSchema) {
        const QString name = QLatin1String(t.name);
        const auto own = tableColumns(m_db, u"main"_s, name);
        QStringList cols, originalCols;
        const auto original = hasOriginal() ? tableColumns(m_db, u"grt"_s, name) : decltype(own){};
        for (const auto &[col, type] : own) {
            cols << col;
            const bool there = std::ranges::any_of(original, [&](const auto &c) { return c.first.compare(col, Qt::CaseInsensitive) == 0; });
            originalCols << (there ? col : u"NULL AS "_s + col);
        }
        QString sql = u"CREATE TEMP VIEW %1 AS SELECT %2 FROM main.%1"_s.arg(name, cols.join(u", "_s));
        if (!original.empty()) sql += u" UNION ALL SELECT %1 FROM grt.%2"_s.arg(originalCols.join(u", "_s), name);
        if (!exec(m_db, u"DROP VIEW IF EXISTS temp.%1"_s.arg(name), error) || !exec(m_db, sql, error)) return false;
    }
    return true;
}

void Database::close()
{
    if (m_connection.isEmpty()) return;
    if (m_db.isOpen()) m_db.close();
    m_db = {};
    QSqlDatabase::removeDatabase(m_connection);
    m_connection.clear();
    m_originalPath.clear();
    m_ownPath.clear();
}

int Database::count(Table t) const
{
    QSqlQuery q(m_db);
    if (q.exec(QStringLiteral("SELECT count(*) FROM %1 WHERE geloescht = 0").arg(QLatin1String(tableName(t)))) && q.next())
        return q.value(0).toInt();
    return 0;
}

int Database::ownCount(Table t) const
{
    QSqlQuery q(m_db);
    if (q.exec(QStringLiteral("SELECT count(*) FROM main.%1 WHERE geloescht = 0").arg(QLatin1String(tableName(t)))) && q.next())
        return q.value(0).toInt();
    return 0;
}

QStringList Database::columns(Table t) const
{
    QStringList out;
    for (const auto &[name, type] : tableColumns(m_db, u"main"_s, QLatin1String(tableName(t)))) out << name;
    return out;
}

std::optional<QVariantMap> Database::record(Table t, int id) const
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT * FROM %1 WHERE id = ?").arg(QLatin1String(tableName(t))));
    q.addBindValue(id);
    if (!q.exec() || !q.next()) return std::nullopt;
    const QSqlRecord r = q.record();
    QVariantMap out;
    for (int i = 0; i < r.count(); ++i) out.insert(r.fieldName(i), q.value(i));
    return out;
}

std::optional<int> Database::insert(Table t, const QVariantMap &values, QString *error)
{
    const QString name = QLatin1String(tableName(t));
    const QString today = QDate::currentDate().toString(Qt::ISODate);
    QStringList cols, marks;
    QVariantList binds;
    for (const auto &[col, type] : tableColumns(m_db, u"main"_s, name)) {
        if (col == u"id") continue;
        QVariant v;
        for (auto it = values.begin(); it != values.end(); ++it)
            if (it.key().compare(col, Qt::CaseInsensitive) == 0) v = notNull(it.value());
        if (!v.isValid()) {
            if (col == u"geloescht") v = 0;
            else if (type == u"DATE") v = today;
            else if (type == u"TEXT") v = u""_s;  // not a null string: that would be NULL
            else v = 0;
        }
        cols << col;
        marks << u"?"_s;
        binds << v;
    }
    QSqlQuery q(m_db);
    q.prepare(u"INSERT INTO main.%1 (%2) VALUES (%3)"_s.arg(name, cols.join(u", "_s), marks.join(u", "_s)));
    for (const QVariant &v : binds) q.addBindValue(v);
    if (!q.exec()) {
        if (error) *error = lastError(q);
        return std::nullopt;
    }
    return q.lastInsertId().toInt();
}

bool Database::update(Table t, int id, const QVariantMap &values, QString *error)
{
    if (!isOwnId(id)) {
        if (error) *error = u"records of the original database cannot be changed"_s;
        return false;
    }
    const QString name = QLatin1String(tableName(t));
    QStringList sets;
    QVariantList binds;
    for (const auto &[col, type] : tableColumns(m_db, u"main"_s, name)) {
        if (col == u"id") continue;
        for (auto it = values.begin(); it != values.end(); ++it)
            if (it.key().compare(col, Qt::CaseInsensitive) == 0) {
                sets << col + u" = ?"_s;
                binds << notNull(it.value());
            }
    }
    if (sets.isEmpty()) return true;
    QSqlQuery q(m_db);
    q.prepare(u"UPDATE main.%1 SET %2 WHERE id = ?"_s.arg(name, sets.join(u", "_s)));
    for (const QVariant &v : binds) q.addBindValue(v);
    q.addBindValue(id);
    if (!q.exec() || q.numRowsAffected() != 1) {
        if (error) *error = q.lastError().isValid() ? lastError(q) : u"no record with id %1"_s.arg(id);
        return false;
    }
    return true;
}

bool Database::remove(Table t, int id, QString *error)
{
    return update(t, id, {{u"geloescht"_s, 1}}, error);
}

}  // namespace grt
