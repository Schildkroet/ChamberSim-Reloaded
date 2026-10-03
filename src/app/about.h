#pragma once

#include <QString>

#include <optional>

class QWidget;

namespace about
{

// The records of one part of the database.
struct Counts
{
    int calibers{0};
    int projectiles{0};
    int propellants{0};
};

// The three parts of the database (Database: project records, own records, the original program's database).
struct DatabaseInfo
{
    Counts project;
    Counts own;
    Counts original;
    QString originalPath;  // empty: no original database chosen
};

// The text of the About box as rich text (version, purpose, features, accuracy, safety notice, versions of the
// libraries in use, and the parts of the database if one is open).
[[nodiscard]] QString html(const std::optional<DatabaseInfo> &database = std::nullopt);

void show(QWidget *parent, const std::optional<DatabaseInfo> &database = std::nullopt);

}  // namespace about
