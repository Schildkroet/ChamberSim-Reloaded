#include "recordfields.h"

#include "unitprefs.h"

#include <QLocale>

#include <algorithm>
#include <cmath>

namespace recordfields
{

const csr::units::Unit *unitOf(const csr::dbfields::Field &f)
{
    return f.quantity == csr::units::Quantity::None ? nullptr : &UnitPreferences::instance().unitFor(f.quantity);
}

QString shownNumber(const csr::dbfields::Field &f, double stored)
{
    QLocale locale;
    locale.setNumberOptions(QLocale::OmitGroupSeparator);
    const auto *u = unitOf(f);
    const double v = u ? csr::units::toDisplay(stored * f.toBase, *u) : stored;
    for (int d = u ? u->decimals : 0; d < 8; ++d)
    {
        const double scale = std::pow(10.0, d);
        if (std::abs(std::round(v * scale) / scale - v) <= 1e-9 * std::max(1.0, std::abs(v))) return locale.toString(v, 'f', d);
    }
    return locale.toString(v, 'g', 10);
}

}  // namespace recordfields
