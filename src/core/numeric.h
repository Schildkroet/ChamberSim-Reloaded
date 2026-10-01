#pragma once

#include <QLocale>
#include <QString>

#include <cmath>
#include <optional>

namespace grt {

// A finite number from text, or nullopt. QString::toDouble also accepts "nan" and "inf": those are rejected,
// they would pass every "> 0" check of the model and end up in the results.
[[nodiscard]] inline std::optional<double> parseFinite(const QString &text)
{
    bool ok = false;
    const double v = text.toDouble(&ok);
    return ok && std::isfinite(v) ? std::optional(v) : std::nullopt;
}

// The same for text typed by the user: the user's locale first, then the C locale ("1.5" also works in German).
[[nodiscard]] inline std::optional<double> parseFiniteLocale(const QString &text)
{
    const QString t = text.trimmed();
    bool ok = false;
    double v = QLocale().toDouble(t, &ok);
    if (!ok) v = QLocale::c().toDouble(t, &ok);
    return ok && std::isfinite(v) ? std::optional(v) : std::nullopt;
}

// A double as int within [lo, hi]: a plain static_cast is undefined for NaN and values out of the int range.
[[nodiscard]] constexpr int toIntClamped(double v, int lo, int hi) noexcept
{
    if (!(v > lo)) return lo;  // also NaN
    if (v >= hi) return hi;
    return static_cast<int>(v);
}

}  // namespace grt
