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

#include "database.h"
#include "burnfactors.h"
#include "dates.h"
#include "log.h"

#include "projectdata.h"

#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStandardPaths>
#include <QUrl>
#include <QUuid>

#include <algorithm>
#include <array>
#include <cmath>

using namespace Qt::StringLiterals;

namespace csr
{

namespace
{

// The tables of the original program's database (same columns, plus the project's own ones of kAddedColumns), created in the own database.
struct TableSchema
{
    const char *name;
    const char *columns;
};
constexpr std::array<TableSchema, 5> kSchema{{
    {"caliber",
     "id INTEGER PRIMARY KEY AUTOINCREMENT, cipname TEXT NOT NULL, altname TEXT NOT NULL, "
     "standard TEXT NOT NULL, ciporigin TEXT NOT NULL, ciptype TEXT NOT NULL, primer TEXT NOT NULL DEFAULT '', cipdate DATE NOT NULL, "
     "ciprevdate DATE NOT NULL, cippdf TEXT NOT NULL, L0 FLOAT NOT NULL, L1 FLOAT NOT NULL, "
     "L2 FLOAT NOT NULL, L3 FLOAT NOT NULL, L4 FLOAT NOT NULL, L5 FLOAT NOT NULL, L6 FLOAT NOT NULL, "
     "R FLOAT NOT NULL, R1 FLOAT NOT NULL, R3 FLOAT NOT NULL, E FLOAT NOT NULL, E1 FLOAT NOT NULL, "
     "Emin FLOAT NOT NULL, delta TEXT NOT NULL, FG FLOAT NOT NULL, beta TEXT NOT NULL, P0 FLOAT NOT NULL, "
     "P1 FLOAT NOT NULL, P2 FLOAT NOT NULL, alpha TEXT NOT NULL, S FLOAT NOT NULL, r1min FLOAT NOT NULL, "
     "r2 FLOAT NOT NULL, H1 FLOAT NOT NULL, H2 FLOAT NOT NULL, G1 FLOAT NOT NULL, G2 FLOAT NOT NULL, "
     "f FLOAT NOT NULL, L3G FLOAT NOT NULL, Pmax FLOAT NOT NULL, PmaxSAAMI FLOAT NOT NULL DEFAULT 0, PK FLOAT NOT NULL, PE FLOAT NOT NULL, "
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
    {"projectile", "id INTEGER PRIMARY KEY AUTOINCREMENT, mname TEXT COLLATE NOCASE NOT NULL, "
                   "pname TEXT COLLATE NOCASE NOT NULL, lotid TEXT COLLATE NOCASE NOT NULL, "
                   "caliber TEXT COLLATE NOCASE NOT NULL, gdia FLOAT NOT NULL, glen FLOAT NOT NULL, gValid BOOLEAN NOT NULL DEFAULT 0, "
                   "gmass FLOAT NOT NULL, gpressure FLOAT NOT NULL, gfriction FLOAT NOT NULL, gtaildiaA FLOAT NOT NULL, "
                   "gtaildiaB FLOAT NOT NULL, gtailh FLOAT NOT NULL, gtailtype INTEGER NOT NULL, "
                   "gdepthmax FLOAT NOT NULL, g1bc FLOAT NOT NULL, g7bc FLOAT NOT NULL, "
                   "gUBCS TEXT COLLATE NOCASE NOT NULL, cdate DATE NOT NULL, cby TEXT COLLATE NOCASE NOT NULL, "
                   "mdate DATE NOT NULL, mby TEXT COLLATE NOCASE NOT NULL, type TEXT COLLATE NOCASE NOT NULL, "
                   "mode TEXT COLLATE NOCASE NOT NULL, status TEXT COLLATE NOCASE NOT NULL, "
                   "origin TEXT COLLATE NOCASE NOT NULL, descr TEXT COLLATE NOCASE NOT NULL, imageUuid TEXT NOT NULL, "
                   "geloescht INTEGER NOT NULL"},
    {"propellant", "id INTEGER PRIMARY KEY AUTOINCREMENT, mname TEXT COLLATE NOCASE NOT NULL, "
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
    {"gun", "id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL, data TEXT NOT NULL, cdate DATE NOT NULL, "
            "cby TEXT NOT NULL, mdate DATE NOT NULL, mby TEXT NOT NULL, type TEXT NOT NULL, mode TEXT NOT NULL, "
            "status TEXT NOT NULL, descr TEXT NOT NULL, geloescht INTEGER NOT NULL"},
    {"image", "id INTEGER PRIMARY KEY AUTOINCREMENT, uuid TEXT NOT NULL, name TEXT NOT NULL, descr TEXT NOT NULL, "
              "scale FLOAT NOT NULL, scaleunit TEXT NOT NULL, data TEXT NOT NULL, cdate DATE NOT NULL, "
              "cby TEXT NOT NULL, type TEXT NOT NULL, mode TEXT NOT NULL, status TEXT NOT NULL, "
              "geloescht INTEGER NOT NULL"},
}};

// Columns the project added to the original's tables: own databases of older versions get them (ALTER TABLE) when opened.
// The views show such a column right after `after` (an older own database has it at the end of its table).
struct AddedColumn
{
    const char *table;
    const char *name;
    const char *declaration;
    const char *after;
};
constexpr std::array<AddedColumn, 3> kAddedColumns{{
    {"caliber", "PmaxSAAMI", "FLOAT NOT NULL DEFAULT 0", "Pmax"},  // SAAMI maximum average pressure [bar]; Pmax is the C.I.P. one
    {"caliber", "primer", "TEXT NOT NULL DEFAULT ''",
     "ciptype"},  // usual primer type (primer.h), preset in a new load; empty: from the case
    {"projectile", "gValid", "BOOLEAN NOT NULL DEFAULT 0",
     "glen"},  // true: the record was validated by hand (measured on a bullet, checked against the original database), false: not
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

// The own table's columns in the order the program shows them: the project's added columns (kAddedColumns) next to the column they
// belong to, also in an own database of an older version that has them at the end.
std::vector<std::pair<QString, QString>> orderedColumns(const QSqlDatabase &db, const QString &table)
{
    auto cols = tableColumns(db, u"main"_s, table);
    for (const AddedColumn &c : kAddedColumns)
    {
        if (table != QLatin1String(c.table)) continue;
        const auto it = std::ranges::find_if(cols, [&](const auto &col) { return col.first == QLatin1String(c.name); });
        if (it == cols.end()) continue;
        const auto moved = *it;
        cols.erase(it);
        const auto anchor = std::ranges::find_if(cols, [&](const auto &col) { return col.first == QLatin1String(c.after); });
        cols.insert(anchor == cols.end() ? anchor : anchor + 1, moved);
    }
    return cols;
}

// The columns are NOT NULL: a null string is stored as an empty one. Values SQLite would store as NULL (no value,
// NaN) come back invalid: the caller puts the column's empty value there.
QVariant notNull(const QVariant &v)
{
    if (v.typeId() == QMetaType::QString) return v.toString().isNull() ? QVariant(u""_s) : v;
    if (!v.isValid() || v.isNull()) return {};
    if (v.typeId() == QMetaType::Double && !std::isfinite(v.toDouble())) return {};
    return v;
}

// The value a NOT NULL column gets when none is given.
QVariant emptyValue(const QString &column, const QString &type)
{
    if (column == u"geloescht") return 0;
    if (type == u"DATE") return QDate::currentDate().toString(Qt::ISODate);
    if (type == u"TEXT") return u""_s;  // not a null string: that would be NULL
    return 0;
}

// DATE columns hold "yyyy-MM-dd" (normalizedDate): a given value is brought into that form; false (with `error`) if
// it is not a date.
bool normalizeDateColumn(const QString &column, const QString &type, QVariant &v, QString *error)
{
    if (type != u"DATE" || !v.isValid()) return true;
    const auto date = normalizedDate(v);
    if (!date)
    {
        if (error) *error = u"\"%1\" is not a date: \"%2\" (use yyyy-MM-dd)"_s.arg(column, v.toString());
        return false;
    }
    v = date->isEmpty() ? QVariant(u""_s) : QVariant(*date);  // "" and not a null string: the columns are NOT NULL
    return true;
}

// The single number a "SELECT count(*) ..." returns (0 if it fails).
int countOf(const QSqlDatabase &db, const QString &sql)
{
    QSqlQuery q(db);
    return q.exec(sql) && q.next() ? q.value(0).toInt() : 0;
}

QString &projectLocation()
{
    static QString location = u":/data"_s;
    return location;
}

}  // namespace

void Database::setProjectDataLocation(const QString &location)
{
    projectLocation() = location;
}

QString Database::projectDataLocation()
{
    return projectLocation();
}

Database::~Database()
{
    close();
}

QString Database::defaultOwnPath()
{
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).filePath(u"ChamberSimReloaded.db"_s);
}

bool Database::open(const QString &originalPath, const QString &ownPath, QString *error)
{
    close();
    const QString own = ownPath.isEmpty() ? defaultOwnPath() : ownPath;
    QDir().mkpath(QFileInfo(own).absolutePath());
    m_connection = u"csr-"_s + QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_db = QSqlDatabase::addDatabase(u"QSQLITE"_s, m_connection);
    m_db.setDatabaseName(own);
    // URIs for attaching the original read-only; wait for a lock (a second instance, a backup) instead of failing
    m_db.setConnectOptions(u"QSQLITE_OPEN_URI;QSQLITE_BUSY_TIMEOUT=5000"_s);
    if (!m_db.open())
    {
        if (error) *error = m_db.lastError().text();
        LOG_Error() << "Cannot open the own database" << own << ":" << m_db.lastError().text();
        close();
        return false;
    }
    if (!createOwnTables(error))
    {
        LOG_Error() << "Cannot prepare the own database" << own << ":" << (error ? *error : QString());
        close();
        return false;
    }
    m_ownPath = own;
    LOG_Info() << "Own database:" << own;
    if (!loadProjectData(error))
    {
        LOG_Error() << "Cannot load the project data:" << (error ? *error : QString());
        close();
        return false;
    }

    bool ok = true;
    if (!originalPath.isEmpty())
    {
        QSqlQuery q(m_db);
        q.prepare(u"ATTACH DATABASE ? AS csr"_s);
        q.addBindValue(QUrl::fromLocalFile(QFileInfo(originalPath).absoluteFilePath()).toString(QUrl::FullyEncoded) + u"?mode=ro"_s);
        if (!QFileInfo::exists(originalPath))
        {
            if (error) *error = QStringLiteral("%1 does not exist").arg(originalPath);
            ok = false;
        }
        else if (!q.exec())
        {
            if (error) *error = lastError(q);
            ok = false;
        }
        else
        {
            // an encrypted original database fails here with "file is not a database"
            QSqlQuery check(m_db);
            if (!check.exec(u"SELECT count(*) FROM csr.caliber"_s) || !check.next())
            {
                if (error) *error = lastError(check);
                QSqlQuery(m_db).exec(u"DETACH DATABASE csr"_s);
                ok = false;
            }
            else
            {
                m_originalPath = originalPath;
                LOG_Info() << "Original database attached (read-only):" << originalPath;
            }
        }
    }
    if (!createViews(error))
    {
        close();
        return false;
    }
    return ok;
}

bool Database::createOwnTables(QString *error)
{
    // all tables or none: a migration that stops half-way would leave some records in the old id range
    if (!m_db.transaction())
    {
        if (error) *error = m_db.lastError().text();
        return false;
    }
    if (!createOwnTablesInTransaction(error))
    {
        m_db.rollback();
        return false;
    }
    if (!m_db.commit())
    {
        if (error) *error = m_db.lastError().text();
        m_db.rollback();
        return false;
    }
    return true;
}

bool Database::createOwnTablesInTransaction(QString *error)
{
    constexpr int kOldOwnIdBase = kProjectIdBase;  // before the project records came, own ids started there
    for (const TableSchema &t : kSchema)
    {
        const QString name = QLatin1String(t.name);
        if (!exec(m_db, u"CREATE TABLE IF NOT EXISTS main.%1 (%2)"_s.arg(name, QLatin1String(t.columns)), error)) return false;
        // the flag of the projectiles was called glenMeasured for a short while (unreleased): keep its values under the new name
        if (name == u"projectile")
        {
            const auto cols = tableColumns(m_db, u"main"_s, name);
            const auto has = [&](const QString &n) { return std::ranges::any_of(cols, [&](const auto &c) { return c.first == n; }); };
            if (has(u"glenMeasured"_s))
            {
                if (!has(u"gValid"_s))
                {
                    if (!exec(m_db, u"ALTER TABLE main.projectile RENAME COLUMN glenMeasured TO gValid"_s, error)) return false;
                }
                else if (!exec(m_db, u"UPDATE main.projectile SET gValid = 1 WHERE glenMeasured <> 0"_s, error) ||
                         !exec(m_db, u"ALTER TABLE main.projectile DROP COLUMN glenMeasured"_s, error))
                    return false;
            }
        }
        for (const AddedColumn &c : kAddedColumns)
        {
            if (name != QLatin1String(c.table)) continue;
            const auto have = tableColumns(m_db, u"main"_s, name);
            if (std::ranges::none_of(have,
                                     [&](const auto &col) { return col.first.compare(QLatin1String(c.name), Qt::CaseInsensitive) == 0; }) &&
                !exec(m_db, u"ALTER TABLE main.%1 ADD COLUMN %2 %3"_s.arg(name, QLatin1String(c.name), QLatin1String(c.declaration)),
                      error))
                return false;
        }
        // own records of older versions move up to the own range (load files keep values, not ids)
        if (!exec(m_db,
                  u"UPDATE main.%1 SET id = id + %2 WHERE id >= %3 AND id < %4"_s.arg(name)
                      .arg(kOwnIdBase - kOldOwnIdBase)
                      .arg(kOldOwnIdBase)
                      .arg(kOwnIdBase),
                  error))
            return false;
        // own ids start at kOwnIdBase (AUTOINCREMENT continues after the highest sequence value)
        if (!exec(m_db,
                  u"INSERT INTO main.sqlite_sequence (name, seq) SELECT '%1', 0 WHERE NOT EXISTS "
                  "(SELECT 1 FROM main.sqlite_sequence WHERE name = '%1')"_s.arg(name),
                  error) ||
            !exec(m_db,
                  u"UPDATE main.sqlite_sequence SET seq = max(%2, (SELECT ifnull(max(id), 0) FROM main.%1)) WHERE name = '%1'"_s.arg(name)
                      .arg(kOwnIdBase - 1),
                  error))
            return false;
    }
    return true;
}

// The project's records: an in-memory schema "proj" with the same tables, filled from the JSON files.
bool Database::loadProjectData(QString *error)
{
    m_projectErrors.clear();
    if (!exec(m_db, u"ATTACH DATABASE ':memory:' AS proj"_s, error)) return false;
    for (const TableSchema &t : kSchema)
    {
        const QString name = QLatin1String(t.name);
        if (!exec(m_db, u"CREATE TABLE proj.%1 (%2)"_s.arg(name, QLatin1String(t.columns)), error)) return false;
    }
    // one transaction for speed; the records live in memory only, a failed commit is an error of the program
    if (!m_db.transaction())
    {
        if (error) *error = m_db.lastError().text();
        return false;
    }
    for (const Table t : kRecordTables)
    {
        const projectdata::Loaded loaded = projectdata::load(projectLocation(), t, schema(t));
        m_projectErrors << loaded.errors;
        int id = kProjectIdBase;
        for (const QVariantMap &record : loaded.records)
        {
            QString err;
            if (!insertInto(u"proj"_s, t, record, id++, &err)) m_projectErrors << projectdata::folderName(t) + u": "_s + err;
        }
    }
    if (!m_db.commit())
    {
        if (error) *error = m_db.lastError().text();
        m_db.rollback();
        return false;
    }
    LOG_Info() << "Project data:" << projectCount(Table::Caliber) << "calibers," << projectCount(Table::Projectile) << "projectiles,"
               << projectCount(Table::Propellant) << "propellants," << m_projectErrors.size() << "problems";
    return true;
}

// temp.<table> = own rows + project rows + original rows; unqualified names resolve to the temp schema first.
bool Database::createViews(QString *error)
{
    for (const TableSchema &t : kSchema)
    {
        const QString name = QLatin1String(t.name);
        const auto own = orderedColumns(m_db, name);
        QStringList cols, originalCols;
        const auto original = hasOriginal() ? tableColumns(m_db, u"csr"_s, name) : decltype(own){};
        for (const auto &[col, type] : own)
        {
            cols << col;
            const bool there = std::ranges::any_of(original, [&](const auto &c) { return c.first.compare(col, Qt::CaseInsensitive) == 0; });
            originalCols << (there ? col : u"NULL AS "_s + col);
        }
        // "source": which part a row comes from (Database::kSource...)
        const QString list = cols.join(u", "_s);
        QString sql = u"CREATE TEMP VIEW %1 AS SELECT '%3' AS source, %2 FROM main.%1 UNION ALL SELECT '%4', %2 FROM proj.%1"_s.arg(
            name, list, QLatin1String(kSourceOwn), QLatin1String(kSourceProject));
        if (!original.empty())
            sql += u" UNION ALL SELECT '%1', %2 FROM csr.%3"_s.arg(QLatin1String(kSourceOriginal), originalCols.join(u", "_s), name);
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
    m_projectErrors.clear();
}

int Database::count(Table t) const
{
    return countOf(m_db, u"SELECT count(*) FROM %1 WHERE geloescht = 0"_s.arg(QLatin1String(tableName(t))));
}

int Database::projectCount(Table t) const
{
    return countOf(m_db, u"SELECT count(*) FROM proj.%1"_s.arg(QLatin1String(tableName(t))));
}

int Database::ownCount(Table t) const
{
    return countOf(m_db, u"SELECT count(*) FROM main.%1 WHERE geloescht = 0"_s.arg(QLatin1String(tableName(t))));
}

QStringList Database::columns(Table t) const
{
    QStringList out;
    for (const auto &[name, type] : orderedColumns(m_db, QLatin1String(tableName(t)))) out << name;
    return out;
}

std::vector<std::pair<QString, QString>> Database::schema(Table t) const
{
    return orderedColumns(m_db, QLatin1String(tableName(t)));
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

std::optional<QString> Database::caliberPrimer(const QString &cipname) const
{
    if (cipname.isEmpty() || !m_db.isOpen()) return std::nullopt;
    QSqlQuery q(m_db);
    q.prepare(u"SELECT primer FROM caliber WHERE cipname = ? AND primer IS NOT NULL AND primer <> '' AND geloescht = 0 "
              "ORDER BY CASE source WHEN '%1' THEN 0 WHEN '%2' THEN 1 ELSE 2 END LIMIT 1"_s.arg(QLatin1String(kSourceOwn),
                                                                                                QLatin1String(kSourceProject)));
    q.addBindValue(cipname);
    if (!q.exec() || !q.next()) return std::nullopt;
    return q.value(0).toString();
}

std::optional<Database::Image> Database::image(const QString &uuid) const
{
    if (uuid.isEmpty() || !m_db.isOpen()) return std::nullopt;
    QSqlQuery q(m_db);
    q.prepare(u"SELECT name, descr, scale, scaleunit, data, type FROM image WHERE uuid = ? AND geloescht = 0 LIMIT 1"_s);
    q.addBindValue(uuid);
    if (!q.exec() || !q.next()) return std::nullopt;
    Image img{.uuid = uuid,
              .name = q.value(0).toString(),
              .descr = q.value(1).toString(),
              .scale = q.value(2).toDouble(),
              .scaleUnit = q.value(3).toString(),
              .data = QByteArray::fromBase64(q.value(4).toString().toLatin1()),
              .type = q.value(5).toString()};
    if (img.data.isEmpty()) return std::nullopt;
    return img;
}

std::optional<QString> Database::addImage(const Image &image, QString *error)
{
    if (!m_db.isOpen() || image.data.isEmpty())
    {
        if (error) *error = u"no picture data"_s;
        return std::nullopt;
    }
    const QString uuid = image.uuid.isEmpty() ? QUuid::createUuid().toString(QUuid::Id128) : image.uuid;
    QSqlQuery exists(m_db);
    exists.prepare(u"SELECT 1 FROM main.image WHERE uuid = ? AND geloescht = 0"_s);
    exists.addBindValue(uuid);
    if (exists.exec() && exists.next()) return uuid;
    QSqlQuery q(m_db);
    q.prepare(u"INSERT INTO main.image (uuid, name, descr, scale, scaleunit, data, cdate, cby, type, mode, status, geloescht) "
              "VALUES (?, ?, ?, ?, ?, ?, ?, '', ?, '', '', 0)"_s);
    const auto text = [](const QString &t) { return t.isNull() ? QStringLiteral("") : t; };  // the columns are NOT NULL
    q.addBindValue(uuid);
    q.addBindValue(text(image.name));
    q.addBindValue(text(image.descr));
    q.addBindValue(image.scale);
    q.addBindValue(text(image.scaleUnit));
    q.addBindValue(QString::fromLatin1(image.data.toBase64()));
    q.addBindValue(QDateTime::currentDateTime().toString(u"yyyy-MM-dd HH:mm:ss"_s));
    q.addBindValue(text(image.type));
    if (!q.exec())
    {
        if (error) *error = lastError(q);
        LOG_Warn() << "Cannot store picture" << image.name << ":" << lastError(q);
        return std::nullopt;
    }
    LOG_Info() << "Picture stored:" << image.name << uuid << "(" << image.data.size() << "bytes )";
    return uuid;
}

void Database::keepImages(const QVariantMap &values)
{
    for (const QString &column : {u"imageUuid"_s, u"imageDetailUuid"_s})
    {
        const QString uuid = values.value(column).toString();
        if (uuid.isEmpty()) continue;
        if (const auto img = image(uuid)) (void)addImage(*img);  // a no-op when the own database has it
        else LOG_Warn() << "Picture" << uuid << "of a saved record was not found";
    }
}

std::optional<int> Database::insert(Table t, const QVariantMap &values, QString *error)
{
    const auto id = insertInto(u"main"_s, t, values, -1, error);
    if (id) keepImages(values);
    return id;
}

// `id` < 0: the next free one (AUTOINCREMENT)
std::optional<int> Database::insertInto(const QString &schemaName, Table t, const QVariantMap &given, int id, QString *error)
{
    QVariantMap values = given;
    if (t == Table::Propellant && burnFactors(values).Br > 0.0) fillBurnFactors(values);  // derived, see burnfactors.h
    const QString name = QLatin1String(tableName(t));
    QStringList cols, marks;
    QVariantList binds;
    for (const auto &[col, type] : tableColumns(m_db, u"main"_s, name))
    {
        QVariant v;
        if (col == u"id")
        {
            if (id < 0) continue;
            v = id;
        }
        for (auto it = values.begin(); col != u"id" && it != values.end(); ++it)
            if (it.key().compare(col, Qt::CaseInsensitive) == 0) v = notNull(it.value());
        if (!v.isValid()) v = emptyValue(col, type);
        if (!normalizeDateColumn(col, type, v, error))
        {
            LOG_Warn() << "Cannot insert into" << schemaName << name << ":" << (error ? *error : QString());
            return std::nullopt;
        }
        cols << col;
        marks << u"?"_s;
        binds << v;
    }
    QSqlQuery q(m_db);
    q.prepare(u"INSERT INTO %1.%2 (%3) VALUES (%4)"_s.arg(schemaName, name, cols.join(u", "_s), marks.join(u", "_s)));
    for (const QVariant &v : binds) q.addBindValue(v);
    if (!q.exec())
    {
        if (error) *error = lastError(q);
        LOG_Warn() << "Cannot insert into" << schemaName << name << ":" << lastError(q);
        return std::nullopt;
    }
    const int newId = id >= 0 ? id : q.lastInsertId().toInt();
    if (schemaName == u"main") LOG_Info() << "Record added:" << name << "id" << newId;
    return newId;
}

bool Database::update(Table t, int id, const QVariantMap &values, QString *error)
{
    if (!isOwnId(id))
    {
        if (error) *error = u"only own records can be changed (not those of the original database or the project)"_s;
        LOG_Warn() << "Refused to change" << tableName(t) << "id" << id << ": not an own record";
        return false;
    }
    const QString name = QLatin1String(tableName(t));
    QStringList sets;
    QVariantList binds;
    for (const auto &[col, type] : tableColumns(m_db, u"main"_s, name))
    {
        if (col == u"id") continue;
        for (auto it = values.begin(); it != values.end(); ++it)
            if (it.key().compare(col, Qt::CaseInsensitive) == 0)
            {
                QVariant v = notNull(it.value());
                if (!v.isValid()) v = emptyValue(col, type);
                if (!normalizeDateColumn(col, type, v, error)) return false;
                sets << col + u" = ?"_s;
                binds << v;
            }
    }
    if (sets.isEmpty()) return true;
    QSqlQuery q(m_db);
    q.prepare(u"UPDATE main.%1 SET %2 WHERE id = ?"_s.arg(name, sets.join(u", "_s)));
    for (const QVariant &v : binds) q.addBindValue(v);
    q.addBindValue(id);
    if (!q.exec() || q.numRowsAffected() != 1)
    {
        if (error) *error = q.lastError().isValid() ? lastError(q) : u"no record with id %1"_s.arg(id);
        LOG_Warn() << "Cannot update" << name << "id" << id << ":" << (error ? *error : QString());
        return false;
    }
    LOG_Info() << "Record changed:" << name << "id" << id << "fields" << sets.size();
    keepImages(values);
    if (t == Table::Propellant)  // Br, Bp, Brp follow the model values
        if (auto changed = record(t, id))
        {
            fillBurnFactors(*changed);
            QSqlQuery factors(m_db);
            factors.prepare(u"UPDATE main.propellant SET Br = ?, Bp = ?, Brp = ? WHERE id = ?"_s);
            for (const char *col : {"Br", "Bp", "Brp"}) factors.addBindValue(changed->value(QLatin1String(col)));
            factors.addBindValue(id);
            if (!factors.exec()) LOG_Warn() << "Cannot update the burn factors of propellant" << id << ":" << lastError(factors);
        }
    return true;
}

bool Database::remove(Table t, int id, QString *error)
{
    LOG_Info() << "Removing record:" << tableName(t) << "id" << id;
    return update(t, id, {{u"geloescht"_s, 1}}, error);
}

}  // namespace csr
