#include "mainwindow.h"
#include "appfont.h"
#include "theme.h"
#include "translation.h"
#include "unitprefs.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>
#include <QTimer>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("GRTReloaded"));
    QCoreApplication::setApplicationName(QStringLiteral("GRTReloaded"));
    QCoreApplication::setApplicationVersion(QStringLiteral(GRT_VERSION));
    QGuiApplication::setDesktopFileName(QStringLiteral("grtreloaded"));  // Wayland: the icon of the .desktop entry
    QIcon icon;
    for (const int size : {32, 64, 256}) icon.addFile(QStringLiteral(":/resources/icons/grtreloaded-%1.png").arg(size));
    QApplication::setWindowIcon(icon);

    QCommandLineParser cli;
    cli.setApplicationDescription(QStringLiteral("Interior ballistics for reloaders"));
    cli.addHelpOption();
    cli.addPositionalArgument(QStringLiteral("load"), QStringLiteral("*.grtload files to open"), QStringLiteral("[load...]"));
    const QCommandLineOption dbOpt({QStringLiteral("d"), QStringLiteral("database")}, QStringLiteral("SQLite database to use"), QStringLiteral("file"));
    const QCommandLineOption refOpt({QStringLiteral("r"), QStringLiteral("reference")}, QStringLiteral("reference result (*.json) to overlay on the first load"), QStringLiteral("file"));
    const QCommandLineOption shotOpt(QStringLiteral("screenshot"), QStringLiteral("save a screenshot of the window and exit"), QStringLiteral("png"));
    const QCommandLineOption tabOpt(QStringLiteral("tab"), QStringLiteral("tab index to show for --screenshot"), QStringLiteral("index"));
    const QCommandLineOption resultTabOpt(QStringLiteral("result-tab"), QStringLiteral("result tab to show for --screenshot (0 results, 1 measurements, 2 report)"), QStringLiteral("index"));
    const QCommandLineOption langOpt(QStringLiteral("language"), QStringLiteral("user interface language, e.g. en_US or de_DE"), QStringLiteral("lang"));
    const QCommandLineOption themeOpt(QStringLiteral("theme"), QStringLiteral("light or dark (overrides the saved choice)"), QStringLiteral("mode"));
    const QCommandLineOption unitsOpt(QStringLiteral("units"), QStringLiteral("unit system: metric, imperial or original (saved)"), QStringLiteral("system"));
    cli.addOptions({dbOpt, refOpt, shotOpt, tabOpt, resultTabOpt, langOpt, themeOpt, unitsOpt});
    cli.process(a);
    appfont::apply(a, appfont::storedSize());  // the platform font is remembered before the first change
    translation::install(a, cli.value(langOpt));
    theme::apply(a, cli.isSet(themeOpt) ? (cli.value(themeOpt) == QLatin1String("dark") ? theme::Mode::Dark : theme::Mode::Light) : theme::storedMode());

    if (cli.isSet(unitsOpt)) {
        const QString u = cli.value(unitsOpt);
        UnitPreferences::instance().setSystem(u == QLatin1String("metric") ? grt::units::System::Metric
                                              : u == QLatin1String("imperial") ? grt::units::System::Imperial : grt::units::System::Original);
    }
    MainWindow w;
    if (cli.isSet(dbOpt)) w.openDatabase(cli.value(dbOpt));
    const QStringList loads = cli.positionalArguments();
    for (const QString &f : loads) w.openLoad(f);
    if (cli.isSet(refOpt) && !loads.isEmpty()) w.overlayReference(cli.value(refOpt));
    w.show();
    if (cli.isSet(shotOpt)) {
        QTimer::singleShot(1200, &w, [&] {
            if (cli.isSet(tabOpt)) w.showTab(cli.value(tabOpt).toInt());
            if (cli.isSet(resultTabOpt)) w.showResultTab(cli.value(resultTabOpt).toInt());
            QTimer::singleShot(600, &w, [&] {
                w.grab().save(cli.value(shotOpt));
                a.quit();
            });
        });
    }
    return a.exec();
}
