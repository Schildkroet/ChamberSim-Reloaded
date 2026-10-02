#include "chronograph.h"

#include <QCoreApplication>
#include <QStringList>

#include <algorithm>

using namespace Qt::StringLiterals;

namespace grt
{

namespace
{

// Metres per second for the unit texts the chronographs print; 0 = unknown.
double velocityFactor(QString unit)
{
    unit = unit.trimmed().toLower();
    if (unit == u"ft/s" || unit == u"fps") return 0.3048;
    if (unit == u"m/s" || unit == u"mps") return 1.0;
    if (unit == u"km/h" || unit == u"kph") return 1.0 / 3.6;
    return 0.0;
}

// A decimal number with '.' or ',' as separator.
std::optional<double> number(QString text)
{
    text = text.trimmed();
    if (text.isEmpty()) return std::nullopt;
    text.replace(u',', u'.');
    bool ok = false;
    const double v = text.toDouble(&ok);
    if (!ok) return std::nullopt;
    return v;
}

QStringList splitLine(const QString &line, QChar separator)
{
    QStringList fields = line.split(separator);
    for (QString &f : fields) f = f.trimmed();
    return fields;
}

QString tr(const char *text)
{
    return QCoreApplication::translate("grt::chronograph", text);
}

std::optional<ChronographFile> readMagnetoSpeed(const QStringList &lines, QString *error)
{
    ChronographFile file{.format = ChronographFormat::MagnetoSpeed, .series = {}};
    for (const QString &line : lines)
    {
        const QStringList f = splitLine(line, u',');
        if (f.size() >= 4 && f[0].compare(u"Series", Qt::CaseInsensitive) == 0 && f[2].compare(u"Shots:", Qt::CaseInsensitive) == 0)
        {
            file.series.push_back({.name = tr("Series %1").arg(f[1]), .velocities_ms = {}});
            continue;
        }
        if (f.size() < 4 || file.series.empty()) continue;  // statistics, headers, the "----" end mark
        bool seriesOk = false, shotOk = false;
        f[0].toInt(&seriesOk);
        f[1].toInt(&shotOk);
        const auto v = number(f[2]);
        const double factor = velocityFactor(f[3]);
        if (!seriesOk || !shotOk || !v || factor <= 0.0) continue;
        if (*v > 0.0) file.series.back().velocities_ms.push_back(*v * factor);
    }
    std::erase_if(file.series, [](const ChronographSeries &s) { return s.velocities_ms.empty(); });
    if (file.series.empty())
    {
        if (error) *error = tr("The file contains no shots.");
        return std::nullopt;
    }
    return file;
}

std::optional<ChronographFile> readLabradar(const QStringList &lines, QString *error)
{
    ChronographFile file{.format = ChronographFormat::Labradar, .series = {}};
    ChronographSeries series;
    double factor = 0.0;
    int v0 = -1;
    for (const QString &line : lines)
    {
        const QStringList f = splitLine(line, u';');
        if (f.isEmpty()) continue;
        if (v0 < 0)
        {
            if (f[0] == u"Series No" && f.size() > 1) series.name = u"SR"_s + f[1];
            else if (f[0] == u"Units velocity" && f.size() > 1) factor = velocityFactor(f[1]);
            else if (f[0] == u"Shot ID") v0 = static_cast<int>(f.indexOf(u"V0"_s));
            continue;
        }
        bool idOk = false;
        f[0].toInt(&idOk);
        if (!idOk || v0 >= f.size()) continue;
        if (const auto v = number(f[v0]); v && *v > 0.0) series.velocities_ms.push_back(*v * factor);
    }
    if (factor <= 0.0)
    {
        if (error) *error = tr("The velocity unit of the file is unknown.");
        return std::nullopt;
    }
    if (series.velocities_ms.empty())
    {
        if (error) *error = tr("The file contains no shots.");
        return std::nullopt;
    }
    if (series.name.isEmpty()) series.name = tr("Series");
    file.series.push_back(std::move(series));
    return file;
}

}  // namespace

QString formatName(ChronographFormat f)
{
    switch (f)
    {
    case ChronographFormat::MagnetoSpeed: return u"MagnetoSpeed"_s;
    case ChronographFormat::Labradar: return u"Labradar"_s;
    default: return {};
    }
}

std::optional<ChronographFile> readChronograph(const QByteArray &data, QString *error)
{
    QByteArray bytes = data;
    bytes.remove(0, bytes.startsWith("\xEF\xBB\xBF") ? 3 : 0);
    bytes.replace('\0', "");  // the Labradar pads its lines with NUL bytes
    const QStringList lines = QString::fromUtf8(bytes).split(u'\n');
    const bool labradar = std::ranges::any_of(lines, [](const QString &l) { return l.trimmed().startsWith(u"Shot ID;"); });
    if (labradar) return readLabradar(lines, error);
    const bool magneto = std::ranges::any_of(lines, [](const QString &l)
                                             { return l.contains(u"Shots:", Qt::CaseInsensitive) && l.trimmed().startsWith(u"Series"); });
    if (magneto) return readMagnetoSpeed(lines, error);
    if (error) *error = tr("Not a MagnetoSpeed or Labradar file.");
    return std::nullopt;
}

}  // namespace grt
