#pragma once

#include <QByteArray>
#include <QSettings>
#include <QSplitter>
#include <QString>

// Remembers the sizes of a splitter across sessions (saved whenever a handle is moved).
inline void rememberSplitter(QSplitter *splitter, const QString &key)
{
    const QString path = QStringLiteral("layout/") + key;
    splitter->restoreState(QSettings().value(path).toByteArray());
    QObject::connect(splitter, &QSplitter::splitterMoved, splitter, [splitter, path] { QSettings().setValue(path, splitter->saveState()); });
}
