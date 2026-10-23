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

#include "theme.h"
#include "log.h"
#include "dialogframe.h"

#include <array>

#include <QApplication>
#include <QDialog>
#include <QEvent>
#include <QPainter>
#include <QPalette>
#include <QAbstractItemView>
#include <QProxyStyle>
#include <QSettings>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleHints>
#include <QStyleOption>
#include <QtGlobal>

namespace theme
{

namespace
{
constexpr auto kKey = "ui/theme";

// Colors of a dark or light scheme; the palettes of Gruvbox (Pavel Pertsev, MIT), Nord (Sven Greb, MIT) and Dracula (Zeno Rocha, MIT)
// and Catppuccin (MIT) use their published values
struct Scheme
{
    QRgb window, base, alt, text, link, highlight, highlightedText, placeholder, disabled;
};

// Fusion's classic light colors, fixed: Fusion's standardPalette() follows the system color scheme (dark on a dark desktop)
constexpr Scheme kLight{0xefefef, 0xffffff, 0xf7f7f7, 0x000000, 0x0000ff, 0x308cc6, 0xffffff, 0x808080, 0xbebebe};
constexpr Scheme kDark{0x2b313b, 0x222730, 0x2f3641, 0xdde3ea, 0x6cb0f0, 0x3b6ea8, 0xffffff, 0x8a94a0, 0x6a727c};
constexpr Scheme kGruvboxDark{0x282828, 0x1d2021, 0x3c3836, 0xebdbb2, 0x83a598, 0x458588, 0xebdbb2, 0x928374, 0x7c6f64};
constexpr Scheme kGruvboxLight{0xebdbb2, 0xfbf1c7, 0xf2e5bc, 0x3c3836, 0x076678, 0x458588, 0xfbf1c7, 0x928374, 0xa89984};
constexpr Scheme kNord{0x3b4252, 0x2e3440, 0x434c5e, 0xd8dee9, 0x88c0d0, 0x5e81ac, 0xeceff4, 0x616e88, 0x616e88};
constexpr Scheme kDracula{0x282a36, 0x21222c, 0x343746, 0xf8f8f2, 0x8be9fd, 0x44475a, 0xf8f8f2, 0x6272a4, 0x6272a4};
constexpr Scheme kCatppuccinLatte{0xe6e9ef, 0xeff1f5, 0xdce0e8, 0x4c4f69, 0x1e66f5, 0x1e66f5, 0xeff1f5, 0x8c8fa1, 0x9ca0b0};
constexpr Scheme kCatppuccinFrappe{0x303446, 0x292c3c, 0x414559, 0xc6d0f5, 0x8caaee, 0x8caaee, 0x232634, 0x838ba7, 0x737994};

[[nodiscard]] const Scheme *schemeFor(Mode mode)
{
    switch (mode)
    {
    case Mode::Light: return &kLight;
    case Mode::Dark: return &kDark;
    case Mode::GruvboxDark: return &kGruvboxDark;
    case Mode::GruvboxLight: return &kGruvboxLight;
    case Mode::Nord: return &kNord;
    case Mode::Dracula: return &kDracula;
    case Mode::CatppuccinLatte: return &kCatppuccinLatte;
    case Mode::CatppuccinFrappe: return &kCatppuccinFrappe;
    case Mode::System: break;
    }
    return nullptr;
}

// `part` (0..1) of the way from color `from` to color `to`
QColor mixed(const QColor &from, const QColor &to, double part)
{
    return QColor::fromRgbF(from.redF() + (to.redF() - from.redF()) * part, from.greenF() + (to.greenF() - from.greenF()) * part,
                            from.blueF() + (to.blueF() - from.blueF()) * part);
}

QPalette schemePalette(const Scheme &c)
{
    // every role from the scheme: a default QPalette would carry the bevel roles (Light, Mid, Dark, Shadow) of the palette before
    const QColor window(c.window), base(c.base), alt(c.alt), text(c.text);
    QPalette p(window, window);
    const auto mix = [&](double part) { return mixed(window, text, part); };
    const bool dark = window.lightness() < 128;
    p.setColor(QPalette::Light, dark ? mix(0.25) : window.lighter(150));
    p.setColor(QPalette::Midlight, dark ? mix(0.15) : window.lighter(125));
    p.setColor(QPalette::Mid, mix(0.3));
    p.setColor(QPalette::Dark, dark ? window.darker(150) : mix(0.45));
    p.setColor(QPalette::Shadow, dark ? QColor(0, 0, 0) : mix(0.7));
    p.setColor(QPalette::Window, window);
    p.setColor(QPalette::WindowText, text);
    p.setColor(QPalette::Base, base);
    p.setColor(QPalette::AlternateBase, alt);
    p.setColor(QPalette::ToolTipBase, base);
    p.setColor(QPalette::ToolTipText, text);
    p.setColor(QPalette::Text, text);
    p.setColor(QPalette::Button, window);
    p.setColor(QPalette::ButtonText, text);
    p.setColor(QPalette::BrightText, Qt::red);
    p.setColor(QPalette::Link, QColor(c.link));
    p.setColor(QPalette::Highlight, QColor(c.highlight));
    p.setColor(QPalette::HighlightedText, QColor(c.highlightedText));
    p.setColor(QPalette::PlaceholderText, QColor(c.placeholder));
    // disabled: clearly dimmer for every text role (menus and labels use WindowText), no etched shadow
    const QColor disabled(c.disabled);
    for (const QPalette::ColorRole role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText, QPalette::HighlightedText})
        p.setColor(QPalette::Disabled, role, disabled);
    p.setColor(QPalette::Disabled, QPalette::Light, window);
    p.setColor(QPalette::Disabled, QPalette::Highlight, alt);
    return p;
}

// `part` (0..1) of the way from the window color to the text color
QColor towardsText(const QPalette &palette, double part)
{
    return mixed(palette.color(QPalette::Window), palette.color(QPalette::WindowText), part);
}

// Combo boxes open as a list below the field (at most maxVisibleItems rows, with a scroll bar) instead of the
// Fusion/GTK popup that covers the field and tries to show every item. A long list (e.g. the countries) does not
// fit on the screen and gets moved; on Wayland the mouse release then lands outside and closes it at once.
//
// Item views: smooth scrolling by pixel in both directions, not by row and column.
//
// Dialogs: a visible frame and, with the custom title bar, their own title bar (DialogFrame). Scroll bars: with a dark palette a lighter handle.
//
// Check boxes and radio buttons: Fusion draws their frame a little darker than the window, on a dark window an unchecked box is
// then nearly invisible. With a dark palette the frame is drawn again in a light grey.
class ListPopupStyle : public QProxyStyle
{
public:
    using QProxyStyle::QProxyStyle;
    void polish(QWidget *widget) override
    {
        QProxyStyle::polish(widget);
        if (auto *dialog = qobject_cast<QDialog *>(widget)) DialogFrame::install(dialog);
    }
    using QProxyStyle::polish;
    int styleHint(StyleHint hint, const QStyleOption *option, const QWidget *widget, QStyleHintReturn *ret) const override
    {
        if (hint == SH_ComboBox_Popup) return 0;
        if (hint == SH_ItemView_ScrollMode) return QAbstractItemView::ScrollPerPixel;  // the default scrolls by item (a column jumps)
        return QProxyStyle::styleHint(hint, option, widget, ret);
    }
    void drawComplexControl(ComplexControl control, const QStyleOptionComplex *option, QPainter *painter,
                            const QWidget *widget) const override
    {
        const auto *bar = qstyleoption_cast<const QStyleOptionSlider *>(option);
        if (control != CC_ScrollBar || !bar || option->palette.color(QPalette::Window).lightness() >= 128)
            return QProxyStyle::drawComplexControl(control, option, painter, widget);
        // Fusion fills groove and handle with shades of Button = the window color: it draws the groove and the arrows, the
        // handle is drawn here in a light grey (lighter while hovered or dragged)
        QStyleOptionSlider rest(*bar);
        rest.subControls &= ~SC_ScrollBarSlider;
        QProxyStyle::drawComplexControl(control, &rest, painter, widget);
        if (!(bar->subControls & SC_ScrollBarSlider)) return;
        const QRect handle = subControlRect(control, bar, SC_ScrollBarSlider, widget);
        if (handle.isEmpty()) return;
        const bool active = (bar->activeSubControls & SC_ScrollBarSlider) && (bar->state & (State_MouseOver | State_Sunken));
        const bool enabled = bar->state & State_Enabled;
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(Qt::NoPen);
        painter->setBrush(towardsText(option->palette, !enabled ? 0.15 : active ? 0.55 : 0.38));
        const QRectF r = bar->orientation == Qt::Horizontal ? QRectF(handle).adjusted(1, 2, -1, -2) : QRectF(handle).adjusted(2, 1, -2, -1);
        painter->drawRoundedRect(r, 3, 3);
        painter->restore();
    }
    // Splitter handles: Fusion's dots are barely visible, draw a row of clearly visible dots in both themes
    void drawControl(ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const override
    {
        if (element != CE_Splitter) return QProxyStyle::drawControl(element, option, painter, widget);
        const QRect r = option->rect;
        painter->fillRect(r, option->palette.color(QPalette::Window));
        const bool horizontal = option->state & State_Horizontal;  // horizontal = side-by-side panels, vertical handle
        const bool dark = option->palette.color(QPalette::Window).lightness() < 128;
        QColor dot = option->palette.color(QPalette::WindowText);
        dot.setAlpha(option->state & State_MouseOver ? 220 : dark ? 150 : 130);
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(Qt::NoPen);
        painter->setBrush(dot);
        constexpr int count = 5;
        constexpr double size = 3.8, gap = 8.0;
        const QPointF c = QRectF(r).center();
        for (int i = 0; i < count; ++i)
        {
            const double off = (i - (count - 1) / 2.0) * gap;
            painter->drawEllipse(horizontal ? QPointF(c.x(), c.y() + off) : QPointF(c.x() + off, c.y()), size / 2, size / 2);
        }
        painter->restore();
    }
    void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const override
    {
        QProxyStyle::drawPrimitive(element, option, painter, widget);
        const bool box = element == PE_IndicatorCheckBox, radio = element == PE_IndicatorRadioButton;
        if ((!box && !radio) || option->palette.color(QPalette::Window).lightness() >= 128) return;
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, radio);
        const bool enabled = option->state & State_Enabled;
        painter->setPen(QPen(towardsText(option->palette, enabled ? 0.65 : 0.3), 1.0));
        painter->setBrush(Qt::NoBrush);
        const QRectF r = QRectF(option->rect).adjusted(0.5, 0.5, -0.5, -0.5);
        if (radio) painter->drawEllipse(r.adjusted(0.5, 0.5, -0.5, -0.5));
        else painter->drawRect(r);
        painter->restore();
    }
};

void setStyle(QApplication &app, const QString &name)
{
    app.setStyle(new ListPopupStyle(QStyleFactory::create(name)));
}
}  // namespace

namespace
{
struct ModeKey
{
    Mode mode;
    const char *key;
};
constexpr std::array<ModeKey, 9> kKeys{{{Mode::System, "system"},
                                        {Mode::Light, "light"},
                                        {Mode::Dark, "dark"},
                                        {Mode::GruvboxLight, "gruvbox-light"},
                                        {Mode::GruvboxDark, "gruvbox-dark"},
                                        {Mode::Nord, "nord"},
                                        {Mode::Dracula, "dracula"},
                                        {Mode::CatppuccinLatte, "catppuccin-latte"},
                                        {Mode::CatppuccinFrappe, "catppuccin-frappe"}}};
}  // namespace

QString modeKey(Mode mode)
{
    for (const auto &[m, key] : kKeys)
        if (m == mode) return QLatin1String(key);
    return QStringLiteral("system");
}

Mode modeFromKey(const QString &key)
{
    for (const auto &[m, k] : kKeys)
        if (key == QLatin1String(k)) return m;
    return Mode::System;
}

Mode storedMode()
{
    return modeFromKey(QSettings().value(QLatin1String(kKey)).toString());
}

void storeMode(Mode mode)
{
    LOG_Info() << "Theme set to" << modeKey(mode);
    QSettings s;
    if (mode == Mode::System) s.remove(QLatin1String(kKey));
    else s.setValue(QLatin1String(kKey), modeKey(mode));
}

void apply(QApplication &app, Mode mode)
{
    static const QString systemStyle = app.style()->name();
    static const QPalette systemPalette = app.palette();
    switch (mode)
    {
    case Mode::System:
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
        // Some platform styles ignore the OS dark/light setting (e.g. the Windows styles of older Qt 6 versions): then the
        // palette disagrees with the reported color scheme and the matching built-in scheme is used instead
        const Qt::ColorScheme scheme = app.styleHints()->colorScheme();
        const bool paletteDark = systemPalette.color(QPalette::Window).lightness() < 128;
        if (scheme != Qt::ColorScheme::Unknown && paletteDark != (scheme == Qt::ColorScheme::Dark))
        {
            setStyle(app, QStringLiteral("Fusion"));
            app.setPalette(schemePalette(scheme == Qt::ColorScheme::Dark ? kDark : kLight));
            break;
        }
#endif
        setStyle(app, systemStyle);
        app.setPalette(systemPalette);
        break;
    }
    default:
        setStyle(app, QStringLiteral("Fusion"));
        app.setPalette(schemePalette(*schemeFor(mode)));
        break;
    }
}

}  // namespace theme
