#pragma once

#include <QByteArray>
#include <QSqlDatabase>
#include <QString>
#include <QStringList>
#include <QVariantMap>

#include <optional>
#include <utility>
#include <vector>

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

// The records the program works with, seen as ONE database made of three parts:
//  * the original program's database (a decrypted, plain copy of GordonsReloadingTool.db), attached read-only;
//  * the project's records (data/*/*.json in the repository, built into the program), loaded read-only into memory;
//  * the user's own database (same tables, created on first use) that takes every new record.
// Each table name ("caliber", ...) is a view over all three, so all queries and models see a single table. The id
// ranges never collide: original below kProjectIdBase, project from kProjectIdBase, own records from kOwnIdBase.
class Database {
public:
    static constexpr int kProjectIdBase = 1'000'000;
    // Every row of the combined tables has a column "source" with one of these values (not stored in a file).
    static constexpr const char *kSourceOwn = "own";
    static constexpr const char *kSourceProject = "project";
    static constexpr const char *kSourceOriginal = "original";
    static constexpr int kOwnIdBase = 10'000'000;

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
    // Where the project's records are read from (default ":/data", the files built into the program); a directory
    // works as well. Takes effect with the next open().
    static void setProjectDataLocation(const QString &location);
    [[nodiscard]] static QString projectDataLocation();
    // Number of project records and the problems of files that could not be used (one line each).
    [[nodiscard]] int projectCount(Table t) const;
    [[nodiscard]] const QStringList &projectErrors() const noexcept { return m_projectErrors; }

    // Number of non-deleted rows ("geloescht" = 0) of both databases.
    [[nodiscard]] int count(Table t) const;
    // ... of the own database only.
    [[nodiscard]] int ownCount(Table t) const;

    // Column names of the table in schema order.
    [[nodiscard]] QStringList columns(Table t) const;
    // Column names with their SQL types (TEXT, FLOAT, INTEGER, DATE) in schema order.
    [[nodiscard]] std::vector<std::pair<QString, QString>> schema(Table t) const;

    // One row as column-name -> value (with "source"), or nullopt if the id does not exist.
    [[nodiscard]] std::optional<QVariantMap> record(Table t, int id) const;

    // A picture of the image table (propellant photos): the encoded file (JPEG/PNG) and its scale.
    struct Image {
        QString name;
        QString descr;
        double scale{};       // as stored, e.g. 29.5 with the unit "px/mm"
        QString scaleUnit;
        QByteArray data;      // decoded (the database stores it as base64 text)
    };
    // The image with this uuid (propellant.imageUuid / imageDetailUuid), or nullopt if there is none.
    [[nodiscard]] std::optional<Image> image(const QString &uuid) const;

    [[nodiscard]] static bool isOwnId(int id) noexcept { return id >= kOwnIdBase; }
    [[nodiscard]] static bool isProjectId(int id) noexcept { return id >= kProjectIdBase && id < kOwnIdBase; }

    // Adds a record to the own database; columns that are not given get empty values (0, "", today for dates).
    // Returns the new id.
    [[nodiscard]] std::optional<int> insert(Table t, const QVariantMap &values, QString *error = nullptr);
    // Changes a record of the own database (records of the original cannot be changed).
    [[nodiscard]] bool update(Table t, int id, const QVariantMap &values, QString *error = nullptr);
    // Marks a record of the own database as deleted, like the original program does ("geloescht" = 1).
    [[nodiscard]] bool remove(Table t, int id, QString *error = nullptr);

private:
    [[nodiscard]] bool createOwnTables(QString *error);
    [[nodiscard]] bool createOwnTablesInTransaction(QString *error);
    [[nodiscard]] bool loadProjectData(QString *error);
    [[nodiscard]] bool createViews(QString *error);
    [[nodiscard]] std::optional<int> insertInto(const QString &schema, Table t, const QVariantMap &values, int id, QString *error);

    QSqlDatabase m_db;
    QString m_originalPath;
    QString m_ownPath;
    QString m_connection;
    QStringList m_projectErrors;
};

}  // namespace grt
