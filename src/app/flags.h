#pragma once

#include <QIcon>
#include <QString>

// Small national flags for the language menu (resources/flags: SVG sources and the PNG icons built from them).
namespace flags
{

// The flag of a language's country ("de_DE" -> Germany, "en_US" -> USA); a null icon if there is none.
[[nodiscard]] QIcon forLanguage(const QString &language);

}  // namespace flags
