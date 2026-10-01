#pragma once

#include <QAbstractButton>
#include <QWidget>

class QLabel;
class QMenuBar;

// The window's own title bar (used on Linux, where Wayland compositors like GNOME leave the decoration to the
// program and Qt's fallback is a tiny bar): icon, the menu bar, the title and large window buttons in one row.
// Dragging the empty area moves the window, a double click maximizes it. Without decoration only the menu bar is
// shown, so the window keeps one menu widget in both modes.
class TitleBar : public QWidget {
    Q_OBJECT
public:
    TitleBar(QMenuBar *menu, QWidget *window);

    void setDecorated(bool on);
    [[nodiscard]] bool isDecorated() const noexcept { return m_decorated; }

    // Stored choice (default: on for Linux).
    [[nodiscard]] static bool stored();
    static void store(bool on);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void changeEvent(QEvent *event) override;

private:
    void toggleMaximized();
    void startMove();
    void updateSizes();
    void updateState();

    QWidget *m_window;
    QMenuBar *m_menu;
    QLabel *m_icon;
    QLabel *m_title;
    QAbstractButton *m_minimize;
    QAbstractButton *m_maximize;
    QAbstractButton *m_close;
    bool m_decorated{false};
};

// Minimize / maximize (restore) / close, drawn in the palette's colors.
class WindowButton : public QAbstractButton {
    Q_OBJECT
public:
    enum class Kind { Minimize, Maximize, Restore, Close };
    explicit WindowButton(Kind kind, QWidget *parent = nullptr);
    void setKind(Kind kind);
    [[nodiscard]] Kind kind() const noexcept { return m_kind; }

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    Kind m_kind;
    bool m_hover{false};
};
