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

// Owns the connection to the (plain, unencrypted) SQLite database that has the
// same schema as the original GordonsReloadingTool.db.
class Database {
public:
    Database() = default;
    ~Database();
    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;

    // Opens the file; read-only access unless `readOnly` is false.
    [[nodiscard]] bool open(const QString &path, bool readOnly = true, QString *error = nullptr);
    void close();
    [[nodiscard]] bool isOpen() const { return m_db.isOpen(); }
    [[nodiscard]] const QString &path() const noexcept { return m_path; }
    [[nodiscard]] QSqlDatabase handle() const { return m_db; }

    // Number of non-deleted rows ("geloescht" = 0).
    [[nodiscard]] int count(Table t) const;

    // Column names of the table in schema order.
    [[nodiscard]] QStringList columns(Table t) const;

    // One row as column-name -> value, or nullopt if the id does not exist.
    [[nodiscard]] std::optional<QVariantMap> record(Table t, int id) const;

private:
    QSqlDatabase m_db;
    QString m_path;
    QString m_connection;
};

}  // namespace grt
