#pragma once

#include <QSettings>
#include <QString>
#include <QStringList>

// The loads that were open when the program was closed, and the option to open them again at the next start (Settings > Start-up).
namespace session
{

[[nodiscard]] inline bool reopen()
{
    return QSettings().value(QStringLiteral("session/reopen"), false).toBool();
}

inline void setReopen(bool on)
{
    QSettings().setValue(QStringLiteral("session/reopen"), on);
}

[[nodiscard]] inline QStringList loads()
{
    return QSettings().value(QStringLiteral("session/loads")).toStringList();
}

// The files of the open loads in the order of their tabs, and the one that was shown (an index into `files`, -1: none of them).
[[nodiscard]] inline int current()
{
    return QSettings().value(QStringLiteral("session/current"), -1).toInt();
}

inline void store(const QStringList &files, int current)
{
    QSettings s;
    s.setValue(QStringLiteral("session/loads"), files);
    s.setValue(QStringLiteral("session/current"), current);
}

}  // namespace session
