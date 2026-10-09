#pragma once

#include <QString>
#include <QVariant>

#include <optional>

namespace csr
{

// The database stores dates as text "yyyy-MM-dd" (like the original program). This brings a date into that form.
// Accepted: yyyy-MM-dd (also with a time, "yyyy-MM-dd hh:mm:ss" or ISO "...T..."), yyyy/MM/dd, dd.MM.yyyy and
// d.M.yyyy, and QDate / QDateTime values. An empty value and the original's "0000-00-00" give "" (no date);
// anything else that is not a valid date gives nullopt.
[[nodiscard]] std::optional<QString> normalizedDate(const QVariant &value);

}  // namespace csr
