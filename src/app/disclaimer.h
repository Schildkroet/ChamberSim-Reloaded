#pragma once

#include <memory>

class QMessageBox;
class QWidget;

// The safety notice at start-up: handloading is dangerous, the results are estimates, no liability. "Do not show this again" is
// remembered (setting ui/hideDisclaimer) only together with "I understand".
namespace disclaimer
{

// Whether the notice is to be shown (not hidden by the user).
[[nodiscard]] bool wanted();

// The notice as a dialog: buttons "I understand" (accept) and "Quit", the check box "Do not show this again".
[[nodiscard]] std::unique_ptr<QMessageBox> create(QWidget *parent = nullptr);

// Shows the notice if wanted; false when the user quits.
[[nodiscard]] bool confirm(QWidget *parent = nullptr);

}  // namespace disclaimer
