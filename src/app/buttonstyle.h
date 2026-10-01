#pragma once

#include <QCursor>
#include <QPushButton>

// Makes a button stand out as a main action: filled with the theme's accent color, bold text, a hand cursor.
// The colors are palette references, so the look follows the light/dark theme; a disabled button turns
// into an ordinary flat gray one so it is obvious that it cannot be used yet.
inline void markPrimary(QPushButton *button)
{
    button->setCursor(Qt::PointingHandCursor);
    button->setMinimumHeight(30);
    button->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: palette(highlight); color: palette(highlighted-text);"
        "              border: 1px solid palette(highlight); border-radius: 4px; padding: 4px 14px; font-weight: bold; }"
        "QPushButton:hover { background-color: palette(link); border-color: palette(link); }"
        "QPushButton:pressed { background-color: palette(dark); border-color: palette(dark); }"
        "QPushButton:focus { border: 2px solid palette(text); }"
        "QPushButton:disabled { background-color: palette(button); color: palette(mid);"
        "                       border: 1px solid palette(mid); font-weight: normal; }"));
}
