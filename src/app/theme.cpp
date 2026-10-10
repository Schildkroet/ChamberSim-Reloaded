#include "theme.h"
#include "log.h"
#include "dialogframe.h"

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
#include <QStyleOption>

namespace theme
{

namespace
{
constexpr auto kKey = "ui/theme";

QPalette darkPalette()
{
    QPalette p;
    const QColor window(0x2b, 0x31, 0x3b), base(0x22, 0x27, 0x30), alt(0x2f, 0x36, 0x41), text(0xdd, 0xe3, 0xea);
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
    p.setColor(QPalette::Link, QColor(0x6c, 0xb0, 0xf0));
    p.setColor(QPalette::Highlight, QColor(0x3b, 0x6e, 0xa8));
    p.setColor(QPalette::HighlightedText, Qt::white);
    p.setColor(QPalette::PlaceholderText, QColor(0x8a, 0x94, 0xa0));
    // disabled: clearly dimmer for every text role (menus and labels use WindowText), no etched shadow
    const QColor disabled(0x6a, 0x72, 0x7c);
    for (const QPalette::ColorRole role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText, QPalette::HighlightedText})
        p.setColor(QPalette::Disabled, role, disabled);
    p.setColor(QPalette::Disabled, QPalette::Light, window);
    p.setColor(QPalette::Disabled, QPalette::Highlight, alt);
    return p;
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
        painter->setBrush(!enabled ? QColor(0x45, 0x4c, 0x57) : active ? QColor(0x8a, 0x94, 0xa0) : QColor(0x6a, 0x73, 0x80));
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
        painter->setPen(QPen(enabled ? QColor(0x9a, 0xa4, 0xb0) : QColor(0x5a, 0x62, 0x6c), 1.0));
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

Mode storedMode()
{
    const QString v = QSettings().value(QLatin1String(kKey)).toString();
    return v == u"dark" ? Mode::Dark : v == u"light" ? Mode::Light : Mode::System;
}

void storeMode(Mode mode)
{
    LOG_Info() << "Theme set to" << (mode == Mode::System ? "system" : mode == Mode::Dark ? "dark" : "light");
    QSettings s;
    if (mode == Mode::System) s.remove(QLatin1String(kKey));
    else s.setValue(QLatin1String(kKey), mode == Mode::Dark ? QStringLiteral("dark") : QStringLiteral("light"));
}

void apply(QApplication &app, Mode mode)
{
    static const QString systemStyle = app.style()->name();
    static const QPalette systemPalette = app.palette();
    switch (mode)
    {
    case Mode::System:
        setStyle(app, systemStyle);
        app.setPalette(systemPalette);
        break;
    case Mode::Light:
        setStyle(app, QStringLiteral("Fusion"));
        app.setPalette(app.style()->standardPalette());
        break;
    case Mode::Dark:
        setStyle(app, QStringLiteral("Fusion"));
        app.setPalette(darkPalette());
        break;
    }
}

}  // namespace theme
