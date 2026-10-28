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

#include "titlebar.h"

#include <QApplication>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QMouseEvent>
#include <QPainter>
#include <QSettings>
#include <QWindow>

#include <algorithm>
#include <cmath>

namespace
{
constexpr auto kKey = "ui/customTitleBar";
}

WindowButton::WindowButton(Kind kind, QWidget *parent)
    : QAbstractButton(parent)
    , m_kind(kind)
{
    setFocusPolicy(Qt::NoFocus);
    setAttribute(Qt::WA_Hover);
}

void WindowButton::setKind(Kind kind)
{
    m_kind = kind;
    update();
}

void WindowButton::enterEvent(QEnterEvent *event)
{
    m_hover = true;
    update();
    QAbstractButton::enterEvent(event);
}

void WindowButton::leaveEvent(QEvent *event)
{
    m_hover = false;
    update();
    QAbstractButton::leaveEvent(event);
}

void WindowButton::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const bool close = m_kind == Kind::Close;
    if (m_hover || isDown())
    {
        QColor bg = close ? QColor(0xc4, 0x2b, 0x1c) : palette().color(QPalette::Text);
        if (!close) bg.setAlphaF(isDown() ? 0.22 : 0.12);
        else if (isDown()) bg = bg.darker(120);
        p.fillRect(rect(), bg);
    }
    const QColor fg = close && (m_hover || isDown()) ? Qt::white : palette().color(QPalette::WindowText);
    // the glyph: about 10 px at the default size, scaled with the button
    const double s = std::round(height() * 0.3);
    const QPointF c = QRectF(rect()).center();
    p.setPen(QPen(fg, std::max(1.0, height() / 30.0)));
    p.setBrush(Qt::NoBrush);
    switch (m_kind)
    {
    case Kind::Minimize: p.drawLine(QPointF(c.x() - s / 2, c.y()), QPointF(c.x() + s / 2, c.y())); break;
    case Kind::Maximize: p.drawRect(QRectF(c.x() - s / 2, c.y() - s / 2, s, s)); break;
    case Kind::Restore:
    {
        const double d = s * 0.25;
        p.drawRect(QRectF(c.x() - s / 2, c.y() - s / 2 + d, s - d, s - d));
        p.drawPolyline(QList<QPointF>{{c.x() - s / 2 + d, c.y() - s / 2 + d},
                                      {c.x() - s / 2 + d, c.y() - s / 2},
                                      {c.x() + s / 2, c.y() - s / 2},
                                      {c.x() + s / 2, c.y() + s / 2 - d},
                                      {c.x() + s / 2 - d, c.y() + s / 2 - d}});
        break;
    }
    case Kind::Close:
        p.drawLine(QPointF(c.x() - s / 2, c.y() - s / 2), QPointF(c.x() + s / 2, c.y() + s / 2));
        p.drawLine(QPointF(c.x() - s / 2, c.y() + s / 2), QPointF(c.x() + s / 2, c.y() - s / 2));
        break;
    }
}

TitleBar::TitleBar(QMenuBar *menu, QWidget *window)
    : m_window(window)
    , m_menu(menu)
    , m_icon(new QLabel)
    , m_title(new QLabel)
    , m_minimize(new WindowButton(WindowButton::Kind::Minimize))
    , m_maximize(new WindowButton(WindowButton::Kind::Maximize))
    , m_close(new WindowButton(WindowButton::Kind::Close))
{
    setObjectName(QStringLiteral("titleBar"));
#ifndef Q_OS_MACOS
    m_menu->setNativeMenuBar(false);  // it lives in this row, not in a global menu
#endif
    m_title->setAlignment(Qt::AlignCenter);
    m_title->setTextFormat(Qt::PlainText);
    m_title->setMinimumWidth(40);
    m_title->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);  // elided by the space left
    m_minimize->setToolTip(tr("Minimize"));
    m_close->setToolTip(tr("Close"));
    m_minimize->setObjectName(QStringLiteral("minimizeButton"));
    m_maximize->setObjectName(QStringLiteral("maximizeButton"));
    m_close->setObjectName(QStringLiteral("closeButton"));

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_icon);
    layout->addWidget(m_menu, 0, Qt::AlignVCenter);
    layout->addWidget(m_title, 1);
    layout->addWidget(m_minimize);
    layout->addWidget(m_maximize);
    layout->addWidget(m_close);

    connect(m_minimize, &QAbstractButton::clicked, m_window, &QWidget::showMinimized);
    connect(m_maximize, &QAbstractButton::clicked, this, &TitleBar::toggleMaximized);
    connect(m_close, &QAbstractButton::clicked, m_window, &QWidget::close);
    m_window->installEventFilter(this);
    m_menu->installEventFilter(this);
    setDecorated(false);
}

bool TitleBar::stored()
{
#ifdef Q_OS_LINUX
    constexpr bool kDefault = true;
#else
    constexpr bool kDefault = false;
#endif
    return QSettings().value(QLatin1String(kKey), kDefault).toBool();
}

void TitleBar::store(bool on)
{
    QSettings().setValue(QLatin1String(kKey), on);
}

void TitleBar::setDecorated(bool on)
{
    m_decorated = on;
#ifdef Q_OS_MACOS
    m_menu->setNativeMenuBar(!on);  // macOS: the menu is the global one at the top of the screen, unless the own title bar holds it
#endif
    for (QWidget *w : std::initializer_list<QWidget *>{m_icon, m_title, m_minimize, m_maximize, m_close}) w->setVisible(on);
    updateSizes();
    updateState();
}

void TitleBar::updateSizes()
{
    // a comfortable height that follows the font size (the menu entries sit in it)
    const int h = std::max(34, static_cast<int>(fontMetrics().height() * 2.0));
    setFixedHeight(m_decorated ? h : QWIDGETSIZE_MAX);
    if (!m_decorated) setMinimumHeight(0);
    for (QAbstractButton *b : {m_minimize, m_maximize, m_close}) b->setFixedSize(static_cast<int>(h * 1.35), h);
    const int iconSize = h - 12;
    m_icon->setFixedSize(h, h);
    m_icon->setAlignment(Qt::AlignCenter);
    m_icon->setPixmap(m_window->windowIcon().pixmap(iconSize, iconSize));
    // the window title: bold and larger than the menu entries (it follows the application font size)
    QFont titleFont = font();
    titleFont.setBold(true);
    if (titleFont.pointSizeF() > 0.0) titleFont.setPointSizeF(titleFont.pointSizeF() * 1.1);
    else if (titleFont.pixelSize() > 0) titleFont.setPixelSize(static_cast<int>(titleFont.pixelSize() * 1.1));
    m_title->setFont(titleFont);
}

void TitleBar::updateState()
{
    m_title->setText(m_window->windowTitle());
    const bool max = m_window->isMaximized();
    static_cast<WindowButton *>(m_maximize)->setKind(max ? WindowButton::Kind::Restore : WindowButton::Kind::Maximize);
    m_maximize->setToolTip(max ? tr("Restore") : tr("Maximize"));
}

void TitleBar::toggleMaximized()
{
    if (m_window->isMaximized()) m_window->showNormal();
    else m_window->showMaximized();
}

void TitleBar::startMove()
{
    if (QWindow *w = m_window->windowHandle()) w->startSystemMove();
}

bool TitleBar::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_window)
    {
        switch (event->type())
        {
        case QEvent::WindowTitleChange:
        case QEvent::WindowStateChange:
        case QEvent::WindowIconChange:
            updateState();
            if (event->type() == QEvent::WindowIconChange) updateSizes();
            break;
        default: break;
        }
    }
    else if (watched == m_menu && m_decorated)
    {
        // the empty part of the menu bar belongs to the title bar
        if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick)
        {
            auto *me = static_cast<QMouseEvent *>(event);
            if (me->button() == Qt::LeftButton && !m_menu->actionAt(me->position().toPoint()))
            {
                if (event->type() == QEvent::MouseButtonDblClick) toggleMaximized();
                else startMove();
                return true;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

void TitleBar::mousePressEvent(QMouseEvent *event)
{
    if (m_decorated && event->button() == Qt::LeftButton) startMove();
    QWidget::mousePressEvent(event);
}

void TitleBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (m_decorated && event->button() == Qt::LeftButton) toggleMaximized();
    QWidget::mouseDoubleClickEvent(event);
}

void TitleBar::paintEvent(QPaintEvent *)
{
    if (!m_decorated) return;
    // the window's color (the menu bar paints the same), with a separating line
    QPainter p(this);
    p.fillRect(rect(), palette().color(QPalette::Window));
    p.setPen(palette().color(QPalette::PlaceholderText));  // Mid is almost black in the dark theme
    p.drawLine(rect().bottomLeft(), rect().bottomRight());
}

void TitleBar::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::FontChange || event->type() == QEvent::ApplicationFontChange) updateSizes();
}
