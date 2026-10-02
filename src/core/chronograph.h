#pragma once

#include <QByteArray>
#include <QString>

#include <optional>
#include <vector>

namespace grt
{

enum class ChronographFormat
{
    Unknown,
    MagnetoSpeed,
    Labradar
};

// One series of a chronograph file: the velocities in m/s in shot order (missed shots are left out).
struct ChronographSeries
{
    QString name;
    std::vector<double> velocities_ms;
};

struct ChronographFile
{
    ChronographFormat format{ChronographFormat::Unknown};
    std::vector<ChronographSeries> series;
};

[[nodiscard]] QString formatName(ChronographFormat f);

// Reads a chronograph export. Recognised: MagnetoSpeed CSV ("Series, n, Shots:, count" blocks with one
// "series, shot, speed, unit" line per shot; one file can hold several series) and the Labradar "Report.csv"
// ("Series No", "Units velocity" and a "Shot ID;V0;..." table, V0 = muzzle velocity). nullopt with `error` set if
// the text is none of them or has no shots.
[[nodiscard]] std::optional<ChronographFile> readChronograph(const QByteArray &data, QString *error = nullptr);

}  // namespace grt
