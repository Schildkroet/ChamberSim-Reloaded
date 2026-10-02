#pragma once

#include <QDebug>
#include <QString>

#include <optional>

namespace grt::log
{

enum class Level
{
    Debug,
    Info,
    Warn,
    Error,
    Off
};  // Off: nothing is logged

struct Config
{
    Level level{Level::Warn};  // messages below this level are dropped
    bool console{true};        // write to stderr
    bool file{false};          // write to `path`
    QString path;              // empty: defaultPath()
};

[[nodiscard]] QString levelName(Level level);
[[nodiscard]] std::optional<Level> levelFromName(const QString &name);
[[nodiscard]] QString defaultPath();  // GRTReloaded.log in the application data folder

// The active configuration; setConfig() applies it at once (it does not save it).
[[nodiscard]] Config config();
void setConfig(const Config &config);
// Persistent in QSettings (log/level, log/console, log/file, log/path).
[[nodiscard]] Config storedConfig();
void storeConfig(const Config &config);

[[nodiscard]] QString activePath();  // the file the log goes to when `file` is on
[[nodiscard]] bool enabled(Level level);
void clearFile();  // deletes the log file (and its rotated copy)

// Also routes qDebug/qInfo/qWarning/qCritical of Qt and of this program through the log.
void installMessageHandler();

void write(Level level, const char *file, int line, const QString &message);

// LOG_Info() << "opened" << path;   one line per statement, formatted like qDebug (QString without quotes).
class Line
{
public:
    Line(Level level, const char *file, int line)
        : m_level(level)
        , m_file(file)
        , m_line(line)
        , m_active(enabled(level))
        , m_stream(&m_text)
    {
        m_stream.noquote();
    }
    Line(const Line &) = delete;
    Line &operator=(const Line &) = delete;
    ~Line()
    {
        if (m_active) write(m_level, m_file, m_line, m_text.trimmed());  // QDebug leaves a trailing space
    }
    template <typename T>
    Line &operator<<(const T &value)
    {
        if (m_active) m_stream << value;
        return *this;
    }

private:
    Level m_level;
    const char *m_file;
    int m_line;
    bool m_active;
    QString m_text;
    QDebug m_stream;
};

}  // namespace grt::log

#define LOG_Debug() ::grt::log::Line(::grt::log::Level::Debug, __FILE__, __LINE__)
#define LOG_Info() ::grt::log::Line(::grt::log::Level::Info, __FILE__, __LINE__)
#define LOG_Warn() ::grt::log::Line(::grt::log::Level::Warn, __FILE__, __LINE__)
#define LOG_Error() ::grt::log::Line(::grt::log::Level::Error, __FILE__, __LINE__)
