#pragma once

#include <algorithm>

#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QStringList>

// The most recently used load files (File menu), newest first, kept in the settings.
namespace recentfiles
{

inline constexpr int kMaxFiles = 12;

[[nodiscard]] inline QStringList load()
{
    return QSettings().value(QStringLiteral("files/recent")).toStringList();
}

inline void store(const QStringList &files)
{
    QSettings().setValue(QStringLiteral("files/recent"), files);
}

// Moves `path` to the front (case-sensitive on the absolute path) and keeps at most kMaxFiles entries.
[[nodiscard]] inline QStringList pushed(QStringList files, const QString &path)
{
    const QString abs = QFileInfo(path).absoluteFilePath();
    files.removeAll(abs);
    files.prepend(abs);
    while (files.size() > kMaxFiles) files.removeLast();
    return files;
}

}  // namespace recentfiles

// A file name typed without any extension gets ".csrload" (names with another extension are left alone). Only letters make an extension:
// the ".5x55SE" of a cartridge name like "6.5x55SE" is part of the name.
[[nodiscard]] inline QString withLoadExtension(const QString &path)
{
    if (path.isEmpty()) return path;
    const QString suffix = QFileInfo(path).suffix();
    if (!suffix.isEmpty() && std::ranges::all_of(suffix, [](QChar c) { return c.isLetter(); })) return path;
    return path + QStringLiteral(".csrload");
}
