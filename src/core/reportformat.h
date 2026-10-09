#pragma once

#include <QString>

namespace csr::report
{

// Formats a number with a format string of the report language (see the original's "Format string" page):
//   #  digit if present        0  digit, or a zero            .  decimal point
//   ,  thousands separators    %  value times 100, shows "%"   (  )  literal parentheses
//   +  sign: "+" or "-"        -  "-" for negative numbers     E  scientific notation ("0.00E+00")
// Any other characters are output as they are, e.g. "#.000 mm²". Numbers always use '.' as the decimal
// point and ',' as the thousands separator, independent of the user's language (ISO style).
[[nodiscard]] QString formatNumber(double value, const QString &format);

}  // namespace csr::report
