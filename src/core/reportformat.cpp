#include "reportformat.h"

#include <QLocale>

#include <algorithm>
#include <cmath>

namespace grt::report
{

namespace
{

constexpr QStringView kNumeric = u"#0.,";

// Rounds half away from zero at `decimals` places.
double roundTo(double v, int decimals)
{
    const double p = std::pow(10.0, decimals);
    return std::round(v * p) / p;
}

QString groupThousands(const QString &digits)
{
    QString out;
    for (qsizetype i = 0; i < digits.size(); ++i)
    {
        if (i > 0 && (digits.size() - i) % 3 == 0) out += u',';
        out += digits[i];
    }
    return out;
}

}  // namespace

QString formatNumber(double value, const QString &format)
{
    if (std::isnan(value)) return QStringLiteral("NaN");
    if (std::isinf(value)) return value < 0 ? QStringLiteral("-INF") : QStringLiteral("INF");

    // split: prefix | numeric pattern | suffix
    const qsizetype first = [&]
    {
        for (qsizetype i = 0; i < format.size(); ++i)
            if (format[i] == u'#' || format[i] == u'0' || format[i] == u'.') return i;
        return qsizetype(-1);
    }();
    QString prefix, pattern, suffix = format;
    if (first >= 0)
    {
        prefix = format.left(first);
        qsizetype end = first;
        while (end < format.size() && kNumeric.contains(format[end])) ++end;
        pattern = format.mid(first, end - first);
        suffix = format.mid(end);
    }
    else
    {
        pattern = QStringLiteral("#.##");  // no pattern given: a sensible default, the text is kept
        prefix = format;
        suffix.clear();
    }

    // scientific notation: "E+00" / "E00" directly after the pattern
    bool scientific = false, expPlus = false;
    int expDigits = 2;
    if (suffix.startsWith(u'E'))
    {
        qsizetype i = 1;
        if (i < suffix.size() && (suffix[i] == u'+' || suffix[i] == u'-')) expPlus = suffix[i++] == u'+';
        qsizetype zeros = 0;
        while (i + zeros < suffix.size() && suffix[i + zeros] == u'0') ++zeros;
        scientific = true;
        expDigits = zeros > 0 ? static_cast<int>(zeros) : 2;
        suffix = suffix.mid(i + zeros);
    }

    const qsizetype dot = pattern.indexOf(u'.');
    const QString intPattern = dot < 0 ? pattern : pattern.left(dot);
    const QString decPattern = dot < 0 ? QString() : pattern.mid(dot + 1);
    const bool thousands = pattern.contains(u',');
    const int minInt = static_cast<int>(std::count(intPattern.begin(), intPattern.end(), u'0'));
    const int decimals =
        static_cast<int>(std::count_if(decPattern.begin(), decPattern.end(), [](QChar c) { return c == u'#' || c == u'0'; }));
    const int minDec = [&] {  // '0' placeholders force digits up to the last '0'
        int last = 0, pos = 0;
        for (QChar c : decPattern)
        {
            if (c == u'#' || c == u'0') ++pos;
            if (c == u'0') last = pos;
        }
        return last;
    }();

    // percent and sign markers in prefix/suffix
    const bool percent = prefix.contains(u'%') || suffix.contains(u'%');
    if (percent) value *= 100.0;
    const bool plusMode = prefix.contains(u'+') || suffix.contains(u'+');
    const bool minusMode = prefix.contains(u'-') || suffix.contains(u'-');

    int exponent = 0;
    double v = value;
    if (scientific)
    {
        v = std::fabs(value);
        if (v != 0.0)
        {
            exponent = static_cast<int>(std::floor(std::log10(v)));
            v /= std::pow(10.0, exponent);
            if (roundTo(v, decimals) >= 10.0)
            {
                v /= 10.0;
                ++exponent;
            }  // 9.999 -> 10.0 -> 1.0E+1
        }
        v = std::copysign(v, value);
    }
    const double rounded = roundTo(v, decimals);
    const bool negative = rounded < 0.0;

    QString number = QString::number(std::fabs(rounded), 'f', decimals);
    QString intPart = number.section(u'.', 0, 0), decPart = number.section(u'.', 1, 1);
    while (decPart.size() > minDec && decPart.endsWith(u'0')) decPart.chop(1);  // '#' positions: no trailing zeros
    if (intPart == u"0" && minInt == 0 && !decPart.isEmpty()) intPart.clear();  // "#.00" shows ".50" for 0.5
    while (intPart.size() < minInt) intPart.prepend(u'0');
    if (thousands) intPart = groupThousands(intPart);
    QString text = intPart + (decPart.isEmpty() ? QString() : u'.' + decPart);
    if (text.isEmpty()) text = QStringLiteral("0");

    if (scientific)
    {
        QString e = QString::number(std::abs(exponent)).rightJustified(expDigits, u'0');
        text += u'E' + (exponent < 0 ? QStringLiteral("-") : expPlus ? QStringLiteral("+") : QString()) + e;
    }

    // signs: explicit markers are replaced by the sign, otherwise a minus precedes the number
    const QString signText = negative ? QStringLiteral("-") : (plusMode ? QStringLiteral("+") : QString());
    const auto fill = [&](QString part)
    {
        part.replace(u'%', percent ? QStringLiteral("%") : QString());
        if (plusMode) part.replace(u'+', signText);
        else if (minusMode) part.replace(u'-', negative ? QStringLiteral("-") : QString());
        return part;
    };
    const bool signPlaced =
        (plusMode || minusMode) && (prefix.contains(u'+') || prefix.contains(u'-') || suffix.contains(u'+') || suffix.contains(u'-'));
    const QString lead = signPlaced ? QString() : (negative ? QStringLiteral("-") : QString());
    return fill(prefix) + lead + text + fill(suffix);
}

}  // namespace grt::report
