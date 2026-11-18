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

// Frame of a top-level dialog, created by the application style. With the custom title bar (TitleBar::stored()) the dialog is
// frameless and gets a title bar, a border and resize edges; without it only the border.
class DialogFrame : public QWidget
{
    Q_OBJECT
public:
    static constexpr auto kName = "dialogFrame";
    static void install(QDialog *dialog);
    // Rounded corners need compositing (X11: an owner of _NET_WM_CM_S0; elsewhere always). CSR_SQUARE_CORNERS or the setting force
    // square corners; `forced` (tests) overrides both for later dialogs.
    [[nodiscard]] static bool roundedCorners();
    // The user's choice (File > Settings, default on) and whether the screen can show them; open dialogs keep theirs.
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
