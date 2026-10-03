#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QScrollBar>
#include <QTextBrowser>
#include <QSplitter>
#include <QSignalSpy>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSpinBox>
#include <QChartView>
#include <QChart>
#include <QAbstractAxis>
#include <QPushButton>
#include <QTabBar>
#include <QTabWidget>
#include <QSettings>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QTableView>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QProxyStyle>
#include <QStyleOptionComboBox>
#include <QTimer>
#include <QtTest>

#include "about.h"
#include "burnratedialog.h"
#include "theme.h"
#include "flags.h"
#include "titlebar.h"
#include "appfont.h"
#include "cartridgeview.h"
#include "chamberview.h"
#include "photoview.h"
#include "chronographimportdialog.h"
#include "database.h"
#include "loadbuilder.h"
#include "loadview.h"
#include "recorddialog.h"
#include "measurement.h"
#include "projectdata.h"
#include "measurementview.h"
#include "powderfitdialog.h"
#include "propertypanel.h"
#include "recentfiles.h"
#include "reportview.h"
#include "selectrecorddialog.h"
#include "settingsdialog.h"
#include "solverprefs.h"
#include "unitprefs.h"
#include "disclaimer.h"
#include "mainwindow.h"
#include "newloaddialog.h"
#include "solver.h"
#include "tablebrowser.h"

#include <optional>

using namespace Qt::StringLiterals;

namespace
{

// the buttons of the input tabs: "Select Caliber…", "Select Projectile…", "Select Propellant…"
bool isSelectButton(const QPushButton *b)
{
    const QString t = b->text();
    return t == u"Select Caliber…"_s || t == u"Select Projectile…"_s || t == u"Select Propellant…"_s;
}

}  // namespace

class GuiTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void tableBrowserFiltersAndSelects();
    void rowFilterRestrictsTheTable();
    void newLoadDialogBuildsARunnableLoad();
    void newLoadDialogNeedsAllThreeRecords();
    void codedFieldsShowReadableText();
    void panelShowsAndEditsInTheChosenUnit();
    void perFieldUnitOverridesTheSystem();
    void resultsFollowTheUnitSystem();
    void ladderCurvesAndFocus();
    void fontSizeIsLimitedAndStored();
    void fontSizeAppliesToTheWholeApplication();
    void settingsDialogChangesAndRevertsTheFont();
    void fileMenuHasSettingsAndPrinting();
    void helpMenuHasAbout();
    void unitsLiveInSettingsNotInViewMenu();
    void layoutIsRemembered();
    void recentFilesAreListedInTheFileMenu();
    void hoveringATableRowShowsItsMeasureInTheDrawing();
    void reportTabRendersTheLoad();
    void reportTemplateIsCustomisable();
    void actionButtonsAreEasyToSee();
    void reportScaleIsLimitedAndStored();
    void reportScaleChangesTheLayout();
    void propertyNamesStartWithACapital();
    void reportExportsAPdf();
    void inputsAreGroupedInThreeTabs();
    void componentsCanBeReplacedAfterSetup();
    void chamberGeometryFollowsTheDimensions();
    void drawingPanelHasCartridgeAndChamberTabs();
    void cartridgeGeometryMatchesTheCaseCapacity();
    void cartridgeViewDrawsAndHandlesStraightCases();
    void measurementTabShowsTheFilesResults();
    void measurementsAreEditedAndSaved();
    void measurementSeriesAreEditedAndSaved();
    void chronographImportAddsCharges();
    void propellantPhotoIsShown();
    void ignoredInputsAreNamed();
    void recordDialogDatesAndWidth();
    void ownPropellantWithPhotoWorksWithoutTheOriginal();
    void disclaimerAtStartup();
    void fitToMeasurementsChangesThePowder();
    void originalDatabaseIsChosenInTheSettings();
    void burnRateTableSortsAndFilters();
    void solverCalibrationIsChosenInTheSettings();
    void overallLengthAndSeatingDepthStayInStep();
    void recordDialogShowsReadableFieldsInUserUnits();
    void recordsAreAddedCopiedEditedAndDeleted();
    void comboBoxesOpenAsAScrollingList();
    void loadMenuEntriesNeedALoad();
    void ownTitleBarCanBeSwitched();
    void languagesHaveFlags();

private:
    static int idOf(grt::Database &db, grt::Table t, const QString &where);

    grt::Database m_db;
};

void GuiTest::initTestCase()
{
    // never touch the user's real settings or own database
    QStandardPaths::setTestModeEnabled(true);
    QFile::remove(grt::Database::defaultOwnPath());  // start every run with an empty own database
    static QTemporaryDir settingsDir;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDir.path());
    QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));
    UnitPreferences::instance().resetFieldUnits();
    UnitPreferences::instance().setSystem(grt::units::System::Original);

    const QString dbPath = QStringLiteral(GRT_SOURCE_DIR "/tools/GordonsReloadingTool.plain.db");
    if (!QFile::exists(dbPath)) QSKIP("plain database not available");
    QVERIFY(m_db.open(dbPath));
}

int GuiTest::idOf(grt::Database &db, grt::Table t, const QString &where)
{
    QSqlQuery q(db.handle());
    if (q.exec(u"SELECT id FROM %1 WHERE geloescht = 0 AND %2 ORDER BY id"_s.arg(QLatin1String(grt::tableName(t)), where)) && q.next())
        return q.value(0).toInt();
    return -1;
}

void GuiTest::burnRateTableSortsAndFilters()
{
    BurnRateDialog dialog(m_db);
    const int all = dialog.rowCount();
    QVERIFY(all > 50);
    const auto sortedDescending = [&](BurnRateDialog::Factor f)
    {
        for (int i = 1; i < dialog.rowCount(); ++i)
            if (dialog.valueAt(i - 1, f) < dialog.valueAt(i, f)) return false;
        return true;
    };
    QCOMPARE(dialog.factor(), BurnRateDialog::Progressivity);
    QVERIFY(sortedDescending(BurnRateDialog::Progressivity));
    for (const auto f : {BurnRateDialog::Brisance, BurnRateDialog::Combined})
    {
        dialog.setFactor(f);
        QCOMPARE(dialog.factor(), f);
        QVERIFY(sortedDescending(f));
    }
    // the project's records are in the table with derived factors, one row per powder
    QStringList names;
    for (int i = 0; i < all; ++i) names << dialog.powderAt(i);
    QCOMPARE(names.count(u"Vihtavuori N150"_s), 1);
    dialog.setSearch(u"vihtavuori"_s);
    QVERIFY(dialog.rowCount() >= 16 && dialog.rowCount() < all);
    for (int i = 0; i < dialog.rowCount(); ++i) QVERIFY(dialog.powderAt(i).startsWith(u"Vihtavuori"_s));
    dialog.setSearch(u"no such powder"_s);
    QCOMPARE(dialog.rowCount(), 0);
}

void GuiTest::solverCalibrationIsChosenInTheSettings()
{
    auto &prefs = SolverPreferences::instance();
    QCOMPARE(prefs.profile(), grt::CalibrationProfile::Physical);  // the default: the own physical model
    prefs.setProfile(grt::CalibrationProfile::Original);
    SettingsDialog dlg;
    QComboBox *calibration = nullptr;
    for (auto *c : dlg.findChildren<QComboBox *>())
        if (c->findText(SolverPreferences::name(grt::CalibrationProfile::Published)) >= 0) calibration = c;
    QVERIFY(calibration);
    QSignalSpy changed(&prefs, &SolverPreferences::changed);
    calibration->setCurrentIndex(calibration->findText(SolverPreferences::name(grt::CalibrationProfile::Published)));
    dlg.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
    QCOMPARE(prefs.profile(), grt::CalibrationProfile::Published);
    QCOMPARE(grt::calibrationProfile(), grt::CalibrationProfile::Published);
    QCOMPARE(changed.count(), 1);
    QCOMPARE(QSettings().value(u"solver/calibration"_s).toString(), u"published"_s);
    prefs.setProfile(grt::CalibrationProfile::Original);  // leave the others as they were
    QCOMPARE(grt::calibrationProfile(), grt::CalibrationProfile::Original);

    // an open load is recalculated with the other constants (the shown result comes from the ladder simulation)
    LoadView view;
    QString error;
    QVERIFY2(view.load(QStringLiteral(GRT_TEST_DATA_DIR "/8x57is.grtload"), &error), qPrintable(error));
    const double original = view.result().maxPressure_bar;
    QVERIFY(original > 0.0);
    prefs.setProfile(grt::CalibrationProfile::Published);
    const double published = view.result().maxPressure_bar;
    QVERIFY(std::abs(published / original - 1.0) > 0.01);
    prefs.setProfile(grt::CalibrationProfile::Original);
    QCOMPARE(view.result().maxPressure_bar, original);
    prefs.setProfile(grt::CalibrationProfile::Physical, false);  // the default again, for the other tests
}

void GuiTest::tableBrowserFiltersAndSelects()
{
    TableBrowser b(grt::Table::Propellant);
    b.setDatabase(&m_db);
    QVERIFY(b.selectId(562));
    QCOMPARE(b.currentId(), 562);
    QVERIFY(!b.selectId(-1));
    b.setDatabase(nullptr);
    QCOMPARE(b.currentId(), -1);
}

void GuiTest::rowFilterRestrictsTheTable()
{
    TableBrowser b(grt::Table::Projectile);
    b.setDatabase(&m_db);
    QVERIFY(b.selectId(4542));  // Sellier-Bellot FMJ-BT 2910, .323
    b.setRowFilter(u"ABS(gdia - 4.5) < 0.25"_s);
    QVERIFY(!b.selectId(4542));  // no longer listed
    b.setRowFilter({});
    QVERIFY(b.selectId(4542));
}

void GuiTest::newLoadDialogBuildsARunnableLoad()
{
    NewLoadDialog dlg(m_db);
    const int cal = idOf(m_db, grt::Table::Caliber, u"cipname = '8 x 57 IS'"_s);
    const int prj = idOf(m_db, grt::Table::Projectile, u"mname = 'Sellier-Bellot' AND pname = 'FMJ-BT 2910'"_s);
    const int pro = idOf(m_db, grt::Table::Propellant, u"mname = 'Vihtavuori' AND pname = 'N150' AND lotid = '2021-11-13'"_s);
    QVERIFY(cal > 0 && prj > 0 && pro > 0);
    QVERIFY(dlg.select(cal, prj, pro));

    const grt::LoadFile lf = dlg.result();
    QCOMPARE(lf.caliber.text(u"CaliberName"), u"8 x 57 IS (8 x 57 JS)"_s);
    QCOMPARE(lf.propellant.text(u"pname"), u"N150"_s);
    QCOMPARE(lf.gun.number(u"xe"), 600.0);                            // rifle default
    QVERIFY(std::abs(lf.projectile.number(u"gdepth") - 7.9) < 0.01);  // default: cartridge at its maximum length (57 + 32.9 - 82)
    const grt::SimResult r = grt::simulate(grt::SimInput::fromLoad(lf));
    QVERIFY(r.ok());
    QVERIFY(r.maxPressure_bar > 1000.0 && r.maxPressure_bar < 5000.0);
    dlg.grab().save(QDir::temp().filePath(u"grt_newload_dialog.png"_s));
}

void GuiTest::newLoadDialogNeedsAllThreeRecords()
{
    NewLoadDialog dlg(m_db);
    QVERIFY(dlg.result().propellant.entries().empty());  // nothing selected yet
}

namespace
{

QTableWidget *tableOf(PropertyPanel &p)
{
    return p.findChild<QTableWidget *>();
}

// Row of the table whose value cell belongs to the entry `key`.
int rowOf(QTableWidget *t, const QString &key)
{
    for (int r = 0; r < t->rowCount(); ++r)
        if (t->item(r, 1) && t->item(r, 1)->data(Qt::UserRole + 1).toString() == key) return r;
    return -1;
}

}  // namespace

void GuiTest::codedFieldsShowReadableText()
{
    grt::EntryList list;
    const auto add = [&](const char *name, const char *value, const char *descr)
    {
        list.entries().push_back({.name = QString::fromLatin1(name),
                                  .value = QString::fromLatin1(value),
                                  .unit = {},
                                  .type = u"decimal"_s,
                                  .descr = QString::fromLatin1(descr)});
    };
    add("gTailType", "2", "projectile base type");
    add("gfrictionUnlock", "false", "unlocked friction");
    add("shotshell", "true", "true if shotshell");
    PropertyPanel panel;
    panel.setSections({{0, u"projectile"_s, u"Projectile"_s, &list}});
    QTableWidget *t = tableOf(panel);
    const int rTail = rowOf(t, u"gTailType"_s), rLock = rowOf(t, u"gfrictionUnlock"_s), rShell = rowOf(t, u"shotshell"_s);
    QCOMPARE(t->item(rTail, 1)->text(), u"boat tail"_s);  // not "2"
    QCOMPARE(t->item(rLock, 1)->text(), u"locked"_s);
    QCOMPARE(t->item(rShell, 1)->text(), u"yes"_s);
    QCOMPARE(t->item(rShell, 0)->text(), u"Shotshell"_s);
    QCOMPARE(t->item(rTail, 0)->text(), u"Projectile Base Type"_s);

    // choosing from the drop-down stores the code again
    QSignalSpy spy(&panel, &PropertyPanel::valueEdited);
    t->item(rTail, 1)->setText(u"hollow base"_s);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(1).toString(), u"gTailType"_s);
    QCOMPARE(spy.at(0).at(2).toString(), u"1"_s);
    QCOMPARE(t->item(rTail, 1)->text(), u"hollow base"_s);
    t->item(rLock, 1)->setText(u"unlocked"_s);
    QCOMPARE(spy.at(1).at(2).toString(), u"true"_s);
    // something that is not a choice is ignored and the old text is restored
    t->item(rTail, 1)->setText(u"banana"_s);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(t->item(rTail, 1)->text(), u"hollow base"_s);

    // a code the program does not know stays visible instead of being hidden
    grt::EntryList odd;
    odd.entries().push_back({.name = u"gTailType"_s, .value = u"3"_s, .unit = {}, .type = u"decimal"_s, .descr = u"base"_s});
    PropertyPanel panel2;
    panel2.setSections({{0, u"projectile"_s, u"Projectile"_s, &odd}});
    QCOMPARE(tableOf(panel2)->item(rowOf(tableOf(panel2), u"gTailType"_s), 1)->text(), u"type 3"_s);
}

// The solver uses the seating depth; the cartridge overall length is derived from it, so editing the length must change the depth.
void GuiTest::overallLengthAndSeatingDepthStayInStep()
{
    UnitPreferences::instance().setSystem(grt::units::System::Original);  // mm
    LoadView view;
    QString error;
    QVERIFY2(view.load(QStringLiteral(GRT_TEST_DATA_DIR "/8x57is.grtload"), &error), qPrintable(error));
    const auto edit = [&](const QString &key, const QString &text)
    {
        for (PropertyPanel *panel : view.findChildren<PropertyPanel *>())
        {
            QTableWidget *t = tableOf(*panel);
            if (const int row = rowOf(t, key); row > 0) return t->item(row, 1)->setText(text);
        }
        QFAIL("field not found");
    };
    const auto saved = [&]
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(u"x.grtload"_s);
        QString err;
        if (!view.save(path, &err)) return std::optional<grt::LoadFile>{};
        return grt::LoadFile::read(path);
    };
    auto lf = saved();
    QVERIFY(lf);
    QCOMPARE(lf->caliber.number(u"oal"_s), 79.5);
    QCOMPARE(lf->projectile.number(u"gdepth"_s), 10.4);

    edit(u"oal"_s, u"78.5"_s);  // a shorter cartridge: the bullet sits deeper
    lf = saved();
    QVERIFY(lf);
    QVERIFY(std::abs(lf->projectile.number(u"gdepth"_s) - 11.4) < 1e-6);
    QCOMPARE(lf->caliber.number(u"oal"_s), 78.5);

    edit(u"gdepth"_s, u"9.4"_s);  // and the other way round
    lf = saved();
    QVERIFY(lf);
    QVERIFY(std::abs(lf->caliber.number(u"oal"_s) - 80.5) < 1e-6);

    // the initial gas pressure exists in two sections; the solver uses the caliber's
    edit(u"gpressure"_s, u"400"_s);
    lf = saved();
    QVERIFY(lf);
    QCOMPARE(lf->caliber.number(u"ps"_s), 400.0);
    edit(u"ps"_s, u"300"_s);
    lf = saved();
    QVERIFY(lf);
    QCOMPARE(lf->projectile.number(u"gpressure"_s), 300.0);
}

void GuiTest::panelShowsAndEditsInTheChosenUnit()
{
    using grt::units::System;
    grt::EntryList list;
    list.entries().push_back(
        {.name = u"mp"_s, .value = u"12.700586359951799"_s, .unit = u"g"_s, .type = u"decimal"_s, .descr = u"projectile weight"_s});
    list.entries().push_back(
        {.name = u"Dbul"_s, .value = u"8.2"_s, .unit = u"mm"_s, .type = u"decimal"_s, .descr = u"projectile diameter"_s});
    list.entries().push_back({.name = u"Qex"_s, .value = u"3790"_s, .unit = u"kJ/kg"_s, .type = u"decimal"_s, .descr = u"heat"_s});
    PropertyPanel panel;
    panel.setSections({{0, u"projectile"_s, u"Projectile"_s, &list}});
    QTableWidget *t = tableOf(panel);
    const int rMp = rowOf(t, u"mp"_s), rD = rowOf(t, u"Dbul"_s), rQ = rowOf(t, u"Qex"_s);
    QVERIFY(rMp > 0 && rD > 0 && rQ > 0);

    UnitPreferences::instance().setSystem(System::Original);
    QCOMPARE(t->item(rMp, 1)->text(), u"196.00"_s);
    QCOMPARE(t->item(rMp, 2)->text(), u"grain"_s);
    QCOMPARE(t->item(rD, 1)->text(), u"8.20"_s);
    UnitPreferences::instance().setSystem(System::Metric);
    QCOMPARE(t->item(rMp, 1)->text(), u"12.701"_s);
    QCOMPARE(t->item(rMp, 2)->text(), u"g"_s);
    UnitPreferences::instance().setSystem(System::Imperial);
    QCOMPARE(t->item(rD, 1)->text(), u"0.323"_s);
    QCOMPARE(t->item(rD, 2)->text(), u"in"_s);
    QCOMPARE(t->item(rQ, 1)->text(), u"3790"_s);  // kJ/kg has no alternatives: untouched
    QCOMPARE(t->item(rQ, 2)->text(), u"kJ/kg"_s);

    // typing in inches stores millimetres (the file's unit)
    QSignalSpy spy(&panel, &PropertyPanel::valueEdited);
    t->item(rD, 1)->setText(u"0.5"_s);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(1).toString(), u"Dbul"_s);
    QVERIFY(std::abs(spy.at(0).at(2).toString().toDouble() - 12.7) < 1e-9);
    QCOMPARE(t->item(rD, 1)->text(), u"0.500"_s);  // shown again in inches

    // garbage is rejected and the old value comes back
    t->item(rD, 1)->setText(u"abc"_s);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(t->item(rD, 1)->text(), u"0.500"_s);
    UnitPreferences::instance().setSystem(System::Original);
}

void GuiTest::perFieldUnitOverridesTheSystem()
{
    using grt::units::Quantity;
    using grt::units::System;
    grt::EntryList list;
    list.entries().push_back(
        {.name = u"mp"_s, .value = u"12.700586359951799"_s, .unit = u"g"_s, .type = u"decimal"_s, .descr = u"weight"_s});
    list.entries().push_back({.name = u"glen"_s, .value = u"25.4"_s, .unit = u"mm"_s, .type = u"decimal"_s, .descr = u"length"_s});
    PropertyPanel panel;
    panel.setSections({{0, u"projectile"_s, u"Projectile"_s, &list}});
    QTableWidget *t = tableOf(panel);
    UnitPreferences &prefs = UnitPreferences::instance();
    prefs.setSystem(System::Metric);
    const int rMp = rowOf(t, u"mp"_s), rLen = rowOf(t, u"glen"_s);

    prefs.setFieldUnit(u"projectile/glen"_s, Quantity::Length, u"in"_s);  // only this field in inches
    QCOMPARE(t->item(rLen, 1)->text(), u"1.000"_s);
    QCOMPARE(t->item(rMp, 2)->text(), u"g"_s);  // other fields keep the system's unit
    prefs.setSystem(System::Imperial);
    QCOMPARE(t->item(rLen, 2)->text(), u"in"_s);
    prefs.setFieldUnit(u"projectile/mp"_s, Quantity::BulletMass, u"oz"_s);
    QCOMPARE(t->item(rMp, 2)->text(), u"oz"_s);
    QVERIFY(prefs.hasOverride(u"projectile/mp"_s));

    // the override survives a restart (settings are persisted)
    QCOMPARE(QSettings().value(u"units/field/projectile/mp"_s).toString(), u"oz"_s);
    prefs.resetFieldUnits();
    QCOMPARE(t->item(rMp, 2)->text(), u"grain"_s);
    prefs.setSystem(System::Original);
}

void GuiTest::resultsFollowTheUnitSystem()
{
    using grt::units::System;
    LoadView view;
    QString error;
    QVERIFY2(view.load(QStringLiteral(GRT_TEST_DATA_DIR "/8x57is.grtload"), &error), qPrintable(error));
    const auto allText = [&]
    {
        QStringList s;
        for (const QLabel *l : view.findChildren<QLabel *>()) s << l->text();
        return s.join(u'\n');
    };
    UnitPreferences::instance().setSystem(System::Metric);
    QVERIFY(allText().contains(u" bar"_s));
    QVERIFY(allText().contains(u" m/s"_s));
    UnitPreferences::instance().setSystem(System::Imperial);
    const QString imperial = allText();
    QVERIFY2(imperial.contains(u" psi"_s), qPrintable(imperial));
    QVERIFY(imperial.contains(u" ft/s"_s));
    QVERIFY(imperial.contains(u" ft·lbf"_s));
    QVERIFY(!imperial.contains(u" bar"_s));
    UnitPreferences::instance().setSystem(System::Original);
}

void GuiTest::ladderCurvesAndFocus()
{
    QTemporaryDir dir;
    auto lf = grt::LoadFile::read(QStringLiteral(GRT_TEST_DATA_DIR "/8x57is.grtload"));
    QVERIFY(lf);
    const double mc = lf->propellant.number(u"mc");
    lf->propellant.set(u"laddercnt", 4.0);
    lf->propellant.set(u"laddermc", 0.2);
    const QString path = dir.filePath(u"ladder.grtload"_s);
    QVERIFY(lf->write(path));

    LoadView view;
    QString error;
    QVERIFY2(view.load(path, &error), qPrintable(error));
    view.resize(1400, 900);
    view.show();
    QCOMPARE(view.ladderSize(), 4);
    QCOMPARE(view.ladderFocus(), 0);
    QVERIFY(std::abs(view.focusCharge_g() - mc) < 1e-12);

    // a tab per charge above the chart and a row per charge in the comparison table
    QTabBar *bar = nullptr;
    for (QTabBar *b : view.findChildren<QTabBar *>())
        if (b->count() == 4) bar = b;
    QVERIFY(bar && bar->isVisible());
    QTableWidget *table = nullptr;
    for (QTableWidget *w : view.findChildren<QTableWidget *>())
        if (w->columnCount() == 5 && w->rowCount() == 4) table = w;
    QVERIFY(table && table->isVisible());
    const QString maxP0 = table->item(0, 1)->text(), maxP3 = table->item(3, 1)->text();
    QVERIFY(maxP0 != maxP3);

    // selecting a lower charge: results follow, the file is modified, and the choice is saved
    QVERIFY(!view.isModified());
    bar->setCurrentIndex(2);
    QCOMPARE(view.ladderFocus(), 2);
    QVERIFY(view.isModified());
    QVERIFY(std::abs(view.focusCharge_g() - (mc - 0.4)) < 1e-12);
    const QString out = dir.filePath(u"saved.grtload"_s);
    QVERIFY(view.save(out, &error));
    const auto saved = grt::LoadFile::read(out);
    QVERIFY(saved);
    QCOMPARE(saved->config.number(u"CurveFocusIdx"), 2.0);
    // reopening shows the same curve selected
    LoadView again;
    QVERIFY(again.load(out, &error));
    QCOMPARE(again.ladderFocus(), 2);

    // clicking a table row selects too; out-of-range requests and no-ops change nothing
    table->selectRow(1);
    QTest::mouseClick(table->viewport(), Qt::LeftButton, Qt::NoModifier, table->visualItemRect(table->item(1, 0)).center());
    QCOMPARE(view.ladderFocus(), 1);
    view.selectLadderStep(9);
    view.selectLadderStep(-1);
    QCOMPARE(view.ladderFocus(), 1);

    // a stored focus beyond the number of curves is clamped to the last one
    lf->config.set(u"CurveFocusIdx", 7.0);
    QVERIFY(lf->write(path));
    LoadView clamped;
    QVERIFY(clamped.load(path, &error));
    QCOMPARE(clamped.ladderFocus(), 3);

    // switching the ladder off removes the controls
    LoadView plain;
    QVERIFY(plain.load(QStringLiteral(GRT_TEST_DATA_DIR "/8x57is.grtload"), &error));
    plain.show();
    QCOMPARE(plain.ladderSize(), 1);
    // without a ladder the tab strip and the comparison table are hidden
    for (QTabBar *b : plain.findChildren<QTabBar *>())
        if (b->count() == 0) QVERIFY(!b->isVisible());
    for (QTableWidget *w : plain.findChildren<QTableWidget *>())
        if (w->columnCount() == 5 && w->horizontalHeaderItem(0) && w->horizontalHeaderItem(0)->text() == u"Charge"_s)
            QVERIFY(!w->isVisible());
}

void GuiTest::fontSizeIsLimitedAndStored()
{
    QCOMPARE(appfont::clampSize(3), appfont::kMinSize);
    QCOMPARE(appfont::clampSize(-5), appfont::kMinSize);
    QCOMPARE(appfont::clampSize(99), appfont::kMaxSize);
    QCOMPARE(appfont::clampSize(12), 12);
    QVERIFY(appfont::kMinSize >= 6 && appfont::kMaxSize <= 24 && appfont::kMinSize < appfont::kMaxSize);
    const int system = appfont::systemSize();
    QVERIFY(system >= appfont::kMinSize && system <= appfont::kMaxSize);

    appfont::storeSize(system);  // the default is no setting at all
    QVERIFY(!appfont::hasCustomSize());
    QCOMPARE(appfont::storedSize(), system);
    appfont::storeSize(system == 14 ? 15 : 14);
    QVERIFY(appfont::hasCustomSize());
    QCOMPARE(appfont::storedSize(), system == 14 ? 15 : 14);
    appfont::storeSize(500);  // out of range is limited when stored ...
    QCOMPARE(appfont::storedSize(), appfont::kMaxSize);
    QSettings().setValue(u"ui/fontSize"_s, 2);  // ... and when read back from a hand-edited file
    QCOMPARE(appfont::storedSize(), appfont::kMinSize);
    QSettings().setValue(u"ui/fontSize"_s, u"abc"_s);  // garbage falls back to the default
    QCOMPARE(appfont::storedSize(), system);
    appfont::storeSize(system);
    QVERIFY(!appfont::hasCustomSize());
}

void GuiTest::fontSizeAppliesToTheWholeApplication()
{
    auto &app = *qobject_cast<QApplication *>(QCoreApplication::instance());
    const int system = appfont::systemSize();
    const QString family = QApplication::font().family();
    QCOMPARE(appfont::apply(app, 16), 16);
    QCOMPARE(static_cast<int>(QApplication::font().pointSizeF()), 16);
    QCOMPARE(QApplication::font().family(), family);  // only the size changes
    // widgets created later and the existing ones use it
    QLabel label(u"x"_s);
    QCOMPARE(static_cast<int>(label.font().pointSizeF()), 16);
    QCOMPARE(appfont::apply(app, 100), appfont::kMaxSize);
    QCOMPARE(appfont::apply(app, 1), appfont::kMinSize);

    // charts have their own fonts and follow: the axis labels grow with the application font
    LoadView view;
    QString error;
    QVERIFY2(view.load(QStringLiteral(GRT_TEST_DATA_DIR "/8x57is.grtload"), &error), qPrintable(error));
    view.show();
    const auto axisLabelSize = [&]
    {
        for (QChartView *cv : view.findChildren<QChartView *>())
            if (cv->chart()->axes(Qt::Vertical).size() >= 3) return cv->chart()->axes(Qt::Vertical).first()->labelsFont().pointSizeF();
        return -1.0;
    };
    appfont::apply(app, 10);
    QApplication::processEvents();
    const double at10 = axisLabelSize();
    appfont::apply(app, 18);
    QApplication::processEvents();
    const double at18 = axisLabelSize();
    QVERIFY2(at10 > 0 && at18 > at10 * 1.5, qPrintable(u"%1 -> %2"_s.arg(at10).arg(at18)));
    appfont::apply(app, system);
}

void GuiTest::settingsDialogChangesAndRevertsTheFont()
{
    auto &app = *qobject_cast<QApplication *>(QCoreApplication::instance());
    const int system = appfont::systemSize();
    appfont::storeSize(system);
    appfont::apply(app, system);
    {  // OK applies and stores
        SettingsDialog dlg;
        auto *spin = dlg.findChild<QSpinBox *>();
        QVERIFY(spin);
        QCOMPARE(spin->minimum(), appfont::kMinSize);  // the dialog cannot leave the allowed range
        QCOMPARE(spin->maximum(), appfont::kMaxSize);
        QCOMPARE(spin->value(), system);
        spin->setValue(system == 13 ? 14 : 13);
        const int chosen = spin->value();
        auto *buttons = dlg.findChild<QDialogButtonBox *>();
        QVERIFY(buttons);
        buttons->button(QDialogButtonBox::Ok)->click();
        QCOMPARE(dlg.result(), static_cast<int>(QDialog::Accepted));
        QCOMPARE(static_cast<int>(QApplication::font().pointSizeF()), chosen);
        QCOMPARE(appfont::storedSize(), chosen);
    }
    {  // Apply is live; Cancel afterwards puts everything back
        SettingsDialog dlg;
        auto *spin = dlg.findChild<QSpinBox *>();
        const int before = static_cast<int>(QApplication::font().pointSizeF());
        spin->setValue(before == 18 ? 17 : 18);
        dlg.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
        QCOMPARE(static_cast<int>(QApplication::font().pointSizeF()), spin->value());
        dlg.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Cancel)->click();
        QCOMPARE(static_cast<int>(QApplication::font().pointSizeF()), before);
        QCOMPARE(appfont::storedSize(), before);
    }
    {  // the default button
        SettingsDialog dlg;
        auto *spin = dlg.findChild<QSpinBox *>();
        spin->setValue(spin->maximum());
        for (QPushButton *b : dlg.findChildren<QPushButton *>())
            if (b->text().contains(u"Default"_s)) b->click();
        QCOMPARE(spin->value(), system);
    }
    appfont::storeSize(system);
    appfont::apply(app, system);
}

void GuiTest::fileMenuHasSettingsAndPrinting()
{
    MainWindow w;
    QMenu *file = nullptr;
    for (QAction *a : w.mainMenu()->actions())
        if (a->menu() && a->text().remove(u'&') == u"File"_s) file = a->menu();
    QVERIFY(file);
    QStringList items;
    for (QAction *a : file->actions()) items << a->text().remove(u'&');
    QVERIFY2(items.contains(u"Settings…"_s), qPrintable(items.join(u'|')));
    QVERIFY(items.contains(u"Print…"_s));
    QVERIFY(items.contains(u"Print preview…"_s));
    QVERIFY(items.contains(u"Export report as PDF…"_s));
    // settings sit above Quit
    QVERIFY(items.indexOf(u"Settings…"_s) < items.indexOf(u"Quit"_s));
}

void GuiTest::unitsLiveInSettingsNotInViewMenu()
{
    MainWindow w;
    for (QAction *a : w.mainMenu()->actions())
        if (a->menu() && a->text().remove(u'&') == u"View"_s)
            for (QAction *item : a->menu()->actions()) QVERIFY(item->text().remove(u'&') != u"Units"_s);

    const auto original = UnitPreferences::instance().system();
    {
        SettingsDialog dlg;
        auto *combo = dlg.findChild<QComboBox *>();
        QVERIFY(combo);
        dlg.show();
        const int target = combo->findData(
            static_cast<int>(original == grt::units::System::Imperial ? grt::units::System::Metric : grt::units::System::Imperial));
        QVERIFY(target >= 0);
        combo->setCurrentIndex(target);
        dlg.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
        QVERIFY(UnitPreferences::instance().system() != original);
        dlg.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Cancel)->click();  // Cancel undoes what Apply did
    }
    QCOMPARE(UnitPreferences::instance().system(), original);
}

void GuiTest::hoveringATableRowShowsItsMeasureInTheDrawing()
{
    LoadView view;
    QString error;
    QVERIFY2(view.load(QStringLiteral(GRT_TEST_DATA_DIR "/8x57is.grtload"), &error), qPrintable(error));
    view.resize(1400, 900);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    for (QWidget *w : {static_cast<QWidget *>(view.findChild<CartridgeView *>()), static_cast<QWidget *>(view.findChild<ChamberView *>())})
    {
        QVERIFY(w);
        if (auto *tabs = qobject_cast<QTabWidget *>(w->parentWidget()->parentWidget())) tabs->setCurrentWidget(w);
        QTest::qWait(50);
        const QImage before = w->grab().toImage();
        // sweep the mouse down the table half of the widget: some row must change the picture
        bool changed = false;
        for (int y = 60; y < w->height() - 10 && !changed; y += 6)
        {
            QTest::mouseMove(w, QPoint(w->width() - 40, y));
            changed = w->grab().toImage() != before;
        }
        QVERIFY(changed);
        QTest::mouseMove(w, QPoint(5, 5));  // off the table: the drawing is back as before
        QTRY_VERIFY(w->grab().toImage() == before);
    }
}

void GuiTest::recentFilesAreListedInTheFileMenu()
{
    // the list logic: newest first, no duplicates, at most 12
    QStringList list;
    for (int i = 0; i < 15; ++i) list = recentfiles::pushed(list, u"/tmp/load%1.grtload"_s.arg(i));
    QCOMPARE(list.size(), 12);
    QCOMPARE(list.first(), u"/tmp/load14.grtload"_s);
    list = recentfiles::pushed(list, u"/tmp/load8.grtload"_s);
    QCOMPARE(list.size(), 12);
    QCOMPARE(list.first(), u"/tmp/load8.grtload"_s);
    QCOMPARE(list.count(u"/tmp/load8.grtload"_s), 1);

    QCOMPARE(withLoadExtension(u"/tmp/mine"_s), u"/tmp/mine.grtload"_s);
    QCOMPARE(withLoadExtension(u"/tmp/mine.grtload"_s), u"/tmp/mine.grtload"_s);
    QCOMPARE(withLoadExtension(u"/tmp/mine.xml"_s), u"/tmp/mine.xml"_s);
    QCOMPARE(withLoadExtension(u"/tmp.d/mine"_s), u"/tmp.d/mine.grtload"_s);  // a dot in a folder name is no extension
    QVERIFY(withLoadExtension({}).isEmpty());

    // opening a load puts it into the File menu
    recentfiles::store({});
    MainWindow w;
    QMenu *file = nullptr;
    for (QAction *a : w.mainMenu()->actions())
        if (a->menu() && a->text().remove(u'&') == u"File"_s) file = a->menu();
    QVERIFY(file);
    QMenu *recent = nullptr;  // the entries are in a submenu, not in the File menu itself
    for (QAction *a : file->actions())
        if (a->menu() && a->text().remove(u'&') == u"Open recent"_s) recent = a->menu();
    QVERIFY(recent);
    for (QAction *a : file->actions()) QVERIFY(a->data().toString().isEmpty());
    QVERIFY(!recent->isEnabled());
    const auto visibleRecent = [&]
    {
        QStringList items;
        for (QAction *a : recent->actions())
            if (a->isVisible() && !a->data().toString().isEmpty()) items << a->text();
        return items;
    };
    QVERIFY(visibleRecent().isEmpty());
    QVERIFY(w.openLoad(QStringLiteral(GRT_TEST_DATA_DIR "/8x57is.grtload")));
    QCOMPARE(visibleRecent(), QStringList{u"&1 8x57is.grtload"_s});
    QVERIFY(recent->isEnabled());
    QCOMPARE(recentfiles::load().size(), 1);
    recentfiles::store({});
}

void GuiTest::layoutIsRemembered()
{
    {
        LoadView view;
        view.resize(1400, 800);
        view.show();
        auto splitters = view.findChildren<QSplitter *>();
        QVERIFY(splitters.size() >= 3);
        QSplitter *main = nullptr;
        for (auto *sp : splitters)
            if (sp->parentWidget() == &view) main = sp;  // the main splitter (others live in the tabs)
        QVERIFY(main);
        main->setSizes({600, 800});
        emit main->splitterMoved(600, 1);
        auto panels = view.findChildren<PropertyPanel *>();
        QVERIFY(!panels.empty());
        auto *table = tableOf(*panels.front());
        table->setColumnWidth(0, 111);
        table->setColumnWidth(1, 77);
    }
    LoadView again;
    again.resize(1400, 800);
    again.show();
    QSplitter *main = nullptr;
    for (auto *sp : again.findChildren<QSplitter *>())
        if (sp->parentWidget() == &again) main = sp;
    QVERIFY(main);
    QVERIFY2(main->sizes().first() > 500, qPrintable(QString::number(main->sizes().first())));
    auto *table = tableOf(*again.findChildren<PropertyPanel *>().back());
    QCOMPARE(table->columnWidth(0), 111);
    QCOMPARE(table->columnWidth(1), 77);
}

void GuiTest::helpMenuHasAbout()
{
    MainWindow w;
    QMenu *help = nullptr;
    QStringList menus;
    for (QAction *a : w.mainMenu()->actions())
    {
        menus << a->text().remove(u'&');
        if (a->menu() && a->text().remove(u'&') == u"Help"_s) help = a->menu();
    }
    QVERIFY2(help, qPrintable(menus.join(u'|')));
    QCOMPARE(menus.last(), u"Help"_s);  // Help is the last menu, as usual
    QStringList items;
    for (QAction *a : help->actions()) items << a->text().remove(u'&');
    QVERIFY(items.contains(u"About GRT Reloaded…"_s));
    QVERIFY(!items.contains(u"About Qt…"_s));

    // the text: name, version, purpose, calibration, safety, libraries; the database only when one is open
    const QString plain = about::html();
    QVERIFY(plain.contains(u"GRT Reloaded"_s));
    QVERIFY(plain.contains(QStringLiteral(GRT_VERSION)));
    QVERIFY(plain.contains(u"Qt"_s) && plain.contains(QString::fromLatin1(qVersion())));
    QVERIFY(plain.contains(u"Safety"_s) && plain.contains(u"at your own risk"_s));
    QVERIFY(plain.contains(u"not affiliated"_s));
    QVERIFY(plain.contains(u"Disclaimer"_s) && plain.contains(u"without warranty"_s) &&
            plain.contains(u"no responsibility or liability"_s));
    QVERIFY(!plain.contains(u"Database:"_s));
    const QString withDb = about::html(u"/some/dir/data <1>.db"_s, 578, 5659, 683);
    QVERIFY(withDb.contains(u"Database:"_s) && withDb.contains(u"578 calibers"_s) && withDb.contains(u"683 propellants"_s));
    QVERIFY(withDb.contains(u"data &lt;1&gt;.db"_s) && !withDb.contains(u"/some/dir"_s));  // file name only, escaped
    // the real dialog: about twice as wide as a default message box
    int shownWidth = 0;
    bool noHorizontalScroll = false;
    QTimer::singleShot(100,
                       [&]
                       {
                           if (auto *box = qobject_cast<QDialog *>(QApplication::activeModalWidget()))
                           {
                               shownWidth = box->width();
                               // nothing may be cut off: the text browser needs no horizontal scrolling
                               if (auto *tb = box->findChild<QTextBrowser *>())
                                   noHorizontalScroll = tb->horizontalScrollBar()->maximum() == 0;
                               box->grab().save(QDir::temp().filePath(u"grt_about.png"_s));
                               box->accept();
                           }
                       });
    about::show(nullptr, u"GordonsReloadingTool.plain.db"_s, 578, 5659, 683);
    QVERIFY2(shownWidth >= 700, qPrintable(QString::number(shownWidth)));
    QVERIFY(noHorizontalScroll);
}

void GuiTest::reportTabRendersTheLoad()
{
    ReportView::storeTemplate({});
    UnitPreferences::instance().setSystem(grt::units::System::Metric);
    LoadView view;
    QString error;
    QVERIFY2(view.load(QStringLiteral(GRT_TEST_DATA_DIR "/8x57is.grtload"), &error), qPrintable(error));
    view.resize(1500, 950);
    view.show();
    ReportView *report = view.report();
    report->refresh();
    const QString html = report->html();
    QVERIFY(html.contains(u"8 x 57 IS"_s));
    QVERIFY(html.contains(u"N150"_s));
    QVERIFY(html.contains(u" bar"_s) && html.contains(u" m/s"_s));
    QVERIFY(!html.contains(u"[work."_s) && !html.contains(u"[result."_s));  // nothing unresolved
    // the diagrams are in the report as pictures (chart + drawing)
    const grt::report::Data d = view.reportData();
    for (const char *name :
         {"result.diagramm.png", "result.diagramCurves.png", "result.diagramPropellant.png", "caliber.drawing.png", "caliber.chamber.png"})
    {
        const auto *v = d.find(QLatin1String(name));
        QVERIFY2(v && v->kind == grt::report::Value::Kind::Image, name);
        QVERIFY2(QImage::fromData(v->png).width() > 300, name);
    }
    QVERIFY(html.count(u"<img"_s) >= 2);

    // the unit system applies to the report too
    UnitPreferences::instance().setSystem(grt::units::System::Imperial);
    QVERIFY(report->html().contains(u" psi"_s) && report->html().contains(u" ft/s"_s));
    QVERIFY(!report->html().contains(u" bar"_s));
    UnitPreferences::instance().setSystem(grt::units::System::Original);

    // editing the load changes the report
    const QString before = report->html();
    const int n340 = idOf(m_db, grt::Table::Propellant, u"mname = 'Vihtavuori' AND pname = 'N340'"_s);
    view.setDatabase(&m_db);
    QVERIFY(view.applyRecord(LoadView::Component::Propellant, n340));
    QVERIFY(report->html() != before);
    QVERIFY(report->html().contains(u"N340"_s));
}

void GuiTest::reportTemplateIsCustomisable()
{
    ReportView::storeTemplate({});
    QVERIFY(!ReportView::hasCustomTemplate());
    LoadView view;
    QString error;
    QVERIFY2(view.load(QStringLiteral(GRT_TEST_DATA_DIR "/8x57is.grtload"), &error), qPrintable(error));
    ReportView *report = view.report();
    ReportView::storeTemplate(
        u"====== My report ======\nCaliber: ~~work.CaliberName~~, Pmax ~~result.MaxPressure(\"0\")~~ bar.\n~~work.nothing~~"_s);
    QVERIFY(ReportView::hasCustomTemplate());
    report->refresh();
    const QString html = report->html();
    QVERIFY(html.contains(u"My report"_s));
    QVERIFY(html.contains(u"Caliber: 8 x 57 IS"_s));
    QVERIFY(html.contains(u"[work.nothing?]"_s));  // a missing value is visible
    QVERIFY(!html.contains(u"Input data"_s));      // the default report is gone
    // writing the default template (or nothing) removes the custom one
    ReportView::storeTemplate(grt::report::defaultTemplate());
    QVERIFY(!ReportView::hasCustomTemplate());
    report->refresh();
    QVERIFY(report->html().contains(u"Input data"_s));
}

void GuiTest::actionButtonsAreEasyToSee()
{
    LoadView view;
    QString error;
    QVERIFY2(view.load(QStringLiteral(GRT_TEST_DATA_DIR "/8x57is.grtload"), &error), qPrintable(error));
    view.setDatabase(&m_db);
    view.resize(1500, 950);
    view.show();
    int select = 0;
    for (QPushButton *b : view.findChildren<QPushButton *>())
    {
        if (!isSelectButton(b)) continue;
        ++select;
        QVERIFY(b->styleSheet().contains(u"palette(highlight)"_s));  // filled with the theme's accent color
        QVERIFY(b->styleSheet().contains(u"bold"_s));
        QVERIFY(b->styleSheet().contains(u":disabled"_s));  // and a different look when it cannot be used
        QVERIFY(b->minimumHeight() >= 28);
        QVERIFY(!b->icon().isNull());
        QCOMPARE(b->cursor().shape(), Qt::PointingHandCursor);
    }
    QCOMPARE(select, 3);
    // the report's output buttons stand out as well
    int output = 0;
    for (QPushButton *b : view.report()->findChildren<QPushButton *>())
        if (b->text().contains(u"Print"_s) || b->text().contains(u"PDF"_s))
        {
            ++output;
            QVERIFY(b->styleSheet().contains(u"palette(highlight)"_s));
        }
    QCOMPARE(output, 3);
    // the "Create load" button of the new-load dialog
    NewLoadDialog dlg(m_db);
    QVERIFY(dlg.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->styleSheet().contains(u"palette(highlight)"_s));
    // both themes render them (palette references): grab once in the dark palette to check nothing breaks
    QPalette dark;
    dark.setColor(QPalette::Window, QColor(0x2b, 0x31, 0x3b));
    dark.setColor(QPalette::Highlight, QColor(0x3b, 0x6e, 0xa8));
    view.setPalette(dark);
    QVERIFY(!view.grab().isNull());
}

void GuiTest::reportScaleIsLimitedAndStored()
{
    QCOMPARE(ReportView::clampScale(10), ReportView::kMinScale);
    QCOMPARE(ReportView::clampScale(1000), ReportView::kMaxScale);
    QCOMPARE(ReportView::clampScale(120), 120);
    ReportView::storeScale(100);
    QCOMPARE(ReportView::storedScale(), 100);
    QVERIFY(!QSettings().contains(u"report/scale"_s));  // 100 % is the default: no setting
    ReportView::storeScale(150);
    QCOMPARE(ReportView::storedScale(), 150);
    ReportView::storeScale(9999);
    QCOMPARE(ReportView::storedScale(), ReportView::kMaxScale);
    QSettings().setValue(u"report/scale"_s, 3);  // hand-edited out-of-range value
    QCOMPARE(ReportView::storedScale(), ReportView::kMinScale);
    QSettings().setValue(u"report/scale"_s, u"abc"_s);  // garbage
    QCOMPARE(ReportView::storedScale(), 100);
    ReportView::storeScale(100);
}

void GuiTest::reportScaleChangesTheLayout()
{
    ReportView::storeTemplate({});
    ReportView::storeScale(100);
    LoadView view;
    QString error;
    QVERIFY2(view.load(QStringLiteral(GRT_TEST_DATA_DIR "/8x57is.grtload"), &error), qPrintable(error));
    ReportView *report = view.report();
    const QSizeF a4(595.0, 842.0);  // points, margins ignored here

    // the control shows and limits the scale
    QSpinBox *spin = nullptr;
    for (QSpinBox *s : report->findChildren<QSpinBox *>())
        if (s->suffix().contains(u"%"_s)) spin = s;
    QVERIFY(spin);
    QCOMPARE(spin->minimum(), ReportView::kMinScale);
    QCOMPARE(spin->maximum(), ReportView::kMaxScale);
    QCOMPARE(spin->value(), 100);

    report->setScale(50);
    const int small = report->printedPageCount(a4);
    report->setScale(100);
    const int normal = report->printedPageCount(a4);
    report->setScale(200);
    const int large = report->printedPageCount(a4);
    QVERIFY2(small <= normal && normal < large, qPrintable(u"%1 %2 %3"_s.arg(small).arg(normal).arg(large)));
    QVERIFY(small >= 1);

    // typing a value into the control changes and stores it; out-of-range input is limited by the control
    spin->setValue(75);
    QCOMPARE(ReportView::storedScale(), 75);
    QCOMPARE(report->scale(), 75);
    spin->setValue(1000);
    QCOMPARE(spin->value(), ReportView::kMaxScale);

    // the PDF follows the scale: more pages at 200 % than at 50 %
    QTemporaryDir dir;
    const auto pdfPages = [&](int scale)
    {
        report->setScale(scale);
        const QString path = dir.filePath(u"r%1.pdf"_s.arg(scale));
        if (!report->exportPdf(path)) return -1;
        QFile f(path);
        f.open(QIODevice::ReadOnly);
        const QByteArray data = f.readAll();
        int count = 0;
        for (qsizetype i = data.indexOf("/Type /Page\n"); i >= 0; i = data.indexOf("/Type /Page\n", i + 1)) ++count;
        return count;
    };
    const int p50 = pdfPages(50), p200 = pdfPages(200);
    QVERIFY2(p50 >= 1 && p200 > p50, qPrintable(u"%1 vs %2"_s.arg(p50).arg(p200)));
    ReportView::storeScale(100);
}

void GuiTest::propertyNamesStartWithACapital()
{
    grt::EntryList list;
    list.entries().push_back({.name = u"casevol"_s, .value = u"1"_s, .unit = u"cm3"_s, .type = u"decimal"_s, .descr = u"case volume"_s});
    list.entries().push_back(
        {.name = u"xe"_s, .value = u"1"_s, .unit = {}, .type = u"decimal"_s, .descr = {}});  // no description: the key is shown
    list.entries().push_back(
        {.name = u"Dz"_s, .value = u"1"_s, .unit = {}, .type = u"decimal"_s, .descr = u"Groove caliber"_s});  // already capital
    PropertyPanel panel;
    panel.setSections({{0, u"caliber"_s, u"Caliber"_s, &list}});
    QTableWidget *t = tableOf(panel);
    QCOMPARE(t->item(rowOf(t, u"casevol"_s), 0)->text(), u"Case Volume"_s);
    QCOMPARE(t->item(rowOf(t, u"xe"_s), 0)->text(), u"Xe"_s);
    QCOMPARE(t->item(rowOf(t, u"Dz"_s), 0)->text(), u"Groove Caliber"_s);
    // the tooltip keeps the exact key of the file
    QCOMPARE(t->item(rowOf(t, u"casevol"_s), 0)->toolTip(), u"casevol"_s);
    {
        grt::EntryList l;
        l.entries().push_back({.name = u"laddercnt"_s,
                               .value = u"1"_s,
                               .unit = {},
                               .type = u"decimal"_s,
                               .descr = u"ladder steps (1 to 10, below 2: disabled)"_s});
        PropertyPanel pp;
        pp.setSections({{0, u"propellant"_s, u"Propellant"_s, &l}});
        QTableWidget *tt = tableOf(pp);
        QCOMPARE(tt->item(rowOf(tt, u"laddercnt"_s), 0)->text(), u"Ladder Steps"_s);
        QVERIFY(tt->item(rowOf(tt, u"laddercnt"_s), 0)->toolTip().contains(u"1 to 10"_s));  // still available as a tooltip
        QCOMPARE(tt->item(rowOf(tt, u"laddercnt"_s), 1)->text(), u"1"_s);
        QVERIFY(tt->item(rowOf(tt, u"laddercnt"_s), 1)->data(Qt::UserRole + 5).toBool());  // edited with a drop-down (ChoiceRole)
    }
    // every label of a real load starts with a capital letter
    LoadView view;
    QString error;
    QVERIFY2(view.load(QStringLiteral(GRT_TEST_DATA_DIR "/8x57is.grtload"), &error), qPrintable(error));
    int labels = 0;
    for (PropertyPanel *p : view.findChildren<PropertyPanel *>())
        for (int r = 0; r < tableOf(*p)->rowCount(); ++r)
        {
            const QTableWidgetItem *it = tableOf(*p)->item(r, 0);
            if (!it || it->text().isEmpty() || !tableOf(*p)->item(r, 1)) continue;  // section header rows have no value cell
            ++labels;
            QVERIFY2(it->text().at(0).isUpper() || !it->text().at(0).isLetter(), qPrintable(it->text()));
            QVERIFY2(!it->text().startsWith(u"True if"_s), qPrintable(it->text()));
            QVERIFY2(!it->text().contains(u'('), qPrintable(it->text()));
            const auto words = it->text().split(u' ', Qt::SkipEmptyParts);
            for (const QString &w : words) QVERIFY2(w.at(0).isUpper() || !w.at(0).isLetter(), qPrintable(it->text()));
        }
    QVERIFY(labels > 30);
}

void GuiTest::reportExportsAPdf()
{
    ReportView::storeTemplate({});
    LoadView view;
    QString error;
    QVERIFY2(view.load(QStringLiteral(GRT_TEST_DATA_DIR "/8x57is.grtload"), &error), qPrintable(error));
    QTemporaryDir dir;
    const QString pdf = dir.filePath(u"report.pdf"_s);
    QVERIFY(view.report()->exportPdf(pdf));
    QFile f(pdf);
    QVERIFY(f.open(QIODevice::ReadOnly));
    const QByteArray data = f.readAll();
    QVERIFY(data.startsWith("%PDF"));
    QVERIFY(data.size() > 20000);  // text, tables and the embedded diagrams
    QVERIFY(data.contains("/Type /Page") || data.contains("/Type/Page"));
    QFile::copy(pdf, QDir::temp().filePath(u"grt_report_sample.pdf"_s));
}

void GuiTest::inputsAreGroupedInThreeTabs()
{
    LoadView view;
    QString error;
    QVERIFY2(view.load(QStringLiteral(GRT_TEST_DATA_DIR "/8x57is.grtload"), &error), qPrintable(error));
    const auto tabs = view.findChildren<QTabWidget *>();
    QTabWidget *inputs = nullptr;
    for (QTabWidget *t : tabs)
        if (t->count() == 3) inputs = t;
    QVERIFY(inputs);
    QCOMPARE(inputs->tabText(1), u"Projectile"_s);
    QCOMPARE(inputs->tabText(2), u"Propellant"_s);
    // every tab has a "Select <component>…" button; it is off until a database is available
    const auto buttons = view.findChildren<QPushButton *>();
    int selectButtons = 0;
    for (QPushButton *b : buttons)
        if (isSelectButton(b))
        {
            ++selectButtons;
            QVERIFY(!b->isEnabled());
        }
    QCOMPARE(selectButtons, 3);
    QVERIFY(!view.applyRecord(LoadView::Component::Propellant, 562));  // no database yet
    view.setDatabase(&m_db);
    for (QPushButton *b : buttons)
        if (isSelectButton(b)) QVERIFY(b->isEnabled());
}

void GuiTest::componentsCanBeReplacedAfterSetup()
{
    LoadView view;
    QString error;
    QVERIFY2(view.load(QStringLiteral(GRT_TEST_DATA_DIR "/8x57is.grtload"), &error), qPrintable(error));
    view.setDatabase(&m_db);
    QSignalSpy modified(&view, &LoadView::modifiedChanged);
    const auto allText = [&]
    {
        QStringList s;
        for (const QLabel *l : view.findChildren<QLabel *>()) s << l->text();
        return s.join(u'\n');
    };
    QVERIFY(!view.isModified());
    QVERIFY(allText().contains(u"N150"_s));

    // another powder: the header shows it and the load counts as modified; the charge is kept
    const int n340 = idOf(m_db, grt::Table::Propellant, u"mname = 'Vihtavuori' AND pname = 'N340'"_s);
    QVERIFY(n340 > 0);
    QVERIFY(view.applyRecord(LoadView::Component::Propellant, n340));
    QVERIFY(view.isModified());
    QCOMPARE(modified.count(), 1);
    QVERIFY(allText().contains(u"N340"_s));
    QVERIFY(!allText().contains(u"N150"_s));

    // another bullet
    const int other = idOf(m_db, grt::Table::Projectile, u"caliber = '.323' AND gmass > 150 AND mname <> 'Sellier-Bellot'"_s);
    QVERIFY(other > 0);
    QVERIFY(view.applyRecord(LoadView::Component::Projectile, other));
    QVERIFY(!allText().contains(u"Sellier-Bellot"_s));

    // another caliber, with the dependent values reset: a pistol barrel is much shorter
    const int pistol = idOf(m_db, grt::Table::Caliber, u"cipname = '.45 Auto'"_s);
    QVERIFY(pistol > 0);
    QVERIFY(view.applyRecord(LoadView::Component::Caliber, pistol, true));
    QVERIFY(allText().contains(u".45 Auto"_s));
    // the saved file reflects all three replacements and loads again
    QTemporaryDir dir;
    const QString out = dir.filePath(u"changed.grtload"_s);
    QVERIFY(view.save(out, &error));
    const auto lf = grt::LoadFile::read(out);
    QVERIFY(lf);
    QCOMPARE(lf->propellant.text(u"pname"), u"N340"_s);
    QCOMPARE(lf->caliberFile.text(u"cipname"), u".45 Auto"_s);
    QCOMPARE(lf->gun.number(u"xe"), 150.0);
    QVERIFY(!view.applyRecord(LoadView::Component::Propellant, -5));  // unknown id
}

void GuiTest::chamberGeometryFollowsTheDimensions()
{
    const auto lf = grt::LoadFile::read(QStringLiteral(GRT_TEST_DATA_DIR "/8x57is.grtload"));
    QVERIFY(lf);
    QMap<QString, double> d;
    for (const grt::Entry &e : lf->caliberFile.entries()) d.insert(e.name, e.value.toDouble());
    // chamber of the 8x57 IS: P1 11.97, P2 10.98 at L1 46.16, H1 9.11 at L2 48.85, H2 9.10 at L3 57.3, lands 7.89, grooves 8.2
    const auto g = ChamberView::buildGeometry(d, 90.0);
    QVERIFY(g.valid);
    QCOMPARE(g.lands, 7.89);
    QCOMPARE(g.grooves, 8.2);
    QCOMPARE(g.throatY, 57.3);
    const auto radiusAt = [&](double y)
    {
        for (std::size_t i = 1; i < g.cavity.size(); ++i)
            if (y >= g.cavity[i - 1].y() && y <= g.cavity[i].y() && g.cavity[i].y() > g.cavity[i - 1].y())
                return g.cavity[i - 1].x() +
                       (g.cavity[i].x() - g.cavity[i - 1].x()) * (y - g.cavity[i - 1].y()) / (g.cavity[i].y() - g.cavity[i - 1].y());
        return -1.0;
    };
    QVERIFY(std::abs(radiusAt(46.16) * 2 - 10.98) < 1e-6);  // P2 at L1
    QVERIFY(std::abs(radiusAt(48.85) * 2 - 9.11) < 1e-6);   // H1 at L2
    QVERIFY(std::abs(radiusAt(57.3) * 2 - 9.10) < 1e-6);    // H2 at L3
    QVERIFY(std::abs(g.cavity.back().y() - 90.0) < 1e-9);   // drawn up to the requested height
    // leade cone: from G1 (8.24) at the throat to the lands diameter (7.89) over 35 mm, never wider than G1
    QVERIFY(radiusAt(60.0) * 2 <= 8.24 + 1e-9 && radiusAt(60.0) * 2 > 7.89);
    QVERIFY(g.cavity.back().x() * 2 < 8.24);
    // the cavity never gets narrower than the bore
    for (const QPointF &q : g.cavity)
        if (q.y() > 57.3) QVERIFY(q.x() * 2 >= 7.89 - 1e-9);
    // incomplete data gives no drawing instead of garbage
    QVERIFY(!ChamberView::buildGeometry({}, 50.0).valid);
}

void GuiTest::drawingPanelHasCartridgeAndChamberTabs()
{
    LoadView view;
    QString error;
    QVERIFY2(view.load(QStringLiteral(GRT_TEST_DATA_DIR "/8x57is.grtload"), &error), qPrintable(error));
    view.resize(1500, 950);
    view.show();
    QTabWidget *drawings = nullptr;
    for (QTabWidget *t : view.findChildren<QTabWidget *>())
        if (t->count() == 3 && t->tabText(0) == u"Cartridge"_s) drawings = t;  // + the hidden powder photo tab
    QVERIFY(drawings);
    QCOMPARE(drawings->tabText(1), u"Chamber"_s);
    auto *chamber = drawings->findChild<ChamberView *>();
    QVERIFY(chamber && chamber->geometry().valid);
    drawings->setCurrentIndex(1);
    chamber->resize(420, 560);
    const QImage img = chamber->grab().toImage();
    QVERIFY(!img.isNull());
    img.save(QDir::temp().filePath(u"grt_chamber_8x57.png"_s));
}

// The drawn interior is calibrated to the case capacity, so the powder fill level is consistent.
void GuiTest::cartridgeGeometryMatchesTheCaseCapacity()
{
    const auto lf = grt::LoadFile::read(QStringLiteral(GRT_TEST_DATA_DIR "/8x57is.grtload"));
    QVERIFY(lf);
    QMap<QString, double> dims;
    for (const grt::Entry &e : lf->caliberFile.entries()) dims.insert(e.name, e.value.toDouble());
    const double capacity = lf->caliber.number(u"casevol");  // cm3
    const auto g = CartridgeView::buildGeometry(dims, capacity, 0.5 * capacity);
    QVERIFY(g.valid);
    QVERIFY(std::abs(g.cavityVolume_mm3 / (capacity * 1e3) - 1.0) < 0.01);  // 1 % of the capacity
    QVERIFY(g.wall > 0.1 && g.wall < 2.0);                                  // a plausible brass wall
    // half the capacity of powder fills clearly less than the whole case, more than nothing
    QVERIFY(g.powderHeight_mm > 5.0 && g.powderHeight_mm < dims.value(u"L3"_s));
    // a bigger charge fills higher
    const auto g2 = CartridgeView::buildGeometry(dims, capacity, 0.8 * capacity);
    QVERIFY(g2.powderHeight_mm > g.powderHeight_mm);
    // bottleneck outline: wider at the head than at the mouth, ends at the case length
    QVERIFY(g.outer.front().x() == 0.0);
    QVERIFY(g.outer.back().y() == dims.value(u"L3"_s));
    QVERIFY(g.outer.back().x() < dims.value(u"P1"_s) / 2.0);
}

void GuiTest::cartridgeViewDrawsAndHandlesStraightCases()
{
    CartridgeView view;
    view.resize(420, 520);
    // a bottleneck case from the sample load
    auto lf = grt::LoadFile::read(QStringLiteral(GRT_TEST_DATA_DIR "/8x57is.grtload"));
    QVERIFY(lf);
    view.setLoad(*lf, grt::SimInput::fromLoad(*lf));
    QVERIFY(view.geometry().valid);
    const QImage a = view.grab().toImage();
    QVERIFY(!a.isNull());

    // a straight pistol case (L1/L2/H1/P2 are zero in the database): no crash, still valid
    if (!QFile::exists(QStringLiteral(GRT_SOURCE_DIR "/tools/GordonsReloadingTool.plain.db"))) return;
    grt::Database db;
    QVERIFY(db.open(QStringLiteral(GRT_SOURCE_DIR "/tools/GordonsReloadingTool.plain.db")));
    const int cal = idOf(db, grt::Table::Caliber, u"cipname = '.45 Auto'"_s);
    const int prj = idOf(db, grt::Table::Projectile, u"caliber = '.451'"_s);
    const int pro = idOf(db, grt::Table::Propellant, u"mname = 'Vihtavuori' AND pname = 'N340'"_s);
    QVERIFY(cal > 0 && prj > 0 && pro > 0);
    const grt::LoadFile pistol = grt::buildLoad(*db.record(grt::Table::Caliber, cal), *db.record(grt::Table::Projectile, prj),
                                                *db.record(grt::Table::Propellant, pro), {}, db.columns(grt::Table::Caliber));
    view.setLoad(pistol, grt::SimInput::fromLoad(pistol));
    QVERIFY(view.geometry().valid);
    QVERIFY(view.geometry().outer.size() >= 4);
    QVERIFY(!view.grab().toImage().isNull());
    view.clear();
    QVERIFY(!view.geometry().valid);
}

void GuiTest::measurementTabShowsTheFilesResults()
{
    UnitPreferences::instance().setSystem(grt::units::System::Metric);
    LoadView view;
    QVERIFY(view.load(QStringLiteral(GRT_TEST_DATA_DIR "/POWDER-MEASURE-TEMPLATE.grtload")));
    view.showResultTab(1);
    view.resize(1400, 900);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    MeasurementView *m = view.measurements();
    QVERIFY(m->isVisible());
    auto *charges = m->findChild<QTableWidget *>(u"charges"_s);
    QVERIFY(charges);
    QCOMPARE(charges->rowCount(), 2);
    QCOMPARE(charges->item(0, 0)->text(), u"Charge 1"_s);
    QCOMPARE(charges->item(0, 2)->text(), u"4"_s);
    QCOMPARE(charges->item(0, 3)->text(), u"848.3"_s);  // mean of the four shots [m/s]
    QVERIFY(charges->item(0, 6)->text() != u"–"_s);     // simulated for comparison
    QVERIFY(charges->item(0, 7)->text().endsWith(u" %"_s));
    QCOMPARE(m->currentCharge(), 0);
    QVERIFY(!view.isModified());
    // the default report gets a measurement section with the table and the chart
    view.report()->refresh();
    const QString html = view.report()->html();
    QVERIFY(html.contains(u"MEASUREMENTS"_s));
    QVERIFY(html.contains(u"Charge 1"_s));
    QVERIFY(html.contains(u"848.3 m/s"_s));
    QVERIFY(!html.contains(u"measurement.section?"_s));
    UnitPreferences::instance().setSystem(grt::units::System::Original);
}

// Dates in the record dialog: a date picker in the form yyyy-MM-dd ("– not set –" = empty), the program's dates as
// text; propellant and projectile dialogs are wide enough for two columns.
void GuiTest::recordDialogDatesAndWidth()
{
    RecordDialog cal(m_db, grt::Table::Caliber, RecordDialog::Mode::Copy,
                     *m_db.record(grt::Table::Caliber, idOf(m_db, grt::Table::Caliber, u"cipdate != ''"_s)));
    auto *picker = cal.findChild<QDateEdit *>(u"cipdate"_s);
    QVERIFY(picker);
    QCOMPARE(picker->displayFormat(), u"yyyy-MM-dd"_s);
    QVERIFY(picker->calendarPopup());
    if (const QString shots = qEnvironmentVariable("GRT_SHOT_DIR"); !shots.isEmpty()) cal.grab().save(shots + u"/caliberdates.png"_s);
    cal.setText(u"cipdate"_s, u"14.6.1984"_s);
    QCOMPARE(cal.text(u"cipdate"_s), u"1984-06-14"_s);
    QCOMPARE(cal.values().value(u"cipdate"_s).toString(), u"1984-06-14"_s);
    cal.setText(u"cipdate"_s, QString());
    QCOMPARE(cal.values().value(u"cipdate"_s).toString(), QString());  // "– not set –"
    QVERIFY(!cal.findChild<QDateEdit *>(u"mdate"_s));                  // the date of the last change is set by the program
    QCOMPARE(cal.values().value(u"cdate"_s).toString(), QDate::currentDate().toString(Qt::ISODate));  // a copy: today

    // the creation date can be entered for every kind of record (a picker, today for a new record)
    for (const grt::Table t : {grt::Table::Caliber, grt::Table::Projectile, grt::Table::Propellant})
    {
        RecordDialog dlg(m_db, t, RecordDialog::Mode::New);
        auto *created = dlg.findChild<QDateEdit *>(u"cdate"_s);
        QVERIFY(created);
        QVERIFY(created->isEnabled() && !created->isReadOnly());
        QCOMPARE(dlg.text(u"cdate"_s), QDate::currentDate().toString(Qt::ISODate));
        dlg.setText(u"cdate"_s, u"2020-05-01"_s);
        QCOMPARE(dlg.values().value(u"cdate"_s).toString(), u"2020-05-01"_s);
        QCOMPARE(dlg.values().value(u"mdate"_s).toString(), QString());
    }

    for (const grt::Table t : {grt::Table::Projectile, grt::Table::Propellant})
    {
        RecordDialog dlg(m_db, t, RecordDialog::Mode::New);
        QCOMPARE_GE(dlg.width(), 800);
    }
}

// The notes under the results: the bullet jump is part of the model; a shotshell gets a note how to enter it.
void GuiTest::ignoredInputsAreNamed()
{
    QFile f(QStringLiteral(GRT_TEST_DATA_DIR "/8x57is.grtload"));
    QVERIFY(f.open(QIODevice::ReadOnly));
    QString xml = QString::fromUtf8(f.readAll());
    const auto set = [&](const QString &name, const QString &value)
    {
        const QRegularExpression re(u"(<input name=\"%1\" value=\")[^\"]*"_s.arg(name));
        QVERIFY(re.match(xml).hasMatch());
        xml.replace(re, u"\\1"_s + value);
    };
    const auto statusText = [](LoadView &v)
    {
        for (QLabel *l : v.findChildren<QLabel *>())
            if (l->text().contains(u"Calibrated against"_s) || l->text().contains(u"Physical model"_s)) return l->text();  // per profile
        return QString();
    };
    QTemporaryDir dir;
    const QString path = dir.filePath(u"j.grtload"_s);
    const auto save = [&]
    {
        QFile out(path);
        QVERIFY(out.open(QIODevice::WriteOnly));
        out.write(xml.toUtf8());
    };
    LoadView plain;
    QVERIFY(plain.load(QStringLiteral(GRT_TEST_DATA_DIR "/8x57is.grtload")));
    QVERIFY(!statusText(plain).contains(u"Not taken into account"_s));
    // the jump distance is an input; its lock is not shown (the original uses the distance either way)
    QStringList labels;
    for (QTableWidget *t : plain.findChildren<QTableWidget *>())
        for (int r = 0; r < t->rowCount(); ++r)
            if (t->item(r, 0)) labels << t->item(r, 0)->text();
    QVERIFY(labels.contains(u"Bullet Jump Distance"_s));
    QVERIFY(!labels.contains(u"Unlocked Bullet Jump"_s));

    set(u"gjump"_s, u"1.5"_s);  // the bullet jump is part of the model now: no notice
    set(u"gjumpUnlock"_s, u"true"_s);
    save();
    LoadView jumped;
    QVERIFY(jumped.load(path));
    QVERIFY(!statusText(jumped).contains(u"Not taken into account"_s));

    set(u"shotshell"_s, u"true"_s);  // a shotshell: modelled, with a note how to enter it
    save();
    LoadView view;
    QVERIFY(view.load(path));
    const QString text = statusText(view);
    QVERIFY2(!text.contains(u"Not taken into account"_s), qPrintable(text));
    QVERIFY2(text.contains(u"Shotshell:"_s), qPrintable(text));
}

void GuiTest::propellantPhotoIsShown()
{
    // the scale bar: about a fifth of the width, 1, 2 or 5 times a power of ten
    QCOMPARE(PhotoView::scaleBarMm(0.0, 900, 300), 0.0);
    QCOMPARE(PhotoView::scaleBarMm(29.5, 900, 450), 5.0);   // 450 px wide = 30.5 mm: a fifth = 6.1 mm
    QCOMPARE(PhotoView::scaleBarMm(144.3, 900, 300), 1.0);  // 300 px = 6.2 mm: 1.2 mm
    QCOMPARE(PhotoView::scaleBarMm(144.3, 900, 900), 1.0);

    PhotoView view;
    QVERIFY(!view.hasPhotos());
    QImage img(90, 80, QImage::Format_RGB32);
    img.fill(Qt::gray);
    view.setPhotos({}, {.image = img, .name = u"detail"_s, .pxPerMm = 100.0}, u"k"_s);
    QVERIFY(view.hasPhotos());
    QCOMPARE(view.shown(), 1);  // only the detail exists
    view.setPhotos({.image = img, .name = u"overview"_s, .pxPerMm = 0.0}, {.image = img, .name = {}, .pxPerMm = 0.0}, u"k2"_s);
    QCOMPARE(view.shown(), 0);
    view.show(1);
    QCOMPARE(view.shown(), 1);
    view.clear();
    QVERIFY(!view.hasPhotos());
    QCOMPARE(view.shown(), -1);

    // a load with a propellant record of the original database shows its photos
    LoadView lv;
    QVERIFY(lv.load(QStringLiteral(GRT_TEST_DATA_DIR "/8x57is.grtload")));
    grt::Database db;
    QVERIFY(db.open(QStringLiteral(GRT_SOURCE_DIR "/tools/GordonsReloadingTool.plain.db")));
    lv.setDatabase(&db);
    const auto *photo = lv.findChild<PhotoView *>();
    QVERIFY(photo);
    const int withImage = idOf(db, grt::Table::Propellant, u"imageUuid != '' AND imageDetailUuid != ''"_s);
    QVERIFY(withImage > 0);
    QVERIFY(lv.applyRecord(LoadView::Component::Propellant, withImage));
    QVERIFY(photo->hasPhotos());
    QCOMPARE(photo->shown(), 0);
    const int without = idOf(db, grt::Table::Propellant, u"imageUuid = '' AND imageDetailUuid = ''"_s);
    QVERIFY(without > 0);
    QVERIFY(lv.applyRecord(LoadView::Component::Propellant, without));
    QVERIFY(!photo->hasPhotos());
    QVERIFY(lv.applyRecord(LoadView::Component::Propellant, withImage));
    lv.setDatabase(nullptr);  // the pictures live in the database
    QVERIFY(!photo->hasPhotos());
}

void GuiTest::ownPropellantWithPhotoWorksWithoutTheOriginal()
{
    QTemporaryDir dir;
    grt::Database own;  // only the program's own database: no original GRT database
    QVERIFY(own.open({}, dir.filePath(u"own.db"_s)));

    RecordDialog dlg(own, grt::Table::Propellant, RecordDialog::Mode::New);
    dlg.setText(u"pname"_s, u"Home powder"_s);
    QVERIFY(dlg.pendingImages().empty());
    QVERIFY(dlg.values().value(u"imageUuid"_s).toString().isEmpty());
    QImage big(2400, 1600, QImage::Format_RGB32);  // larger than stored: scaled down, the scale follows
    big.fill(Qt::darkGray);
    dlg.setPhoto(0, big, 200.0, u"grains.png"_s);
    QImage close(300, 200, QImage::Format_RGB32);
    close.fill(Qt::lightGray);
    dlg.setPhoto(1, close, 0.0, u"grain.png"_s);
    if (const QString shots = qEnvironmentVariable("GRT_SHOT_DIR"); !shots.isEmpty())
    {
        dlg.resize(620, 900);
        dlg.grab().save(shots + u"/recordphotos.png"_s);
    }
    const auto pending = dlg.pendingImages();
    QCOMPARE(pending.size(), std::size_t{2});
    QCOMPARE(dlg.values().value(u"imageUuid"_s).toString(), pending[0].uuid);
    QCOMPARE(dlg.values().value(u"imageDetailUuid"_s).toString(), pending[1].uuid);
    QCOMPARE(pending[0].scaleUnit, u"px/mm"_s);
    QCOMPARE_LT(std::abs(pending[0].scale - 100.0), 0.5);  // 2400 -> 1200 px: half the scale
    QVERIFY(pending[1].scaleUnit.isEmpty());
    QVERIFY(!QImage::fromData(pending[0].data).isNull());
    QCOMPARE(QImage::fromData(pending[0].data).width(), 1200);

    for (const auto &img : pending) QVERIFY(own.addImage(img));
    const auto id = own.insert(grt::Table::Propellant, dlg.values());
    QVERIFY(id);

    // a load that uses the record shows the photo (Powder tab) from the own database alone
    LoadView view;
    QVERIFY(view.load(QStringLiteral(GRT_TEST_DATA_DIR "/8x57is.grtload")));
    view.setDatabase(&own);
    QVERIFY(view.applyRecord(LoadView::Component::Propellant, *id));
    const auto *photo = view.findChild<PhotoView *>();
    QVERIFY(photo && photo->hasPhotos());

    // an edit that removes the detail photo
    RecordDialog edit(own, grt::Table::Propellant, RecordDialog::Mode::Edit, *own.record(grt::Table::Propellant, *id));
    edit.removePhoto(1);
    QVERIFY(edit.values().value(u"imageDetailUuid"_s).toString().isEmpty());
    QCOMPARE(edit.values().value(u"imageUuid"_s).toString(), pending[0].uuid);
    QVERIFY(edit.pendingImages().empty());
}

void GuiTest::chronographImportAddsCharges()
{
    LoadView view;
    QVERIFY(view.load(QStringLiteral(GRT_TEST_DATA_DIR "/8x57is.grtload")));
    MeasurementView *m = view.measurements();
    const auto file = grt::readChronograph(
        QByteArray("Series, 1, Shots:,2\r\n 1, 1, 800.0, m/s\r\n 1, 2, 810.0, m/s\r\nSeries, 2, Shots:,1\r\n 2, 1, 820.0, m/s\r\n"));
    QVERIFY(file);
    ChronographImportDialog dialog(*file, 3.0);
    dialog.setChecked(1, false);
    const auto picked = dialog.selected();
    QCOMPARE(picked.size(), std::size_t{1});
    QCOMPARE(picked[0].charge_g, 3.0);
    QCOMPARE(m->importCharges(picked), 1);
    QCOMPARE(m->measurement().charges.size(), std::size_t{1});
    QCOMPARE(grt::velocityStatistics(m->measurement().charges[0]).count, 2);
    QVERIFY(view.isModified());
}

void GuiTest::measurementSeriesAreEditedAndSaved()
{
    LoadView view;
    QVERIFY(view.load(QStringLiteral(GRT_TEST_DATA_DIR "/8x57is.grtload")));
    MeasurementView *m = view.measurements();
    m->addCharge();
    m->addSeries();  // a second, empty series is shown
    QCOMPARE(m->allMeasurements().size(), std::size_t{2});
    QCOMPARE(m->currentSeries(), 1);
    QVERIFY(m->measurement().charges.empty());
    m->addCharge();
    m->addShot();
    m->selectSeries(0);
    QCOMPARE(m->measurement().charges.size(), std::size_t{1});
    QVERIFY(m->measurement().charges[0].shots.empty());

    QTemporaryDir dir;
    const QString path = dir.filePath(u"s.grtload"_s);
    QVERIFY(view.save(path));
    const auto lf = grt::LoadFile::read(path);
    QVERIFY(lf);
    QCOMPARE(grt::readMeasurements(lf->appendixXml).size(), std::size_t{2});

    LoadView again;
    QVERIFY(again.load(path));
    QCOMPARE(again.measurements()->allMeasurements().size(), std::size_t{2});
    QCOMPARE(again.measurements()->allMeasurements()[1].charges[0].shots.size(), std::size_t{1});
    again.measurements()->selectSeries(1);
    again.measurements()->removeSeries();
    QCOMPARE(again.measurements()->allMeasurements().size(), std::size_t{1});
    QVERIFY(again.save(path));
    QCOMPARE(grt::readMeasurements(grt::LoadFile::read(path)->appendixXml).size(), std::size_t{1});
}

void GuiTest::measurementsAreEditedAndSaved()
{
    UnitPreferences::instance().setSystem(grt::units::System::Metric);
    LoadView view;
    QVERIFY(view.load(QStringLiteral(GRT_TEST_DATA_DIR "/8x57is.grtload")));
    MeasurementView *m = view.measurements();
    QVERIFY(m->measurement().charges.empty());
    QVERIFY(!view.isModified());

    m->addCharge();  // the load's charge
    QVERIFY(view.isModified());
    QCOMPARE(m->measurement().charges.size(), std::size_t{1});
    QCOMPARE(m->measurement().charges[0].charge_g(), view.focusCharge_g());
    QCOMPARE(m->pasteShots(u"Velocity\tPressure\n812.5\t3500\n815.5\n\n818.0;3600\n"_s), 3);
    const grt::ShotStatistics v = grt::velocityStatistics(m->measurement().charges[0]);
    QCOMPARE(v.count, 3);
    QCOMPARE(v.max, 818.0);
    QCOMPARE(grt::pressureStatistics(m->measurement().charges[0]).count, 2);

    QTemporaryDir dir;
    const QString path = dir.filePath(u"m.grtload"_s);
    QVERIFY(view.save(path));
    const auto lf = grt::LoadFile::read(path);
    QVERIFY(lf);
    const auto saved = grt::readMeasurement(lf->appendixXml);
    QVERIFY(saved);
    QCOMPARE(saved->charges.size(), std::size_t{1});
    QCOMPARE(saved->charges[0].shots.size(), std::size_t{3});
    QCOMPARE(saved->charges[0].shots[0].pressure_bar, 3500.0);

    // removing everything keeps an (empty) block, and reopening shows it again
    m->selectCharge(0);
    m->removeCharge();
    QVERIFY(m->measurement().charges.empty());
    LoadView again;
    QVERIFY(again.load(path));
    QCOMPARE(again.measurements()->measurement().charges.size(), std::size_t{1});
    UnitPreferences::instance().setSystem(grt::units::System::Original);
}

void GuiTest::originalDatabaseIsChosenInTheSettings()
{
    const QString original = QStringLiteral(GRT_SOURCE_DIR "/tools/GordonsReloadingTool.plain.db");
    const QString before = QSettings().value(u"database/path"_s).toString();
    {
        MainWindow w;
        // File > Open database is gone: the database is a setting
        for (QAction *a : w.mainMenu()->actions().front()->menu()->actions())
            QVERIFY(!a->text().contains(u"database"_s, Qt::CaseInsensitive));

        // without an original only the own database is there (searching still works, it is just empty)
        QVERIFY(w.openDatabase(QString(), false));
        QCOMPARE(QSettings().value(u"database/path"_s).toString(), QString());
        auto *browser = w.findChild<TableBrowser *>();
        QVERIFY(browser);

        SettingsDialog dlg;
        dlg.setOriginalDatabase(QString(), u"none"_s);
        QSignalSpy chosen(&dlg, &SettingsDialog::originalDatabaseChosen);
        auto *edit = dlg.findChild<QLineEdit *>(u"originalDatabase"_s);
        QVERIFY(edit);
        QVERIFY(edit->text().isEmpty());
        edit->setText(original);
        dlg.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
        QCOMPARE(chosen.count(), 1);
        QCOMPARE(chosen.front().front().toString(), original);
        dlg.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
        QCOMPARE(chosen.count(), 1);  // unchanged: nothing to do
        dlg.setOriginalDatabase(original, u"GordonsReloadingTool.plain.db: 578 calibers, 5659 projectiles, 683 propellants"_s);
        if (const QString shots = qEnvironmentVariable("GRT_SHOT_DIR"); !shots.isEmpty())
        {  // for looking at the layout
            dlg.adjustSize();
            dlg.grab().save(shots + u"/settings.png"_s);
        }

        QVERIFY(w.openDatabase(original));
        QCOMPARE(QSettings().value(u"database/path"_s).toString(), original);
        QVERIFY(browser->selectId(562));  // a powder of the original
    }
    QSettings().setValue(u"database/path"_s, before);
}

void GuiTest::recordDialogShowsReadableFieldsInUserUnits()
{
    UnitPreferences::instance().setSystem(grt::units::System::Metric);
    const auto powder = m_db.record(grt::Table::Propellant, 562);
    QVERIFY(powder);
    RecordDialog copy(m_db, grt::Table::Propellant, RecordDialog::Mode::Copy, *powder);
    QCOMPARE(copy.text(u"pname"_s), u"N150 (copy)"_s);
    QCOMPARE(copy.text(u"Ba"_s), u"0.5814"_s);
    QCOMPARE(copy.text(u"pcd"_s), u"890"_s);              // kg/m3
    QCOMPARE(copy.text(u"Qex"_s), u"3790"_s);             // no thousands separator in a field
    QVERIFY(!copy.findChild<QWidget *>(u"geloescht"_s));  // internal columns are not shown
    bool found = false;
    for (QLabel *l : copy.findChildren<QLabel *>()) found = found || l->text() == u"Burn rate coefficient (Ba):"_s;
    QVERIFY(found);
    QVERIFY(copy.firstError().isEmpty());
    copy.setText(u"Ba"_s, u"fast"_s);
    QCOMPARE(copy.firstError(), u"Burn rate coefficient (Ba)"_s);
    copy.setText(u"Ba"_s, u"0.62"_s);
    copy.setText(u"pname"_s, QString());
    QCOMPARE(copy.firstError(), u"Name"_s);  // required
    copy.setText(u"pname"_s, u"N150 own lot"_s);
    const QVariantMap v = copy.values();
    QCOMPARE(v.value(u"Ba"_s).toDouble(), 0.62);
    QCOMPARE(v.value(u"pname"_s).toString(), u"N150 own lot"_s);
    QCOMPARE(v.value(u"cdate"_s).toString(), QDate::currentDate().toString(Qt::ISODate));
    QVERIFY(!v.contains(u"id"_s));

    // the bullet mass is stored in grains but shown and typed in the user's unit (grams here)
    const auto bullet = m_db.record(grt::Table::Projectile, 4542);
    QVERIFY(bullet);
    RecordDialog edit(m_db, grt::Table::Projectile, RecordDialog::Mode::Edit, *bullet);
    QCOMPARE(edit.text(u"gmass"_s).left(6), u"12.700"_s);  // 196 grain, all digits kept
    QCOMPARE(edit.text(u"gtailtype"_s), u"boat tail"_s);
    edit.setText(u"gmass"_s, u"13"_s);
    QVERIFY(std::abs(edit.values().value(u"gmass"_s).toDouble() - 13.0 / 0.06479891) < 1e-9);
    QCOMPARE(edit.values().value(u"gtailtype"_s).toInt(), 2);
    // the country of origin is chosen from a list: no free text
    RecordDialog caliber(m_db, grt::Table::Caliber, RecordDialog::Mode::Copy, *m_db.record(grt::Table::Caliber, 1));
    auto *origin = caliber.findChild<QComboBox *>(u"ciporigin"_s);
    QVERIFY(origin);
    QVERIFY(!origin->isEditable());
    QCOMPARE(caliber.text(u"ciporigin"_s), u"US"_s);
    QVERIFY(origin->findData(u"DE"_s) >= 0);
    QVERIFY(origin->itemText(origin->findData(u"DE"_s)).contains(u"Germany"_s));
    QVERIFY(origin->findData(u"SU"_s) >= 0);
    caliber.setText(u"ciporigin"_s, u"DE"_s);
    QCOMPARE(caliber.values().value(u"ciporigin"_s).toString(), u"DE"_s);
    caliber.setText(u"ciporigin"_s, u"Bavaria"_s);  // not a country: ignored
    QCOMPARE(caliber.values().value(u"ciporigin"_s).toString(), u"DE"_s);
    origin->setCurrentIndex(0);
    QCOMPARE(caliber.values().value(u"ciporigin"_s).toString(), QString());  // not specified
    QVariantMap combined = *m_db.record(grt::Table::Caliber, 1);
    combined.insert(u"ciporigin"_s, u"DE/AT"_s);
    RecordDialog keeps(m_db, grt::Table::Caliber, RecordDialog::Mode::Copy, combined);
    QCOMPARE(keeps.text(u"ciporigin"_s), u"DE/AT"_s);  // an unusual value of an existing record is kept

    // standard and case type too; the stored values stay those of the original database
    QCOMPARE(caliber.text(u"standard"_s), u"WildCat"_s);
    QCOMPARE(caliber.text(u"ciptype"_s), u"5 rimfire"_s);
    QVERIFY(!caliber.findChild<QComboBox *>(u"ciptype"_s)->isEditable());
    caliber.setText(u"standard"_s, u"SAAMI"_s);
    caliber.setText(u"ciptype"_s, u"4 pistol/revolver"_s);
    QCOMPARE(caliber.values().value(u"standard"_s).toString(), u"SAAMI"_s);
    QCOMPARE(caliber.values().value(u"ciptype"_s).toString(), u"4 pistol/revolver"_s);
    combined.insert(u"standard"_s, u"Wildcat"_s);  // spelled differently in a few records
    combined.insert(u"ciptype"_s, u"4 pistol"_s);
    RecordDialog normalized(m_db, grt::Table::Caliber, RecordDialog::Mode::Copy, combined);
    QCOMPARE(normalized.text(u"standard"_s), u"WildCat"_s);
    QCOMPARE(normalized.text(u"ciptype"_s), u"4 pistol"_s);

    if (const QString shots = qEnvironmentVariable("GRT_SHOT_DIR"); !shots.isEmpty())
    {
        copy.grab().save(shots + u"/record.png"_s);
        RecordDialog cal(m_db, grt::Table::Caliber, RecordDialog::Mode::Copy, *m_db.record(grt::Table::Caliber, 1));
        cal.grab().save(shots + u"/record_caliber.png"_s);
    }
    UnitPreferences::instance().setSystem(grt::units::System::Original);
}

void GuiTest::recordsAreAddedCopiedEditedAndDeleted()
{
    const QString before = QSettings().value(u"database/path"_s).toString();
    {
        MainWindow w;
        QVERIFY(w.openDatabase(QStringLiteral(GRT_SOURCE_DIR "/tools/GordonsReloadingTool.plain.db")));
        const auto tabs = w.findChildren<QTabWidget *>();
        QTabWidget *dbTabs = nullptr;
        for (QTabWidget *t : tabs)
            if (t->count() == 3 && t->tabText(2) == u"Propellants"_s) dbTabs = t;
        QVERIFY(dbTabs);
        dbTabs->setCurrentIndex(2);
        QCOMPARE(w.currentTable(), grt::Table::Propellant);
        auto *browser = qobject_cast<TableBrowser *>(dbTabs->currentWidget());
        QVERIFY(browser->selectId(562));
        const auto button = [&](const QString &text)
        {
            for (QPushButton *b : w.findChildren<QPushButton *>())
                if (b->text() == text) return b;
            return static_cast<QPushButton *>(nullptr);
        };
        QVERIFY(button(u"Copy…"_s)->isEnabled());
        QVERIFY(!button(u"Edit…"_s)->isEnabled());  // a record of the original
        QVERIFY(!button(u"Delete"_s)->isEnabled());

        // a copy goes into the own database and appears in the same list
        RecordDialog dlg(m_db, grt::Table::Propellant, RecordDialog::Mode::Copy, *m_db.record(grt::Table::Propellant, 562));
        dlg.setText(u"Ba"_s, u"0.6"_s);
        const int count = browser->findChild<QTableView *>()->model()->rowCount();
        const int id = w.storeRecord(grt::Table::Propellant, RecordDialog::Mode::Copy, 562, dlg.values());
        QCOMPARE(id, grt::Database::kOwnIdBase);
        QCOMPARE(browser->currentId(), id);
        QCOMPARE(browser->findChild<QTableView *>()->model()->rowCount(), count + 1);
        QVERIFY(button(u"Edit…"_s)->isEnabled());
        QVERIFY(button(u"Delete"_s)->isEnabled());

        // the first column says where a record comes from; the filter finds them by it
        auto *view = browser->findChild<QTableView *>();
        QCOMPARE(view->model()->headerData(0, Qt::Horizontal).toString(), u"Source"_s);
        QCOMPARE(view->currentIndex().siblingAtColumn(0).data().toString(), u"Personal"_s);
        QVERIFY(browser->selectId(562));
        QCOMPARE(view->currentIndex().siblingAtColumn(0).data().toString(), u"Original GRT"_s);
        browser->findChild<QLineEdit *>()->setText(u"Personal"_s);
        QCOMPARE(view->model()->rowCount(), 1);
        if (const QString shots = qEnvironmentVariable("GRT_SHOT_DIR"); !shots.isEmpty())
        {
            w.resize(1100, 400);
            browser->findChild<QLineEdit *>()->setText(u"N150"_s);
            w.grab().save(shots + u"/dbsource.png"_s);
        }
        browser->findChild<QLineEdit *>()->clear();
        QVERIFY(browser->selectId(id));

        // an own record can be exported as a project data file (records of the original cannot)
        QVERIFY(button(u"Export…"_s)->isEnabled());
        QTemporaryDir exportDir;
        const QString exported = exportDir.filePath(u"data/propellants/n150.json"_s);
        QDir().mkpath(QFileInfo(exported).absolutePath());
        QVERIFY(w.exportRecord(exported));
        const auto loaded =
            grt::projectdata::load(exportDir.filePath(u"data"_s), grt::Table::Propellant, m_db.schema(grt::Table::Propellant));
        QVERIFY2(loaded.errors.isEmpty(), qPrintable(loaded.errors.join(u'\n')));
        QCOMPARE(loaded.records.size(), std::size_t{1});
        QCOMPARE(loaded.records[0].value(u"Ba"_s).toDouble(), 0.6);

        // the new powder is offered wherever powders are chosen
        SelectRecordDialog select(m_db, grt::Table::Propellant, u"x"_s);
        QVERIFY(select.findChild<TableBrowser *>()->selectId(id));

        // changing it
        QVariantMap changed = dlg.values();
        changed.insert(u"Ba"_s, 0.65);
        QCOMPARE(w.storeRecord(grt::Table::Propellant, RecordDialog::Mode::Edit, id, changed), id);
        QCOMPARE(m_db.record(grt::Table::Propellant, id)->value(u"Ba"_s).toDouble(), 0.65);
        QCOMPARE(m_db.record(grt::Table::Propellant, 562)->value(u"Ba"_s).toDouble(), 0.5814);  // the original is unchanged

        // deleting it
        QVERIFY(w.deleteRecord(false));
        QCOMPARE(browser->findChild<QTableView *>()->model()->rowCount(), count);
        QVERIFY(!browser->selectId(id));
        QVERIFY(browser->selectId(562));
        QVERIFY(!w.deleteRecord(false));  // records of the original cannot be deleted
        QVERIFY(!button(u"Export…"_s)->isEnabled());
        QCOMPARE(m_db.count(grt::Table::Propellant), count);
    }
    QSettings().setValue(u"database/path"_s, before);
}

// Long lists (the countries) must not open as a full-height popup: on Wayland that one closes again at once.
void GuiTest::comboBoxesOpenAsAScrollingList()
{
    auto &app = *qobject_cast<QApplication *>(QCoreApplication::instance());
    const QString before = app.style()->name();
    for (const theme::Mode mode : {theme::Mode::System, theme::Mode::Light, theme::Mode::Dark})
    {
        theme::apply(app, mode);
        QComboBox combo;
        for (int i = 0; i < 300; ++i) combo.addItem(QString::number(i));
        QStyleOptionComboBox opt;
        opt.initFrom(&combo);
        QCOMPARE(combo.style()->styleHint(QStyle::SH_ComboBox_Popup, &opt, &combo), 0);
        if (mode == theme::Mode::Dark)
        {  // disabled entries (menus use WindowText) must look disabled
            const QPalette p = app.palette();
            for (const QPalette::ColorRole role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText})
                QCOMPARE_GT(p.color(QPalette::Active, role).lightness() - p.color(QPalette::Disabled, role).lightness(), 80);
        }
    }
    theme::apply(app, theme::Mode::System);
    auto *proxy = qobject_cast<QProxyStyle *>(app.style());
    QVERIFY(proxy);
    QCOMPARE(proxy->baseStyle()->name(), before);  // the system style itself is kept
}

// File menu entries that act on a load are disabled while none is shown.
void GuiTest::loadMenuEntriesNeedALoad()
{
    MainWindow w;
    QMenu *file = w.mainMenu()->actions().front()->menu();
    const auto action = [&](const QString &start)
    {
        for (QAction *a : file->actions())
            if (a->text().remove(u'&').startsWith(start)) return a;
        return static_cast<QAction *>(nullptr);
    };
    const QStringList needLoad{u"Save as"_s, u"Print preview"_s, u"Print…"_s, u"Export report"_s, u"Overlay reference"_s};
    for (const QString &name : needLoad)
    {
        QVERIFY2(action(name), qPrintable(name));
        QVERIFY2(!action(name)->isEnabled(), qPrintable(name));
    }
    QVERIFY(!action(u"Save"_s)->isEnabled());
    QVERIFY(!action(u"Clear reference"_s)->isEnabled());
    QVERIFY(action(u"New load"_s)->isEnabled());
    QVERIFY(action(u"Open load"_s)->isEnabled());
    QVERIFY(action(u"Settings"_s)->isEnabled());
    if (const QString shots = qEnvironmentVariable("GRT_SHOT_DIR"); !shots.isEmpty())
    {  // for looking at the contrast
        auto &app = *qobject_cast<QApplication *>(QCoreApplication::instance());
        for (const theme::Mode mode : {theme::Mode::Dark, theme::Mode::Light})
        {
            theme::apply(app, mode);
            QApplication::processEvents();
            file->adjustSize();
            file->grab().save(shots + (mode == theme::Mode::Dark ? u"/menu_dark.png"_s : u"/menu_light.png"_s));
        }
        theme::apply(app, theme::Mode::System);
    }

    QVERIFY(w.openLoad(QStringLiteral(GRT_TEST_DATA_DIR "/8x57is.grtload")));
    for (const QString &name : needLoad) QVERIFY2(action(name)->isEnabled(), qPrintable(name));
    QVERIFY(!action(u"Save"_s)->isEnabled());             // nothing changed yet
    QVERIFY(!action(u"Clear reference"_s)->isEnabled());  // no overlay

    // back on the database tab nothing refers to a load
    auto *tabs = qobject_cast<QTabWidget *>(w.centralWidget());
    tabs->setCurrentIndex(0);
    QVERIFY(!action(u"Print preview"_s)->isEnabled());
    tabs->setCurrentIndex(1);
    QVERIFY(action(u"Print preview"_s)->isEnabled());

    // closing the load disables them again
    emit tabs->tabCloseRequested(1);
    QVERIFY(!action(u"Print preview"_s)->isEnabled());
    recentfiles::store({});
}

// Linux (Wayland): the window draws its own, larger title bar with the menu in it; it can be switched off.
void GuiTest::ownTitleBarCanBeSwitched()
{
    MainWindow w;
    w.resize(1000, 700);
    w.setCustomTitleBar(true);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    auto *bar = w.findChild<TitleBar *>();
    QVERIFY(bar);
    QVERIFY(bar->isDecorated());
    QVERIFY(w.windowFlags() & Qt::FramelessWindowHint);
    QCOMPARE(w.contentsMargins().left(), 5);  // the border to resize the window
    QVERIFY(bar->height() >= 34);
    QVERIFY(w.mainMenu()->isVisible());
    QCOMPARE(w.mainMenu()->parentWidget(), bar);  // the menu sits in the title bar row
    for (const char *name : {"minimizeButton", "maximizeButton", "closeButton"})
    {
        auto *b = bar->findChild<QAbstractButton *>(QLatin1String(name));
        QVERIFY2(b && b->isVisible(), name);
        QVERIFY(b->width() >= 40);
    }
    // the title follows the window (and names the shown load)
    QVERIFY(w.openLoad(QStringLiteral(GRT_TEST_DATA_DIR "/8x57is.grtload")));
    QVERIFY(w.windowTitle().startsWith(u"8x57is"_s));
    bool titled = false;
    for (QLabel *l : bar->findChildren<QLabel *>()) titled = titled || l->text() == w.windowTitle();
    QVERIFY(titled);

    // switched off: the desktop's title bar, the row only holds the menu
    w.setCustomTitleBar(false);
    QVERIFY(!(w.windowFlags() & Qt::FramelessWindowHint));
    QVERIFY(!bar->isDecorated());
    QCOMPARE(w.contentsMargins().left(), 0);
    QVERIFY(!bar->findChild<QAbstractButton *>(u"closeButton"_s)->isVisible());
    QCOMPARE(w.mainMenu()->actions().size(), 4);  // File, View, Tools, Help still there
    w.setCustomTitleBar(true);
    QVERIFY(bar->isDecorated());
    recentfiles::store({});

    // the choice is a setting
    const bool before = TitleBar::stored();
    SettingsDialog dlg;
    QSignalSpy changed(&dlg, &SettingsDialog::customTitleBarChanged);
    auto *box = dlg.findChild<QCheckBox *>();
    QVERIFY(box);
    QCOMPARE(box->isChecked(), before);
    box->setChecked(!before);
    dlg.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
    QCOMPARE(changed.count(), 1);
    QCOMPARE(TitleBar::stored(), !before);
    dlg.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Cancel)->click();  // undone
    QCOMPARE(changed.count(), 2);
    QCOMPARE(TitleBar::stored(), before);
}

// Measurements tab > Fit to measurements: the fitted values go into the load, or into an own propellant record.
// The safety notice at start-up: two buttons, "Do not show this again"; hidden only when the setting says so.
void GuiTest::disclaimerAtStartup()
{
    QSettings().remove(u"ui/hideDisclaimer"_s);
    QVERIFY(disclaimer::wanted());
    const auto box = disclaimer::create();
    QVERIFY(box->text().contains(u"dangerous"_s));
    QVERIFY(box->informativeText().contains(u"not responsible"_s));
    QVERIFY(box->checkBox());
    QCOMPARE(box->buttons().size(), 2);
    if (const QString shots = qEnvironmentVariable("GRT_SHOT_DIR"); !shots.isEmpty())
    {
        box->show();
        box->grab().save(shots + u"/disclaimer.png"_s);
    }
    QSettings().setValue(u"ui/hideDisclaimer"_s, true);
    QVERIFY(!disclaimer::wanted());
    QVERIFY(disclaimer::confirm());  // hidden: confirmed without a dialog
    QSettings().remove(u"ui/hideDisclaimer"_s);
}

void GuiTest::fitToMeasurementsChangesThePowder()
{
    UnitPreferences::instance().setSystem(grt::units::System::Metric);
    LoadView view;
    QVERIFY(view.load(QStringLiteral(GRT_TEST_DATA_DIR "/POWDER-MEASURE-TEMPLATE.grtload")));
    auto *button = [&]
    {
        for (QPushButton *b : view.measurements()->findChildren<QPushButton *>())
            if (b->text() == u"Fit to measurements…"_s) return b;
        return static_cast<QPushButton *>(nullptr);
    }();
    QVERIFY(button);
    QVERIFY(button->isEnabled());  // the template has measured charges

    // the dialog: fits right away, Save hands over the result
    QStringList warnings;
    grt::LoadFile file = *grt::LoadFile::read(QStringLiteral(GRT_TEST_DATA_DIR "/POWDER-MEASURE-TEMPLATE.grtload"));
    const auto samples = grt::powderfit::samples(file, u"template"_s, &warnings);
    QCOMPARE(samples.size(), std::size_t{2});
    PowderFitDialog dlg(samples, warnings);
    std::optional<grt::powderfit::Result> saved;
    dlg.setSaveHandler(
        [&](const grt::powderfit::Result &r)
        {
            saved = r;
            return true;
        });
    dlg.runFit();
    QCOMPARE(dlg.results().size(), std::size_t{2});  // both models
    for (const auto &r : dlg.results())
    {
        QVERIFY(r.improved);
        QCOMPARE(r.params.size(), std::size_t{1});  // two charges: Ba only
        QCOMPARE_LT(r.velocityRmsAfter, r.velocityRmsBefore);
    }
    QPushButton *save = nullptr;
    for (QPushButton *b : dlg.findChildren<QPushButton *>())
        if (b->text().startsWith(u"Save as own propellant"_s)) save = b;
    QVERIFY(save && save->isEnabled());
    save->click();
    QVERIFY(saved);
    QCOMPARE(saved->model, grt::powderfit::Model::Original);  // a record holds the original model's values
    if (const QString shots = qEnvironmentVariable("GRT_SHOT_DIR"); !shots.isEmpty()) dlg.grab().save(shots + u"/powderfit.png"_s);
    QVERIFY(dlg.findChild<QTableWidget *>()->item(0, 5)->text().endsWith(u"%"_s));  // original, after
    QVERIFY(dlg.findChild<QTableWidget *>()->item(0, 8)->text().endsWith(u"%"_s));  // physical, after

    // through the button: shots about 5 % faster than simulated need a higher burn rate; "Apply to this load" puts
    // the fitted value into the propellant
    const double before = file.propellant.number(u"Ba");
    MeasurementView *m = view.measurements();
    m->selectCharge(0);
    QCOMPARE(m->pasteShots(u"895\n900\n905\n898\n902\n895\n900\n905\n898\n902"_s), 10);
    m->selectCharge(1);
    QCOMPARE(m->pasteShots(u"955\n960\n965\n958\n962\n955\n960\n965\n958\n962"_s), 10);
    QVERIFY(view.save(QDir::temp().filePath(u"grt-fit-test.grtload"_s)));  // unmodified again: only the fit can modify it now
    QVariantMap record;
    view.setRecordSaver(
        [&](grt::Table, const QVariantMap &values)
        {
            record = values;
            return 42;
        });
    QTimer::singleShot(0,
                       [&]
                       {
                           auto *fit = qobject_cast<PowderFitDialog *>(QApplication::activeModalWidget());
                           QVERIFY(fit);
                           for (QPushButton *b : fit->findChildren<QPushButton *>())
                               if (b->text() == u"Apply to this load"_s) b->click();
                       });
    QVERIFY(!view.isModified());
    button->click();
    QVERIFY(view.isModified());
    QFile::remove(QDir::temp().filePath(u"grt-fit-test.grtload"_s));
    grt::LoadFile after;
    QTemporaryDir dir;
    QVERIFY(view.save(dir.filePath(u"fitted.grtload"_s)));
    after = *grt::LoadFile::read(dir.filePath(u"fitted.grtload"_s));
    QVERIFY2(after.propellant.number(u"Ba") > before * 1.02, qPrintable(u"Ba %1 -> %2"_s.arg(before).arg(after.propellant.number(u"Ba"))));
    QVERIFY(grt::readPhysicalBurn(after.propellant));  // and the physical model's values

    // a record made from it: the database's columns, 21 degC, the photos of the powder
    const QVariantMap rec = grt::powderfit::toRecord(after.propellant, *saved, m_db.schema(grt::Table::Propellant), u"test"_s);
    QCOMPARE(rec.value(u"pt"_s).toDouble(), 21.0);
    QVERIFY(rec.contains(u"imageUuid"_s));
    QVERIFY(grt::projectdata::check(grt::Table::Propellant, rec, m_db.schema(grt::Table::Propellant)).isEmpty());
    UnitPreferences::instance().setSystem(grt::units::System::Original);
}

// View > Language shows the flag of each language's country (drawn, no image files).
void GuiTest::languagesHaveFlags()
{
    for (const char *lang : {"en_US", "de_DE", "fr_FR", "it_IT"})
    {
        const QIcon icon = flags::forLanguage(QLatin1String(lang));
        QVERIFY2(!icon.isNull(), lang);
        const QImage img = icon.pixmap(32, 32).toImage();
        QCOMPARE(img.width(), 32);
        QVERIFY(img.pixelColor(16, 16).alpha() == 255);  // the flag fills the middle
        QVERIFY(img.pixelColor(16, 2).alpha() == 0);     // 3:2 flag in a square icon: transparent above
    }
    // the colors are the country's: black-red-gold, blue-white-red, green-white-red
    const auto at = [](const char *lang, int x, int y)
    { return flags::forLanguage(QLatin1String(lang)).pixmap(48, 48).toImage().pixelColor(x, y); };
    QCOMPARE(at("de_DE", 24, 10), QColor(Qt::black));
    QCOMPARE(at("fr_FR", 6, 24), QColor(0x00, 0x55, 0xa4));
    QCOMPARE(at("it_IT", 6, 24), QColor(0x00, 0x92, 0x46));
    QVERIFY(flags::forLanguage(u"xx"_s).isNull());

    MainWindow w;
    QMenu *view = nullptr;
    for (QAction *a : w.mainMenu()->actions())
        if (a->text() == u"&View"_s) view = a->menu();
    QVERIFY(view);
    QMenu *lang = nullptr;
    for (QAction *a : view->actions())
        if (a->text() == u"&Language"_s) lang = a->menu();
    QVERIFY(lang);
    QVERIFY(lang->actions().front()->icon().isNull());                              // "System default"
    for (QAction *a : lang->actions()) QCOMPARE(a->font().bold(), a->isChecked());  // the chosen language is bold
    if (const QString shots = qEnvironmentVariable("GRT_SHOT_DIR"); !shots.isEmpty())
    {
        lang->addAction(flags::forLanguage(u"de_DE"_s), u"Deutsch (test)"_s);
        lang->addAction(flags::forLanguage(u"fr_FR"_s), u"français (test)"_s);
        lang->addAction(flags::forLanguage(u"it_IT"_s), u"italiano (test)"_s);
        lang->addAction(flags::forLanguage(u"en_US"_s), u"English (test)"_s);
        for (const theme::Mode mode : {theme::Mode::Light, theme::Mode::Dark})
        {
            theme::apply(*qobject_cast<QApplication *>(QCoreApplication::instance()), mode);
            QApplication::processEvents();
            lang->adjustSize();
            lang->grab().save(shots + (mode == theme::Mode::Dark ? u"/flags_dark.png"_s : u"/flags_light.png"_s));
        }
        theme::apply(*qobject_cast<QApplication *>(QCoreApplication::instance()), theme::Mode::System);
    }
}

QTEST_MAIN(GuiTest)
#include "tst_gui.moc"
