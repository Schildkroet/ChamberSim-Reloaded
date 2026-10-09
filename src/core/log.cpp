#include "log.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QSettings>
#include <QStandardPaths>

#include <array>
#include <atomic>
#include <cstdio>
#include <cstdlib>

using namespace Qt::StringLiterals;

namespace csr::log
{

namespace
{

constexpr qint64 kMaxFileSize = 1024 * 1024;  // then the file becomes <name>.1 (the previous .1 is dropped)

struct State
{
    QMutex mutex;
    Config config;
    std::atomic<Level> level{Level::Warn};  // read without the lock by enabled()
};

State &state()
{
    static State s;
    return s;
}

QString activePathLocked(const Config &c)
{
    return c.path.trimmed().isEmpty() ? defaultPath() : c.path.trimmed();
}

void rotate(const QString &path)
{
    if (QFileInfo(path).size() < kMaxFileSize) return;
    QFile::remove(path + u".1"_s);
    QFile::rename(path, path + u".1"_s);
}

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    const Level level = type == QtDebugMsg     ? Level::Debug
                        : type == QtInfoMsg    ? Level::Info
                        : type == QtWarningMsg ? Level::Warn
                                               : Level::Error;
    write(level, context.file, context.line, message);
    if (type == QtFatalMsg) std::abort();
}

}  // namespace

QString levelName(Level level)
{
    switch (level)
    {
    case Level::Debug: return u"Debug"_s;
    case Level::Info: return u"Info"_s;
    case Level::Warn: return u"Warning"_s;
    case Level::Error: return u"Error"_s;
    case Level::Off: return u"Off"_s;
    }
    return {};
}

std::optional<Level> levelFromName(const QString &name)
{
    const QString n = name.trimmed().toLower();
    for (const Level l : {Level::Debug, Level::Info, Level::Warn, Level::Error, Level::Off})
        if (n == levelName(l).toLower()) return l;
    if (n == u"warn") return Level::Warn;
    return std::nullopt;
}

QString defaultPath()
{
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).filePath(u"ChamberSimReloaded.log"_s);
}

Config config()
{
    State &s = state();
    QMutexLocker lock(&s.mutex);
    return s.config;
}

void setConfig(const Config &config)
{
    State &s = state();
    QMutexLocker lock(&s.mutex);
    s.config = config;
    s.level = config.level;
}

Config storedConfig()
{
    const QSettings settings;
    Config c;
    c.level = levelFromName(settings.value(u"log/level"_s).toString()).value_or(c.level);
    c.console = settings.value(u"log/console"_s, c.console).toBool();
    c.file = settings.value(u"log/file"_s, c.file).toBool();
    c.path = settings.value(u"log/path"_s).toString();
    return c;
}

void storeConfig(const Config &config)
{
    QSettings settings;
    settings.setValue(u"log/level"_s, levelName(config.level));
    settings.setValue(u"log/console"_s, config.console);
    settings.setValue(u"log/file"_s, config.file);
    settings.setValue(u"log/path"_s, config.path);
}

QString activePath()
{
    State &s = state();
    QMutexLocker lock(&s.mutex);
    return activePathLocked(s.config);
}

bool enabled(Level level)
{
    const Level min = state().level.load();
    return min != Level::Off && level >= min;
}

void clearFile()
{
    State &s = state();
    QMutexLocker lock(&s.mutex);
    const QString path = activePathLocked(s.config);
    QFile::remove(path);
    QFile::remove(path + u".1"_s);
}

void installMessageHandler()
{
    qInstallMessageHandler(messageHandler);
}

void write(Level level, const char *file, int line, const QString &message)
{
    if (!enabled(level)) return;
    static constexpr std::array<const char *, 4> tags{"DEBUG", "INFO ", "WARN ", "ERROR"};
    const QString where = file ? u" %1:%2"_s.arg(QFileInfo(QString::fromUtf8(file)).fileName()).arg(line) : QString();
    const QString text = QDateTime::currentDateTime().toString(u"yyyy-MM-dd HH:mm:ss.zzz"_s) + u" ["_s +
                         QLatin1String(tags[static_cast<std::size_t>(level)]) + u"]"_s + where + u"  "_s + message + u'\n';

    State &s = state();
    QMutexLocker lock(&s.mutex);
    if (s.config.console)
    {
        const QByteArray bytes = text.toLocal8Bit();
        std::fwrite(bytes.constData(), 1, static_cast<std::size_t>(bytes.size()), stderr);
    }
    if (s.config.file)
    {
        const QString path = activePathLocked(s.config);
        QDir().mkpath(QFileInfo(path).absolutePath());
        rotate(path);
        if (QFile f(path); f.open(QIODevice::Append | QIODevice::Text)) f.write(text.toUtf8());
    }
}

}  // namespace csr::log
