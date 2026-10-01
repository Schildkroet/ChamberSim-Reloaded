#pragma once

#include <QSqlDatabase>
#include <QString>
#include <QStringList>
#include <QVariantMap>

#include <optional>

namespace grt {

enum class Table { Caliber, Projectile, Propellant };

[[nodiscard]] constexpr const char *tableName(Table t) noexcept
{
    switch (t) {
    case Table::Caliber: return "caliber";
    case Table::Projectile: return "projectile";
    case Table::Propellant: return "propellant";
    }
    return "";
}

// The records the program works with, seen as ONE database made of two files:
//  * the original program's database (a decrypted, plain copy of GordonsReloadingTool.db), attached read-only;
//  * an own database (same tables, created on first use) that takes every new record.
// Each table name ("caliber", ...) is a view over both, so all queries and models see a single table. Records of
// the own database have ids from kOwnIdBase on and never collide with the original's.
class Database {
public:
    static constexpr int kOwnIdBase = 1'000'000;

    Database() = default;
    ~Database();
    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;

    // Opens the own database at `ownPath` (created if missing; empty = defaultOwnPath()) and attaches the original
    // at `originalPath` read-only (empty = none). Returns false with `error` if either fails; when only the original
    // cannot be used, the own database stays open on its own.
    [[nodiscard]] bool open(const QString &originalPath, const QString &ownPath = {}, QString *error = nullptr);
    void close();
    [[nodiscard]] bool isOpen() const { return m_db.isOpen(); }
    [[nodiscard]] bool hasOriginal() const noexcept { return !m_originalPath.isEmpty(); }
    // The original database file (empty if none is attached).
    [[nodiscard]] const QString &path() const noexcept { return m_originalPath; }
    [[nodiscard]] const QString &ownPath() const noexcept { return m_ownPath; }
    [[nodiscard]] QSqlDatabase handle() const { return m_db; }

    // Where the own database lives unless told otherwise (the application data folder).
    [[nodiscard]] static QString defaultOwnPath();

    // Number of non-deleted rows ("geloescht" = 0) of both databases.
    [[nodiscard]] int count(Table t) const;
    // ... of the own database only.
    [[nodiscard]] int ownCount(Table t) const;

    // Column names of the table in schema order.
    [[nodiscard]] QStringList columns(Table t) const;

    // One row as column-name -> value, or nullopt if the id does not exist.
    [[nodiscard]] std::optional<QVariantMap> record(Table t, int id) const;

    [[nodiscard]] static bool isOwnId(int id) noexcept { return id >= kOwnIdBase; }

    // Adds a record to the own database; columns that are not given get empty values (0, "", today for dates).
    // Returns the new id.
    [[nodiscard]] std::optional<int> insert(Table t, const QVariantMap &values, QString *error = nullptr);
    // Changes a record of the own database (records of the original cannot be changed).
    [[nodiscard]] bool update(Table t, int id, const QVariantMap &values, QString *error = nullptr);
    // Marks a record of the own database as deleted, like the original program does ("geloescht" = 1).
    [[nodiscard]] bool remove(Table t, int id, QString *error = nullptr);

private:
    [[nodiscard]] bool createOwnTables(QString *error);
    [[nodiscard]] bool createViews(QString *error);

    QSqlDatabase m_db;
    QString m_originalPath;
    QString m_ownPath;
    QString m_connection;
};

}  // namespace grt
