#pragma once

#include <QIcon>
#include <QString>

// Small national flags for the language menu, drawn with QPainter (no image files; sharp at every scale).
namespace flags {

// The flag of a language's country ("de_DE" -> Germany, "en_US" -> USA); a null icon if there is none.
[[nodiscard]] QIcon forLanguage(const QString &language);

}  // namespace flags
