#include "disclaimer.h"
#include "mainwindow.h"
#include "session.h"
#include "solverprefs.h"
#include "appfont.h"
#include "log.h"
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
    cli.addPositionalArgument(QStringLiteral("load"), QStringLiteral("*.grtreload (or the original's *.grtload) files to open"),
                              QStringLiteral("[load...]"));
    const QCommandLineOption dbOpt({QStringLiteral("d"), QStringLiteral("database")}, QStringLiteral("SQLite database to use"),
                                   QStringLiteral("file"));
    const QCommandLineOption refOpt({QStringLiteral("r"), QStringLiteral("reference")},
                                    QStringLiteral("reference result (*.json) to overlay on the first load"), QStringLiteral("file"));
    const QCommandLineOption shotOpt(QStringLiteral("screenshot"), QStringLiteral("save a screenshot of the window and exit"),
                                     QStringLiteral("png"));
    const QCommandLineOption tabOpt(QStringLiteral("tab"), QStringLiteral("tab index to show for --screenshot"), QStringLiteral("index"));
    const QCommandLineOption resultTabOpt(QStringLiteral("result-tab"),
                                          QStringLiteral("result tab to show for --screenshot (0 results, 1 measurements, 2 report)"),
                                          QStringLiteral("index"));
    const QCommandLineOption inputTabOpt(
        QStringLiteral("input-tab"), QStringLiteral("input tab to show for --screenshot (0 cartridge && gun, 1 projectile, 2 propellant)"),
        QStringLiteral("index"));
    const QCommandLineOption langOpt(QStringLiteral("language"), QStringLiteral("user interface language, e.g. en_US or de_DE"),
                                     QStringLiteral("lang"));
    const QCommandLineOption themeOpt(QStringLiteral("theme"), QStringLiteral("light or dark (overrides the saved choice)"),
                                      QStringLiteral("mode"));
    const QCommandLineOption unitsOpt(QStringLiteral("units"), QStringLiteral("unit system: metric, imperial or original (saved)"),
                                      QStringLiteral("system"));
    const QCommandLineOption calibrationOpt(
        QStringLiteral("calibration"),
        QStringLiteral("solver calibration for this run: original, published or physical (overrides the settings)"),
        QStringLiteral("profile"));
    const QCommandLineOption logOpt(QStringLiteral("log-level"),
                                    QStringLiteral("log level for this run: debug, info, warning, error or off (overrides the settings)"),
                                    QStringLiteral("level"));
    cli.addOptions({dbOpt, refOpt, shotOpt, tabOpt, resultTabOpt, inputTabOpt, langOpt, themeOpt, unitsOpt, calibrationOpt, logOpt});
    cli.process(a);
    grt::log::Config logConfig = grt::log::storedConfig();
    if (cli.isSet(logOpt))
        if (const auto level = grt::log::levelFromName(cli.value(logOpt))) logConfig.level = *level;
    grt::log::setConfig(logConfig);
    grt::log::installMessageHandler();
    LOG_Info() << "GRT Reloaded" << GRT_VERSION << "started";
    appfont::apply(a, appfont::storedSize());  // the platform font is remembered before the first change
    translation::install(a, cli.value(langOpt));
    theme::apply(a, cli.isSet(themeOpt) ? (cli.value(themeOpt) == QLatin1String("dark") ? theme::Mode::Dark : theme::Mode::Light)
                                        : theme::storedMode());

    if (cli.isSet(unitsOpt))
    {
        const QString u = cli.value(unitsOpt);
        UnitPreferences::instance().setSystem(u == QLatin1String("metric")     ? grt::units::System::Metric
                                              : u == QLatin1String("imperial") ? grt::units::System::Imperial
                                                                               : grt::units::System::Original);
    }
    if (cli.isSet(calibrationOpt))
        if (const auto profile = SolverPreferences::fromKey(cli.value(calibrationOpt)))
            SolverPreferences::instance().setProfile(*profile, false);
    MainWindow w;
    if (cli.isSet(dbOpt)) w.openDatabase(cli.value(dbOpt));
    const QStringList loads = cli.positionalArguments();
    for (const QString &f : loads) w.openLoad(f);
    if (loads.isEmpty() && !cli.isSet(shotOpt) && session::reopen()) w.restoreSession();  // Settings > Start-up
    if (cli.isSet(refOpt) && !loads.isEmpty()) w.overlayReference(cli.value(refOpt));
    w.show();
    // the safety notice over the main window, once it is on the screen (not in the screenshot mode: no one to answer it)
    if (!cli.isSet(shotOpt))
        QTimer::singleShot(0, &w,
                           [&]
                           {
                               if (!disclaimer::confirm(&w)) a.quit();
                           });
    if (cli.isSet(shotOpt))
    {
        QTimer::singleShot(1200, &w,
                           [&]
                           {
                               if (cli.isSet(tabOpt)) w.showTab(cli.value(tabOpt).toInt());
                               if (cli.isSet(inputTabOpt)) w.showInputTab(cli.value(inputTabOpt).toInt());
                               if (cli.isSet(resultTabOpt)) w.showResultTab(cli.value(resultTabOpt).toInt());
                               QTimer::singleShot(600, &w,
                                                  [&]
                                                  {
                                                      w.grab().save(cli.value(shotOpt));
                                                      a.quit();
                                                  });
                           });
    }
    return a.exec();
}
