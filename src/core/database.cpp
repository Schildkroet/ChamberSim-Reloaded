#include "database.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QUuid>

using namespace Qt::StringLiterals;

namespace grt {

Database::~Database()
{
    close();
}

bool Database::open(const QString &path, bool readOnly, QString *error)
{
    close();
    m_connection = u"grt-"_s + QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_db = QSqlDatabase::addDatabase(u"QSQLITE"_s, m_connection);
    m_db.setDatabaseName(path);
    if (readOnly) m_db.setConnectOptions(u"QSQLITE_OPEN_READONLY"_s);
    if (!m_db.open()) {
        if (error) *error = m_db.lastError().text();
        m_db = {};
        QSqlDatabase::removeDatabase(m_connection);
        m_connection.clear();
        return false;
    }
    // an encrypted original database would fail here with "file is not a database"
    QSqlQuery q(m_db);
    if (!q.exec(u"SELECT count(*) FROM sqlite_master"_s) || !q.next()) {
        if (error) *error = q.lastError().text();
        close();
        return false;
    }
    m_path = path;
    return true;
}

void Database::close()
{
    if (m_connection.isEmpty()) return;
    if (m_db.isOpen()) m_db.close();
    m_db = {};
    QSqlDatabase::removeDatabase(m_connection);
    m_connection.clear();
    m_path.clear();
}

int Database::count(Table t) const
{
    QSqlQuery q(m_db);
    if (q.exec(QStringLiteral("SELECT count(*) FROM %1 WHERE geloescht = 0").arg(QLatin1String(tableName(t))))
        && q.next())
        return q.value(0).toInt();
    return 0;
}

QStringList Database::columns(Table t) const
{
    QSqlQuery q(m_db);
    QStringList out;
    if (q.exec(QStringLiteral("PRAGMA table_info(%1)").arg(QLatin1String(tableName(t)))))
        while (q.next()) out << q.value(1).toString();
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

}  // namespace grt
