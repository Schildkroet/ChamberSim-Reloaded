/*
 * This file is part of ChamberSim Reloaded.
 *
 * Copyright (C) 2026 Schildkroet
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <QWidget>

#include <optional>

class QDialog;
class QLabel;

// The own frame of a dialog while the custom title bar is on (TitleBar::stored(), the Linux default; Wayland compositors like GNOME
// leave the decoration to the program and draw no border): the dialog becomes frameless and gets a title bar (title and close button,
// taller than the main window's), a border around the whole window (paintWindowBorder) and, unless its size is fixed, edges that resize it.
// Without the custom title bar only the border over the dialog's content is drawn. Created by the application style when a top-level
// dialog is polished; it is a child of the dialog and lives as long as the dialog.
class DialogFrame : public QWidget
{
    Q_OBJECT
public:
    static constexpr auto kName = "dialogFrame";
    static void install(QDialog *dialog);
    // Rounded corners need a translucent window, and on X11 without a compositing window manager that shows black corners: then the
    // corners stay square. Detected once (X11: owner of the _NET_WM_CM_S0 selection; Wayland, Windows, macOS: always available);
    // the environment variable CSR_SQUARE_CORNERS forces square corners, as does the setting. `forced` (tests) overrides both for dialogs created later.
    [[nodiscard]] static bool roundedCorners();
    // The user's choice (File > Settings, default on) and whether the screen can show them at all (compositing; the choice is kept
    // either way). Dialogs that are open keep their corners; the next one follows the choice.
    [[nodiscard]] static bool roundedStored();
    static void storeRounded(bool on);
    [[nodiscard]] static bool roundedCornersPossible();
    static void forceRoundedCorners(std::optional<bool> forced);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    explicit DialogFrame(QDialog *dialog);
    void layoutParts();
    [[nodiscard]] Qt::Edges edgesAt(const QPoint &pos) const;
    [[nodiscard]] bool resizable() const;

    QDialog *m_dialog;
    QWidget *m_titleBar{nullptr};  // null: no own title bar
    QLabel *m_title{nullptr};
    int m_titleHeight{0};
    int m_radius{0};  // corner radius, 0: square
};
