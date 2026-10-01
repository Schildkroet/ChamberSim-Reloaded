#pragma once

#include <QString>

class QApplication;

// Light / dark appearance. "System" keeps the platform's palette.
namespace theme {

enum class Mode { System, Light, Dark };

[[nodiscard]] Mode storedMode();
void storeMode(Mode mode);

// Applies the mode to the whole application (palette + Fusion style for the dark variant).
void apply(QApplication &app, Mode mode);

}  // namespace theme
