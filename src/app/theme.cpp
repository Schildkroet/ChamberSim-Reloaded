#include "theme.h"

#include <QApplication>
#include <QPalette>
#include <QSettings>
#include <QStyle>
#include <QStyleFactory>

namespace theme {

namespace {
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
    p.setColor(QPalette::Disabled, QPalette::Text, QColor(0x7a, 0x84, 0x90));
    p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(0x7a, 0x84, 0x90));
    return p;
}
}  // namespace

Mode storedMode()
{
    const QString v = QSettings().value(QLatin1String(kKey)).toString();
    return v == u"dark" ? Mode::Dark : v == u"light" ? Mode::Light : Mode::System;
}

void storeMode(Mode mode)
{
    QSettings s;
    if (mode == Mode::System) s.remove(QLatin1String(kKey));
    else s.setValue(QLatin1String(kKey), mode == Mode::Dark ? QStringLiteral("dark") : QStringLiteral("light"));
}

void apply(QApplication &app, Mode mode)
{
    static const QString systemStyle = app.style()->objectName();
    static const QPalette systemPalette = app.palette();
    switch (mode) {
    case Mode::System:
        app.setStyle(systemStyle);
        app.setPalette(systemPalette);
        break;
    case Mode::Light:
        app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
        app.setPalette(app.style()->standardPalette());
        break;
    case Mode::Dark:
        app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
        app.setPalette(darkPalette());
        break;
    }
}

}  // namespace theme
