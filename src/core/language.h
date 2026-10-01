#pragma once

#include <QString>
#include <QStringList>

namespace grt {

// The source strings of the program are American English; it is the default
// language and needs no translation file.
inline const QString kDefaultLanguage = QStringLiteral("en_US");

// Picks the language to use. `requested` is in order of preference (e.g. the
// user's setting, then the system's UI languages, in BCP-47 or POSIX spelling
// such as "de-AT" / "de_AT"); `available` are the languages that ship a
// translation (e.g. "de_DE", "fr_FR"). Matching order per requested entry:
//   1. exact (de_AT == de_AT)
//   2. same language, other country (de_AT -> de_DE)
// Returns kDefaultLanguage when nothing matches.
[[nodiscard]] QString resolveLanguage(const QStringList &requested, const QStringList &available);

// "de-AT" / "de_AT.UTF-8" -> "de_AT"
[[nodiscard]] QString normalizeLanguage(const QString &tag);

}  // namespace grt
