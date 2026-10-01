#pragma once

class QApplication;

// The application-wide font size, chosen by the user in the settings. Only the point size changes; the
// font family and style stay those of the platform. The size is limited to a range in which the program's
// layouts stay usable.
namespace appfont {

inline constexpr int kMinSize = 8;   // points
inline constexpr int kMaxSize = 20;

// Limits a requested size to [kMinSize, kMaxSize].
[[nodiscard]] int clampSize(int points);

// The platform's own font size (read once, before the program changes anything), within the allowed range.
[[nodiscard]] int systemSize();

// The user's choice; systemSize() if none was stored.
[[nodiscard]] int storedSize();
[[nodiscard]] bool hasCustomSize();
// Stores the choice (clamped). A value equal to the system size removes the setting.
void storeSize(int points);

// Sets the font size of the whole application (clamped) and returns the size that was applied.
int apply(QApplication &app, int points);

}  // namespace appfont
