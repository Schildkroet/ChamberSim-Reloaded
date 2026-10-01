#pragma once

#include <QString>
#include <QStringList>

class QCoreApplication;

// Installs the translators for the language in use and exposes the choices for
// the language menu. American English (the source language) is the default and
// is used whenever no translation matches.
namespace translation {

// Languages that ship a translation, always including "en_US". Normalized ("de_DE").
[[nodiscard]] QStringList availableLanguages();

// The user's stored choice; empty means "follow the system language".
[[nodiscard]] QString storedLanguage();
void storeLanguage(const QString &language);

// Resolves the language (override, else stored choice, else system UI languages),
// installs the application and Qt translators and sets the default QLocale.
// Returns the language that is now active.
QString install(QCoreApplication &app, const QString &overrideLanguage = {});

// "Deutsch (Deutschland)" style display name for menus, in the language's own words.
[[nodiscard]] QString displayName(const QString &language);

}  // namespace translation
