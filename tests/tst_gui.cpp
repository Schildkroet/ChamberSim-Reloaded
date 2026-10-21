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

#include <limits>
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
#include <QDialog>
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
#include <QProgressBar>
#include <QGuiApplication>
#include <QScreen>
#include <QPushButton>
#include <QTabBar>
#include <QTabWidget>
#include <QSettings>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QHeaderView>
#include <QTableView>
#include <QTableWidget>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QProxyStyle>
#include <QStyleOptionComboBox>
#include <QTimer>
#include <QTranslator>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QStyleOptionButton>
#include <QtTest>

#include "about.h"
#include "burnratedialog.h"
#include "recordviewdialog.h"
#include "primer.h"
#include "shotparts.h"
#include "unitconverterdialog.h"
#include "theme.h"
#include "flags.h"
#include "titlebar.h"
#include "dialogframe.h"
#include "windowborder.h"
#include "appfont.h"
#include "cartridgeview.h"
#include "chamberview.h"
#include "curvechart.h"
#include "photoview.h"
#include "chronographimportdialog.h"
#include "database.h"
#include "loadbuilder.h"
#include "loadview.h"
#include "physicalpowders.h"
#include "recorddialog.h"
#include "measurement.h"
#include "projectdata.h"
#include "measurementview.h"
#include "powderfitdialog.h"
#include "propertypanel.h"
#include "recentfiles.h"
#include "searchfilter.h"
#include "reportview.h"
#include "qualityrow.h"
#include "screenfit.h"
#include "selectrecorddialog.h"
#include "session.h"
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

// the buttons of the input tabs: "Select Caliber", "Select Projectile", "Select Propellant"
bool isSelectButton(const QPushButton *b)
{
    const QString t = b->text();
    return t == u"Select Caliber"_s || t == u"Select Projectile"_s || t == u"Select Propellant"_s;
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
    void shotshellPartsAreChoices();
    void shotshellIsDrawnAsAShell();
    void panelShowsAndEditsInTheChosenUnit();
    void perFieldUnitOverridesTheSystem();
    void resultsFollowTheUnitSystem();
    void ladderCurvesAndFocus();
    void fontSizeIsLimitedAndStored();
    void fontSizeAppliesToTheWholeApplication();
    void settingsDialogChangesAndRevertsTheFont();
    void lastLoadsAreReopened();
    void fileMenuHasSettingsAndPrinting();
    void helpMenuHasAbout();
    void unitsLiveInSettingsNotInViewMenu();
    void layoutIsRemembered();
    void recentFilesAreListedInTheFileMenu();
    void searchToleratesSpellings();
    void noteWithoutPhysicalBurnValues();
    void hoveringATableRowShowsItsMeasureInTheDrawing();
    void reportTabRendersTheLoad();
    void startingOnTheReportTabRendersAfterTheShow();
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
    void recordViewerShowsAllData();
    void selectPropellantShowsTheQuality();
    void dialogsFitTheScreen();
    void startPageHasLoadButtons();
    void newLoadAndDatabaseShowTheQuality();
    void recordDialogCalculatesTheBoreCrossSection();
    void ownPropellantWithPhotoWorksWithoutTheOriginal();
    void disclaimerAtStartup();
    void checkBoxesVisibleInDarkMode();
    void dialogsHaveAVisibleBorder();
    void settingsInfoTextIsNotCutOff();
    void squareCornersWithoutCompositing();
    void roundedCornersCanBeSwitchedOffInTheSettings();
    void scrollBarsVisibleInDarkMode();
    void chartCursorShowsTheValues();
    void editsCanBeUndoneAndRedone();
    void newLoadDialogKeepsEditsOfTheSecondStep();
    void overpressureIsWarnedAbout();
    void fitToMeasurementsChangesThePowder();
    void originalDatabaseIsChosenInTheSettings();
    void burnRateTableSortsAndFilters();
    void unitConverterConvertsAllUnits();
    void solverCalibrationIsChosenInTheSettings();
    void grtFilesCanUseGrtOriginal();
    void overallLengthAndSeatingDepthStayInStep();
    void recordDialogShowsReadableFieldsInUserUnits();
    void recordsAreAddedCopiedEditedAndDeleted();
    void comboBoxesOpenAsAScrollingList();
    void loadMenuEntriesNeedALoad();
    void ownTitleBarCanBeSwitched();
    void languagesHaveFlags();

private:
    static int idOf(csr::Database &db, csr::Table t, const QString &where);

    csr::Database m_db;
};

void GuiTest::initTestCase()
{
    // the shotshell parts are in the resources of the program only: the tests read the file of the source tree
    csr::setShotPartsFile(QStringLiteral(CSR_SOURCE_DIR "/data/shotshell/parts.json"));
    // never touch the user's real settings or own database
    QStandardPaths::setTestModeEnabled(true);
    QFile::remove(csr::Database::defaultOwnPath());  // start every run with an empty own database
    static QTemporaryDir settingsDir;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDir.path());
    QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));
    UnitPreferences::instance().resetFieldUnits();
    UnitPreferences::instance().setSystem(csr::units::System::Original);

    const QString dbPath = QStringLiteral(CSR_SOURCE_DIR "/tools/GordonsReloadingTool.plain.db");
    if (!QFile::exists(dbPath)) QSKIP("plain database not available");
    QVERIFY(m_db.open(dbPath));
}

int GuiTest::idOf(csr::Database &db, csr::Table t, const QString &where)
{
    QSqlQuery q(db.handle());
    if (q.exec(u"SELECT id FROM %1 WHERE geloescht = 0 AND %2 ORDER BY id"_s.arg(QLatin1String(csr::tableName(t)), where)) && q.next())
        return q.value(0).toInt();
    return -1;
}

void GuiTest::unitConverterConvertsAllUnits()
{
    UnitConverterDialog dialog;
    const auto value = [&](const QString &symbol)
    {
        for (int i = 0; i < dialog.rowCount(); ++i)
            if (dialog.symbolAt(i) == symbol) return QLocale().toDouble(dialog.valueAt(i));
        return std::numeric_limits<double>::quiet_NaN();
    };
    dialog.setQuantity(csr::units::Quantity::Pressure);
    dialog.setFromUnit(u"bar"_s);
    dialog.setInput(u"1000"_s);
    QCOMPARE(dialog.rowCount(), 4);
    QCOMPARE(value(u"bar"_s), 1000.0);
    QVERIFY(std::abs(value(u"psi"_s) - 14503.77) < 0.01);
    QCOMPARE(value(u"MPa"_s), 100.0);
    // an offset unit: Celsius to Fahrenheit
    dialog.setQuantity(csr::units::Quantity::Temperature);
    dialog.setFromUnit(u"cel"_s);
    dialog.setInput(u"100"_s);
    QVERIFY(std::abs(value(u"°F"_s) - 212.0) < 1e-6);
    // text that is no number clears the values
    dialog.setInput(u"abc"_s);
    QVERIFY(dialog.valueAt(0).isEmpty());
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
    QCOMPARE(prefs.profile(), csr::CalibrationProfile::Physical);  // the default: the own physical model
    prefs.setProfile(csr::CalibrationProfile::Original);
    SettingsDialog dlg;
    QComboBox *calibration = nullptr;
    for (auto *c : dlg.findChildren<QComboBox *>())
        if (c->findText(SolverPreferences::name(csr::CalibrationProfile::Published)) >= 0) calibration = c;
    QVERIFY(calibration);
    QSignalSpy changed(&prefs, &SolverPreferences::changed);
    calibration->setCurrentIndex(calibration->findText(SolverPreferences::name(csr::CalibrationProfile::Published)));
    dlg.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
    QCOMPARE(prefs.profile(), csr::CalibrationProfile::Published);
    QCOMPARE(csr::calibrationProfile(), csr::CalibrationProfile::Published);
    QCOMPARE(changed.count(), 1);
    QCOMPARE(QSettings().value(u"solver/calibration"_s).toString(), u"published"_s);
    prefs.setProfile(csr::CalibrationProfile::Original);  // leave the others as they were
    QCOMPARE(csr::calibrationProfile(), csr::CalibrationProfile::Original);

    // an open load is recalculated with the other constants (the shown result comes from the ladder simulation)
    LoadView view;
    QString error;
    QVERIFY2(view.load(QStringLiteral(CSR_FIXTURE_DIR "/8x57is.grtload"), &error), qPrintable(error));
    const double original = view.result().maxPressure_bar;
    QVERIFY(original > 0.0);
    prefs.setProfile(csr::CalibrationProfile::Published);
    const double published = view.result().maxPressure_bar;
    QVERIFY(std::abs(published / original - 1.0) > 0.01);
    prefs.setProfile(csr::CalibrationProfile::Original);
    QCOMPARE(view.result().maxPressure_bar, original);
    const auto modelLine = [&]
    {
        for (QLabel *l : view.findChildren<QLabel *>())
            if (l->text().startsWith(u"Model: "_s)) return l->text();
        return QString();
    };
    QCOMPARE(modelLine(), u"Model: GRT Original"_s);              // above the results: only the model used
    prefs.setProfile(csr::CalibrationProfile::Physical, false);   // the default again, for the other tests
    QVERIFY(modelLine().startsWith(u"Model: Physical Model"_s));  // (the sample's N150 has the original's lot values: with the note)
}

void GuiTest::grtFilesCanUseGrtOriginal()
{
    auto &prefs = SolverPreferences::instance();
    QCOMPARE(prefs.profile(), csr::CalibrationProfile::Physical);
    QVERIFY(!prefs.originalForGrtFiles());  // off by default: every load uses the chosen model
    LoadView view;
    QString error;
    QVERIFY2(view.load(QStringLiteral(CSR_FIXTURE_DIR "/8x57is.grtload"), &error), qPrintable(error));
    QVERIFY(view.isGrtFile());
    const double physical = view.result().maxPressure_bar;
    prefs.setProfile(csr::CalibrationProfile::Original, false);
    const double original = view.result().maxPressure_bar;
    const double originalVelocity = view.result().muzzleVelocity_ms;
    prefs.setProfile(csr::CalibrationProfile::Physical, false);
    QCOMPARE(view.result().maxPressure_bar, physical);
    QVERIFY(std::abs(physical / original - 1.0) > 0.01);

    // Settings > Solver: the option, applied to the open load
    SettingsDialog dlg;
    auto *box = dlg.findChild<QCheckBox *>(u"originalForGrtFiles"_s);
    QVERIFY(box && !box->isChecked());
    box->setChecked(true);
    dlg.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
    QVERIFY(prefs.originalForGrtFiles());
    QCOMPARE(QSettings().value(u"solver/originalForGrtFiles"_s).toBool(), true);
    QCOMPARE(prefs.profile(), csr::CalibrationProfile::Physical);  // the chosen model stays
    QCOMPARE(view.result().maxPressure_bar, original);             // exactly what GRT Original gives
    QCOMPARE(view.result().muzzleVelocity_ms, originalVelocity);
    QVERIFY(view.findChild<QWidget *>(u"burnValuesNotice"_s)->isHidden());  // no note about the Physical Model's values
    bool named = false;
    for (QLabel *l : view.findChildren<QLabel *>()) named |= l->text().startsWith(u"Model: GRT Original (original GRT file"_s);
    QVERIFY(named);

    // the model follows the format of the file: a *.csrload uses the chosen one, an export to *.grtload GRT Original again
    QTemporaryDir dir;
    QVERIFY(view.save(dir.filePath(u"load.csrload"_s)));
    QVERIFY(!view.isGrtFile());
    QCOMPARE(view.result().maxPressure_bar, physical);
    QVERIFY(view.save(dir.filePath(u"load.grtload"_s)));
    QCOMPARE(view.result().maxPressure_bar, original);

    prefs.setOriginalForGrtFiles(false);  // the default again, for the other tests
    QCOMPARE(view.result().maxPressure_bar, physical);
}

void GuiTest::tableBrowserFiltersAndSelects()
{
    TableBrowser b(csr::Table::Propellant);
    b.setDatabase(&m_db);
    QVERIFY(b.selectId(562));
    QCOMPARE(b.currentId(), 562);
    QVERIFY(!b.selectId(-1));
    b.setDatabase(nullptr);
    QCOMPARE(b.currentId(), -1);

    // the filter finds "8 x 57 IS" whatever the spelling
    TableBrowser calibers(csr::Table::Caliber);
    calibers.setDatabase(&m_db);
    auto *edit = calibers.findChild<QLineEdit *>();
    auto *view = calibers.findChild<QTableView *>();
    QVERIFY(edit && view);
    edit->setText(u"8 x 57"_s);
    const int spaced = view->model()->rowCount();
    QVERIFY(spaced > 0);
    edit->setText(u"8x57"_s);
    QCOMPARE(view->model()->rowCount(), spaced);
}

void GuiTest::rowFilterRestrictsTheTable()
{
    TableBrowser b(csr::Table::Projectile);
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
    const int cal = idOf(m_db, csr::Table::Caliber, u"cipname = '8 x 57 IS'"_s);
    const int prj = idOf(m_db, csr::Table::Projectile, u"mname = 'Sellier-Bellot' AND pname = 'FMJ-BT 2910'"_s);
    const int pro = idOf(m_db, csr::Table::Propellant, u"mname = 'Vihtavuori' AND pname = 'N150' AND lotid = '2021-11-13'"_s);
    QVERIFY(cal > 0 && prj > 0 && pro > 0);
    QVERIFY(dlg.select(cal, prj, pro));

    const csr::LoadFile lf = dlg.result();
    QCOMPARE(lf.caliber.text(u"CaliberName"), u"8 x 57 IS (8 x 57 JS)"_s);
    QCOMPARE(lf.propellant.text(u"pname"), u"N150"_s);
    QCOMPARE(lf.gun.number(u"xe"), 600.0);                            // rifle default
    QVERIFY(std::abs(lf.projectile.number(u"gdepth") - 7.9) < 0.01);  // default: cartridge at its maximum length (57 + 32.9 - 82)
    const csr::SimResult r = csr::simulate(csr::SimInput::fromLoad(lf));
    QVERIFY(r.ok());
    QVERIFY(r.maxPressure_bar > 1000.0 && r.maxPressure_bar < 5000.0);
    dlg.grab().save(QDir::temp().filePath(u"csr_newload_dialog.png"_s));
}

void GuiTest::newLoadDialogKeepsEditsOfTheSecondStep()
{
    UnitPreferences::instance().setSystem(csr::units::System::Original);  // mm, g
    NewLoadDialog dlg(m_db);
    const int cal = idOf(m_db, csr::Table::Caliber, u"cipname = '8 x 57 IS'"_s);
    const int prj = idOf(m_db, csr::Table::Projectile, u"mname = 'Sellier-Bellot' AND pname = 'FMJ-BT 2910'"_s);
    const int pro = idOf(m_db, csr::Table::Propellant, u"mname = 'Vihtavuori' AND pname = 'N150' AND lotid = '2021-11-13'"_s);
    QVERIFY(dlg.select(cal, prj, pro));  // second step, with the defaults

    const auto button = [&](const QString &text)
    {
        for (QPushButton *b : dlg.findChildren<QPushButton *>())
            if (b->text() == text) return b;
        return static_cast<QPushButton *>(nullptr);
    };
    QPushButton *create = button(u"Create load"_s);
    QVERIFY(create);
    const auto barrel = [&]
    {
        for (QDoubleSpinBox *s : dlg.findChildren<QDoubleSpinBox *>())
            if (s->suffix().contains(u"mm"_s) && s->value() > 300.0 && s->value() < 1000.0) return s;
        return static_cast<QDoubleSpinBox *>(nullptr);
    };
    QDoubleSpinBox *b = barrel();
    QVERIFY(b);
    const double standard = b->value();
    b->setValue(standard - 100.0);
    QCOMPARE(dlg.result().gun.number(u"xe"_s), standard - 100.0);  // the edited value goes into the load

    button(u"< Back"_s)->click();  // the same selection: the edit stays
    button(u"Next >"_s)->click();
    QCOMPARE(barrel()->value(), standard - 100.0);

    button(u"< Back"_s)->click();  // another powder: the data are filled in again
    const int other = idOf(m_db, csr::Table::Propellant, u"mname = 'Vihtavuori' AND pname = 'N140' AND lotid = '2021-11-13'"_s);
    if (other > 0)
    {
        QVERIFY(dlg.select(cal, prj, other));
        QCOMPARE(barrel()->value(), standard);
    }
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

void GuiTest::shotshellIsDrawnAsAShell()
{
    csr::LoadFile file;
    file.caliber.set(u"shotshell", u"true"_s);
    file.caliber.set(u"caselen", 70.0);
    csr::SimInput in;
    in.shotshell = true;
    in.caseLength_mm = 70.0;
    in.caseVolume_cm3 = 17.24;
    in.bulletDiameter_mm = 18.2;
    in.bulletLength_mm = in.seatingDepth_mm = 40.5;
    in.bulkDensity_kgm3 = 650.0;
    in.charge_g = 1.13;
    CartridgeView view;
    view.resize(420, 520);
    view.setLoad(file, in);
    const auto &round = view.round();
    QVERIFY(round.shotshell && round.geometry.valid);
    QVERIFY(std::abs(round.hullDiameter - 20.3) < 0.01);                                   // 12 gauge: 20.3 mm hull
    QVERIFY(round.powderTop > round.floorHeight && round.powderTop < round.bulletBase());  // the powder lies on the base wad, below the wad
    const QImage img = view.grab().toImage();
    QVERIFY(!img.isNull());
    // the hull is red: there are red pixels in the drawing (a bullet alone has none)
    int red = 0;
    for (int y = 0; y < img.height(); ++y)
        for (int x = 0; x < img.width(); ++x)
        {
            const QColor c = img.pixelColor(x, y);
            red += c.red() > 140 && c.green() < 110 && c.blue() < 110;
        }
    if (const QString shots = qEnvironmentVariable("CSR_SHOT_DIR"); !shots.isEmpty()) img.save(shots + u"/shotshell.png"_s);
    QVERIFY2(red > 100, qPrintable(QString::number(red)));
    ChamberView chamber;
    chamber.setLoad(file, in);  // without C.I.P. dimensions: nothing to draw, but no crash
    QVERIFY(!chamber.grab().toImage().isNull());
}

void GuiTest::shotshellPartsAreChoices()
{
    csr::EntryList caliber;
    caliber.set(u"shotshell", u"true"_s);
    QVERIFY(csr::ensureShotParts(caliber));   // an older shotshell load gets the entries ...
    QVERIFY(!csr::ensureShotParts(caliber));  // ... once
    // an individual part (a file of an earlier version) is replaced by the group it belongs to
    caliber.set(u"shothull", u"winchester-aa-hs"_s);
    caliber.set(u"shotwad", u"12-waa12sl"_s);
    QVERIFY(csr::ensureShotParts(caliber));
    QCOMPARE(caliber.text(u"shothull"), u"group-high"_s);
    QCOMPARE(caliber.text(u"shotwad"), u"group-low"_s);
    QVERIFY(!csr::ensureShotParts(caliber));
    caliber.set(u"shotprimer", u"nonsense"_s);  // an id of another version stays visible
    PropertyPanel panel;
    panel.setSections({{0, u"caliber"_s, u"Caliber"_s, &caliber}});
    QTableWidget *t = tableOf(panel);
    const QString hull = t->item(rowOf(t, u"shothull"_s), 1)->text();
    QCOMPARE(hull, u"High pressure"_s);  // the brands are in the tooltip only
    const QString wad = t->item(rowOf(t, u"shotwad"_s), 1)->text();
    QCOMPARE(wad, u"Low pressure"_s);
    QCOMPARE(t->item(rowOf(t, u"shotprimer"_s), 1)->text(), u"nonsense"_s);
    // the tooltip of a group lists the brands (the individual parts), one per line, and nothing else
    const QString tip = t->item(rowOf(t, u"shotwad"_s), 1)->toolTip();
    QVERIFY2(tip.contains(u"Remington TGT12"_s) && tip.contains(u"Winchester WAA12SL"_s), qPrintable(tip));
    QVERIFY2(!tip.contains(u"group"_s, Qt::CaseInsensitive) && !tip.contains(u"×"_s), qPrintable(tip));
    QVERIFY(t->item(rowOf(t, u"shothull"_s), 1)->toolTip().contains(u"Winchester AA / HS"_s));
    caliber.set(u"shothull", QString());
    panel.syncValues();
    QCOMPARE(t->item(rowOf(t, u"shothull"_s), 1)->text(), u"not specified (average)"_s);
    // a cartridge that is no shotshell gets nothing
    csr::EntryList rifle;
    rifle.set(u"shotshell", u"false"_s);
    QVERIFY(!csr::ensureShotParts(rifle));
}

void GuiTest::codedFieldsShowReadableText()
{
    csr::EntryList list;
    const auto add = [&](const char *name, const char *value, const char *descr)
    {
        list.entries().push_back({.name = QString::fromLatin1(name),
                                  .value = QString::fromLatin1(value),
                                  .unit = {},
                                  .type = u"decimal"_s,
                                  .descr = QString::fromLatin1(descr)});
    };
    add("gTailType", "2", "projectile base type");
    add("shotshell", "true", "true if shotshell");
    PropertyPanel panel;
    panel.setSections({{0, u"projectile"_s, u"Projectile"_s, &list}});
    QTableWidget *t = tableOf(panel);
    const int rTail = rowOf(t, u"gTailType"_s), rShell = rowOf(t, u"shotshell"_s);
    QCOMPARE(t->item(rTail, 1)->text(), u"boat tail"_s);  // not "2"
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
    t->item(rShell, 1)->setText(u"no"_s);
    QCOMPARE(spy.at(1).at(2).toString(), u"false"_s);
    // something that is not a choice is ignored and the old text is restored
    t->item(rTail, 1)->setText(u"banana"_s);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(t->item(rTail, 1)->text(), u"hollow base"_s);

    // a code the program does not know stays visible instead of being hidden
    csr::EntryList odd;
    odd.entries().push_back({.name = u"gTailType"_s, .value = u"3"_s, .unit = {}, .type = u"decimal"_s, .descr = u"base"_s});
    PropertyPanel panel2;
    panel2.setSections({{0, u"projectile"_s, u"Projectile"_s, &odd}});
    QCOMPARE(tableOf(panel2)->item(rowOf(tableOf(panel2), u"gTailType"_s), 1)->text(), u"type 3"_s);
}

// The solver uses the seating depth; the cartridge overall length is derived from it, so editing the length must change the depth.
void GuiTest::editsCanBeUndoneAndRedone()
{
    UnitPreferences::instance().setSystem(csr::units::System::Original);  // mm
    LoadView view;
    QString error;
    QVERIFY2(view.load(QStringLiteral(CSR_FIXTURE_DIR "/8x57is.grtload"), &error), qPrintable(error));
    QVERIFY(!view.canUndo());
    QVERIFY(!view.canRedo());
    const auto edit = [&](const QString &key, const QString &text)
    {
        for (PropertyPanel *panel : view.findChildren<PropertyPanel *>())
        {
            QTableWidget *t = tableOf(*panel);
            if (const int row = rowOf(t, key); row > 0) return t->item(row, 1)->setText(text);
        }
        QFAIL("field not found");
    };
    const double v0 = view.result().muzzleVelocity_ms;

    edit(u"psadd"_s, u"50"_s);
    edit(u"oal"_s, u"78.5"_s);
    QVERIFY(view.canUndo());
    QVERIFY(view.isModified());
    const double v2 = view.result().muzzleVelocity_ms;

    view.undo();  // the cartridge length
    QVERIFY(view.canRedo());
    QVERIFY(view.isModified());
    view.undo();  // the start pressure: the loaded state again
    QVERIFY(!view.canUndo());
    QVERIFY(!view.isModified());
    QCOMPARE(view.result().muzzleVelocity_ms, v0);

    view.redo();
    view.redo();
    QVERIFY(!view.canRedo());
    QVERIFY(view.isModified());
    QCOMPARE(view.result().muzzleVelocity_ms, v2);

    // a saved state is the clean one; a new edit after an undo drops the redo steps
    QTemporaryDir dir;
    QVERIFY(view.save(dir.filePath(u"x.csrload"_s), &error));
    QVERIFY(!view.isModified());
    view.undo();
    QVERIFY(view.isModified());
    view.redo();
    QVERIFY(!view.isModified());
    view.undo();
    edit(u"psadd"_s, u"100"_s);
    QVERIFY(!view.canRedo());
    view.undo();
    view.undo();
    QVERIFY(view.isModified());  // the saved state was on the dropped branch
}

void GuiTest::overallLengthAndSeatingDepthStayInStep()
{
    UnitPreferences::instance().setSystem(csr::units::System::Original);  // mm
    LoadView view;
    QString error;
    QVERIFY2(view.load(QStringLiteral(CSR_FIXTURE_DIR "/8x57is.grtload"), &error), qPrintable(error));
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
        if (!view.save(path, &err)) return std::optional<csr::LoadFile>{};
        return csr::LoadFile::read(path);
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

    // a trimmed case: the cartridge length stays, seating depth and case volume follow
    const double volume = lf->caliber.number(u"casevol"_s);
    edit(u"caselen"_s, u"56"_s);
    lf = saved();
    QVERIFY(lf);
    QVERIFY(std::abs(lf->caliber.number(u"oal"_s) - 80.5) < 1e-6);
    QVERIFY(std::abs(lf->projectile.number(u"gdepth"_s) - 8.4) < 1e-6);
    QVERIFY(lf->caliber.number(u"casevol"_s) < volume);

    // the initial gas pressure exists in two sections; the solver uses the caliber's. With a primer it is the primer's start pressure
    // plus the additional one: editing the total sets the additional one, editing that one the total.
    edit(u"gpressure"_s, u"400"_s);
    lf = saved();
    QVERIFY(lf);
    QCOMPARE(lf->caliber.number(u"ps"_s), 400.0);
    const auto savedOwn = [&]  // the own format keeps primer and psadd (the export leaves them out)
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(u"x.csrload"_s);
        QString err;
        if (!view.save(path, &err)) return std::optional<csr::LoadFile>{};
        return csr::LoadFile::read(path);
    };
    lf = savedOwn();
    QVERIFY(lf);
    QVERIFY(lf->caliber.find(u"psadd"_s));
    const double primerStart = csr::primerStartPressure_bar(lf->caliber.text(u"primer"_s), lf->caliber.number(u"casevol"_s));
    QVERIFY(std::abs(lf->caliber.number(u"psadd"_s) - (400.0 - primerStart)) < 1e-6);
    edit(u"psadd"_s, u"30"_s);
    lf = saved();
    QVERIFY(lf);
    QVERIFY(std::abs(lf->projectile.number(u"gpressure"_s) - (primerStart + 30.0)) < 1e-6);
    QVERIFY(std::abs(lf->caliber.number(u"ps"_s) - (primerStart + 30.0)) < 1e-6);
    QVERIFY(!lf->caliber.find(u"psadd"_s));  // not in the export
    // the primer is shown by its name (a drop-down), ps is not shown: it is the primer's start pressure plus the additional one
    bool primerShown = false;
    for (PropertyPanel *panel : view.findChildren<PropertyPanel *>())
    {
        QTableWidget *t = tableOf(*panel);
        if (const int row = rowOf(t, u"primer"_s); row > 0)
        {
            QVERIFY(t->item(row, 1)->text().startsWith(csr::primerName(u"LR"_s) + u" ("_s));  // with its start pressure
            QVERIFY(t->item(row, 1)->text().contains(u"bar"_s));
            QCOMPARE(rowOf(t, u"ps"_s), -1);
            QVERIFY(rowOf(t, u"psadd"_s) > 0);
            primerShown = true;
        }
    }
    QVERIFY(primerShown);
    edit(u"psadd"_s, u"900"_s);  // limited to +-400 bar
    lf = savedOwn();
    QVERIFY(lf);
    QCOMPARE(lf->caliber.number(u"psadd"_s), 400.0);
}

void GuiTest::panelShowsAndEditsInTheChosenUnit()
{
    using csr::units::System;
    csr::EntryList list;
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

    // the editor keeps the value right-aligned, as shown
    t->openPersistentEditor(t->item(rD, 1));
    auto *edit = qobject_cast<QLineEdit *>(t->cellWidget(rD, 1));
    QVERIFY(edit);
    QVERIFY(edit->alignment() & Qt::AlignRight);
    t->closePersistentEditor(t->item(rD, 1));
    UnitPreferences::instance().setSystem(System::Original);
}

void GuiTest::perFieldUnitOverridesTheSystem()
{
    using csr::units::Quantity;
    using csr::units::System;
    csr::EntryList list;
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
    using csr::units::System;
    LoadView view;
    QString error;
    QVERIFY2(view.load(QStringLiteral(CSR_FIXTURE_DIR "/8x57is.grtload"), &error), qPrintable(error));
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
    auto lf = csr::LoadFile::read(QStringLiteral(CSR_FIXTURE_DIR "/8x57is.grtload"));
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
    const auto saved = csr::LoadFile::read(out);
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
    QVERIFY(plain.load(QStringLiteral(CSR_FIXTURE_DIR "/8x57is.grtload"), &error));
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
    QVERIFY2(view.load(QStringLiteral(CSR_FIXTURE_DIR "/8x57is.grtload"), &error), qPrintable(error));
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

void GuiTest::lastLoadsAreReopened()
{
    session::setReopen(false);
    session::store({}, -1);
    // the option is in the settings: OK stores it, Cancel puts the old value back
    {
        SettingsDialog dlg;
        auto *box = dlg.findChild<QCheckBox *>(u"reopenLastLoads"_s);
        QVERIFY(box && !box->isChecked());
        box->setChecked(true);
        dlg.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
        QVERIFY(session::reopen());
        dlg.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Cancel)->click();
        QVERIFY(!session::reopen());
    }
    {
        SettingsDialog dlg;
        dlg.findChild<QCheckBox *>(u"reopenLastLoads"_s)->setChecked(true);
        dlg.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
        QVERIFY(session::reopen());
    }
    // two loads (copies of a sample, so that the files can be removed) and the one that was shown
    static QTemporaryDir dir;
    const QString a = dir.filePath(u"first.grtload"_s), b = dir.filePath(u"second.grtload"_s);
    for (const QString &f : {a, b}) QVERIFY(QFile::copy(QStringLiteral(CSR_FIXTURE_DIR "/8x57is.grtload"), f));
    {
        MainWindow w;
        QVERIFY(w.openLoad(a));
        QVERIFY(w.openLoad(b));
        w.showTab(1);  // the first load (tab 0 is the database)
        w.saveSession();
    }
    QCOMPARE(session::loads(), (QStringList{a, b}));
    QCOMPARE(session::current(), 0);
    QVERIFY(QFile::remove(b));  // a file that is gone is skipped
    MainWindow w2;
    w2.restoreSession();
    int loads = 0;
    QString shown;
    for (auto *tabs : w2.findChildren<QTabWidget *>())
        for (int i = 0; i < tabs->count(); ++i)
            if (auto *v = qobject_cast<LoadView *>(tabs->widget(i)))
            {
                ++loads;
                if (i == tabs->currentIndex()) shown = v->path();
            }
    QCOMPARE(loads, 1);
    QCOMPARE(shown, a);
    session::setReopen(false);
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

void GuiTest::startPageHasLoadButtons()
{
    MainWindow w;
    w.show();
    auto *create = w.findChild<QPushButton *>(u"newLoadButton"_s);
    QVERIFY(create);
    QVERIFY(w.findChild<QPushButton *>(u"openLoadButton"_s));
    QVERIFY(create->isVisible());
    // the button opens the New load dialog (the database is open: the own one is always there)
    QVERIFY(create->isEnabled());
    bool opened = false;
    QTimer::singleShot(0,
                       [&]
                       {
                           for (QWidget *top : QApplication::topLevelWidgets())
                               if (auto *d = qobject_cast<NewLoadDialog *>(top))
                               {
                                   opened = true;
                                   d->reject();
                               }
                       });
    create->click();
    QVERIFY(opened);
    if (const QString shots = qEnvironmentVariable("CSR_SHOT_DIR"); !shots.isEmpty()) w.grab().save(u"%1/startpage.png"_s.arg(shots));
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
            static_cast<int>(original == csr::units::System::Imperial ? csr::units::System::Metric : csr::units::System::Imperial));
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
    QVERIFY2(view.load(QStringLiteral(CSR_FIXTURE_DIR "/8x57is.grtload"), &error), qPrintable(error));
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

void GuiTest::noteWithoutPhysicalBurnValues()
{
    // the table of the physical burn values is a resource of the program only: the file of the source tree for this test
    csr::setPhysicalBurnFile(QStringLiteral(CSR_SOURCE_DIR "/data/physical/propellants.json"));
    const auto restore = qScopeGuard([] { csr::setPhysicalBurnFile(QStringLiteral(":/data/physical/propellants.json")); });
    // the orange banner (hidden: no text)
    const auto statusOf = [](LoadView &v)
    {
        const auto *banner = v.findChild<QWidget *>(u"burnValuesNotice"_s);
        return banner && !banner->isHidden() ? v.findChild<QLabel *>(u"burnValuesNoticeText"_s)->text() : QString();
    };
    // the sample's N150 has the original's values: those of the project's record are different, so it has no physical ones
    LoadView original;
    QVERIFY(original.load(QStringLiteral(CSR_FIXTURE_DIR "/8x57is.grtload")));
    QVERIFY(statusOf(original).contains(u"No physical burn values"_s));
    QVERIFY(statusOf(original).contains(u"Use the physical values"_s));  // the table has N150: the user can take them

    // the user is asked; "No" changes nothing, "Yes" attaches the values (the load is modified, the note is gone)
    const auto answerBox = [](QMessageBox::StandardButton b)
    {
        QTimer::singleShot(0,
                           [b]
                           {
                               auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
                               QVERIFY(box);
                               box->button(b)->click();
                           });
    };
    const auto statusLabel = [](LoadView &v) { return v.findChild<QLabel *>(u"burnValuesNoticeText"_s); };
    QVERIFY(!original.isModified());
    answerBox(QMessageBox::No);
    emit statusLabel(original)->linkActivated(u"physical"_s);
    QVERIFY(!original.isModified());
    QVERIFY(statusOf(original).contains(u"No physical burn values"_s));
    answerBox(QMessageBox::Yes);
    emit statusLabel(original)->linkActivated(u"physical"_s);
    QVERIFY(original.isModified());
    QVERIFY(!statusOf(original).contains(u"No physical burn values"_s));
    QTemporaryDir saveDir;
    QVERIFY(original.save(saveDir.filePath(u"taken.csrload"_s)));  // saved with the load
    QVERIFY(csr::readPhysicalBurn(csr::LoadFile::read(saveDir.filePath(u"taken.csrload"_s))->propellant));

    // with the values of the project's record (N150: Ba 0.7878, a0 0, z1 0.3, z2 0.6) the powder's physical values are attached
    auto lf = *csr::LoadFile::read(QStringLiteral(CSR_FIXTURE_DIR "/8x57is.grtload"));
    for (const auto &[name, value] : {std::pair{u"Ba"_s, 0.7878}, {u"a0"_s, 0.0}, {u"z1"_s, 0.3}, {u"z2"_s, 0.6}})
        lf.propellant.set(name, value);
    QTemporaryDir dir;
    const QString path = dir.filePath(u"record.csrload"_s);
    QVERIFY(lf.write(path));
    LoadView known;
    QVERIFY(known.load(path));
    QVERIFY(!statusOf(known).contains(u"No physical burn values"_s));

    // edited burn values: the physical ones no longer belong to them
    lf.propellant.set(u"Ba", 0.8);
    QVERIFY(lf.write(path));
    LoadView edited;
    QVERIFY(edited.load(path));
    QVERIFY(statusOf(edited).contains(u"No physical burn values"_s));
}

void GuiTest::searchToleratesSpellings()
{
    const QStringList irs{u"8 x 57 IRS"_s, u"Sellier & Bellot"_s};
    for (const QString &q : {u"8x57"_s, u"8 x 57"_s, u"8X57"_s, u"8×57"_s, u"8*57"_s, u" 8x57 "_s, u"857"_s, u"8x57 irs"_s,
                             u"bellot 8x57"_s, u"8x"_s, u"8 x"_s, u"x57"_s, u"8x5"_s})
        QVERIFY2(searchfilter::matches(q, irs), qPrintable(q));
    QVERIFY(searchfilter::matches(u"6.5x284"_s, {u"6.5 - 284 Norma"_s}));
    QVERIFY(searchfilter::matches(u"30 06"_s, {u"30-06 Springfield"_s}));
    QVERIFY(searchfilter::matches(u"9mm"_s, {u"9 mm Luger"_s}));
    QVERIFY(searchfilter::matches(u"nobel"_s, {u"Nöbel Sport"_s}));
    QVERIFY(searchfilter::matches({}, irs));
    QVERIFY(!searchfilter::matches(u"8x58"_s, irs));
    QVERIFY(!searchfilter::matches(u"7x57"_s, irs));
    QVERIFY(!searchfilter::matches(u"lapua 8x57"_s, irs));
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

    QCOMPARE(withLoadExtension(u"/tmp/mine"_s), u"/tmp/mine.csrload"_s);
    QCOMPARE(withLoadExtension(u"/tmp/mine.csrload"_s), u"/tmp/mine.csrload"_s);
    QCOMPARE(withLoadExtension(u"/tmp/mine.xml"_s), u"/tmp/mine.xml"_s);
    QCOMPARE(withLoadExtension(u"/tmp.d/mine"_s), u"/tmp.d/mine.csrload"_s);      // a dot in a folder name is no extension
    QCOMPARE(withLoadExtension(u"/tmp/6.5x55SE"_s), u"/tmp/6.5x55SE.csrload"_s);  // the "extension" has digits: part of the name
    QCOMPARE(withLoadExtension(u"/tmp/.223 Rem 55gr"_s), u"/tmp/.223 Rem 55gr.csrload"_s);
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
    QVERIFY(w.openLoad(QStringLiteral(CSR_FIXTURE_DIR "/8x57is.grtload")));
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
    QVERIFY(items.contains(u"About ChamberSim Reloaded…"_s));
    QVERIFY(!items.contains(u"About Qt…"_s));

    // the text: name, version, purpose, calibration, safety, libraries; the database only when one is open
    const QString plain = about::html();
    QVERIFY(plain.contains(u"ChamberSim Reloaded"_s));
    QVERIFY(plain.contains(QStringLiteral(CSR_VERSION)));
    QVERIFY(plain.contains(u"Qt"_s) && plain.contains(QString::fromLatin1(qVersion())));
    QVERIFY(plain.contains(u"Safety"_s) && plain.contains(u"at your own risk"_s));
    QVERIFY(plain.contains(u"not affiliated"_s));
    QVERIFY(plain.contains(u"Disclaimer"_s) && plain.contains(u"without warranty"_s) &&
            plain.contains(u"no responsibility or liability"_s));
    QVERIFY(plain.contains(u"Physical Model"_s) && plain.contains(u"GRT Original"_s));  // the accuracy of both models
    QVERIFY(plain.contains(u"Measurements"_s) && plain.contains(u"Labradar"_s));
    QVERIFY(!plain.contains(u"Project records"_s));
    // the three parts of the database, each with its counts; the original's file name only, escaped. The English plural
    // forms come from the en_US translation, which the program always loads.
    QTranslator english;
    QVERIFY(english.load(u"ChamberSimReloaded_en_US"_s, QStringLiteral(CSR_QM_DIR)));
    QCoreApplication::installTranslator(&english);
    const auto removeEnglish = qScopeGuard([&] { QCoreApplication::removeTranslator(&english); });
    const about::DatabaseInfo info{
        .project = {467, 1761, 108}, .own = {1, 0, 2}, .original = {578, 5659, 683}, .originalPath = u"/some/dir/data <1>.db"_s};
    const QString withDb = about::html(info);
    QVERIFY2(withDb.contains(u"Project records: 467 calibers, 1,761 projectiles, 108 propellants"_s), qPrintable(withDb));
    QVERIFY(withDb.contains(u"Own records: 1 caliber, 0 projectiles, 2 propellants"_s));
    QVERIFY(withDb.contains(u"(data &lt;1&gt;.db): 578 calibers, 5,659 projectiles, 683 propellants"_s));
    QVERIFY(!withDb.contains(u"/some/dir"_s));
    about::DatabaseInfo noOriginal = info;
    noOriginal.originalPath.clear();
    QVERIFY(about::html(noOriginal).contains(u"Original program's database: none chosen"_s));
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
                               box->grab().save(QDir::temp().filePath(u"csr_about.png"_s));
                               box->accept();
                           }
                       });
    about::show(
        nullptr,
        about::DatabaseInfo{.project = {467, 1761, 108}, .original = {578, 5659, 683}, .originalPath = u"GordonsReloadingTool.plain.db"_s});
    QVERIFY2(shownWidth >= 700, qPrintable(QString::number(shownWidth)));
    QVERIFY(noHorizontalScroll);
}

void GuiTest::reportTabRendersTheLoad()
{
    ReportView::storeTemplate({});
    UnitPreferences::instance().setSystem(csr::units::System::Metric);
    LoadView view;
    QString error;
    QVERIFY2(view.load(QStringLiteral(CSR_FIXTURE_DIR "/8x57is.grtload"), &error), qPrintable(error));
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
    const csr::report::Data d = view.reportData();
    for (const char *name :
         {"result.diagramm.png", "result.diagramCurves.png", "result.diagramPropellant.png", "caliber.drawing.png", "caliber.chamber.png"})
    {
        const auto *v = d.find(QLatin1String(name));
        QVERIFY2(v && v->kind == csr::report::Value::Kind::Image, name);
        QVERIFY2(QImage::fromData(v->png).width() > 300, name);
    }
    QVERIFY(html.count(u"<img"_s) >= 2);

    // the unit system applies to the report too
    UnitPreferences::instance().setSystem(csr::units::System::Imperial);
    QVERIFY(report->html().contains(u" psi"_s) && report->html().contains(u" ft/s"_s));
    QVERIFY(!report->html().contains(u" bar"_s));
    UnitPreferences::instance().setSystem(csr::units::System::Original);

    // editing the load changes the report
    const QString before = report->html();
    const int n340 = idOf(m_db, csr::Table::Propellant, u"mname = 'Vihtavuori' AND pname = 'N340'"_s);
    view.setDatabase(&m_db);
    QVERIFY(view.applyRecord(LoadView::Component::Propellant, n340));
    QVERIFY(report->html() != before);
    QVERIFY(report->html().contains(u"N340"_s));
}

void GuiTest::startingOnTheReportTabRendersAfterTheShow()
{
    // a window that starts on the stored Report tab rendered the report inside its own show and crashed
    QSettings().setValue(u"layout/resultTab"_s, 2);
    MainWindow w;
    QVERIFY(w.openLoad(QStringLiteral(CSR_FIXTURE_DIR "/8x57is.grtload")));
    w.showTab(1);
    w.resize(1400, 900);
    w.show();
    QTest::qWait(300);
    QSettings().setValue(u"layout/resultTab"_s, 0);
}

void GuiTest::reportTemplateIsCustomisable()
{
    ReportView::storeTemplate({});
    QVERIFY(!ReportView::hasCustomTemplate());
    LoadView view;
    QString error;
    QVERIFY2(view.load(QStringLiteral(CSR_FIXTURE_DIR "/8x57is.grtload"), &error), qPrintable(error));
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
    ReportView::storeTemplate(csr::report::defaultTemplate());
    QVERIFY(!ReportView::hasCustomTemplate());
    report->refresh();
    QVERIFY(report->html().contains(u"Input data"_s));
}

void GuiTest::actionButtonsAreEasyToSee()
{
    LoadView view;
    QString error;
    QVERIFY2(view.load(QStringLiteral(CSR_FIXTURE_DIR "/8x57is.grtload"), &error), qPrintable(error));
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
    // the "Next" / "Create load" button of the new-load dialog
    NewLoadDialog dlg(m_db);
    int primary = 0;
    for (QPushButton *b : dlg.findChildren<QPushButton *>())
        if (b->text().contains(u"Next"_s))
        {
            ++primary;
            QVERIFY(b->styleSheet().contains(u"palette(highlight)"_s));
        }
    QCOMPARE(primary, 1);
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
    QVERIFY2(view.load(QStringLiteral(CSR_FIXTURE_DIR "/8x57is.grtload"), &error), qPrintable(error));
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
    csr::EntryList list;
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
        csr::EntryList l;
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
    QVERIFY2(view.load(QStringLiteral(CSR_FIXTURE_DIR "/8x57is.grtload"), &error), qPrintable(error));
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
    QVERIFY2(view.load(QStringLiteral(CSR_FIXTURE_DIR "/8x57is.grtload"), &error), qPrintable(error));
    QTemporaryDir dir;
    const QString pdf = dir.filePath(u"report.pdf"_s);
    QVERIFY(view.report()->exportPdf(pdf));
    QFile f(pdf);
    QVERIFY(f.open(QIODevice::ReadOnly));
    const QByteArray data = f.readAll();
    QVERIFY(data.startsWith("%PDF"));
    QVERIFY(data.size() > 20000);  // text, tables and the embedded diagrams
    QVERIFY(data.contains("/Type /Page") || data.contains("/Type/Page"));
    QFile::copy(pdf, QDir::temp().filePath(u"csr_report_sample.pdf"_s));
}

void GuiTest::inputsAreGroupedInThreeTabs()
{
    LoadView view;
    QString error;
    QVERIFY2(view.load(QStringLiteral(CSR_FIXTURE_DIR "/8x57is.grtload"), &error), qPrintable(error));
    const auto tabs = view.findChildren<QTabWidget *>();
    QTabWidget *inputs = nullptr;
    for (QTabWidget *t : tabs)
        if (t->count() == 3) inputs = t;
    QVERIFY(inputs);
    QCOMPARE(inputs->tabText(1), u"Projectile"_s);
    QCOMPARE(inputs->tabText(2), u"Propellant"_s);
    // every tab has a "Select <component>" button; it is off until a database is available
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
    QVERIFY2(view.load(QStringLiteral(CSR_FIXTURE_DIR "/8x57is.grtload"), &error), qPrintable(error));
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
    const int n340 = idOf(m_db, csr::Table::Propellant, u"mname = 'Vihtavuori' AND pname = 'N340'"_s);
    QVERIFY(n340 > 0);
    QVERIFY(view.applyRecord(LoadView::Component::Propellant, n340));
    QVERIFY(view.isModified());
    QCOMPARE(modified.count(), 1);
    QVERIFY(allText().contains(u"N340"_s));
    QVERIFY(!allText().contains(u"N150"_s));

    // another bullet
    const int other = idOf(m_db, csr::Table::Projectile, u"caliber = '.323' AND gmass > 150 AND mname <> 'Sellier-Bellot'"_s);
    QVERIFY(other > 0);
    QVERIFY(view.applyRecord(LoadView::Component::Projectile, other));
    QVERIFY(!allText().contains(u"Sellier-Bellot"_s));

    // another caliber, with the dependent values reset: a pistol barrel is much shorter
    const int pistol = idOf(m_db, csr::Table::Caliber, u"cipname = '.45 Auto'"_s);
    QVERIFY(pistol > 0);
    QVERIFY(view.applyRecord(LoadView::Component::Caliber, pistol, true));
    QVERIFY(allText().contains(u".45 Auto"_s));
    // the saved file reflects all three replacements and loads again
    QTemporaryDir dir;
    const QString out = dir.filePath(u"changed.grtload"_s);
    QVERIFY(view.save(out, &error));
    const auto lf = csr::LoadFile::read(out);
    QVERIFY(lf);
    QCOMPARE(lf->propellant.text(u"pname"), u"N340"_s);
    QCOMPARE(lf->caliberFile.text(u"cipname"), u".45 Auto"_s);
    QCOMPARE(lf->gun.number(u"xe"), 150.0);
    QVERIFY(!view.applyRecord(LoadView::Component::Propellant, -5));  // unknown id
}

void GuiTest::chamberGeometryFollowsTheDimensions()
{
    const auto lf = csr::LoadFile::read(QStringLiteral(CSR_FIXTURE_DIR "/8x57is.grtload"));
    QVERIFY(lf);
    QMap<QString, double> d;
    for (const csr::Entry &e : lf->caliberFile.entries()) d.insert(e.name, e.value.toDouble());
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
    QVERIFY2(view.load(QStringLiteral(CSR_FIXTURE_DIR "/8x57is.grtload"), &error), qPrintable(error));
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
    const auto lf = csr::LoadFile::read(QStringLiteral(CSR_FIXTURE_DIR "/8x57is.grtload"));
    QVERIFY(lf);
    QMap<QString, double> dims;
    for (const csr::Entry &e : lf->caliberFile.entries()) dims.insert(e.name, e.value.toDouble());
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
    auto lf = csr::LoadFile::read(QStringLiteral(CSR_FIXTURE_DIR "/8x57is.grtload"));
    QVERIFY(lf);
    view.setLoad(*lf, csr::SimInput::fromLoad(*lf));
    QVERIFY(view.geometry().valid);
    const QImage a = view.grab().toImage();
    QVERIFY(!a.isNull());

    // a trimmed case: the drawing shows the load's case length and overall length, not the caliber record's
    auto trimmed = *lf;
    trimmed.caliber.set(u"caselen"_s, u"56.8"_s);
    trimmed.caliber.set(u"oal"_s, u"79"_s);
    trimmed.projectile.set(u"gdepth"_s, QString::number(56.8 + trimmed.projectile.number(u"glen") - 79.0));
    const auto round = CartridgeView::makeRound(trimmed, csr::SimInput::fromLoad(trimmed));
    QVERIFY(std::abs(round.caseLength() - 56.8) < 1e-9);
    QVERIFY(std::abs(round.bulletTip() - 79.0) < 1e-6);

    // a straight pistol case (L1/L2/H1/P2 are zero in the database): no crash, still valid
    if (!QFile::exists(QStringLiteral(CSR_SOURCE_DIR "/tools/GordonsReloadingTool.plain.db"))) return;
    csr::Database db;
    QVERIFY(db.open(QStringLiteral(CSR_SOURCE_DIR "/tools/GordonsReloadingTool.plain.db")));
    const int cal = idOf(db, csr::Table::Caliber, u"cipname = '.45 Auto'"_s);
    const int prj = idOf(db, csr::Table::Projectile, u"caliber = '.451'"_s);
    const int pro = idOf(db, csr::Table::Propellant, u"mname = 'Vihtavuori' AND pname = 'N340'"_s);
    QVERIFY(cal > 0 && prj > 0 && pro > 0);
    const csr::LoadFile pistol = csr::buildLoad(*db.record(csr::Table::Caliber, cal), *db.record(csr::Table::Projectile, prj),
                                                *db.record(csr::Table::Propellant, pro), {}, db.columns(csr::Table::Caliber));
    view.setLoad(pistol, csr::SimInput::fromLoad(pistol));
    QVERIFY(view.geometry().valid);
    QVERIFY(view.geometry().outer.size() >= 4);
    QVERIFY(!view.grab().toImage().isNull());
    view.clear();
    QVERIFY(!view.geometry().valid);
}

void GuiTest::measurementTabShowsTheFilesResults()
{
    UnitPreferences::instance().setSystem(csr::units::System::Metric);
    LoadView view;
    QVERIFY(view.load(QStringLiteral(CSR_FIXTURE_DIR "/measurements.grtload")));
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
    UnitPreferences::instance().setSystem(csr::units::System::Original);
}

// Dates in the record dialog: a date picker in the form yyyy-MM-dd ("– not set –" = empty), the program's dates as
// text; propellant and projectile dialogs are wide enough for two columns.
void GuiTest::recordDialogCalculatesTheBoreCrossSection()
{
    UnitPreferences::instance().setSystem(csr::units::System::Metric);
    RecordDialog dlg(m_db, csr::Table::Caliber, RecordDialog::Mode::New);
    auto *button = dlg.findChild<QPushButton *>(u"calculateBoreArea"_s);
    QVERIFY(button);
    // nothing entered yet: no value, a hint
    dlg.setText(u"c_Q"_s, QString());
    // the 10.3 Capra of the C.I.P.: Q = 85.16 mm2
    dlg.setText(u"c_F"_s, u"10.25"_s);
    dlg.setText(u"c_Z"_s, u"10.49"_s);
    dlg.setText(u"c_N"_s, u"6"_s);
    dlg.setText(u"c_b_"_s, u"3.6"_s);
    button->click();
    QVERIFY(std::abs(QLocale().toDouble(dlg.text(u"c_Q"_s)) - 85.16) < 0.2);
    QVERIFY(dlg.values().value(u"c_Q"_s).toDouble() > 85.0);
}

void GuiTest::recordViewerShowsAllData()
{
    UnitPreferences::instance().setSystem(csr::units::System::Metric);
    // the numbers that the original database stores as text are aligned like the other numbers
    {
        RecordDialog cal(m_db, csr::Table::Caliber, RecordDialog::Mode::New);
        for (const QString column : {u"delta"_s, u"beta"_s, u"alpha"_s, u"L3"_s})
        {
            auto *edit = cal.findChild<QLineEdit *>(column);
            QVERIFY2(edit, qPrintable(column));
            QVERIFY2(edit->alignment() & Qt::AlignRight, qPrintable(column));
        }
    }
    for (const csr::Table t : csr::kRecordTables)
    {
        const int id = idOf(m_db, t, u"1=1"_s);
        QVERIFY(id >= 0);
        const auto record = m_db.record(t, id);
        QVERIFY(record);
        RecordViewDialog view(m_db, t, id, *record);
        QVERIFY(view.fieldCount() > 10);
        QVERIFY(!view.windowTitle().isEmpty());
        if (const QString shots = qEnvironmentVariable("CSR_SHOT_DIR"); !shots.isEmpty())
            view.grab().save(u"%1/recordview%2.png"_s.arg(shots).arg(static_cast<int>(t)));
        // the original's records cannot be changed: the viewer offers a copy
        const bool own = csr::Database::isOwnId(id);
        QVERIFY(view.findChild<QPushButton *>(own ? u"editRecordButton"_s : u"copyRecordButton"_s));
        if (t == csr::Table::Propellant)  // the fit quality has a bar next to the number
        {
            QVERIFY(!view.valueText(u"Qlty"_s).isEmpty());
            QVERIFY(view.findChild<QProgressBar *>());
        }
        if (t == csr::Table::Caliber)
        {
            QCOMPARE(view.valueText(u"cipname"_s), record->value(u"cipname"_s).toString());
            QVERIFY(view.valueText(u"L3"_s).contains(QLocale().decimalPoint()));  // the length in mm
        }
    }
}

void GuiTest::selectPropellantShowsTheQuality()
{
    SelectRecordDialog select(m_db, csr::Table::Propellant, u"Select propellant"_s);
    auto *bar = select.findChild<QProgressBar *>();
    QVERIFY(bar);
    auto *browser = select.findChild<TableBrowser *>();
    const int id = idOf(m_db, csr::Table::Propellant, u"Qlty > 0.5"_s);
    QVERIFY(id >= 0);
    QVERIFY(browser->selectId(id));
    const double quality = m_db.record(csr::Table::Propellant, id)->value(u"Qlty"_s).toDouble();
    QCOMPARE(bar->value(), qRound(100 * quality));
    QVERIFY(bar->isVisibleTo(&select));
    if (const QString shots = qEnvironmentVariable("CSR_SHOT_DIR"); !shots.isEmpty())
        select.grab().save(u"%1/selectpropellant.png"_s.arg(shots));
}

void GuiTest::newLoadAndDatabaseShowTheQuality()
{
    NewLoadDialog dlg(m_db);
    auto *row = dlg.findChild<QualityRow *>();
    QVERIFY(row);
    auto *bar = row->findChild<QProgressBar *>();
    QVERIFY(bar);
    const int cal = idOf(m_db, csr::Table::Caliber, u"cipname = '8 x 57 IS'"_s);
    const int prj = idOf(m_db, csr::Table::Projectile, u"mname = 'Sellier-Bellot' AND pname = 'FMJ-BT 2910'"_s);
    const int pro = idOf(m_db, csr::Table::Propellant, u"Qlty > 0.5"_s);
    QVERIFY(dlg.select(cal, prj, pro));
    QCOMPARE(bar->value(), qRound(100 * m_db.record(csr::Table::Propellant, pro)->value(u"Qlty"_s).toDouble()));

    // in the database list the quality column follows the powder's name
    TableBrowser browser(csr::Table::Propellant);
    browser.setDatabase(&m_db);
    auto *view = browser.findChild<QTableView *>();
    auto *header = view->horizontalHeader();
    const auto *model = view->model();
    const auto visualOf = [&](const QString &column)
    {
        for (int c = 0; c < model->columnCount(); ++c)
            if (model->headerData(c, Qt::Horizontal).toString() == column) return header->visualIndex(c);
        return -1;
    };
    QVERIFY(visualOf(u"pname"_s) >= 0);
    QCOMPARE(visualOf(u"Qlty"_s), visualOf(u"pname"_s) + 1);
}

void GuiTest::dialogsFitTheScreen()
{
    const QRect area = QGuiApplication::primaryScreen()->availableGeometry();
    const auto check = [&](QDialog &d, const char *name) { QVERIFY2(d.width() <= area.width() && d.height() <= area.height(), name); };
    SelectRecordDialog select(m_db, csr::Table::Propellant, u"x"_s);
    check(select, "select");
    BurnRateDialog burn(m_db);
    check(burn, "burn rate");
    for (const csr::Table t : csr::kRecordTables)
    {
        const int id = idOf(m_db, t, u"1=1"_s);
        RecordViewDialog view(m_db, t, id, *m_db.record(t, id));
        check(view, "record view");
        RecordDialog edit(m_db, t, RecordDialog::Mode::Copy, *m_db.record(t, id));
        check(edit, "record edit");
    }
    // a small screen: the sizes are capped, whatever the dialog asks for
    QDialog big;
    resizeWithinScreen(&big, {area.width() * 3, area.height() * 3});
    check(big, "capped");
}

void GuiTest::recordDialogDatesAndWidth()
{
    RecordDialog cal(m_db, csr::Table::Caliber, RecordDialog::Mode::Copy,
                     *m_db.record(csr::Table::Caliber, idOf(m_db, csr::Table::Caliber, u"cipdate != ''"_s)));
    auto *picker = cal.findChild<QDateEdit *>(u"cipdate"_s);
    QVERIFY(picker);
    QCOMPARE(picker->displayFormat(), u"yyyy-MM-dd"_s);
    QVERIFY(picker->calendarPopup());
    if (const QString shots = qEnvironmentVariable("CSR_SHOT_DIR"); !shots.isEmpty()) cal.grab().save(shots + u"/caliberdates.png"_s);
    cal.setText(u"cipdate"_s, u"14.6.1984"_s);
    QCOMPARE(cal.text(u"cipdate"_s), u"1984-06-14"_s);
    QCOMPARE(cal.values().value(u"cipdate"_s).toString(), u"1984-06-14"_s);
    cal.setText(u"cipdate"_s, QString());
    QCOMPARE(cal.values().value(u"cipdate"_s).toString(), QString());  // "– not set –"
    QVERIFY(!cal.findChild<QDateEdit *>(u"mdate"_s));                  // the date of the last change is set by the program
    QCOMPARE(cal.values().value(u"cdate"_s).toString(), QDate::currentDate().toString(Qt::ISODate));  // a copy: today

    // the creation date can be entered for every kind of record (a picker, today for a new record)
    for (const csr::Table t : csr::kRecordTables)
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

    for (const csr::Table t : {csr::Table::Projectile, csr::Table::Propellant})
    {
        RecordDialog dlg(m_db, t, RecordDialog::Mode::New);
        QCOMPARE_GE(dlg.width(), std::min(800, static_cast<int>(QGuiApplication::primaryScreen()->availableGeometry().width() * 0.95)));
    }
}

// The notes under the results: the bullet jump is part of the model; a shotshell gets a note how to enter it.
void GuiTest::ignoredInputsAreNamed()
{
    QFile f(QStringLiteral(CSR_FIXTURE_DIR "/8x57is.grtload"));
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
            if (l->text().startsWith(u"Model: "_s)) return l->text();  // the status line: the model used
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
    QVERIFY(plain.load(QStringLiteral(CSR_FIXTURE_DIR "/8x57is.grtload")));
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
    QVERIFY(lv.load(QStringLiteral(CSR_FIXTURE_DIR "/8x57is.grtload")));
    csr::Database db;
    QVERIFY(db.open(QStringLiteral(CSR_SOURCE_DIR "/tools/GordonsReloadingTool.plain.db")));
    lv.setDatabase(&db);
    const auto *photo = lv.findChild<PhotoView *>();
    QVERIFY(photo);
    const int withImage = idOf(db, csr::Table::Propellant, u"imageUuid != '' AND imageDetailUuid != ''"_s);
    QVERIFY(withImage > 0);
    QVERIFY(lv.applyRecord(LoadView::Component::Propellant, withImage));
    QVERIFY(photo->hasPhotos());
    QCOMPARE(photo->shown(), 0);
    const int without = idOf(db, csr::Table::Propellant, u"imageUuid = '' AND imageDetailUuid = ''"_s);
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
    csr::Database own;  // only the program's own database: no original GRT database
    QVERIFY(own.open({}, dir.filePath(u"own.db"_s)));

    RecordDialog dlg(own, csr::Table::Propellant, RecordDialog::Mode::New);
    dlg.setText(u"pname"_s, u"Home powder"_s);
    QVERIFY(dlg.pendingImages().empty());
    QVERIFY(dlg.values().value(u"imageUuid"_s).toString().isEmpty());
    QImage big(2400, 1600, QImage::Format_RGB32);  // larger than stored: scaled down, the scale follows
    big.fill(Qt::darkGray);
    dlg.setPhoto(0, big, 200.0, u"grains.png"_s);
    QImage close(300, 200, QImage::Format_RGB32);
    close.fill(Qt::lightGray);
    dlg.setPhoto(1, close, 0.0, u"grain.png"_s);
    if (const QString shots = qEnvironmentVariable("CSR_SHOT_DIR"); !shots.isEmpty())
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
    const auto id = own.insert(csr::Table::Propellant, dlg.values());
    QVERIFY(id);

    // a load that uses the record shows the photo (Powder tab) from the own database alone
    LoadView view;
    QVERIFY(view.load(QStringLiteral(CSR_FIXTURE_DIR "/8x57is.grtload")));
    view.setDatabase(&own);
    QVERIFY(view.applyRecord(LoadView::Component::Propellant, *id));
    const auto *photo = view.findChild<PhotoView *>();
    QVERIFY(photo && photo->hasPhotos());

    // an edit that removes the detail photo
    RecordDialog edit(own, csr::Table::Propellant, RecordDialog::Mode::Edit, *own.record(csr::Table::Propellant, *id));
    edit.removePhoto(1);
    QVERIFY(edit.values().value(u"imageDetailUuid"_s).toString().isEmpty());
    QCOMPARE(edit.values().value(u"imageUuid"_s).toString(), pending[0].uuid);
    QVERIFY(edit.pendingImages().empty());
}

void GuiTest::chronographImportAddsCharges()
{
    LoadView view;
    QVERIFY(view.load(QStringLiteral(CSR_FIXTURE_DIR "/8x57is.grtload")));
    MeasurementView *m = view.measurements();
    const auto file = csr::readChronograph(
        QByteArray("Series, 1, Shots:,2\r\n 1, 1, 800.0, m/s\r\n 1, 2, 810.0, m/s\r\nSeries, 2, Shots:,1\r\n 2, 1, 820.0, m/s\r\n"));
    QVERIFY(file);
    ChronographImportDialog dialog(*file, 3.0);
    dialog.setChecked(1, false);
    const auto picked = dialog.selected();
    QCOMPARE(picked.size(), std::size_t{1});
    QCOMPARE(picked[0].charge_g, 3.0);
    QCOMPARE(m->importCharges(picked), 1);
    QCOMPARE(m->measurement().charges.size(), std::size_t{1});
    QCOMPARE(csr::velocityStatistics(m->measurement().charges[0]).count, 2);
    QVERIFY(view.isModified());
}

void GuiTest::measurementSeriesAreEditedAndSaved()
{
    LoadView view;
    QVERIFY(view.load(QStringLiteral(CSR_FIXTURE_DIR "/8x57is.grtload")));
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
    const auto lf = csr::LoadFile::read(path);
    QVERIFY(lf);
    QCOMPARE(csr::readMeasurements(lf->appendixXml).size(), std::size_t{2});

    LoadView again;
    QVERIFY(again.load(path));
    QCOMPARE(again.measurements()->allMeasurements().size(), std::size_t{2});
    QCOMPARE(again.measurements()->allMeasurements()[1].charges[0].shots.size(), std::size_t{1});
    again.measurements()->selectSeries(1);
    again.measurements()->removeSeries();
    QCOMPARE(again.measurements()->allMeasurements().size(), std::size_t{1});
    QVERIFY(again.save(path));
    QCOMPARE(csr::readMeasurements(csr::LoadFile::read(path)->appendixXml).size(), std::size_t{1});
}

void GuiTest::measurementsAreEditedAndSaved()
{
    UnitPreferences::instance().setSystem(csr::units::System::Metric);
    LoadView view;
    QVERIFY(view.load(QStringLiteral(CSR_FIXTURE_DIR "/8x57is.grtload")));
    MeasurementView *m = view.measurements();
    QVERIFY(m->measurement().charges.empty());
    QVERIFY(!view.isModified());

    m->addCharge();  // the load's charge
    QVERIFY(view.isModified());
    QCOMPARE(m->measurement().charges.size(), std::size_t{1});
    QCOMPARE(m->measurement().charges[0].charge_g(), view.focusCharge_g());
    QCOMPARE(m->pasteShots(u"Velocity\tPressure\n812.5\t3500\n815.5\n\n818.0;3600\n"_s), 3);
    const csr::ShotStatistics v = csr::velocityStatistics(m->measurement().charges[0]);
    QCOMPARE(v.count, 3);
    QCOMPARE(v.max, 818.0);
    QCOMPARE(csr::pressureStatistics(m->measurement().charges[0]).count, 2);

    QTemporaryDir dir;
    const QString path = dir.filePath(u"m.grtload"_s);
    QVERIFY(view.save(path));
    const auto lf = csr::LoadFile::read(path);
    QVERIFY(lf);
    const auto saved = csr::readMeasurement(lf->appendixXml);
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
    UnitPreferences::instance().setSystem(csr::units::System::Original);
}

void GuiTest::originalDatabaseIsChosenInTheSettings()
{
    const QString original = QStringLiteral(CSR_SOURCE_DIR "/tools/GordonsReloadingTool.plain.db");
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
        if (const QString shots = qEnvironmentVariable("CSR_SHOT_DIR"); !shots.isEmpty())
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
    UnitPreferences::instance().setSystem(csr::units::System::Metric);
    const auto powder = m_db.record(csr::Table::Propellant, 562);
    QVERIFY(powder);
    RecordDialog copy(m_db, csr::Table::Propellant, RecordDialog::Mode::Copy, *powder);
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
    const auto bullet = m_db.record(csr::Table::Projectile, 4542);
    QVERIFY(bullet);
    RecordDialog edit(m_db, csr::Table::Projectile, RecordDialog::Mode::Edit, *bullet);
    QCOMPARE(edit.text(u"gmass"_s).left(6), u"12.700"_s);  // 196 grain, all digits kept
    QCOMPARE(edit.text(u"gtailtype"_s), u"boat tail"_s);
    edit.setText(u"gmass"_s, u"13"_s);
    QVERIFY(std::abs(edit.values().value(u"gmass"_s).toDouble() - 13.0 / 0.06479891) < 1e-9);
    QCOMPARE(edit.values().value(u"gtailtype"_s).toInt(), 2);
    // the country of origin is chosen from a list: no free text
    RecordDialog caliber(m_db, csr::Table::Caliber, RecordDialog::Mode::Copy, *m_db.record(csr::Table::Caliber, 1));
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
    QVariantMap combined = *m_db.record(csr::Table::Caliber, 1);
    combined.insert(u"ciporigin"_s, u"DE/AT"_s);
    RecordDialog keeps(m_db, csr::Table::Caliber, RecordDialog::Mode::Copy, combined);
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
    RecordDialog normalized(m_db, csr::Table::Caliber, RecordDialog::Mode::Copy, combined);
    QCOMPARE(normalized.text(u"standard"_s), u"WildCat"_s);
    QCOMPARE(normalized.text(u"ciptype"_s), u"4 pistol"_s);

    if (const QString shots = qEnvironmentVariable("CSR_SHOT_DIR"); !shots.isEmpty())
    {
        copy.grab().save(shots + u"/record.png"_s);
        RecordDialog cal(m_db, csr::Table::Caliber, RecordDialog::Mode::Copy, *m_db.record(csr::Table::Caliber, 1));
        cal.grab().save(shots + u"/record_caliber.png"_s);
    }
    UnitPreferences::instance().setSystem(csr::units::System::Original);
}

void GuiTest::recordsAreAddedCopiedEditedAndDeleted()
{
    const QString before = QSettings().value(u"database/path"_s).toString();
    {
        MainWindow w;
        QVERIFY(w.openDatabase(QStringLiteral(CSR_SOURCE_DIR "/tools/GordonsReloadingTool.plain.db")));
        const auto tabs = w.findChildren<QTabWidget *>();
        QTabWidget *dbTabs = nullptr;
        for (QTabWidget *t : tabs)
            if (t->count() == 3 && t->tabText(2) == u"Propellants"_s) dbTabs = t;
        QVERIFY(dbTabs);
        dbTabs->setCurrentIndex(2);
        QCOMPARE(w.currentTable(), csr::Table::Propellant);
        auto *browser = qobject_cast<TableBrowser *>(dbTabs->currentWidget());
        QVERIFY(browser->selectId(562));
        const auto button = [&](const QString &text)
        {
            for (QPushButton *b : w.findChildren<QPushButton *>())
                if (b->text() == text) return b;
            return static_cast<QPushButton *>(nullptr);
        };
        QVERIFY(button(u"Copy"_s)->isEnabled());
        QVERIFY(!button(u"Edit"_s)->isEnabled());  // a record of the original
        QVERIFY(!button(u"Delete"_s)->isEnabled());

        // a copy goes into the own database and appears in the same list
        RecordDialog dlg(m_db, csr::Table::Propellant, RecordDialog::Mode::Copy, *m_db.record(csr::Table::Propellant, 562));
        dlg.setText(u"Ba"_s, u"0.6"_s);
        const int count = browser->findChild<QTableView *>()->model()->rowCount();
        const int id = w.storeRecord(csr::Table::Propellant, RecordDialog::Mode::Copy, 562, dlg.values());
        QCOMPARE(id, csr::Database::kOwnIdBase);
        QCOMPARE(browser->currentId(), id);
        QCOMPARE(browser->findChild<QTableView *>()->model()->rowCount(), count + 1);
        QVERIFY(button(u"Edit"_s)->isEnabled());
        QVERIFY(button(u"Delete"_s)->isEnabled());

        // the first column says where a record comes from; the filter finds them by it
        auto *view = browser->findChild<QTableView *>();
        QCOMPARE(view->model()->headerData(0, Qt::Horizontal).toString(), u"Source"_s);
        QCOMPARE(view->currentIndex().siblingAtColumn(0).data().toString(), u"Personal"_s);
        QVERIFY(browser->selectId(562));
        QCOMPARE(view->currentIndex().siblingAtColumn(0).data().toString(), u"Original GRT"_s);
        browser->findChild<QLineEdit *>()->setText(u"Personal"_s);
        QCOMPARE(view->model()->rowCount(), 1);
        if (const QString shots = qEnvironmentVariable("CSR_SHOT_DIR"); !shots.isEmpty())
        {
            w.resize(1100, 400);
            browser->findChild<QLineEdit *>()->setText(u"N150"_s);
            w.grab().save(shots + u"/dbsource.png"_s);
        }
        browser->findChild<QLineEdit *>()->clear();
        QVERIFY(browser->selectId(id));

        // an own record can be exported as a project data file (records of the original cannot)
        QVERIFY(button(u"Export"_s)->isEnabled());
        QTemporaryDir exportDir;
        const QString exported = exportDir.filePath(u"data/propellants/n150.json"_s);
        QDir().mkpath(QFileInfo(exported).absolutePath());
        QVERIFY(w.exportRecord(exported));
        const auto loaded =
            csr::projectdata::load(exportDir.filePath(u"data"_s), csr::Table::Propellant, m_db.schema(csr::Table::Propellant));
        QVERIFY2(loaded.errors.isEmpty(), qPrintable(loaded.errors.join(u'\n')));
        QCOMPARE(loaded.records.size(), std::size_t{1});
        QCOMPARE(loaded.records[0].value(u"Ba"_s).toDouble(), 0.6);

        // the new powder is offered wherever powders are chosen
        SelectRecordDialog select(m_db, csr::Table::Propellant, u"x"_s);
        QVERIFY(select.findChild<TableBrowser *>()->selectId(id));

        // changing it
        QVariantMap changed = dlg.values();
        changed.insert(u"Ba"_s, 0.65);
        QCOMPARE(w.storeRecord(csr::Table::Propellant, RecordDialog::Mode::Edit, id, changed), id);
        QCOMPARE(m_db.record(csr::Table::Propellant, id)->value(u"Ba"_s).toDouble(), 0.65);
        QCOMPARE(m_db.record(csr::Table::Propellant, 562)->value(u"Ba"_s).toDouble(), 0.5814);  // the original is unchanged

        // deleting it
        QVERIFY(w.deleteRecord(false));
        QCOMPARE(browser->findChild<QTableView *>()->model()->rowCount(), count);
        QVERIFY(!browser->selectId(id));
        QVERIFY(browser->selectId(562));
        QVERIFY(!w.deleteRecord(false));  // records of the original cannot be deleted
        QVERIFY(!button(u"Export"_s)->isEnabled());
        QCOMPARE(m_db.count(csr::Table::Propellant), count);
    }
    QSettings().setValue(u"database/path"_s, before);
}

// Long lists (the countries) must not open as a full-height popup: on Wayland that one closes again at once.
void GuiTest::comboBoxesOpenAsAScrollingList()
{
    auto &app = *qobject_cast<QApplication *>(QCoreApplication::instance());
    // the system style (another test may already have applied a theme: then it is the base of the program's proxy style)
    const auto *installed = qobject_cast<QProxyStyle *>(app.style());
    const QString before = installed ? installed->baseStyle()->name() : app.style()->name();
    for (const theme::Mode mode : {theme::Mode::System, theme::Mode::Light, theme::Mode::Dark})
    {
        theme::apply(app, mode);
        QComboBox combo;
        for (int i = 0; i < 300; ++i) combo.addItem(QString::number(i));
        QStyleOptionComboBox opt;
        opt.initFrom(&combo);
        QCOMPARE(combo.style()->styleHint(QStyle::SH_ComboBox_Popup, &opt, &combo), 0);
        QTableWidget table;  // scrolls by pixel, not by item (a column jumped)
        QCOMPARE(table.horizontalScrollMode(), QAbstractItemView::ScrollPerPixel);
        QCOMPARE(table.verticalScrollMode(), QAbstractItemView::ScrollPerPixel);
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
    if (const QString shots = qEnvironmentVariable("CSR_SHOT_DIR"); !shots.isEmpty())
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

    QVERIFY(w.openLoad(QStringLiteral(CSR_FIXTURE_DIR "/8x57is.grtload")));
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
    QVERIFY(w.openLoad(QStringLiteral(CSR_FIXTURE_DIR "/8x57is.grtload")));
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
    QCOMPARE(w.mainMenu()->actions().size(), 5);  // File, Edit, View, Tools, Help still there
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
    if (const QString shots = qEnvironmentVariable("CSR_SHOT_DIR"); !shots.isEmpty())
    {
        box->show();
        box->grab().save(shots + u"/disclaimer.png"_s);
    }
    QSettings().setValue(u"ui/hideDisclaimer"_s, true);
    QVERIFY(!disclaimer::wanted());
    QVERIFY(disclaimer::confirm());  // hidden: confirmed without a dialog
    QSettings().remove(u"ui/hideDisclaimer"_s);
}

// Dark theme: the frame of an unchecked check box stands out from the window (Fusion alone draws it nearly black on dark grey).
void GuiTest::checkBoxesVisibleInDarkMode()
{
    theme::apply(*qApp, theme::Mode::Dark);
    QWidget w;
    auto *layout = new QVBoxLayout(&w);
    auto *box = new QCheckBox(u"Unchecked"_s);
    layout->addWidget(box);
    layout->addWidget(new QRadioButton(u"Radio"_s));
    auto *checked = new QCheckBox(u"Checked"_s);
    checked->setChecked(true);
    layout->addWidget(checked);
    w.resize(200, 100);
    w.show();
    QApplication::processEvents();
    const QImage img = w.grab().toImage();
    if (const QString shots = qEnvironmentVariable("CSR_SHOT_DIR"); !shots.isEmpty()) img.save(shots + u"/darkcheckbox.png"_s);
    QStyleOptionButton opt;
    opt.initFrom(box);
    const QRect r = box->style()->subElementRect(QStyle::SE_CheckBoxIndicator, &opt, box).translated(box->pos());
    const int window = w.palette().color(QPalette::Window).lightness();
    int brightest = 0;  // the frame: the lightest pixel on the indicator's left edge
    for (int y = r.top() + 2; y < r.bottom() - 1; ++y)
        for (int x = r.left(); x <= r.left() + 1; ++x) brightest = std::max(brightest, img.pixelColor(x, y).lightness());
    theme::apply(*qApp, theme::Mode::System);
    QVERIFY2(brightest - window > 60, qPrintable(u"frame %1, window %2"_s.arg(brightest).arg(window)));
}

// Dialogs get a 1 px frame in a light grey (Fusion draws none; the window manager's is not visible on a dark window); it stays
// above children added later and follows the size.
void GuiTest::dialogsHaveAVisibleBorder()
{
    theme::apply(*qApp, theme::Mode::Dark);
    QDialog dialog;
    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(new QLabel(u"Before"_s));
    dialog.resize(200, 100);
    dialog.show();
    QApplication::processEvents();
    auto *later = new QTextBrowser;  // fills the dialog up to its edges
    layout->addWidget(later);
    dialog.resize(240, 140);
    QApplication::processEvents();
    const QWidget *border = dialog.findChild<QWidget *>(u"dialogFrame"_s);
    QVERIFY(border);
    QCOMPARE(border->geometry(), dialog.rect());
    QCOMPARE(dialog.children().last(), border);  // on top
    // with the custom title bar the dialog draws its own, taller one inside the border
    const QWidget *bar = dialog.findChild<QWidget *>(u"dialogTitleBar"_s);
    QCOMPARE(bar != nullptr, TitleBar::stored());
    if (bar)
    {
        QVERIFY(dialog.windowFlags() & Qt::FramelessWindowHint);
        QVERIFY(bar->height() >= 40);
        QCOMPARE(bar->y(), dialog.contentsMargins().left());
        QVERIFY(dialog.contentsMargins().top() > bar->height());
        QVERIFY(bar->geometry().intersected(dialog.rect().adjusted(kWindowBorderWidth, kWindowBorderWidth, -kWindowBorderWidth,
                                                                   -kWindowBorderWidth)) == bar->geometry());
    }
    const QImage img = dialog.grab().toImage();
    if (const QString shots = qEnvironmentVariable("CSR_SHOT_DIR"); !shots.isEmpty()) img.save(shots + u"/dialogborder.png"_s);
    if (bar)  // rounded corners: the corner pixel is transparent, the middle of the window is not
    {
        QCOMPARE(img.pixelColor(0, 0).alpha(), 0);
        QCOMPARE(img.pixelColor(img.width() - 1, img.height() - 1).alpha(), 0);
        QCOMPARE(img.pixelColor(img.width() / 2, img.height() / 2).alpha(), 255);
    }
    const int window = dialog.palette().color(QPalette::Window).lightness();
    const QColor edge = img.pixelColor(img.width() / 2, img.height() - 1), side = img.pixelColor(0, img.height() / 2);
    theme::apply(*qApp, theme::Mode::System);
    QVERIFY2(edge.lightness() - window > 60 && side.lightness() - window > 60,
             qPrintable(u"edge %1, side %2, window %3"_s.arg(edge.lightness()).arg(side.lightness()).arg(window)));
    QMessageBox box;
    box.show();
    QApplication::processEvents();
    QVERIFY(box.findChild<QWidget *>(u"dialogFrame"_s));
}

// Without a compositing window manager (X11) the dialogs keep their frame but not the transparent, rounded corners.
void GuiTest::squareCornersWithoutCompositing()
{
    if (!TitleBar::stored()) QSKIP("the dialogs have no own frame");
    theme::apply(*qApp, theme::Mode::Dark);
    DialogFrame::forceRoundedCorners(false);
    const auto restore = qScopeGuard([] { DialogFrame::forceRoundedCorners(std::nullopt); });
    QDialog dialog;
    new QVBoxLayout(&dialog);
    dialog.resize(240, 140);
    dialog.show();
    for (int i = 0; i < 3; ++i) QApplication::processEvents();
    QVERIFY(dialog.findChild<QWidget *>(u"dialogTitleBar"_s));
    QVERIFY(!dialog.testAttribute(Qt::WA_TranslucentBackground));
    const QImage img = dialog.grab().toImage();
    QCOMPARE(img.pixelColor(0, 0).alpha(), 255);  // square: the border reaches the corner
    QCOMPARE(img.pixelColor(0, 0), dialog.palette().color(QPalette::Highlight));
    theme::apply(*qApp, theme::Mode::System);
}

// File > Settings > Rounded Dialog Corners: off keeps the dialogs' frame but with square corners; the choice is stored.
void GuiTest::roundedCornersCanBeSwitchedOffInTheSettings()
{
    if (!TitleBar::stored()) QSKIP("the dialogs have no own frame");
    theme::apply(*qApp, theme::Mode::Dark);
    const auto restore = qScopeGuard(
        []
        {
            DialogFrame::storeRounded(true);
            theme::apply(*qApp, theme::Mode::System);
        });
    SettingsDialog settings;
    auto *rounded = settings.findChild<QCheckBox *>(u"roundedDialogs"_s);
    QVERIFY(rounded);
    QVERIFY(rounded->isChecked());  // the default
    rounded->setChecked(false);
    settings.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
    QVERIFY(!DialogFrame::roundedStored());
    QVERIFY(!DialogFrame::roundedCorners());
    QDialog dialog;
    new QVBoxLayout(&dialog);
    dialog.resize(240, 140);
    dialog.show();
    for (int i = 0; i < 3; ++i) QApplication::processEvents();
    QVERIFY(dialog.findChild<QWidget *>(u"dialogTitleBar"_s));
    QVERIFY(!dialog.testAttribute(Qt::WA_TranslucentBackground));
    // Cancel undoes what Apply did
    settings.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Cancel)->click();
    QVERIFY(DialogFrame::roundedStored());
}

// A wrapped label (the model description in the Settings) gets the height of all its lines.
void GuiTest::settingsInfoTextIsNotCutOff()
{
    if (qEnvironmentVariableIsSet("CSR_NOBAR")) TitleBar::store(false);
    SettingsDialog dlg;
    dlg.show();
    for (int i = 0; i < 5; ++i) QApplication::processEvents();  // the labels grow, the layout and the dialog follow
    if (const QString shots = qEnvironmentVariable("CSR_SHOT_DIR"); !shots.isEmpty()) dlg.grab().save(shots + u"/settings.png"_s);
    // nothing overlaps its neighbour: the check box row under the model description starts below it
    const QCheckBox *grtFiles = dlg.findChild<QCheckBox *>(u"originalForGrtFiles"_s);
    QVERIFY(grtFiles);
    for (auto *label : dlg.findChildren<QLabel *>())
        if (label->wordWrap() && label->isVisible() && label->text().startsWith(u"Recommended"_s))
            QVERIFY2(label->mapTo(&dlg, QPoint(0, label->height())).y() <= grtFiles->mapTo(&dlg, QPoint(0, 0)).y(),
                     "the description overlaps the next row");
    for (auto *label : dlg.findChildren<QLabel *>())
    {
        if (!label->wordWrap() || !label->isVisible() || label->text().isEmpty()) continue;
        const int needed = label->heightForWidth(label->width());
        QVERIFY2(label->height() >= needed,
                 qPrintable(u"%1: height %2 < %3"_s.arg(label->text().left(30)).arg(label->height()).arg(needed)));
    }
}

// Dark theme: the scroll bar handle stands out from the window (Fusion fills it with the window color).
void GuiTest::scrollBarsVisibleInDarkMode()
{
    theme::apply(*qApp, theme::Mode::Dark);
    QScrollBar bar(Qt::Vertical);
    bar.setRange(0, 100);
    bar.setPageStep(20);
    bar.resize(16, 200);
    bar.show();
    QApplication::processEvents();
    const QImage img = bar.grab().toImage();
    if (const QString shots = qEnvironmentVariable("CSR_SHOT_DIR"); !shots.isEmpty()) img.save(shots + u"/darkscrollbar.png"_s);
    const int window = bar.palette().color(QPalette::Window).lightness();
    int brightest = 0;  // the handle: the lightest pixel down the middle
    for (int y = 0; y < img.height(); ++y) brightest = std::max(brightest, img.pixelColor(img.width() / 2, y).lightness());
    theme::apply(*qApp, theme::Mode::System);
    QVERIFY2(brightest - window > 30, qPrintable(u"handle %1, window %2"_s.arg(brightest).arg(window)));
}

// The mouse over the pressure chart: a cursor with the interpolated values of the curves; gone when the mouse leaves.
void GuiTest::chartCursorShowsTheValues()
{
    const auto lf = csr::LoadFile::read(QStringLiteral(CSR_FIXTURE_DIR "/8x57is.grtload"));
    QVERIFY(lf);
    const csr::SimResult r = csr::simulate(csr::SimInput::fromLoad(*lf));
    QVERIFY(r.ok());
    CurveChart chart;
    chart.setResult(r, 3900.0);
    chart.resize(800, 500);
    chart.show();
    QVERIFY(QTest::qWaitForWindowExposed(&chart));
    const QRectF plot = chart.chart()->plotArea();
    const QPoint inside = chart.mapFromScene(chart.chart()->mapToScene(plot.center()));
    QTest::mouseMove(chart.viewport(), inside);
    QTRY_VERIFY(chart.cursorX().has_value());
    // at the time of the pressure peak the cursor shows the peak pressure
    const csr::SimPoint &peak = r.points[r.indexPmax];
    const auto atPeak = chart.pointAt(peak.t_ms);
    QVERIFY(atPeak);
    QCOMPARE(atPeak->p_bar, peak.p_bar);
    // between two points: interpolated
    const csr::SimPoint &a = r.points[10], &b = r.points[11];
    const auto mid = chart.pointAt((a.t_ms + b.t_ms) / 2.0);
    QVERIFY(mid && std::abs(mid->v_ms - (a.v_ms + b.v_ms) / 2.0) < 1e-9);
    QVERIFY(!chart.pointAt(r.points.back().t_ms + 1.0));
    if (const QString shots = qEnvironmentVariable("CSR_SHOT_DIR"); !shots.isEmpty()) chart.grab().save(shots + u"/chartcursor.png"_s);
    QTest::mouseMove(chart.viewport(), QPoint(2, 2));  // outside the plot
    QTRY_VERIFY(!chart.cursorX().has_value());
}

// A maximum pressure above the permitted one: a red warning banner above the results; none for a load within the limit.
void GuiTest::overpressureIsWarnedAbout()
{
    QString error;
    LoadView normal;
    QVERIFY2(normal.load(QStringLiteral(CSR_FIXTURE_DIR "/8x57is.grtload"), &error), qPrintable(error));
    QVERIFY(normal.result().maxPressure_bar < 3900.0);
    QVERIFY(!normal.overpressureShown());

    auto lf = csr::LoadFile::read(QStringLiteral(CSR_FIXTURE_DIR "/8x57is.grtload"));
    QVERIFY(lf);
    lf->caliber.set(u"pMaxZul", 2000.0);  // below the load's maximum pressure
    QTemporaryDir dir;
    const QString path = dir.filePath(u"over.grtload"_s);
    QVERIFY(lf->write(path));
    LoadView over;
    QVERIFY2(over.load(path, &error), qPrintable(error));
    QVERIFY(over.result().maxPressure_bar > 2000.0);
    QVERIFY(over.overpressureShown());
    const auto *banner = over.findChild<QWidget *>(u"overpressureWarning"_s);
    QVERIFY(banner);
    const auto labels = banner->findChildren<QLabel *>();
    QVERIFY(std::ranges::any_of(labels, [](const QLabel *l) { return l->text().contains(u"Overpressure"_s); }));
    if (const QString shots = qEnvironmentVariable("CSR_SHOT_DIR"); !shots.isEmpty())
    {
        over.resize(1280, 800);
        over.show();
        QApplication::processEvents();
        over.grab().save(shots + u"/overpressure.png"_s);
    }
}

void GuiTest::fitToMeasurementsChangesThePowder()
{
    UnitPreferences::instance().setSystem(csr::units::System::Metric);
    LoadView view;
    QVERIFY(view.load(QStringLiteral(CSR_FIXTURE_DIR "/measurements.grtload")));
    auto *button = [&]
    {
        for (QPushButton *b : view.measurements()->findChildren<QPushButton *>())
            if (b->text() == u"Fit to measurements"_s) return b;
        return static_cast<QPushButton *>(nullptr);
    }();
    QVERIFY(button);
    QVERIFY(button->isEnabled());  // the template has measured charges

    // the dialog: fits right away, Save hands over the result
    QStringList warnings;
    csr::LoadFile file = *csr::LoadFile::read(QStringLiteral(CSR_FIXTURE_DIR "/measurements.grtload"));
    const auto samples = csr::powderfit::samples(file, u"template"_s, &warnings);
    QCOMPARE(samples.size(), std::size_t{2});
    PowderFitDialog dlg(samples, warnings);
    std::optional<csr::powderfit::Result> saved;
    dlg.setSaveHandler(
        [&](const csr::powderfit::Result &r)
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
    QCOMPARE(saved->model, csr::powderfit::Model::Original);  // a record holds the original model's values
    if (const QString shots = qEnvironmentVariable("CSR_SHOT_DIR"); !shots.isEmpty()) dlg.grab().save(shots + u"/powderfit.png"_s);
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
    QVERIFY(view.save(QDir::temp().filePath(u"csr-fit-test.grtload"_s)));  // unmodified again: only the fit can modify it now
    QVariantMap record;
    view.setRecordSaver(
        [&](csr::Table, const QVariantMap &values)
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
    QFile::remove(QDir::temp().filePath(u"csr-fit-test.grtload"_s));
    csr::LoadFile after;
    QTemporaryDir dir;
    QVERIFY(view.save(dir.filePath(u"fitted.csrload"_s)));
    after = *csr::LoadFile::read(dir.filePath(u"fitted.csrload"_s));
    QVERIFY2(after.propellant.number(u"Ba") > before * 1.02, qPrintable(u"Ba %1 -> %2"_s.arg(before).arg(after.propellant.number(u"Ba"))));
    QVERIFY(csr::readPhysicalBurn(after.propellant));  // and the physical model's values

    // a record made from it: the database's columns, 21 degC, the photos of the powder
    const QVariantMap rec = csr::powderfit::toRecord(after.propellant, *saved, m_db.schema(csr::Table::Propellant), u"test"_s);
    QCOMPARE(rec.value(u"pt"_s).toDouble(), 21.0);
    QVERIFY(rec.contains(u"imageUuid"_s));
    QVERIFY(csr::projectdata::check(csr::Table::Propellant, rec, m_db.schema(csr::Table::Propellant)).isEmpty());
    UnitPreferences::instance().setSystem(csr::units::System::Original);
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
    if (const QString shots = qEnvironmentVariable("CSR_SHOT_DIR"); !shots.isEmpty())
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
