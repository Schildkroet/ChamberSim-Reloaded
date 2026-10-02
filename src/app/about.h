#pragma once

#include <QString>

class QWidget;

namespace about
{

// The text of the About box as rich text (version, purpose, calibration, safety notice, versions of the
// libraries in use, and the open database if there is one).
[[nodiscard]] QString html(const QString &databasePath = {}, int calibers = 0, int projectiles = 0, int propellants = 0);

void show(QWidget *parent, const QString &databasePath = {}, int calibers = 0, int projectiles = 0, int propellants = 0);

}  // namespace about
