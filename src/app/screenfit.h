#pragma once

#include <QGuiApplication>
#include <QRect>
#include <QScreen>
#include <QWidget>

#include <algorithm>

// Resizes a dialog to `size`, but never beyond the available area of the screen it will appear on (the parent's, else the primary one;
// 95 % leaves room for the window frame).
inline void resizeWithinScreen(QWidget *dialog, QSize size)
{
    const QScreen *screen =
        dialog->parentWidget() && dialog->parentWidget()->screen() ? dialog->parentWidget()->screen() : QGuiApplication::primaryScreen();
    if (screen)
    {
        const QRect area = screen->availableGeometry();
        size = size.boundedTo({static_cast<int>(area.width() * 0.95), static_cast<int>(area.height() * 0.95)});
    }
    dialog->resize(size);
}
