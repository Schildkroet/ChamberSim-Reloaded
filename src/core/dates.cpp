#include "dates.h"

#include <QDate>
#include <QDateTime>
#include <QRegularExpression>

using namespace Qt::StringLiterals;

namespace csr
{

std::optional<QString> normalizedDate(const QVariant &value)
{
    if (value.typeId() == QMetaType::QDate)
    {
        const QDate d = value.toDate();
        return d.isValid() ? std::optional(d.toString(Qt::ISODate)) : std::nullopt;
    }
    if (value.typeId() == QMetaType::QDateTime)
    {
        const QDateTime d = value.toDateTime();
        return d.isValid() ? std::optional(d.date().toString(Qt::ISODate)) : std::nullopt;
    }
    const QString text = value.toString().trimmed();
    if (text.isEmpty() || text.startsWith(u"0000-00-00")) return QString();
    static const QRegularExpression iso(uR"(^(\d{4})[-/](\d{1,2})[-/](\d{1,2})(?:[ T].*)?$)"_s);
    static const QRegularExpression german(uR"(^(\d{1,2})\.(\d{1,2})\.(\d{4})(?: .*)?$)"_s);
    QDate d;
    if (const auto m = iso.match(text); m.hasMatch()) d = QDate(m.captured(1).toInt(), m.captured(2).toInt(), m.captured(3).toInt());
    else if (const auto g = german.match(text); g.hasMatch())
        d = QDate(g.captured(3).toInt(), g.captured(2).toInt(), g.captured(1).toInt());
    if (!d.isValid()) return std::nullopt;
    return d.toString(Qt::ISODate);
}

}  // namespace csr
