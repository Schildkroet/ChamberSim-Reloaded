/*
 * This file is part of ChamberSim Reloaded.
 *
 * Copyright (C) 2026 Schildkroet
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "chronograph.h"

#include "numeric.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QLocale>
#include <QStringList>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>

using namespace Qt::StringLiterals;

namespace csr
{

namespace
{

// Metres per second for the unit texts the chronographs print; 0 = unknown.
double velocityFactor(QString unit)
{
    unit = unit.trimmed().toLower();
    if (unit == u"ft/s" || unit == u"fps") return kMetersPerFoot;
    if (unit == u"m/s" || unit == u"mps") return 1.0;
    if (unit == u"km/h" || unit == u"kph") return 1.0 / 3.6;
    if (unit == u"mph") return 0.44704;
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
    return QCoreApplication::translate("csr::chronograph", text);
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

// ---- Garmin Xero ----------------------------------------------------------------------------------------------------------------------

// The Xero C1 Pro writes a FIT file (file type 54, "shot session"): a header, definition and data messages, a CRC. Only the messages
// 387 (chrono shot session, not needed) and 388 (chrono shot data: field 0 = speed in m/s * 1000, field 1 = shot number) matter. The
// layout of every message is announced by a definition message, so the file is read generically, without Garmin's SDK.
constexpr int kFitFileId = 0;
constexpr int kFitChronoShotData = 388;
constexpr int kFitShotSessionFile = 54;
constexpr qint64 kFitEpochOffset = 631065600;  // seconds from 1970-01-01 to the FIT epoch 1989-12-31

bool isFit(const QByteArray &d)
{
    return d.size() >= 14 && d.mid(8, 4) == ".FIT" && (static_cast<unsigned char>(d[0]) == 12 || static_cast<unsigned char>(d[0]) == 14);
}

struct FitField
{
    int number{};
    int size{};
};

struct FitDefinition
{
    int global{};
    bool bigEndian{};
    std::vector<FitField> fields;
    int developerBytes{};
};

// An unsigned integer of `size` bytes (1, 2 or 4); the other sizes are not used by the fields read here.
std::optional<std::uint32_t> fitUnsigned(const unsigned char *p, int size, bool bigEndian)
{
    if (size != 1 && size != 2 && size != 4) return std::nullopt;
    std::uint32_t v = 0;
    for (int i = 0; i < size; ++i) v |= static_cast<std::uint32_t>(p[bigEndian ? size - 1 - i : i]) << (8 * i);
    return v;
}

std::optional<ChronographFile> readGarminFit(const QByteArray &data, QString *error)
{
    const auto fail = [&](const QString &message) -> std::optional<ChronographFile>
    {
        if (error) *error = message;
        return std::nullopt;
    };
    const auto *bytes = reinterpret_cast<const unsigned char *>(data.constData());
    const qint64 headerSize = bytes[0];
    const std::optional<std::uint32_t> dataSize = fitUnsigned(bytes + 4, 4, false);
    if (!dataSize || headerSize + static_cast<qint64>(*dataSize) > data.size()) return fail(tr("The FIT file is truncated."));
    const qint64 end = headerSize + *dataSize;

    std::array<std::optional<FitDefinition>, 16> definitions;
    std::vector<double> speeds;
    std::vector<int> numbers;
    std::optional<int> fileType;
    std::optional<std::uint32_t> startTime;
    qint64 pos = headerSize;
    while (pos < end)
    {
        const int header = bytes[pos++];
        const bool compressed = (header & 0x80) != 0;
        if (!compressed && (header & 0x40))  // definition message
        {
            if (pos + 5 > end) return fail(tr("The FIT file is damaged."));
            FitDefinition def;
            def.bigEndian = bytes[pos + 1] != 0;
            def.global = static_cast<int>(*fitUnsigned(bytes + pos + 2, 2, def.bigEndian));
            const int count = bytes[pos + 4];
            pos += 5;
            if (pos + 3 * count > end) return fail(tr("The FIT file is damaged."));
            for (int i = 0; i < count; ++i, pos += 3) def.fields.push_back({.number = bytes[pos], .size = bytes[pos + 1]});
            if (header & 0x20)  // developer fields: only their sizes are needed
            {
                if (pos >= end) return fail(tr("The FIT file is damaged."));
                const int devCount = bytes[pos++];
                if (pos + 3 * devCount > end) return fail(tr("The FIT file is damaged."));
                for (int i = 0; i < devCount; ++i, pos += 3) def.developerBytes += bytes[pos + 1];
            }
            definitions[header & 0x0f] = std::move(def);
            continue;
        }
        const auto &def = definitions[compressed ? (header >> 5) & 0x03 : header & 0x0f];
        if (!def) return fail(tr("The FIT file is damaged."));
        qint64 size = def->developerBytes;
        for (const FitField &f : def->fields) size += f.size;
        if (pos + size > end) return fail(tr("The FIT file is damaged."));
        const unsigned char *p = bytes + pos;
        if (def->global == kFitFileId || def->global == kFitChronoShotData)
        {
            double speed = -1.0;
            int number = -1;
            for (const FitField &f : def->fields)
            {
                const auto value = fitUnsigned(p, f.size, def->bigEndian);
                if (def->global == kFitFileId && f.number == 0 && value) fileType = static_cast<int>(*value);
                if (def->global == kFitChronoShotData && value)
                {
                    if (f.number == 0 && f.size == 4 && *value != 0xFFFFFFFFu) speed = *value / 1000.0;
                    if (f.number == 1 && f.size == 2 && *value != 0xFFFFu) number = static_cast<int>(*value);
                    if (f.number == 253 && f.size == 4 && !startTime && *value != 0xFFFFFFFFu) startTime = *value;
                }
                p += f.size;
            }
            if (def->global == kFitChronoShotData && speed > 0.0)  // a 0 is a missed shot
            {
                speeds.push_back(speed);
                numbers.push_back(number);
            }
        }
        pos += size;
    }
    if (fileType && *fileType != kFitShotSessionFile) return fail(tr("This FIT file is not a shot session of a chronograph."));
    if (speeds.empty()) return fail(tr("The file contains no shots."));

    // in the order of the shot numbers (the file normally has them in order already)
    std::vector<std::size_t> order(speeds.size());
    for (std::size_t i = 0; i < order.size(); ++i) order[i] = i;
    std::ranges::stable_sort(order, [&](std::size_t a, std::size_t b) { return numbers[a] < numbers[b]; });
    ChronographSeries series;
    series.name = tr("Series");
    if (startTime)
        series.name = u"Xero "_s + QLocale().toString(QDateTime::fromSecsSinceEpoch(kFitEpochOffset + *startTime), QLocale::ShortFormat);
    for (std::size_t i : order) series.velocities_ms.push_back(speeds[i]);
    return ChronographFile{.format = ChronographFormat::GarminXero, .series = {std::move(series)}};
}

// A CSV line split at the commas outside double quotes (the app quotes numbers that have a decimal comma: "696,7"); quotes removed.
QStringList splitQuoted(const QString &line)
{
    QStringList fields;
    QString current;
    bool quoted = false;
    for (const QChar c : line)
    {
        if (c == u'"') quoted = !quoted;
        else if (c == u',' && !quoted)
        {
            fields << current.trimmed();
            current.clear();
        }
        else current += c;
    }
    fields << current.trimmed();
    return fields;
}

// The velocity unit in the speed column of the Xero CSV header ("SPEED (MPS)", localized: "Tempo (MPS)", "TEMPO (MPS)"); 0 if the
// line is not that header.
double xeroHeaderFactor(const QStringList &f)
{
    if (f.size() < 2 || f[0] != u"#") return 0.0;
    const qsizetype open = f[1].indexOf(u'(');
    const qsizetype close = f[1].indexOf(u')');
    return open >= 0 && close > open ? velocityFactor(f[1].mid(open + 1, close - open - 1)) : 0.0;
}

// The Garmin Xero app's CSV, in any language of the app: a title line, then "#,SPEED (MPS),Δ AVG (MPS),KE (J),..." and one line per
// shot (number first, speed second) up to a line that starts with "-"; the unit is the one in the header (MPS, FPS, KPH, MPH).
std::optional<ChronographFile> readGarminCsv(const QStringList &lines, QString *error)
{
    ChronographSeries series;
    double factor = 0.0;
    for (const QString &line : lines)
    {
        const QStringList f = splitQuoted(line);
        if (factor <= 0.0)
        {
            factor = xeroHeaderFactor(f);
            const QString title = line.trimmed();
            if (factor <= 0.0 && series.name.isEmpty() && !title.isEmpty()) series.name = QString(title).remove(u'"');
            continue;
        }
        if (f[0].startsWith(u'-')) break;  // the end of the shot table
        bool numberOk = false;
        f[0].toInt(&numberOk);
        const auto v = f.size() > 1 ? number(f[1]) : std::nullopt;
        if (numberOk && v && *v > 0.0) series.velocities_ms.push_back(*v * factor);
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
    return ChronographFile{.format = ChronographFormat::GarminXero, .series = {std::move(series)}};
}

}  // namespace

QString formatName(ChronographFormat f)
{
    switch (f)
    {
    case ChronographFormat::MagnetoSpeed: return u"MagnetoSpeed"_s;
    case ChronographFormat::Labradar: return u"Labradar"_s;
    case ChronographFormat::GarminXero: return u"Garmin Xero"_s;
    default: return {};
    }
}

std::optional<ChronographFile> readChronograph(const QByteArray &data, QString *error)
{
    if (isFit(data)) return readGarminFit(data, error);
    QByteArray bytes = data;
    bytes.remove(0, bytes.startsWith("\xEF\xBB\xBF") ? 3 : 0);
    bytes.replace('\0', "");  // the Labradar pads its lines with NUL bytes
    const QStringList lines = QString::fromUtf8(bytes).remove(u'\uFEFF').split(u'\n');
    const bool xero = std::ranges::any_of(lines, [](const QString &l) { return xeroHeaderFactor(splitQuoted(l)) > 0.0; });
    if (xero) return readGarminCsv(lines, error);
    const bool labradar = std::ranges::any_of(lines, [](const QString &l) { return l.trimmed().startsWith(u"Shot ID;"); });
    if (labradar) return readLabradar(lines, error);
    const bool magneto = std::ranges::any_of(lines, [](const QString &l)
                                             { return l.contains(u"Shots:", Qt::CaseInsensitive) && l.trimmed().startsWith(u"Series"); });
    if (magneto) return readMagnetoSpeed(lines, error);
    if (error) *error = tr("Not a Garmin Xero, MagnetoSpeed or Labradar file.");
    return std::nullopt;
}

}  // namespace csr
