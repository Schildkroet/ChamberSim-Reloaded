#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

#include "burnfactors.h"
#include "chronograph.h"
#include "dates.h"
#include "log.h"
#include "database.h"
#include "language.h"
#include "loadbuilder.h"
#include "loadfile.h"
#include "barrel.h"
#include "shotparts.h"
#include "measurement.h"
#include "numeric.h"
#include "physicalpowders.h"
#include "projectdata.h"
#include "powderfit.h"
#include "reference.h"
#include "report.h"
#include "reportformat.h"
#include "wiki.h"
#include "units.h"
#include "solver.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <limits>

// The sample loads are in tests/fixtures. The original's plain database comes from the original program and is not in the
// repository (.gitignore): without it (CI) the tests that need it are skipped.
#define REQUIRE_PLAIN_DB()                                                                                                                 \
    if (!QFile::exists(QStringLiteral(GRT_SOURCE_DIR "/tools/GordonsReloadingTool.plain.db"))) QSKIP("plain database not available")

using namespace Qt::StringLiterals;

class CoreTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void boreCrossSectionFromTheRifling();
    void shotshellPartsChangeTheFactors();
    void loadFileReadsSections();
    void loadFileRoundTrip();
    void loadFileKeepsAppendix();
    void measurementReadsTheOriginalsBlock();
    void measurementWritesBackWithoutLosingData();
    void measurementSeveralBlocks();
    void chronographFiles();
    void loggingFiltersByLevel();
    void burnRateCurveIsContinuous();
    void bulletJumpLowersThePressure();
    void datesAreStoredAsIso();
    void shotshellModel();
    void databaseImages();
    void ownRecordsKeepTheirPhotos();
    void burnFactorsFollowTheModel();
    void calibrationProfiles();
    void physicalModel();
    void physicalBurnInLoad();
    void tailEstimates();
    void caseLengthChange();
    void sebertDefault();
    void saamiColumnMigration();
    void powderFitPhysicalValues();
    void measurementStatistics();
    void simInputFromLoad();
    void chargeTooHighIsRejected();
    void temperatureCoefficients();
    void fastPowdersAndCovolume();
    void solverMatchesMomentumRule();
    void derivedOutputsMatchReference_data();
    void derivedOutputsMatchReference();
    void chargeRuleMatchesTheOriginal();
    void ratiosMatchTheOriginalsScreen();
    void bulletBaseTypesChangeTheChamberVolume();
    void ladderStepsDescendFromTheSelectedCharge();
    void ladderEdgeCases();
    void nonFiniteInputIsRejected();
    void solverVersusReference_data();
    void solverVersusReference();
    void databaseOpensPlainFile();
    void ownDatabaseTakesNewRecords();
    void ownDatabaseWorksWithoutTheOriginal();
    void ownDatabaseNeverStoresNull();
    void projectRecordsArePartOfTheDatabase();
    void ownRecordsOfOlderVersionsMoveUp();
    void powderFitFindsTheBurnRate();
    void languageResolution_data();
    void languageResolution();
    void normalizeLanguageTags();
    void unitConversions_data();
    void unitConversions();
    void unitSystemsAndParsing();
    void reportNumberFormats_data();
    void reportNumberFormats();
    void wikiBasicFormatting();
    void wikiBlocks();
    void wikiTables();
    void wikiImagesLinksAndTags();
    void wikiEscapesHostileInput();
    void wikiPageBreak();
    void reportPlaceholders();
    void reportUnitsInPlaceholders();
    void reportDataFromALoad();
    void defaultReportRendersEverything();
    void loadBuilderMatchesOriginal();
    void loadBuilderDefaultsAreSane();
    void replacingThePowderKeepsTheRest();
    void replacingTheBulletUpdatesGeometry();
    void replacingTheCaliberCanResetDependentValues();
};

void CoreTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);  // the own database must not be the user's
}

// The shell, wad and primer a shotshell load names scale the model (data/shotshell/parts.json); nothing named, or an unknown id: no change.
void CoreTest::shotshellPartsChangeTheFactors()
{
    grt::setShotPartsFile(QStringLiteral(GRT_SOURCE_DIR "/data/shotshell/parts.json"));
    grt::EntryList caliber;
    caliber.set(u"shotshell", u"true"_s);
    QCOMPARE(grt::shotFactorsOf(caliber).capacity, 1.0);
    const auto *remington = grt::findShotPart(grt::ShotPart::Shell, u"remington-sts-nitro-27-gun-club"_s);
    const auto *federal = grt::findShotPart(grt::ShotPart::Shell, u"federal-gold-medal-plastic"_s);
    QVERIFY(remington && federal);
    QVERIFY(remington->capacityScale < 1.0 && federal->capacityScale > 1.0);  // the Remington hull gives the higher pressure
    caliber.set(u"shothull", u"remington-sts-nitro-27-gun-club"_s);
    QCOMPARE(grt::shotFactorsOf(caliber).capacity, remington->capacityScale);
    caliber.set(u"shotprimer", u"win209"_s);  // the factors of the parts multiply
    const auto *primer = grt::findShotPart(grt::ShotPart::Primer, u"win209"_s);
    QVERIFY(primer);
    QCOMPARE(grt::shotFactorsOf(caliber).capacity, remington->capacityScale * primer->capacityScale);
    caliber.set(u"shothull", u"unknown"_s);
    QCOMPARE(grt::shotFactorsOf(caliber).capacity, primer->capacityScale);
    // groups of similar parts for those who do not know the exact one: low-pressure parts have more powder space than medium ones, those more than high ones
    for (const grt::ShotPart part : grt::kShotParts)
    {
        const auto &groups = grt::shotPartGroups(part);
        QCOMPARE(groups.size(), std::size_t(3));
        QVERIFY(groups[0].group && groups[0].rank == u"low"_s && groups[2].rank == u"high"_s);
        QVERIFY(groups[0].capacityScale > groups[1].capacityScale && groups[1].capacityScale > groups[2].capacityScale);
        QVERIFY(!groups[0].members.isEmpty());
        QCOMPARE(grt::findShotPart(part, u"group-low"_s), &groups[0]);
    }
    caliber.set(u"shothull", u"group-high"_s);
    caliber.set(u"shotprimer", QString());
    QCOMPARE(grt::shotFactorsOf(caliber).capacity, grt::shotPartGroups(grt::ShotPart::Shell)[2].capacityScale);
    // the simulation: the same load with the Remington hull peaks higher than with the Federal one
    grt::SimInput in;
    in.shotshell = true;
    in.boreArea_mm2 = 260.16;
    in.caseVolume_cm3 = 17.24;
    in.caseLength_mm = 70.0;
    in.bulletDiameter_mm = 18.2;
    in.bulletMass_g = 30.85;
    in.bulletLength_mm = in.seatingDepth_mm = 40.5;
    in.charge_g = 1.13;
    in.barrelLength_mm = 711.0;
    in.startPressure_bar = 20.0;
    in.heatQex_kJkg = 4000.0;
    in.density_kgm3 = 1600.0;
    in.kappa = 1.23;
    in.vivacityBa = 11.0;
    in.z1 = 0.3;
    in.z2 = 0.6;
    const double plain = grt::simulate(in).maxPressure_bar;
    in.shotCapacityScale = remington->capacityScale;
    in.shotEnergyScale = remington->energyScale;
    const double high = grt::simulate(in).maxPressure_bar;
    in.shotCapacityScale = federal->capacityScale;
    in.shotEnergyScale = federal->energyScale;
    const double low = grt::simulate(in).maxPressure_bar;
    QVERIFY2(plain > 100.0 && high > plain && low < plain, qPrintable(u"%1 %2 %3"_s.arg(plain).arg(high).arg(low)));
}

// Q of the C.I.P. sheets from F, Z, N and b.
void CoreTest::boreCrossSectionFromTheRifling()
{
    QVERIFY(std::abs(grt::boreCrossSection_mm2(10.25, 10.49, 6, 3.6) / 85.16 - 1.0) < 0.002);  // 10.3 Capra
    QVERIFY(std::abs(grt::boreCrossSection_mm2(7.89, 8.2, 4, 4.4) / 51.78 - 1.0) < 0.005);     // 8 x 57 IS
    QCOMPARE(grt::boreCrossSection_mm2(10.0, 10.0, 6, 3.0), std::numbers::pi / 4.0 * 100.0);   // no groove depth
    QCOMPARE(grt::boreCrossSection_mm2(10.0, 10.4, 0, 3.0), std::numbers::pi / 4.0 * 100.0);   // smooth bore
    QCOMPARE(grt::boreCrossSection_mm2(10.0, 9.0, 6, -3.0), std::numbers::pi / 4.0 * 100.0);   // nonsense input does not subtract
}

void CoreTest::loadFileReadsSections()
{
    QString err;
    const auto lf = grt::LoadFile::read(QStringLiteral(GRT_FIXTURE_DIR "/8x57is.grtload"), &err);
    QVERIFY2(lf.has_value(), qPrintable(err));
    QCOMPARE(lf->title, u"8 x 57 IS (8 x 57 JS) Sellier-Bellot, FMJ-BT #2910, 0.323, 196.00 grain Vihtavuori N150"_s);
    QCOMPARE(lf->caliber.text(u"CaliberName"), u"8 x 57 IS (8 x 57 JS)"_s);
    QCOMPARE(lf->caliber.number(u"casevol"), 4.110028050688221);
    QCOMPARE(lf->gun.number(u"xe"), 600.0);
    QCOMPARE(lf->propellant.text(u"pname"), u"N150"_s);
    QVERIFY(!lf->caliberFile.entries().empty());
    QCOMPARE(lf->caliberFile.text(u"cippdf"), u"https://bobp.cip-bobp.org/uploads/tdcc/tab-i/8-x-57-is-en.pdf"_s);
    QVERIFY(!lf->caliber.flag(u"shotshell"));
}

void CoreTest::loadFileRoundTrip()
{
    const auto a = grt::LoadFile::read(QStringLiteral(GRT_FIXTURE_DIR "/8x57is.grtload"));
    QVERIFY(a);
    const auto b = grt::LoadFile::parse(a->toXml());
    QVERIFY(b);
    QCOMPARE(b->title, a->title);
    QCOMPARE(b->caliberFile.entries().size(), a->caliberFile.entries().size());
    for (const auto sel : {&grt::LoadFile::caliber, &grt::LoadFile::gun, &grt::LoadFile::projectile, &grt::LoadFile::propellant,
                           &grt::LoadFile::caliberFile, &grt::LoadFile::config})
    {
        const auto &ea = ((*a).*sel).entries();
        const auto &eb = ((*b).*sel).entries();
        QCOMPARE(eb.size(), ea.size());
        for (std::size_t i = 0; i < ea.size(); ++i)
        {
            QCOMPARE(eb[i].name, ea[i].name);
            QCOMPARE(eb[i].value, ea[i].value);
            QCOMPARE(eb[i].unit, ea[i].unit);
        }
    }
}

void CoreTest::loadFileKeepsAppendix()
{
    const auto a = grt::LoadFile::read(QStringLiteral(GRT_FIXTURE_DIR "/measurements.grtload"));
    QVERIFY(a);
    QVERIFY(a->appendixXml.contains(u"<Measurement"_s));
    QVERIFY(a->appendixXml.contains(u"<shot"_s));
    const auto b = grt::LoadFile::parse(a->toXml());
    QVERIFY(b);
    QCOMPARE(b->appendixXml.count(u"<shot "_s), a->appendixXml.count(u"<shot "_s));
    QCOMPARE(b->appendixXml.count(u"<charge "_s), 2);
}

void CoreTest::measurementReadsTheOriginalsBlock()
{
    const auto lf = grt::LoadFile::read(QStringLiteral(GRT_FIXTURE_DIR "/measurements.grtload"));
    QVERIFY(lf);
    const auto m = grt::readMeasurement(lf->appendixXml);
    QVERIFY(m);
    QCOMPARE(m->title, u"MEASUREMENTS"_s);
    QCOMPARE(m->charges.size(), std::size_t{2});
    QCOMPARE(m->charges[0].name, u"Charge 1"_s);
    QCOMPARE(m->charges[0].note, u"EXAMPLE ONLY! DO NOT USE THIS LOAD"_s);
    QCOMPARE(m->charges[0].charge_g(), 2.462358579991);
    QCOMPARE(m->charges[0].shots.size(), std::size_t{4});
    QCOMPARE(m->charges[1].shots.size(), std::size_t{5});
    QCOMPARE(m->charges[0].shots[0].velocity_ms, 844.60080000000607);
    QVERIFY(!grt::readMeasurement(u"<note index=\"1\" title=\"x\" />"_s));
}

void CoreTest::measurementWritesBackWithoutLosingData()
{
    // a block like the original writes it, with a pressure trace inside a charge and a note before it
    const QString appendix = uR"(<note index="1" title="N" text="hello%20world" />
<Measurement index="2" showinreport="false" title="Measurement%20MagnetoSpeed">
  <charge name="168.00%20grain" showinreport="true" expanded="true" note="PT%20II" value="0.003175146589988" menu="" source="PressureTrace" sourceoptions="%7B%7D">
    <shot name="" velocity="814.73040000000583" pressure="3614.8908349318885" />
    <shot name="" velocity="815.34" pressure="3500.05" />
    <PressureTraceFile name="a.ptc" type="ptc" overlayidx="-1"><property name="Barrel" type="string" value="24 inch" /></PressureTraceFile>
  </charge>
</Measurement>)"_s;
    auto m = grt::readMeasurement(appendix);
    QVERIFY(m);
    QCOMPARE(m->charges[0].shots[0].pressure_bar, 3614.8908349318885);
    m->charges[0].shots.push_back({.velocity_ms = 820.0});
    m->charges[0].note = u"edited note"_s;
    m->charges.push_back(grt::MeasuredCharge::create(u"Charge 2"_s, 3.3));
    const QString written = grt::writeMeasurement(appendix, *m);
    QVERIFY(written.contains(u"hello%20world"_s));       // the note block is untouched
    QVERIFY(written.contains(u"<PressureTraceFile"_s));  // so is the pressure trace
    QVERIFY(written.contains(u"value=\"24 inch\""_s));
    QVERIFY(written.contains(u"source=\"PressureTrace\""_s));  // and the attributes this program does not edit
    QVERIFY(written.contains(u"showinreport=\"false\""_s));
    QCOMPARE(written.count(u"<Measurement"_s), 1);
    const auto back = grt::readMeasurement(written);
    QVERIFY(back);
    QCOMPARE(back->title, u"Measurement MagnetoSpeed"_s);
    QCOMPARE(back->charges.size(), std::size_t{2});
    QCOMPARE(back->charges[0].shots.size(), std::size_t{3});
    QCOMPARE(back->charges[0].shots[0].velocity_ms, 814.73040000000583);
    QCOMPARE(back->charges[0].note, u"edited note"_s);
    QCOMPARE(back->charges[1].charge_g(), 3.3);
    QCOMPARE(back->charges[0].extraXml.count(u"<property"_s), 1);

    // a load without measurements gets a new block with the next free tab index; the file stays readable
    grt::Measurement fresh{.title = u"MEASUREMENTS"_s, .charges = {grt::MeasuredCharge::create(u"A"_s, 2.5)}, .attributes = {}};
    const QString added = grt::writeMeasurement(u"<note index=\"1\" title=\"N\" />"_s, fresh);
    QVERIFY(added.contains(u"<Measurement index=\"2\""_s));
    QVERIFY(added.contains(u"<note"_s));
    auto lf = grt::LoadFile::read(QStringLiteral(GRT_FIXTURE_DIR "/8x57is.grtload"));
    QVERIFY(lf);
    lf->appendixXml = grt::writeMeasurement(lf->appendixXml, fresh);
    const auto reread = grt::LoadFile::parse(lf->toXml());
    QVERIFY(reread);
    const auto again = grt::readMeasurement(reread->appendixXml);
    QVERIFY(again);
    QCOMPARE(again->charges.size(), std::size_t{1});
    QCOMPARE(again->charges[0].charge_g(), 2.5);
}

void CoreTest::measurementSeveralBlocks()
{
    const QString appendix = uR"(<note index="1" title="N" text="hello" />
<Measurement index="2" showinreport="true" title="Session%201">
  <charge name="A" value="0.0025" note=""><shot name="" velocity="800" pressure="0" /></charge>
</Measurement>
<Measurement index="3" showinreport="false" title="Session%202">
  <charge name="B" value="0.0026" note=""><shot name="" velocity="820" pressure="0" /></charge>
</Measurement>)"_s;
    auto series = grt::readMeasurements(appendix);
    QCOMPARE(series.size(), std::size_t{2});
    QCOMPARE(series[1].title, u"Session 2"_s);
    QCOMPARE(series[1].charges[0].name, u"B"_s);
    QCOMPARE(grt::readMeasurement(appendix)->title, u"Session 1"_s);  // the first-block API still works

    // the second block is edited, the first stays, a third is added with the next free index
    series[1].charges[0].shots.push_back({.velocity_ms = 822.0});
    series.push_back({.title = u"Session 3"_s, .charges = {grt::MeasuredCharge::create(u"C"_s, 2.7)}, .attributes = {}});
    QString written = grt::writeMeasurements(appendix, series);
    QVERIFY(written.contains(u"hello"_s));
    QCOMPARE(written.count(u"<Measurement"_s), 3);
    QVERIFY(written.contains(u"<Measurement index=\"4\""_s));
    QVERIFY(written.contains(u"showinreport=\"false\""_s));
    const auto back = grt::readMeasurements(written);
    QCOMPARE(back.size(), std::size_t{3});
    QCOMPARE(back[1].charges[0].shots.size(), std::size_t{2});
    QCOMPARE(back[2].charges[0].charge_g(), 2.7);

    // removing a series removes its block; replacing only the first keeps the others
    series.erase(series.begin());
    written = grt::writeMeasurements(appendix, std::span<const grt::Measurement>(series).first(1));
    QCOMPARE(written.count(u"<Measurement"_s), 1);
    QCOMPARE(grt::readMeasurements(written)[0].title, u"Session 2"_s);
    grt::Measurement first = *grt::readMeasurement(appendix);
    first.title = u"Renamed"_s;
    const auto kept = grt::readMeasurements(grt::writeMeasurement(appendix, first));
    QCOMPARE(kept.size(), std::size_t{2});
    QCOMPARE(kept[0].title, u"Renamed"_s);
    QCOMPARE(kept[1].title, u"Session 2"_s);
}

void CoreTest::chronographFiles()
{
    const QByteArray ms = "Series,15, Shots:,3\r\nMin,2721, Max,2770  \r\nAvg,2746 ,S-D,10.1  \r\nES ,  49\r\n\r\n"
                          "Series,Shot,Speed   \r\n15, 1, 2721, ft/s   \r\n15, 2, 2722, ft/s   \r\n15, 3, 2745, ft/s   \r\n"
                          "----,----,----,---- \r\n\r\nSeries,16, Shots:,2\r\nSeries,Shot,Speed\r\n16, 1, 830.5, m/s\r\n16, 2, 0, m/s\r\n";
    QString error;
    const auto m = grt::readChronograph(ms, &error);
    QVERIFY2(m, qPrintable(error));
    QCOMPARE(m->format, grt::ChronographFormat::MagnetoSpeed);
    QCOMPARE(m->series.size(), std::size_t{2});
    QCOMPARE(m->series[0].name, u"Series 15"_s);
    QCOMPARE(m->series[0].velocities_ms.size(), std::size_t{3});
    QCOMPARE_LT(std::abs(m->series[0].velocities_ms[0] - 2721 * 0.3048), 1e-9);
    QCOMPARE(m->series[1].velocities_ms, std::vector<double>{830.5});  // a 0 is a missed shot

    // the Labradar report: NUL padded lines, V0 column, unit from the header
    static constexpr char kLabradar[] =
        "sep=;\r\nDevice ID;LBR-1;;\0   \r\n\0Series No;0003;;\0  \r\n\0Units velocity;fps;;\0  \r\n\0\r\n"
        "Shot ID;V0;V1;V2;Ke0\r\n0001;2755;2754;2752;2393;\r\n0002;;2752;2751;;\r\n0003;2754;2753;2751;2391;\r\n";
    const auto l = grt::readChronograph(QByteArray(kLabradar, sizeof kLabradar - 1), &error);
    QVERIFY2(l, qPrintable(error));
    QCOMPARE(l->format, grt::ChronographFormat::Labradar);
    QCOMPARE(l->series.size(), std::size_t{1});
    QCOMPARE(l->series[0].name, u"SR0003"_s);
    QCOMPARE(l->series[0].velocities_ms.size(), std::size_t{2});
    QCOMPARE_LT(std::abs(l->series[0].velocities_ms[1] - 2754 * 0.3048), 1e-9);

    QVERIFY(!grt::readChronograph("a,b,c\n1,2,3\n", &error));
    QVERIFY(!error.isEmpty());
    QVERIFY(!grt::readChronograph("Series,1, Shots:,0\r\n", &error));
}

void CoreTest::loggingFiltersByLevel()
{
    QTemporaryDir dir;
    const QString path = dir.filePath(u"test.log"_s);
    const grt::log::Config before = grt::log::config();
    grt::log::setConfig({.level = grt::log::Level::Warn, .console = false, .file = true, .path = path});
    LOG_Debug() << "hidden debug";
    LOG_Info() << "hidden info";
    LOG_Warn() << "shown" << u"warning"_s << 42;
    LOG_Error() << "shown error";
    grt::log::setConfig({.level = grt::log::Level::Off, .console = false, .file = true, .path = path});
    LOG_Error() << "off: not written";
    grt::log::setConfig({.level = grt::log::Level::Debug, .console = false, .file = true, .path = path});
    LOG_Debug() << "now visible";
    QCOMPARE(grt::log::activePath(), path);

    QFile f(path);
    QVERIFY(f.open(QIODevice::ReadOnly));
    const QString text = QString::fromUtf8(f.readAll());
    QVERIFY(!text.contains(u"hidden"_s));
    QVERIFY(!text.contains(u"not written"_s));
    QVERIFY(text.contains(u"[WARN ]"_s));
    QVERIFY(text.contains(u"shown warning 42"_s));  // QString without quotes
    QVERIFY(text.contains(u"[ERROR]"_s));
    QVERIFY(text.contains(u"[DEBUG]"_s));
    QVERIFY(text.contains(u"tst_core.cpp:"_s));
    QCOMPARE(text.count(u'\n'), 3);

    QCOMPARE(grt::log::levelFromName(u"warning"_s), grt::log::Level::Warn);
    QCOMPARE(grt::log::levelFromName(u"WARN"_s), grt::log::Level::Warn);
    QVERIFY(!grt::log::levelFromName(u"loud"_s));
    grt::log::clearFile();
    QVERIFY(!QFile::exists(path));
    grt::log::setConfig(before);
}

void CoreTest::databaseImages()
{
    QTemporaryDir dir;
    grt::Database db;
    QVERIFY(db.open({}, dir.filePath(u"own.db"_s)));
    QVERIFY(!db.image(u"nope"_s));
    QSqlQuery q(db.handle());
    q.prepare(u"INSERT INTO main.image (uuid, name, descr, scale, scaleunit, data, cdate, cby, type, mode, status, geloescht) "
              "VALUES (?, 'grain.png', '', 29.5, 'px/mm', ?, '', '', 'png', '', '', ?)"_s);
    const QByteArray bytes("not really a picture");
    q.addBindValue(u"abc"_s);
    q.addBindValue(QString::fromLatin1(bytes.toBase64()));
    q.addBindValue(0);
    QVERIFY2(q.exec(), qPrintable(q.lastError().text()));
    const auto img = db.image(u"abc"_s);
    QVERIFY(img);
    QCOMPARE(img->name, u"grain.png"_s);
    QCOMPARE(img->scale, 29.5);
    QCOMPARE(img->scaleUnit, u"px/mm"_s);
    QCOMPARE(img->data, bytes);  // decoded from base64
    QVERIFY(!db.image({}));
}

void CoreTest::ownRecordsKeepTheirPhotos()
{
    REQUIRE_PLAIN_DB();
    QTemporaryDir dir;
    const QString own = dir.filePath(u"own.db"_s);
    QString uuid, detail;
    {
        grt::Database db;
        QVERIFY(db.open(QStringLiteral(GRT_SOURCE_DIR "/tools/GordonsReloadingTool.plain.db"), own));
        QSqlQuery q(db.handle());
        QVERIFY(
            q.exec(u"SELECT id FROM propellant WHERE imageUuid != '' AND imageDetailUuid != '' AND geloescht = 0 ORDER BY id LIMIT 1"_s) &&
            q.next());
        const auto record = db.record(grt::Table::Propellant, q.value(0).toInt());
        QVERIFY(record);
        uuid = record->value(u"imageUuid"_s).toString();
        detail = record->value(u"imageDetailUuid"_s).toString();
        QVERIFY(db.image(uuid) && db.image(detail));
        QSqlQuery inOwn(db.handle());
        QVERIFY(inOwn.exec(u"SELECT count(*) FROM main.image"_s) && inOwn.next());
        QCOMPARE(inOwn.value(0).toInt(), 0);                  // the pictures are in the original database only
        QVERIFY(db.insert(grt::Table::Propellant, *record));  // a copy of the powder as an own record
        QVERIFY(inOwn.exec(u"SELECT count(*) FROM main.image"_s) && inOwn.next());
        QCOMPARE(inOwn.value(0).toInt(), 2);  // ... now copied
        QVERIFY(db.insert(grt::Table::Propellant, *record));
        QVERIFY(inOwn.exec(u"SELECT count(*) FROM main.image"_s) && inOwn.next());
        QCOMPARE(inOwn.value(0).toInt(), 2);  // not twice
    }
    grt::Database alone;  // without the original database
    QVERIFY(alone.open({}, own));
    const auto img = alone.image(uuid);
    QVERIFY(img);
    QVERIFY(img->data.size() > 1000);
    QVERIFY(alone.image(detail));
    QCOMPARE(img->uuid, uuid);
    QCOMPARE(img->type, u"jpg"_s);

    // a new picture: stored with a generated uuid, once
    grt::Database::Image fresh{.name = u"mine.jpg"_s, .scale = 80.0, .scaleUnit = u"px/mm"_s, .data = QByteArray("jpeg bytes")};
    const auto id = alone.addImage(fresh);
    QVERIFY(id && !id->isEmpty());
    fresh.uuid = *id;
    QCOMPARE(alone.addImage(fresh), id);
    QCOMPARE(alone.image(*id)->scale, 80.0);
    QVERIFY(!alone.addImage({}));  // no data
}

// Br, Bp, Brp of the original's N150 record (Br and Brp follow from the documented formulas, Bp uses an approximated phi(z1)).
void CoreTest::burnFactorsFollowTheModel()
{
    const grt::BurnFactors n150 = grt::burnFactors(0.5814, 3790.0, 1.2184, 1.1286, 0.4597, 0.8169);
    QVERIFY(qAbs(n150.Br - 0.1456) < 2e-4);
    QVERIFY(qAbs(n150.Bp / 0.1691 - 1.0) < 0.04);
    QVERIFY(qAbs(n150.Brp / 0.1578 - 1.0) < 0.03);
    QCOMPARE(grt::burnFactors(0.0, 3790.0, 1.22, 1.0, 0.4, 0.8).Br, 0.0);  // no burn rate: nothing to derive

    // the project's records get them when they are loaded; an edited own record gets new ones
    QTemporaryDir dir;
    grt::Database::setProjectDataLocation(QStringLiteral(GRT_SOURCE_DIR "/data"));
    grt::Database db;
    QVERIFY(db.open({}, dir.filePath(u"own.db"_s)));
    QSqlQuery q(db.handle());
    QVERIFY(q.exec(u"SELECT count(*), sum(Br > 0 AND Bp > 0 AND Brp > 0) FROM propellant"_s) && q.next());
    QVERIFY(q.value(0).toInt() > 0);
    QCOMPARE(q.value(1).toInt(), q.value(0).toInt());

    QVariantMap own{{u"mname"_s, u"Test"_s}, {u"pname"_s, u"X"_s}, {u"Ba"_s, 0.6}, {u"Qex"_s, 3800.0},
                    {u"k"_s, 1.23},          {u"a0"_s, 1.0},       {u"z1"_s, 0.4}, {u"z2"_s, 0.8}};
    const auto id = db.insert(grt::Table::Propellant, own);
    QVERIFY(id);
    const double br = db.record(grt::Table::Propellant, *id)->value(u"Br"_s).toDouble();
    QVERIFY(br > 0.0);
    QVERIFY(db.update(grt::Table::Propellant, *id, {{u"Ba"_s, 1.2}}));
    QVERIFY(qAbs(db.record(grt::Table::Propellant, *id)->value(u"Br"_s).toDouble() / br - 2.0) < 1e-9);
    grt::Database::setProjectDataLocation(u":/data"_s);
}

// The solver's default calibration is the original's; the published-data profile changes the results (and only when it is selected),
// shotshells keep the original's constants.
void CoreTest::calibrationProfiles()
{
    QCOMPARE(grt::calibrationProfile(), grt::CalibrationProfile::Original);
    const auto lf = grt::LoadFile::read(QStringLiteral(GRT_FIXTURE_DIR "/8x57is.grtload"));
    QVERIFY(lf);
    const grt::SimInput in = grt::SimInput::fromLoad(*lf);
    const grt::SimResult original = grt::simulate(in);
    QVERIFY(original.ok());
    QCOMPARE(grt::simulate(in, grt::calibrationFor(grt::CalibrationProfile::Original)).maxPressure_bar, original.maxPressure_bar);

    const grt::SimResult published = grt::simulate(in, grt::calibrationFor(grt::CalibrationProfile::Published));
    QVERIFY(published.ok());
    QVERIFY(qAbs(published.maxPressure_bar / original.maxPressure_bar - 1.0) > 0.01);  // really another calibration

    grt::setCalibrationProfile(grt::CalibrationProfile::Published);  // the default of simulate() follows the selection
    QCOMPARE(grt::calibrationProfile(), grt::CalibrationProfile::Published);
    QCOMPARE(grt::simulate(in).maxPressure_bar, published.maxPressure_bar);
    grt::SimInput shot = in;  // a shotshell keeps the original's constants
    shot.shotshell = true;
    grt::setCalibrationProfile(grt::CalibrationProfile::Original);
    const double shotOriginal = grt::simulate(shot).maxPressure_bar;
    grt::setCalibrationProfile(grt::CalibrationProfile::Published);
    QCOMPARE(grt::simulate(shot).maxPressure_bar, shotOriginal);
    QCOMPARE(grt::simulateLadder(in).front().result.maxPressure_bar, published.maxPressure_bar);  // the ladder follows it too
    grt::setCalibrationProfile(grt::CalibrationProfile::Original);
    QCOMPARE(grt::simulate(in).maxPressure_bar, original.maxPressure_bar);
    QCOMPARE(grt::simulateLadder(in).front().result.maxPressure_bar, original.maxPressure_bar);
}

// The project's own model (physical profile): converged (the output grid and the tolerance do not change the result), causal (the
// peak pressure does not depend on the barrel length), losses in the energy balance, and the inputs the original ignores act on it.
void CoreTest::physicalModel()
{
    const auto lf = grt::LoadFile::read(QStringLiteral(GRT_FIXTURE_DIR "/8x57is.grtload"));
    QVERIFY(lf);
    const grt::SimInput in = grt::SimInput::fromLoad(*lf);
    QCOMPARE(grt::calibrationFor(grt::CalibrationProfile::Published).model, grt::SolverModel::Original);  // the empirical constants
    const grt::Calibration cal = grt::calibrationFor(grt::CalibrationProfile::Physical);
    QCOMPARE(cal.model, grt::SolverModel::Physical);
    const grt::SimResult r = grt::simulate(in, cal);
    QVERIFY(r.ok());
    QCOMPARE(r.points.size(), std::size_t{2000});
    QVERIFY(qAbs(r.points.back().x_mm - in.barrelLength_mm) < 1e-6);  // the last point is the muzzle
    QVERIFY(r.heatLoss_J > 0.0 && r.muzzleFrictionEnergy_J > 0.0);
    QVERIFY(r.muzzleEnergy_J + r.heatLoss_J + r.muzzleFrictionEnergy_J < r.propellantEnergy_J);

    const auto rel = [](double a, double b) { return qAbs(a / b - 1.0); };
    const grt::SimResult coarse = grt::simulate(in, cal, 200);
    QVERIFY(rel(coarse.muzzleVelocity_ms, r.muzzleVelocity_ms) < 1e-6);
    QVERIFY(rel(coarse.maxPressure_bar, r.maxPressure_bar) < 0.005);  // only the sampling of the peak
    grt::Calibration fine = cal;
    fine.integrationTolerance = 1e-9;
    QVERIFY(rel(grt::simulate(in, fine).muzzleVelocity_ms, r.muzzleVelocity_ms) < 1e-4);
    for (const double xe : {300.0, 1200.0})
    {
        grt::SimInput other = in;
        other.barrelLength_mm = xe;
        QVERIFY(rel(grt::simulate(other, cal).maxPressure_bar, r.maxPressure_bar) < 1e-3);
    }
    const auto velocity = [&](auto change)
    {
        grt::SimInput other = in;
        change(other);
        return grt::simulate(other, cal).muzzleVelocity_ms;
    };
    QVERIFY(velocity([](grt::SimInput &i) { i.twistLength_mm = 0.0; }) > r.muzzleVelocity_ms);    // spin takes energy
    QVERIFY(velocity([](grt::SimInput &i) { i.bulletLength_mm *= 1.3; }) < r.muzzleVelocity_ms);  // longer bearing surface
    QVERIFY(velocity([](grt::SimInput &i) { i.covolumeEta = 1.1; }) != r.muzzleVelocity_ms);      // the covolume of the record
    QVERIFY(velocity([](grt::SimInput &i) { i.temperature_C = -10.0; }) < r.muzzleVelocity_ms);

    QCOMPARE(r.points.front().v_ms, 0.0);  // the curves begin when the bullet moves: at rest only the first point
    QCOMPARE(r.points.front().t_ms, 0.0);
    QVERIFY(r.points[1].v_ms > 0.0);
    QVERIFY(r.points.front().p_bar >= in.startPressure_bar);

    grt::SimInput shot = in;  // a shotshell keeps the original's constants also with the physical ones given explicitly
    shot.shotshell = true;
    QCOMPARE(grt::simulate(shot, cal).maxPressure_bar, grt::simulate(shot, grt::Calibration{}).maxPressure_bar);

    grt::SimInput weak = in;  // far too little powder for a long, heavy bullet in a long barrel
    weak.charge_g = 0.05;
    weak.bulletMass_g = 40.0;
    weak.barrelLength_mm = 3000.0;
    weak.extraFrictionUnlocked = true;
    weak.extraFriction_bar = 300.0;
    QCOMPARE(grt::simulate(weak, cal).status, grt::SimStatus::BulletStuck);
}

// The physical burn values travel in the load file (physBa ... physRef): written when a project powder with such values is chosen,
// used by the physical model only while Ba, a0, z1, z2 are unchanged, removed for a powder without values.
void CoreTest::physicalBurnInLoad()
{
    QTemporaryDir dir;
    const QString file = dir.filePath(u"physical.json"_s);
    {
        QFile f(file);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(R"({"propellants": [{"mname": "Test", "pname": "P1", "record": {"Ba": 0.6, "a0": 0, "z1": 0.3, "z2": 0.6},
                    "Ba": 0.9, "a0": 1.5, "z1": 0.4, "z2": 0.75}]})");
    }
    grt::setPhysicalBurnFile(file);
    const QVariantMap powder{{u"mname"_s, u"Test"_s}, {u"pname"_s, u"P1"_s}, {u"Ba"_s, 0.6}, {u"a0"_s, 0.0},    {u"z1"_s, 0.3},
                             {u"z2"_s, 0.6},          {u"Qex"_s, 3800.0},    {u"k"_s, 1.22}, {u"pc"_s, 1600.0}, {u"pcd"_s, 900.0}};
    QVERIFY(grt::physicalBurnOf(powder));
    QVariantMap other = powder;
    other.insert(u"Ba"_s, 0.61);  // another lot of the same powder
    QVERIFY(!grt::physicalBurnOf(other));

    auto lf = grt::LoadFile::read(QStringLiteral(GRT_FIXTURE_DIR "/8x57is.grtload"));
    QVERIFY(lf);
    grt::applyPropellant(*lf, powder);
    const auto again = grt::LoadFile::parse(lf->toXml());  // through the file format
    QVERIFY(again);
    grt::SimInput in = grt::SimInput::fromLoad(*again);
    QVERIFY(in.physicalBurn);
    QCOMPARE(in.physicalBurn->Ba, 0.9);
    QCOMPARE(in.physicalBurn->z2, 0.75);
    const grt::Calibration cal = grt::calibrationFor(grt::CalibrationProfile::Physical);
    const double withValues = grt::simulate(in, cal).muzzleVelocity_ms;
    QVERIFY(withValues > 0.0);
    in.physicalBurn.reset();
    QVERIFY(grt::simulate(in, cal).muzzleVelocity_ms != withValues);  // the model really uses them

    grt::LoadFile edited = *again;
    edited.propellant.set(u"Ba", 0.65);  // the user changes the burn rate: the physical values no longer apply
    QVERIFY(!grt::SimInput::fromLoad(edited).physicalBurn);
    grt::applyPropellant(edited, other);  // a powder without values removes them
    QVERIFY(!edited.propellant.find(u"physBa"));

    grt::LoadFile older = *again;  // saved before the physical values existed: they are added when it is opened
    grt::writePhysicalBurn(older.propellant, std::nullopt);
    grt::applyPropellant(older, powder);
    grt::writePhysicalBurn(older.propellant, std::nullopt);
    QVERIFY(!grt::SimInput::fromLoad(older).physicalBurn);
    QVERIFY(grt::attachPhysicalBurn(older.propellant));
    QVERIFY(grt::SimInput::fromLoad(older).physicalBurn);
    QVERIFY(!grt::attachPhysicalBurn(older.propellant));  // already there
    grt::setPhysicalBurnFile(QStringLiteral(GRT_SOURCE_DIR "/data/physical/propellants.json"));
}

// A boat-tail or hollow-base bullet without tail dimensions gets typical ones, so the base type changes the result; given dimensions stay.
void CoreTest::tailEstimates()
{
    auto lf = grt::LoadFile::read(QStringLiteral(GRT_FIXTURE_DIR "/8x57is.grtload"));
    QVERIFY(lf);
    grt::EntryList &prj = lf->projectile;
    QVERIFY(!grt::completeTail(prj));  // boat tail with all dimensions: unchanged
    QCOMPARE(prj.number(u"gtailh"), 5.1);

    for (const QStringView key : {u"gtailh", u"gTailDiaA", u"gTailDiaB"}) prj.set(key, 0.0);
    prj.set(u"gTailType", 0.0);
    QVERIFY(!grt::completeTail(prj));  // flat base: nothing to fill
    const double flat = grt::simulate(grt::SimInput::fromLoad(*lf)).maxPressure_bar;

    prj.set(u"gTailType", 2.0);
    QVERIFY(grt::completeTail(prj));
    const double D = prj.number(u"Dbul");
    QVERIFY(qAbs(prj.number(u"gtailh") - 0.53 * D) < 0.01);
    QVERIFY(qAbs(prj.number(u"gTailDiaB") - 0.835 * D) < 0.01);
    QVERIFY(grt::simulate(grt::SimInput::fromLoad(*lf)).maxPressure_bar < flat);  // more powder space

    prj.set(u"Dbul", 2.0 * D);  // a new diameter: the estimates follow it, entered values would stay
    QVERIFY(grt::completeTail(prj, D));
    QVERIFY(qAbs(prj.number(u"gtailh") - 0.53 * 2.0 * D) < 0.01);
    prj.set(u"gtailh", 7.0);
    QVERIFY(!grt::completeTail(prj, 2.0 * D));
    prj.set(u"Dbul", D);
    QVERIFY(!grt::completeTail(prj, 2.0 * D));  // the length was entered: nothing is replaced
    QCOMPARE(prj.number(u"gtailh"), 7.0);

    prj.set(u"gTailDiaB", 0.0);  // a boat tail ending in a point is a missing value
    QVERIFY(grt::completeTail(prj));
    QVERIFY(prj.number(u"gTailDiaB") > 0.0);

    for (const QStringView key : {u"gtailh", u"gTailDiaA", u"gTailDiaB"}) prj.set(key, 0.0);
    prj.set(u"gTailType", 1.0);
    QVERIFY(grt::completeTail(prj));
    QVERIFY(qAbs(prj.number(u"gtailh") - 0.055 * D) < 0.01);
    QVERIFY(grt::simulate(grt::SimInput::fromLoad(*lf)).maxPressure_bar < flat);  // the cavity adds volume
}

// Trimming the case: the bullet stays where it is (overall length kept), the seating depth and the case volume (neck inside) follow;
// the powder space behind the bullet stays, so the result hardly changes.
void CoreTest::caseLengthChange()
{
    auto lf = grt::LoadFile::read(QStringLiteral(GRT_FIXTURE_DIR "/8x57is.grtload"));
    QVERIFY(lf);
    const grt::SimInput before = grt::SimInput::fromLoad(*lf);
    const double oal = lf->caliber.number(u"oal"), depth = lf->projectile.number(u"gdepth"), volume = lf->caliber.number(u"casevol");
    lf->caliber.set(u"caselen", before.caseLength_mm - 1.0);
    QVERIFY(grt::followCaseLength(lf->caliber, lf->projectile, before.caseLength_mm));
    const grt::SimInput after = grt::SimInput::fromLoad(*lf);
    QCOMPARE(lf->caliber.number(u"oal"), oal);
    QVERIFY(qAbs(lf->projectile.number(u"gdepth") - (depth - 1.0)) < 1e-9);
    const double area = std::numbers::pi / 4.0 * before.bulletDiameter_mm * before.bulletDiameter_mm;
    QVERIFY(qAbs(lf->caliber.number(u"casevol") - (volume - area * 1e-3)) < 1e-9);
    QVERIFY(qAbs(after.startPosition_mm() - before.startPosition_mm()) < 1e-9);  // the bullet base did not move
    for (const auto profile : {grt::CalibrationProfile::Original, grt::CalibrationProfile::Physical})
    {
        const grt::Calibration cal = grt::calibrationFor(profile);
        QVERIFY(qAbs(grt::simulate(after, cal).maxPressure_bar / grt::simulate(before, cal).maxPressure_bar - 1.0) < 0.01);
    }
    QVERIFY(!grt::followCaseLength(lf->caliber, lf->projectile, after.caseLength_mm));  // no change, nothing to do
}

// The original's Ba*phi(z) curve has no jumps (documentation, "Form functions"): the three phases join at z1 and z2
// and the curve drops towards z = 1.
void CoreTest::burnRateCurveIsContinuous()
{
    for (const char *file : {"/8x57is.grtload", "/measurements.grtload"})
    {
        const auto lf = grt::LoadFile::read(QStringLiteral(GRT_FIXTURE_DIR) + QLatin1String(file));
        QVERIFY(lf);
        const grt::SimInput in = grt::SimInput::fromLoad(*lf);
        const std::vector<double> curve = grt::vivacityCurve(in, {}, 2001);
        QCOMPARE(curve.size(), std::size_t{2001});
        const double start = curve.front();
        QVERIFY(start > 0.0);
        double biggestStep = 0.0;
        for (std::size_t i = 1; i < curve.size(); ++i) biggestStep = std::max(biggestStep, std::abs(curve[i] - curve[i - 1]));
        QVERIFY2(biggestStep < 0.01 * start, file);  // a jump of 1 % of the start value within 0.05 % of z
        QVERIFY(curve.back() > 0.0);
        QVERIFY(curve.back() < 0.5 * start);  // it burns out slowly at the end
        // the curve peaks within the first phases and then falls (apart from a tiny bend where phase 3 starts)
        const auto peak = std::ranges::max_element(curve);
        for (auto it = peak + 1; it != curve.end(); ++it) QVERIFY2(*it <= *(it - 1) + 0.005 * start, file);
    }
}

// Measured on the original (batch 10): a bullet jump (also with the field locked) lowers pressure and velocity, the 8x57
// base load by 2.5 % / 0.5 % at 1 mm and 9.4 % / 2.1 % at 4 mm.
void CoreTest::bulletJumpLowersThePressure()
{
    const auto lf = grt::LoadFile::read(QStringLiteral(GRT_FIXTURE_DIR "/8x57is.grtload"));
    QVERIFY(lf);
    grt::SimInput in = grt::SimInput::fromLoad(*lf);
    QCOMPARE(in.bulletJump_mm, 0.0);
    const grt::SimResult base = grt::simulate(in);
    in.bulletJump_mm = 1.0;
    const grt::SimResult j1 = grt::simulate(in);
    in.bulletJump_mm = 4.0;
    const grt::SimResult j4 = grt::simulate(in);
    QVERIFY(base.ok() && j1.ok() && j4.ok());
    const auto rel = [](double a, double b) { return 100.0 * (a / b - 1.0); };
    QCOMPARE_LT(std::abs(rel(j1.maxPressure_bar, base.maxPressure_bar) + 2.5), 1.0);
    QCOMPARE_LT(std::abs(rel(j4.maxPressure_bar, base.maxPressure_bar) + 9.4), 2.0);
    QCOMPARE_LT(std::abs(rel(j4.muzzleVelocity_ms, base.muzzleVelocity_ms) + 2.1), 0.5);
    QCOMPARE(j4.effectiveCaseVolume_cm3, base.effectiveCaseVolume_cm3);  // the case and the charge rule do not change
}

void CoreTest::datesAreStoredAsIso()
{
    QCOMPARE(grt::normalizedDate(u"2021-10-08"_s), u"2021-10-08"_s);
    QCOMPARE(grt::normalizedDate(u"2021-10-08 22:56:13"_s), u"2021-10-08"_s);  // the original's image dates
    QCOMPARE(grt::normalizedDate(u"2021-1-8"_s), u"2021-01-08"_s);
    QCOMPARE(grt::normalizedDate(u"2021/10/08"_s), u"2021-10-08"_s);
    QCOMPARE(grt::normalizedDate(u"8.10.2021"_s), u"2021-10-08"_s);
    QCOMPARE(grt::normalizedDate(QDate(2026, 10, 2)), u"2026-10-02"_s);
    QCOMPARE(grt::normalizedDate(u""_s), QString());
    QCOMPARE(grt::normalizedDate(u"0000-00-00"_s), QString());  // "no date" in the original database
    QVERIFY(!grt::normalizedDate(u"2021-13-01"_s));
    QVERIFY(!grt::normalizedDate(u"31.02.2021"_s));
    QVERIFY(!grt::normalizedDate(u"yesterday"_s));

    QTemporaryDir dir;
    grt::Database db;
    QVERIFY(db.open({}, dir.filePath(u"own.db"_s)));
    QString error;
    const auto id =
        db.insert(grt::Table::Caliber, {{u"cipname"_s, u"Test"_s}, {u"cipdate"_s, u"14.6.1984"_s}, {u"ciprevdate"_s, u""_s}}, &error);
    QVERIFY2(id, qPrintable(error));
    auto rec = db.record(grt::Table::Caliber, *id);
    QCOMPARE(rec->value(u"cipdate"_s).toString(), u"1984-06-14"_s);
    QCOMPARE(rec->value(u"ciprevdate"_s).toString(), QString());
    QCOMPARE(rec->value(u"cdate"_s).toString(), QDate::currentDate().toString(Qt::ISODate));  // not given: today
    QVERIFY(!db.update(grt::Table::Caliber, *id, {{u"cipdate"_s, u"someday"_s}}, &error));
    QVERIFY(error.contains(u"yyyy-MM-dd"_s));
    QVERIFY(!db.insert(grt::Table::Caliber, {{u"cipname"_s, u"Bad"_s}, {u"cdate"_s, u"2021-02-30"_s}}));
    QVERIFY(db.update(grt::Table::Caliber, *id, {{u"ciprevdate"_s, u"2012/9/25"_s}}));
    QCOMPARE(db.record(grt::Table::Caliber, *id)->value(u"ciprevdate"_s).toString(), u"2012-09-25"_s);

    // project files: dates must already be in the form yyyy-MM-dd
    const auto schema = db.schema(grt::Table::Caliber);
    const auto problems = [&](const QString &date)
    { return grt::projectdata::check(grt::Table::Caliber, {{u"cipname"_s, u"X"_s}, {u"cipdate"_s, date}}, schema).join(u'|'); };
    QVERIFY(!problems(u"1984-06-14"_s).contains(u"cipdate"_s));
    QVERIFY(!problems(u""_s).contains(u"cipdate"_s));
    QVERIFY(problems(u"14.06.1984"_s).contains(u"cipdate"_s));
    QVERIFY(problems(u"1984-6-14"_s).contains(u"cipdate"_s));
}

// Shotshells have their own terms (bigger powder space, wad friction, energy); everything else is untouched.
void CoreTest::shotshellModel()
{
    const auto lf = grt::LoadFile::read(QStringLiteral(GRT_FIXTURE_DIR "/8x57is.grtload"));
    QVERIFY(lf);
    grt::SimInput in = grt::SimInput::fromLoad(*lf);
    QVERIFY(!in.shotshell);
    const grt::SimResult cartridge = grt::simulate(in);
    in.shotshell = true;
    const grt::SimResult shot = grt::simulate(in);
    QVERIFY(cartridge.ok() && shot.ok());
    QCOMPARE_LT(shot.maxPressure_bar, 0.8 * cartridge.maxPressure_bar);  // more powder space: much lower pressure
    QCOMPARE_LT(shot.muzzleVelocity_ms, cartridge.muzzleVelocity_ms);
    grt::Calibration neutral;  // the terms off: identical to a cartridge
    neutral.shotCapacityFactor = 1.0;
    neutral.shotFrictionShare = 0.0;
    neutral.shotEnergyFactor = 1.0;
    QCOMPARE(grt::simulate(in, neutral).maxPressure_bar, cartridge.maxPressure_bar);
}

void CoreTest::measurementStatistics()
{
    const std::array values{844.6, 850.1, 0.0, 848.0, 850.4};  // 0 = not measured
    const grt::ShotStatistics s = grt::statistics(values);
    QCOMPARE(s.count, 4);
    QCOMPARE_LT(std::abs(s.mean - 848.275), 1e-9);
    QCOMPARE_LT(std::abs(s.extremeSpread() - 5.8), 1e-9);
    QCOMPARE_LT(std::abs(s.sd - 2.67255), 1e-4);
    QCOMPARE(grt::statistics(std::array{0.0}).count, 0);
    QCOMPARE(grt::statistics(std::array{900.0}).sd, 0.0);
}

void CoreTest::simInputFromLoad()
{
    const auto lf = grt::LoadFile::read(QStringLiteral(GRT_FIXTURE_DIR "/8x57is.grtload"));
    QVERIFY(lf);
    const grt::SimInput in = grt::SimInput::fromLoad(*lf);
    QCOMPARE(in.startPosition_mm(), 46.6);  // 57 - 10.4
    QCOMPARE(in.effectiveMass_g(), 12.700586359951799 + 0.5 * 2.721554219989671);
    QCOMPARE(in.vivacityBa, 0.5814);
    QCOMPARE(in.startPressure_bar, 250.0);
}

// Measured on the original: the default coefficients make cold powder weaker and hot powder stronger; explicit
// coefficients act linearly with their own sign (a positive tcc raises the burn rate when cold).
void CoreTest::temperatureCoefficients()
{
    const auto lf = grt::LoadFile::read(QStringLiteral(GRT_FIXTURE_DIR "/8x57is.grtload"));
    QVERIFY(lf);
    const auto pmax = [&](double T, double tcc, double tch)
    {
        grt::SimInput in = grt::SimInput::fromLoad(*lf);
        in.temperature_C = T;
        in.coldCoeff = tcc;
        in.hotCoeff = tch;
        const grt::SimResult r = grt::simulate(in);
        return r.maxPressure_bar;
    };
    const double base = pmax(21, 0, 0);
    QVERIFY(pmax(-20, 0, 0) < base * 0.97);  // default: about -4 % when cold
    QVERIFY(pmax(60, 0, 0) > base * 1.05);   // and about +8 % when hot
    QVERIFY(pmax(-20, 0.0016, 0) > base);    // explicit positive tcc: slightly more pressure (the original does the same)
    QVERIFY(pmax(-20, -0.0476, 0) < base * 0.92);
    QVERIFY(pmax(60, 0, 0.0668) > base * 1.10);
    QCOMPARE_LT(std::abs(pmax(-20, 0.0016, 0) / base - 1.0), 0.01);  // ... but only slightly (original: +0.3 %)
    // as soon as one coefficient is set, a side with coefficient 0 has no effect at all (no fallback to the default)
    QCOMPARE(pmax(60, -0.0476, 0), base);
    QCOMPARE(pmax(-20, 0, 0.0668), base);
    QVERIFY(std::abs(pmax(-20, -0.0476, 0) / base - 1.0 + 0.098) < 0.01);  // original: -9.8 %
    QVERIFY(std::abs(pmax(60, 0, 0.0668) / base - 1.0 - 0.138) < 0.01);    // original: +13.8 %
}

// Measured on the original with fast powders (reference batch exp_ba*): the energy of the burnt powder reaches the
// gas over the first position steps, so the peak saturates with Ba and falls with the step length (barrel length).
// The covolume grows for powders of low material density; eta has no effect.
void CoreTest::fastPowdersAndCovolume()
{
    const auto lf = grt::LoadFile::read(QStringLiteral(GRT_FIXTURE_DIR "/8x57is.grtload"));
    QVERIFY(lf);
    const auto pmax = [&](double ba, double xe, double pc)
    {
        grt::SimInput in = grt::SimInput::fromLoad(*lf);
        in.vivacityBa = ba;
        in.barrelLength_mm = xe;
        in.density_kgm3 = pc;
        return grt::simulate(in).maxPressure_bar;
    };
    const double pc = grt::SimInput::fromLoad(*lf).density_kgm3;
    const double xe = grt::SimInput::fromLoad(*lf).barrelLength_mm;
    QCOMPARE_LT(std::abs(pmax(7.0, xe, pc) / pmax(3.5, xe, pc) - 1.0), 0.03);  // original: +0.3 %
    QCOMPARE_LT(pmax(3.0, 2400, pc), 0.85 * pmax(3.0, 150, pc));               // original: -24 %
    QCOMPARE_GT(pmax(1.2, xe, 980.0), 1.05 * pmax(1.2, xe, 1600.0));
    // the original ignores the covolume factor eta (a load with eta 1.2 gives the identical curve)
    grt::SimInput eta = grt::SimInput::fromLoad(*lf);
    eta.covolumeEta = 1.2;
    QCOMPARE(grt::simulate(eta).maxPressure_bar, grt::simulate(grt::SimInput::fromLoad(*lf)).maxPressure_bar);
}

void CoreTest::chargeTooHighIsRejected()
{
    auto lf = grt::LoadFile::read(QStringLiteral(GRT_FIXTURE_DIR "/8x57is.grtload"));
    QVERIFY(lf);
    lf->propellant.set(u"mc", 4.35);  // 1.06 g/cm3 > bulk density 0.89 g/cm3 (the original reports invalidData)
    const grt::SimResult r = grt::simulate(grt::SimInput::fromLoad(*lf));
    QCOMPARE(r.status, grt::SimStatus::ChargeTooHigh);
    QVERIFY(r.points.empty());
}

// The original's implicit momentum rule: 0.5*m'*(v1^2 - v0^2) = A*dx*p1, exactly, for every step.
void CoreTest::solverMatchesMomentumRule()
{
    const auto lf = grt::LoadFile::read(QStringLiteral(GRT_FIXTURE_DIR "/8x57is.grtload"));
    QVERIFY(lf);
    const grt::SimInput in = grt::SimInput::fromLoad(*lf);
    const grt::SimResult r = grt::simulate(in);
    QVERIFY(r.ok());
    QCOMPARE(r.points.size(), std::size_t{2000});
    QCOMPARE(r.points.front().x_mm, 46.6);
    QCOMPARE(r.points.front().p_bar, 250.0);
    const double dx = (r.points[1].x_mm - r.points[0].x_mm) * 1e-3;
    QVERIFY(std::abs(dx - (600.0 - 46.6) / 2000 * 1e-3) < 1e-12);
    const double m = in.effectiveMass_g() * 1e-3;
    const double A = in.boreArea_mm2 * 1e-6;
    for (std::size_t i = 1; i < r.points.size(); ++i)
    {
        const double dK = 0.5 * m * (std::pow(r.points[i].v_ms, 2) - std::pow(r.points[i - 1].v_ms, 2));
        const double W = A * dx * r.points[i].p_bar * 1e5;
        QVERIFY2(std::abs(dK / W - 1.0) < 1e-6, qPrintable(u"step %1"_s.arg(i)));
    }
    // sanity: monotonic travel, bounded burn fraction
    QVERIFY(std::ranges::is_sorted(r.points, {}, &grt::SimPoint::x_mm));
    QVERIFY(std::ranges::all_of(r.points, [](const grt::SimPoint &p) { return p.z >= 0.0 && p.z <= 1.0; }));
}

void CoreTest::derivedOutputsMatchReference_data()
{
    QTest::addColumn<QString>("name");
    for (const char *n : {"base", "cal_04", "cal_12", "cal_20", "cal_29", "cal_35", "pw_562", "pw_198", "sebert_1.0", "xe_400"})
        QTest::newRow(n) << QString::fromLatin1(n);
}

// Outputs derived from the simulation (friction, IPSC, impulses, recoil) against the original's own values.
void CoreTest::derivedOutputsMatchReference()
{
    QFETCH(QString, name);
    const QString src = QStringLiteral(GRT_SOURCE_DIR);
    const auto ref = grt::readReferenceRun(u"%1/reference/results/exp_%2.json"_s.arg(src, name));
    const auto lf = grt::LoadFile::read(u"%1/tools/GRTRef/jobs_all/exp_%2.grtload"_s.arg(src, name));
    if (!ref || !lf) QSKIP("reference data not available");
    const grt::SimResult r = grt::simulate(grt::SimInput::fromLoad(*lf));
    QVERIFY(r.ok());
    const auto rel = [](double mine, double theirs) { return std::abs(mine / theirs - 1.0); };

    // exact relations: they only depend on the muzzle velocity, which the solver gets within ~2 %
    QVERIFY(rel(r.ipscFactor, ref->number(u"IPSCFaktor"_s)) < 0.04);
    QVERIFY(rel(r.mipFactor / r.ipscFactor, ref->number(u"MIPFaktor"_s) / ref->number(u"IPSCFaktor"_s)) < 1e-3);
    QVERIFY(rel(r.projectileImpulse_Ns, ref->number(u"RecoilImpulseProjectile"_s)) < 0.04);
    QVERIFY(rel(r.muzzleFrictionEnergy_J, ref->number(u"MuzzleEfriction"_s)) < 0.02);  // purely geometric
    QVERIFY(rel(r.recoilImpulse_Ns, ref->number(u"RecoilImpulse"_s)) < 0.06);
    // depends on the burnt fraction at the muzzle, which the solver still overestimates for partly burnt loads
    QVERIFY(rel(r.propellantEnergy_J, ref->number(u"Eprop"_s)) < 0.15);
}

// Values shown on the original's main screen for its 8x57 IS / N130 load (screenshot main.png):
// case volume 63.30 grain H2O, seating depth 10.90 mm, charge 34.00 grain, bulk density 870 kg/m3,
// twist 240 mm  ->  eff. case volume 3.5688 cm3, load ratio 71.0 %, rotation 165.3k/min at 661.2 m/s.
void CoreTest::ratiosMatchTheOriginalsScreen()
{
    auto lf = grt::LoadFile::read(QStringLiteral(GRT_FIXTURE_DIR "/8x57is.grtload"));
    QVERIFY(lf);
    lf->caliber.set(u"casevol", 63.30 * grt::kGrainToGram);
    lf->projectile.set(u"gdepth", 10.9);
    lf->propellant.set(u"mc", 34.0 * grt::kGrainToGram);
    lf->propellant.set(u"pcd", 870.0);
    const grt::SimResult r = grt::simulate(grt::SimInput::fromLoad(*lf));
    QVERIFY(r.ok());
    QVERIFY(std::abs(r.effectiveCaseVolume_cm3 - 3.5688) < 0.001);
    QVERIFY(std::abs(r.loadRatio - 0.710) < 0.001);
    QVERIFY(std::abs(r.muzzleRpm - r.muzzleVelocity_ms / 0.240 * 60.0) < 1e-6);
    QVERIFY(r.indexZ1 > 0 && r.indexZ1 < r.indexZ2);
    QVERIFY(r.indexEnergy95 > r.indexPmax);
    const std::vector<double> viv = grt::vivacityCurve(grt::SimInput::fromLoad(*lf));
    QCOMPARE(viv.size(), std::size_t{101});
    QVERIFY(std::ranges::all_of(viv, [](double v) { return v > 0.0; }));
}

// Flat base: the whole seated cylinder displaces volume. Boat tail: the cone is missing from it, so more
// volume is left. Hollow base: the cavity is extra volume. Diameters A and B define the cone.
void CoreTest::bulletBaseTypesChangeTheChamberVolume()
{
    auto lf = grt::LoadFile::read(QStringLiteral(GRT_FIXTURE_DIR "/8x57is.grtload"));
    QVERIFY(lf);
    const auto effective = [&](int type)
    {
        lf->projectile.set(u"gTailType", static_cast<double>(type));
        return grt::simulate(grt::SimInput::fromLoad(*lf)).effectiveCaseVolume_cm3;
    };
    const double flat = effective(0), hollow = effective(1), boat = effective(2);
    const double dBul = lf->projectile.number(u"Dbul"), depth = lf->projectile.number(u"gdepth");
    QVERIFY(std::abs(flat - (lf->caliber.number(u"casevol") - M_PI / 4 * dBul * dBul * depth * 1e-3)) < 1e-6);
    // cone between A = 8.2 mm and B = 6.8 mm over 5.1 mm
    const double R = 4.1, r = 3.4, h = 5.1;
    const double cone = M_PI * h / 3.0 * (R * R + R * r + r * r) * 1e-3;  // cm3
    const double cylinder = M_PI * R * R * h * 1e-3;
    QVERIFY(std::abs(hollow - (flat + cone)) < 1e-6);
    QVERIFY(std::abs(boat - (flat + cylinder - cone)) < 1e-6);
    QVERIFY(boat > flat && hollow > flat);
    // the order of A and B does not matter
    const QString a = lf->projectile.text(u"gTailDiaA"), b = lf->projectile.text(u"gTailDiaB");
    lf->projectile.set(u"gTailDiaA", b);
    lf->projectile.set(u"gTailDiaB", a);
    QVERIFY(std::abs(effective(1) - hollow) < 1e-6);
}

void CoreTest::ladderStepsDescendFromTheSelectedCharge()
{
    auto lf = grt::LoadFile::read(QStringLiteral(GRT_FIXTURE_DIR "/8x57is.grtload"));
    QVERIFY(lf);
    const double mc = lf->propellant.number(u"mc");
    lf->propellant.set(u"laddercnt", 4.0);
    lf->propellant.set(u"laddermc", 0.2);
    const auto ladder = grt::simulateLadder(grt::SimInput::fromLoad(*lf));
    QCOMPARE(ladder.size(), std::size_t{4});
    for (std::size_t i = 0; i < ladder.size(); ++i)
    {
        QVERIFY(ladder[i].result.ok());
        QVERIFY(std::abs(ladder[i].charge_g - (mc - 0.2 * static_cast<double>(i))) < 1e-12);
    }
    // less powder, less pressure and less speed
    for (std::size_t i = 1; i < ladder.size(); ++i)
    {
        QVERIFY(ladder[i].result.maxPressure_bar < ladder[i - 1].result.maxPressure_bar);
        QVERIFY(ladder[i].result.muzzleVelocity_ms < ladder[i - 1].result.muzzleVelocity_ms);
    }
    // step 0 is exactly the single simulation of the selected charge
    const grt::SimResult single = grt::simulate(grt::SimInput::fromLoad(*lf));
    QCOMPARE(ladder[0].result.maxPressure_bar, single.maxPressure_bar);
}

void CoreTest::ladderEdgeCases()
{
    auto lf = grt::LoadFile::read(QStringLiteral(GRT_FIXTURE_DIR "/8x57is.grtload"));
    QVERIFY(lf);
    const double mc = lf->propellant.number(u"mc");
    // 1 (or 0) means "no ladder": a single curve
    lf->propellant.set(u"laddercnt", 1.0);
    lf->propellant.set(u"laddermc", 0.2);
    QCOMPARE(grt::simulateLadder(grt::SimInput::fromLoad(*lf)).size(), std::size_t{1});
    // no step size: nothing to step
    lf->propellant.set(u"laddercnt", 5.0);
    lf->propellant.set(u"laddermc", 0.0);
    QCOMPARE(grt::simulateLadder(grt::SimInput::fromLoad(*lf)).size(), std::size_t{1});
    // more than 10 steps are clamped
    lf->propellant.set(u"laddercnt", 25.0);
    lf->propellant.set(u"laddermc", 0.01);
    QCOMPARE(grt::simulateLadder(grt::SimInput::fromLoad(*lf)).size(), std::size_t{grt::kMaxLadderSteps});
    // steps that would leave no powder are dropped
    lf->propellant.set(u"laddercnt", 10.0);
    lf->propellant.set(u"laddermc", mc / 3.5);
    const auto ladder = grt::simulateLadder(grt::SimInput::fromLoad(*lf));
    QCOMPARE(ladder.size(), std::size_t{4});  // mc, 2.5/3.5 mc, 1.5/3.5 mc, 0.5/3.5 mc
    QVERIFY(ladder.back().charge_g > 0.0);
    // counts out of the int range (or not a number) must not be undefined behaviour
    lf->propellant.set(u"laddercnt", u"1e300"_s);
    QCOMPARE(grt::simulateLadder(grt::SimInput::fromLoad(*lf)).size(), std::size_t{4});
    lf->propellant.set(u"laddercnt", u"-1e300"_s);
    QCOMPARE(grt::simulateLadder(grt::SimInput::fromLoad(*lf)).size(), std::size_t{1});
}

// "nan" and "inf" are numbers for QString::toDouble: they must not reach the model.
void CoreTest::nonFiniteInputIsRejected()
{
    QVERIFY(!grt::parseFinite(u"nan"_s));
    QVERIFY(!grt::parseFinite(u"inf"_s));
    QVERIFY(!grt::parseFinite(u"-inf"_s));
    QCOMPARE(grt::parseFinite(u"1.5"_s).value_or(0.0), 1.5);
    QVERIFY(!grt::units::parseToBase(u"nan"_s, grt::units::unitsOf(grt::units::Quantity::Length).front()));
    QVERIFY(!grt::units::parseToBase(u"inf"_s, grt::units::unitsOf(grt::units::Quantity::Length).front()));
    QCOMPARE(grt::toIntClamped(std::nan(""), 0, 2), 0);
    QCOMPARE(grt::toIntClamped(1e300, 0, 2), 2);
    QCOMPARE(grt::toIntClamped(-1e300, 0, 2), 0);
    QCOMPARE(grt::toIntClamped(1.7, 0, 2), 1);

    auto lf = grt::LoadFile::read(QStringLiteral(GRT_FIXTURE_DIR "/8x57is.grtload"));
    QVERIFY(lf);
    const double z1 = lf->propellant.number(u"z1");
    lf->propellant.set(u"z1", u"nan"_s);
    lf->propellant.set(u"gTailType", u"nan"_s);
    QCOMPARE(lf->propellant.number(u"z1", -1.0), -1.0);  // the fallback, not NaN
    const grt::SimInput in = grt::SimInput::fromLoad(*lf);
    QCOMPARE(in.z1, 0.0);
    QVERIFY(z1 > 0.0);

    // a non-finite value given to the solver directly gives no result instead of NaN curves
    grt::SimInput bad = grt::SimInput::fromLoad(*grt::LoadFile::read(QStringLiteral(GRT_FIXTURE_DIR "/8x57is.grtload")));
    QVERIFY(grt::simulate(bad).ok());
    bad.a0 = std::numeric_limits<double>::quiet_NaN();
    QVERIFY(!grt::simulate(bad).ok());
    bad.a0 = 0.5;
    bad.caseVolume_cm3 = std::numeric_limits<double>::infinity();
    QVERIFY(!grt::simulate(bad).ok());
}

// Every load the original rejected as "charge too high" is rejected here, and no accepted load is.
void CoreTest::chargeRuleMatchesTheOriginal()
{
    const QDir results(QStringLiteral(GRT_SOURCE_DIR "/reference/results"));
    const QDir jobs(QStringLiteral(GRT_SOURCE_DIR "/tools/GRTRef/jobs_all"));
    int rejected = 0, accepted = 0;
    for (const QFileInfo &fi : results.entryInfoList({u"exp_*.json"_s}, QDir::Files))
    {
        const QString name = fi.completeBaseName().mid(4);
        const auto ref = grt::readReferenceRun(fi.absoluteFilePath());
        const auto lf = grt::LoadFile::read(jobs.filePath(u"exp_%1.grtload"_s.arg(name)));
        if (!ref || !lf || ref->stale) continue;
        const bool tooHigh = ref->summary.value(u"PropellantChargeTooHigh"_s).toString() == u"true"_s;
        if (ref->points.empty() && !tooHigh) continue;  // e.g. the first job of a batch, read too early
        const grt::SimResult r = grt::simulate(grt::SimInput::fromLoad(*lf));
        if (tooHigh)
        {
            QVERIFY2(r.status == grt::SimStatus::ChargeTooHigh, qPrintable(name));
            ++rejected;
        }
        else
        {
            QVERIFY2(r.ok(), qPrintable(name));
            ++accepted;
        }
    }
    if (rejected + accepted == 0) QSKIP("reference data not available");
    QVERIFY(rejected >= 5);
    QVERIFY(accepted >= 50);
}

void CoreTest::solverVersusReference_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<double>("pTol");  // relative tolerance on max pressure
    QTest::addColumn<double>("vTol");  // relative tolerance on muzzle velocity
    // Provisional model: tolerances are loose and meant to be tightened as the solver improves.
    for (const char *n : {"base", "mc_x0.8", "mc_x1.2", "mp_x0.5", "mp_x2.0", "sebert_0.0", "sebert_1.0", "xe_400", "xe_1000", "pw_562",
                          "pw_599", "pw_675"})
        QTest::newRow(n) << QString::fromLatin1(n) << 0.10 << 0.035;
    // other cartridges (case volumes 1.2 .. 13.7 cm3, bore areas 24 .. 130 mm2)
    for (const char *n : {"cal_04", "cal_08", "cal_12", "cal_16", "cal_20", "cal_24", "cal_29", "cal_31", "cal_35"})
        QTest::newRow(n) << QString::fromLatin1(n) << 0.15 << 0.05;
}

void CoreTest::solverVersusReference()
{
    QFETCH(QString, name);
    QFETCH(double, pTol);
    QFETCH(double, vTol);
    const QString src = QStringLiteral(GRT_SOURCE_DIR);
    const auto ref = grt::readReferenceRun(u"%1/reference/results/exp_%2.json"_s.arg(src, name));
    const auto lf = grt::LoadFile::read(u"%1/tools/GRTRef/jobs_all/exp_%2.grtload"_s.arg(src, name));
    if (!ref || !lf) QSKIP("reference data not available");

    const grt::SimResult r = grt::simulate(grt::SimInput::fromLoad(*lf));
    QVERIFY(r.ok());
    const double refP = ref->number(u"MaxPressure"_s), refV = ref->number(u"MuzzleVelocity"_s);
    const double pErr = r.maxPressure_bar / refP - 1.0;
    const double vErr = r.muzzleVelocity_ms / refV - 1.0;
    qInfo().noquote() << QStringLiteral("%1: pmax %2 vs %3 bar (%4%)  vmuz %5 vs %6 m/s (%7%)")
                             .arg(name)
                             .arg(r.maxPressure_bar, 0, 'f', 0)
                             .arg(refP, 0, 'f', 0)
                             .arg(100 * pErr, 0, 'f', 1)
                             .arg(r.muzzleVelocity_ms, 0, 'f', 1)
                             .arg(refV, 0, 'f', 1)
                             .arg(100 * vErr, 0, 'f', 1);
    QVERIFY(std::abs(pErr) < pTol);
    QVERIFY(std::abs(vErr) < vTol);
}

void CoreTest::databaseOpensPlainFile()
{
    const QString dbPath = QStringLiteral(GRT_SOURCE_DIR "/tools/GordonsReloadingTool.plain.db");
    if (!QFile::exists(dbPath)) QSKIP("plain database not available");
    grt::Database db;
    QString err;
    QTemporaryDir own;
    QVERIFY2(db.open(dbPath, own.filePath(u"own.db"_s), &err), qPrintable(err));
    QVERIFY(db.hasOriginal());
    QCOMPARE(db.path(), dbPath);
    QVERIFY(db.count(grt::Table::Propellant) > 600);
    QVERIFY(db.count(grt::Table::Projectile) > 5000);
    QVERIFY(db.count(grt::Table::Caliber) > 500);
    const auto rec = db.record(grt::Table::Propellant, 1);
    QVERIFY(rec);
    QCOMPARE(rec->value(u"mname"_s).toString(), u"Accurate"_s);
    QVERIFY(!db.record(grt::Table::Propellant, -5));
}

// New records go into the own database; searching sees one database with the original's records and the own ones.
void CoreTest::ownDatabaseTakesNewRecords()
{
    const QString dbPath = QStringLiteral(GRT_SOURCE_DIR "/tools/GordonsReloadingTool.plain.db");
    if (!QFile::exists(dbPath)) QSKIP("plain database not available");
    QTemporaryDir dir;
    const QString ownPath = dir.filePath(u"sub/own.db"_s);
    grt::Database db;
    QString err;
    QVERIFY2(db.open(dbPath, ownPath, &err), qPrintable(err));
    QVERIFY(QFile::exists(ownPath));
    const int before = db.count(grt::Table::Propellant);
    QCOMPARE(db.ownCount(grt::Table::Propellant), 0);

    const auto id = db.insert(grt::Table::Propellant, {{u"mname"_s, u"Own"_s}, {u"pname"_s, u"Test powder"_s}, {u"Ba"_s, 0.61}}, &err);
    QVERIFY2(id, qPrintable(err));
    QVERIFY(grt::Database::isOwnId(*id));
    QCOMPARE(*id, grt::Database::kOwnIdBase);
    QCOMPARE(db.count(grt::Table::Propellant), before + 1);
    QCOMPARE(db.ownCount(grt::Table::Propellant), 1);
    auto rec = db.record(grt::Table::Propellant, *id);
    QVERIFY(rec);
    QCOMPARE(rec->value(u"pname"_s).toString(), u"Test powder"_s);
    QCOMPARE(rec->value(u"Ba"_s).toDouble(), 0.61);
    QCOMPARE(rec->value(u"geloescht"_s).toInt(), 0);
    QVERIFY(!rec->value(u"cdate"_s).toString().isEmpty());
    QCOMPARE(db.columns(grt::Table::Propellant).size() + 1, rec->size());  // + "source"
    QCOMPARE(rec->value(u"source"_s).toString(), u"own"_s);
    QCOMPARE(db.record(grt::Table::Propellant, 1)->value(u"source"_s).toString(), u"original"_s);

    // a query over the table sees both databases
    QSqlQuery q(db.handle());
    QVERIFY(q.exec(u"SELECT count(*) FROM propellant WHERE mname IN ('Own', 'Accurate')"_s) && q.next());
    QVERIFY(q.value(0).toInt() > 1);

    QVERIFY(db.update(grt::Table::Propellant, *id, {{u"Ba"_s, 0.7}}, &err));
    QCOMPARE(db.record(grt::Table::Propellant, *id)->value(u"Ba"_s).toDouble(), 0.7);
    // the original stays untouched
    QVERIFY(!db.update(grt::Table::Propellant, 1, {{u"Ba"_s, 9.0}}));
    QVERIFY(!QSqlQuery(db.handle()).exec(u"UPDATE grt.propellant SET Ba = 9 WHERE id = 1"_s));
    QVERIFY(db.record(grt::Table::Propellant, 1)->value(u"Ba"_s).toDouble() < 9.0);

    // ids keep counting in the own range, and the records survive reopening
    const auto second = db.insert(grt::Table::Propellant, {{u"pname"_s, u"Second"_s}});
    QCOMPARE(*second, grt::Database::kOwnIdBase + 1);
    QVERIFY(db.remove(grt::Table::Propellant, *second));
    QCOMPARE(db.count(grt::Table::Propellant), before + 1);
    db.close();
    QVERIFY(db.open(dbPath, ownPath));
    QCOMPARE(db.record(grt::Table::Propellant, *id)->value(u"pname"_s).toString(), u"Test powder"_s);
    QCOMPARE(db.count(grt::Table::Propellant), before + 1);
}

void CoreTest::ownDatabaseWorksWithoutTheOriginal()
{
    QTemporaryDir dir;
    grt::Database db;
    QVERIFY(db.open({}, dir.filePath(u"own.db"_s)));
    QVERIFY(db.isOpen());
    QVERIFY(!db.hasOriginal());
    QCOMPARE(db.count(grt::Table::Caliber), 0);
    QVERIFY(db.columns(grt::Table::Caliber).contains(u"cipname"_s));
    QVERIFY(db.insert(grt::Table::Caliber, {{u"cipname"_s, u"Own caliber"_s}}));
    QCOMPARE(db.count(grt::Table::Caliber), 1);

    // an unusable original (missing, or encrypted = not a database) is reported, the own database stays usable
    QString err;
    QVERIFY(!db.open(dir.filePath(u"missing.db"_s), dir.filePath(u"own.db"_s), &err));
    QVERIFY(!err.isEmpty());
    QVERIFY(db.isOpen());
    QCOMPARE(db.count(grt::Table::Caliber), 1);
    QFile junk(dir.filePath(u"encrypted.db"_s));
    QVERIFY(junk.open(QIODevice::WriteOnly));
    junk.write(QByteArray(4096, '\x5a'));
    junk.close();
    QVERIFY(!db.open(junk.fileName(), dir.filePath(u"own.db"_s), &err));
    QVERIFY(db.isOpen());
    QVERIFY(!db.hasOriginal());
    QCOMPARE(db.count(grt::Table::Caliber), 1);
}

// The columns are NOT NULL: missing values, null strings and NaN become the column's empty value.
void CoreTest::ownDatabaseNeverStoresNull()
{
    QTemporaryDir dir;
    grt::Database db;
    QVERIFY(db.open({}, dir.filePath(u"own.db"_s)));
    QString err;
    const auto id = db.insert(
        grt::Table::Propellant,
        {{u"pname"_s, u"Test"_s}, {u"Ba"_s, std::numeric_limits<double>::quiet_NaN()}, {u"mname"_s, QVariant()}, {u"lotid"_s, QString()}},
        &err);
    QVERIFY2(id, qPrintable(err));
    auto r = db.record(grt::Table::Propellant, *id);
    QVERIFY(r);
    QCOMPARE(r->value(u"Ba"_s).toDouble(), 0.0);
    QCOMPARE(r->value(u"mname"_s).toString(), QString());
    QVERIFY2(db.update(grt::Table::Propellant, *id,
                       {{u"Qex"_s, std::numeric_limits<double>::infinity()}, {u"mname"_s, QVariant()}, {u"descr"_s, QString()}}, &err),
             qPrintable(err));
    r = db.record(grt::Table::Propellant, *id);
    QCOMPARE(r->value(u"Qex"_s).toDouble(), 0.0);
    QCOMPARE(r->value(u"pname"_s).toString(), u"Test"_s);
}

// The project's records (JSON files in data/) appear in the one database, read-only, in their own id range.
void CoreTest::projectRecordsArePartOfTheDatabase()
{
    QTemporaryDir dir;
    QDir().mkpath(dir.filePath(u"data/propellants"_s));
    QDir().mkpath(dir.filePath(u"data/calibers"_s));
    QDir().mkpath(dir.filePath(u"data/projectiles"_s));
    const auto write = [&](const QString &name, const QByteArray &text)
    {
        QFile f(dir.filePath(u"data/"_s + name));
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(text);
    };
    write(u"propellants/acme-p1.json"_s, R"({"mname": "Acme", "pname": "P1", "Ba": 0.6, "Qex": 3900, "k": 1.22, "a0": 1.1,
        "z1": 0.45, "z2": 0.8, "pc": 1600, "pcd": 900, "pt": 21})");
    write(u"propellants/broken.json"_s, R"({"mname": "Acme", "pname": "P2", "Ba": "fast", "Qex": 3900})");
    write(u"propellants/not-json.json"_s, "this is not json");
    write(u"calibers/unknown-column.json"_s, R"({"cipname": "X", "L3": 50, "V": 60, "colour": "red"})");
    // projectiles: one file per manufacturer with an array of records; a bad record does not cost the others
    write(u"projectiles/acme.json"_s, R"([{"mname": "Acme", "pname": "B1", "gdia": 7.82, "glen": 30, "gmass": 150},
        {"mname": "Acme", "pname": "B2", "gdia": 7.82, "glen": "long", "gmass": 168},
        {"mname": "Acme", "pname": "B3", "gdia": 7.82, "glen": 33, "gmass": 180}])");

    grt::Database::setProjectDataLocation(dir.filePath(u"data"_s));
    grt::Database db;
    QString err;
    QVERIFY2(db.open({}, dir.filePath(u"own.db"_s), &err), qPrintable(err));
    QCOMPARE(db.projectCount(grt::Table::Propellant), 1);
    QCOMPARE(db.projectCount(grt::Table::Caliber), 0);
    QCOMPARE(db.projectCount(grt::Table::Projectile), 2);
    QCOMPARE(db.record(grt::Table::Projectile, grt::Database::kProjectIdBase + 1)->value(u"pname"_s).toString(), u"B3"_s);  // file order
    QCOMPARE(db.count(grt::Table::Propellant), 1);
    const auto rec = db.record(grt::Table::Propellant, grt::Database::kProjectIdBase);
    QVERIFY(rec);
    QVERIFY(grt::Database::isProjectId(grt::Database::kProjectIdBase));
    QVERIFY(!grt::Database::isOwnId(grt::Database::kProjectIdBase));
    QCOMPARE(rec->value(u"pname"_s).toString(), u"P1"_s);
    QCOMPARE(rec->value(u"Ba"_s).toDouble(), 0.6);
    QCOMPARE(rec->value(u"source"_s).toString(), u"project"_s);
    QCOMPARE(rec->value(u"lotid"_s).toString(), QString());  // not in the file: empty
    // the bad files are reported with their problem
    const QString errors = db.projectErrors().join(u'\n');
    QVERIFY2(errors.contains(u"propellants/broken.json: \"Ba\" must be a number"_s), qPrintable(errors));
    QVERIFY(errors.contains(u"propellants/not-json.json"_s));
    QVERIFY(errors.contains(u"calibers/unknown-column.json: unknown column \"colour\""_s));
    QVERIFY2(errors.contains(u"projectiles/acme.json #2 (B2): \"glen\" must be a number"_s), qPrintable(errors));
    // read-only; a copy becomes an own record
    QVERIFY(!db.update(grt::Table::Propellant, grt::Database::kProjectIdBase, {{u"Ba"_s, 0.7}}));
    QVERIFY(!db.remove(grt::Table::Propellant, grt::Database::kProjectIdBase));
    QCOMPARE(*db.insert(grt::Table::Propellant, *rec), grt::Database::kOwnIdBase);
    QCOMPARE(db.count(grt::Table::Propellant), 2);

    // an exported record is a valid project file again (round trip), with a readable name
    const auto schema = db.schema(grt::Table::Propellant);
    QVariantMap own = *db.record(grt::Table::Propellant, grt::Database::kOwnIdBase);
    own.insert(u"lotid"_s, u"Lot 7/2024"_s);
    const QByteArray json = grt::projectdata::toJson(own, schema);
    QVERIFY(!json.contains("\"id\""));
    QVERIFY(!json.contains("geloescht"));
    QVERIFY(json.indexOf("\"mname\"") < json.indexOf("\"Ba\""));  // schema order, one column per line
    QCOMPARE(grt::projectdata::fileName(grt::Table::Propellant, own), u"acme-p1-lot-7-2024.json"_s);
    write(u"propellants/"_s + grt::projectdata::fileName(grt::Table::Propellant, own), json);
    const grt::projectdata::Loaded again = grt::projectdata::load(dir.filePath(u"data"_s), grt::Table::Propellant, schema);
    QCOMPARE(again.records.size(), std::size_t{2});
    const auto exported = std::ranges::find_if(again.records, [](const QVariantMap &r) { return r.value(u"lotid"_s) == u"Lot 7/2024"_s; });
    QVERIFY(exported != again.records.end());
    QCOMPARE(exported->value(u"Ba"_s).toDouble(), 0.6);
    QCOMPARE(exported->value(u"pname"_s).toString(), u"P1"_s);

    // the model needs plausible values
    QVariantMap bad = *exported;
    bad.insert(u"z1"_s, 0.9);
    QVERIFY(grt::projectdata::check(grt::Table::Propellant, bad, schema).join(u' ').contains(u"z1 < z2"_s));
    grt::Database::setProjectDataLocation(u":/data"_s);
}

// Own records of the version before the project records had ids from 1,000,000 on: they move to the own range.
void CoreTest::ownRecordsOfOlderVersionsMoveUp()
{
    QTemporaryDir dir;
    const QString own = dir.filePath(u"own.db"_s);
    {
        grt::Database db;
        QVERIFY(db.open({}, own));
        QVERIFY(db.insert(grt::Table::Caliber, {{u"cipname"_s, u"Old"_s}}));
        QVERIFY(QSqlQuery(db.handle()).exec(u"UPDATE main.caliber SET id = 1000004"_s));
        QVERIFY(QSqlQuery(db.handle()).exec(u"UPDATE main.sqlite_sequence SET seq = 1000004 WHERE name = 'caliber'"_s));
    }
    grt::Database db;
    QVERIFY(db.open({}, own));
    QVERIFY(!db.record(grt::Table::Caliber, 1000004));
    const auto moved = db.record(grt::Table::Caliber, grt::Database::kOwnIdBase + 4);
    QVERIFY(moved);
    QCOMPARE(moved->value(u"cipname"_s).toString(), u"Old"_s);
    QCOMPARE(*db.insert(grt::Table::Caliber, {{u"cipname"_s, u"New"_s}}), grt::Database::kOwnIdBase + 5);
}

// Measurements made with a known burn rate: the fit must find it again from another start value.
void CoreTest::powderFitFindsTheBurnRate()
{
    auto lf = grt::LoadFile::read(QStringLiteral(GRT_FIXTURE_DIR "/8x57is.grtload"));
    QVERIFY(lf);
    const double trueBa = 0.65;
    grt::SimInput truth = grt::SimInput::fromLoad(*lf);
    truth.vivacityBa = trueBa;
    grt::Measurement m{.title = u"M"_s, .charges = {}, .attributes = {}};
    for (const double g : {2.4, 2.6, 2.75})
    {
        truth.charge_g = g;
        const grt::SimResult r = grt::simulate(truth);
        QVERIFY(r.ok());
        grt::MeasuredCharge c = grt::MeasuredCharge::create(u"c"_s, g);
        for (const double d : {-3.0, -1.0, 0.0, 1.0, 3.0})
            c.shots.push_back({.velocity_ms = r.muzzleVelocity_ms + d});  // scatter, mean exact
        m.charges.push_back(c);
    }
    lf->appendixXml = grt::writeMeasurement(lf->appendixXml, m);
    lf->propellant.set(u"Ba", 0.58);  // the start value

    QStringList warnings;
    const auto samples = grt::powderfit::samples(*lf, u"8x57"_s, &warnings);
    QCOMPARE(samples.size(), std::size_t{3});
    QCOMPARE(samples[0].shots, 5);
    QCOMPARE(grt::powderfit::defaultParams(samples).size(), std::size_t{2});  // Ba, a0 for three charges
    const auto res = grt::powderfit::fit(samples, {.params = {grt::powderfit::Param::Ba}, .pressureWeight = 1.0, .maxIterations = 60});
    QVERIFY(res.improved);
    QCOMPARE_LT(std::abs(res.after[0] / trueBa - 1.0), 0.002);
    QCOMPARE_LT(res.velocityRmsAfter, 0.001);
    QCOMPARE_GT(res.velocityRmsBefore, 0.01);

    // the fitted value goes into the load
    grt::powderfit::apply(lf->propellant, res);
    QCOMPARE_LT(std::abs(lf->propellant.number(u"Ba") - trueBa), 0.002);

    QString error;
    QCOMPARE(grt::powderfit::parse(u"Ba, z1"_s, &error).size(), std::size_t{2});
    QVERIFY(grt::powderfit::parse(u"Ba,colour"_s, &error).empty());
    QVERIFY(error.contains(u"colour"_s));
}

void CoreTest::languageResolution_data()
{
    QTest::addColumn<QStringList>("requested");
    QTest::addColumn<QStringList>("available");
    QTest::addColumn<QString>("expected");
    const QStringList none{u"en_US"_s};
    const QStringList de{u"en_US"_s, u"de_DE"_s, u"fr_FR"_s};
    QTest::newRow("no request -> default") << QStringList{} << de << u"en_US"_s;
    QTest::newRow("exact") << QStringList{u"fr_FR"_s} << de << u"fr_FR"_s;
    QTest::newRow("other country, same language") << QStringList{u"de_AT"_s} << de << u"de_DE"_s;
    QTest::newRow("bcp47 tag") << QStringList{u"de-CH"_s} << de << u"de_DE"_s;
    QTest::newRow("posix with encoding") << QStringList{u"fr_FR.UTF-8"_s} << de << u"fr_FR"_s;
    QTest::newRow("unknown language -> default") << QStringList{u"ja_JP"_s} << de << u"en_US"_s;
    QTest::newRow("english variants -> default") << QStringList{u"en-GB"_s} << de << u"en_US"_s;
    QTest::newRow("first usable wins") << QStringList{u"ja"_s, u"fr"_s, u"de"_s} << de << u"fr_FR"_s;
    QTest::newRow("english first stops the search") << QStringList{u"en_US"_s, u"de_DE"_s} << de << u"en_US"_s;
    QTest::newRow("nothing shipped") << QStringList{u"de_DE"_s} << none << u"en_US"_s;
}

void CoreTest::languageResolution()
{
    QFETCH(QStringList, requested);
    QFETCH(QStringList, available);
    QFETCH(QString, expected);
    QCOMPARE(grt::resolveLanguage(requested, available), expected);
}

void CoreTest::normalizeLanguageTags()
{
    QCOMPARE(grt::normalizeLanguage(u"de-at"_s), u"de_AT"_s);
    QCOMPARE(grt::normalizeLanguage(u"DE_de.UTF-8"_s), u"de_DE"_s);
    QCOMPARE(grt::normalizeLanguage(u"sr_RS@latin"_s), u"sr_RS"_s);
    QCOMPARE(grt::normalizeLanguage(u"fr"_s), u"fr"_s);
    QCOMPARE(grt::normalizeLanguage({}), QString());
}

namespace
{

// First non-deleted record of `table` matching the SQL condition.
std::optional<QVariantMap> findRecord(grt::Database &db, grt::Table t, const QString &where)
{
    QSqlQuery q(db.handle());
    if (!q.exec(u"SELECT id FROM %1 WHERE geloescht = 0 AND %2 ORDER BY id"_s.arg(QLatin1String(grt::tableName(t)), where)) || !q.next())
        return std::nullopt;
    return db.record(t, q.value(0).toInt());
}

}  // namespace

void CoreTest::loadBuilderMatchesOriginal()
{
    const QString dbPath = QStringLiteral(GRT_SOURCE_DIR "/tools/GordonsReloadingTool.plain.db");
    if (!QFile::exists(dbPath)) QSKIP("plain database not available");
    grt::Database db;
    QVERIFY(db.open(dbPath));
    const auto cal = findRecord(db, grt::Table::Caliber, u"cipname = '8 x 57 IS'"_s);
    const auto prj = findRecord(db, grt::Table::Projectile, u"mname = 'Sellier-Bellot' AND pname = 'FMJ-BT 2910' AND caliber = '.323'"_s);
    const auto pro = findRecord(db, grt::Table::Propellant, u"mname = 'Vihtavuori' AND pname = 'N150' AND lotid = '2021-11-13'"_s);
    QVERIFY(cal && prj && pro);

    // the same choices as in the sample load of the original program
    const grt::LoadFile built = grt::buildLoad(
        *cal, *prj, *pro,
        {.charge_g = 2.721554219989671, .barrelLength_mm = 600.0, .seatingDepth_mm = 10.4, .twistLength_mm = 240.0, .gunWeight_kg = 3.7},
        db.columns(grt::Table::Caliber));
    const auto orig = grt::LoadFile::read(QStringLiteral(GRT_FIXTURE_DIR "/8x57is.grtload"));
    QVERIFY(orig);

    // (the sample's bullet is spelled "FMJ-BT #2910", the database has "FMJ-BT 2910")
    QCOMPARE(QString(built.title).remove(u'#'), QString(orig->title).remove(u'#'));
    QCOMPARE(built.caliber.text(u"CaliberName"), orig->caliber.text(u"CaliberName"));
    for (const char16_t *k : {u"oal", u"caselen", u"Dz", u"Aeff", u"pMaxZul", u"sebert", u"ps"})
        QCOMPARE(built.caliber.number(k), orig->caliber.number(k));
    QCOMPARE(built.caliber.text(u"method"), orig->caliber.text(u"method"));
    // the original's case volume was entered by hand (4.1100 cm3); the database says 4.1017 cm3
    QVERIFY(std::abs(built.caliber.number(u"casevol") / orig->caliber.number(u"casevol") - 1.0) < 0.003);
    for (const char16_t *k : {u"xe", u"twistlen", u"mgun"}) QCOMPARE(built.gun.number(k), orig->gun.number(k));
    for (const char16_t *k : {u"Dbul", u"mp", u"gdepth", u"glen", u"gpressure", u"gtailh", u"gTailDiaA", u"gTailDiaB", u"gTailType"})
        QCOMPARE(built.projectile.number(k), orig->projectile.number(k));
    QCOMPARE(QString(built.projectile.text(u"ProjectileName")).remove(u'#'),
             QString(orig->projectile.text(u"ProjectileName")).remove(u'#'));
    for (const char16_t *k : {u"Ba", u"Qex", u"k", u"a0", u"z1", u"z2", u"pc", u"pcd", u"mc", u"laddermc"})
        QCOMPARE(built.propellant.number(k), orig->propellant.number(k));

    // same set and order of fields as the original program writes
    for (const auto sel : {&grt::LoadFile::caliber, &grt::LoadFile::gun, &grt::LoadFile::projectile, &grt::LoadFile::propellant})
    {
        const auto &a = (built.*sel).entries();
        const auto &b = ((*orig).*sel).entries();
        QCOMPARE(a.size(), b.size());
        for (std::size_t i = 0; i < a.size(); ++i)
        {
            QCOMPARE(a[i].name, b[i].name);
            QCOMPARE(a[i].unit, b[i].unit);
            QCOMPARE(a[i].type, b[i].type);
        }
    }
    QCOMPARE(built.caliberFile.entries().size(), orig->caliberFile.entries().size());
    QCOMPARE(built.caliberFile.text(u"cipname"), orig->caliberFile.text(u"cipname"));

    // and it survives a write/read cycle and simulates
    const auto again = grt::LoadFile::parse(built.toXml());
    QVERIFY(again);
    QCOMPARE(again->caliber.text(u"CaliberName"), built.caliber.text(u"CaliberName"));
    const grt::SimResult r = grt::simulate(grt::SimInput::fromLoad(built));
    QVERIFY(r.ok());
    QVERIFY(r.muzzleVelocity_ms > 600.0 && r.muzzleVelocity_ms < 760.0);
}

void CoreTest::loadBuilderDefaultsAreSane()
{
    const QString dbPath = QStringLiteral(GRT_SOURCE_DIR "/tools/GordonsReloadingTool.plain.db");
    if (!QFile::exists(dbPath)) QSKIP("plain database not available");
    grt::Database db;
    QVERIFY(db.open(dbPath));
    const auto cal = findRecord(db, grt::Table::Caliber, u"cipname = '.45 Auto'"_s);
    const auto prj = findRecord(db, grt::Table::Projectile, u"caliber = '.451'"_s);
    const auto pro = findRecord(db, grt::Table::Propellant, u"mname = 'Vihtavuori' AND pname = 'N340'"_s);
    QVERIFY(cal && prj && pro);
    QCOMPARE(grt::defaultBarrelLength_mm(*cal), 150.0);  // pistol cartridge
    const grt::LoadFile lf = grt::buildLoad(*cal, *prj, *pro);
    const grt::SimInput in = grt::SimInput::fromLoad(lf);
    QVERIFY(in.charge_g > 0.1 && in.charge_g < 3.0);
    QVERIFY(in.seatingDepth_mm >= 2.0);
    QVERIFY(grt::simulate(in).ok());
}

void CoreTest::unitConversions_data()
{
    using grt::units::Quantity;
    QTest::addColumn<int>("quantity");
    QTest::addColumn<QString>("unit");
    QTest::addColumn<double>("base");
    QTest::addColumn<double>("shown");
    const auto row = [](const char *name, Quantity q, const char *unit, double base, double shown)
    { QTest::newRow(name) << static_cast<int>(q) << QString::fromLatin1(unit) << base << shown; };
    row("25.4 mm = 1 in", Quantity::Length, "in", 25.4, 1.0);
    row("8.2 mm in cm", Quantity::Length, "cm", 8.2, 0.82);
    row("1 grain", Quantity::BulletMass, "grain", 0.06479890999975, 1.0);
    row("196 grain", Quantity::BulletMass, "grain", 12.700586359951, 196.0);
    row("1 bar in psi", Quantity::Pressure, "psi", 1.0, 14.503773773);
    row("3900 bar in MPa", Quantity::Pressure, "mpa", 3900.0, 390.0);
    row("m/s to ft/s", Quantity::Velocity, "fps", 100.0, 328.0839895);
    row("case volume in grain H2O", Quantity::Volume, "grh2o", 63.3 * 0.06479890999975, 63.3);
    row("21 C in F", Quantity::Temperature, "fahr", 21.0, 69.8);
    row("-20 C in F", Quantity::Temperature, "fahr", -20.0, -4.0);
    row("100 C in F", Quantity::Temperature, "fahr", 100.0, 212.0);
    row("J to ft-lbf", Quantity::Energy, "ftlbf", 1.3558179483314, 1.0);
    row("kg to lb", Quantity::GunMass, "lb", 0.45359237, 1.0);
    row("N to lbf", Quantity::Force, "lbf", 4.4482216152605, 1.0);
}

void CoreTest::unitConversions()
{
    QFETCH(int, quantity);
    QFETCH(QString, unit);
    QFETCH(double, base);
    QFETCH(double, shown);
    const auto *u = grt::units::findUnit(static_cast<grt::units::Quantity>(quantity), unit);
    QVERIFY(u);
    QVERIFY(std::abs(grt::units::toDisplay(base, *u) - shown) < 1e-6 * std::max(1.0, std::abs(shown)) * 10);
    QVERIFY(std::abs(grt::units::fromDisplay(shown, *u) - base) < 1e-6 * std::max(1.0, std::abs(base)) * 10);
    // round trip is exact enough for editing
    QVERIFY(std::abs(grt::units::fromDisplay(grt::units::toDisplay(base, *u), *u) - base) < 1e-9 * std::max(1.0, std::abs(base)));
}

void CoreTest::unitSystemsAndParsing()
{
    using grt::units::Quantity;
    using grt::units::System;
    QCOMPARE(grt::units::quantityOfFileUnit(u"mm"_s), Quantity::Length);
    QCOMPARE(grt::units::quantityOfFileUnit(u"Cel"_s), Quantity::Temperature);
    QCOMPARE(grt::units::quantityOfFileUnit(u"kJ/kg"_s), Quantity::None);  // no alternatives
    QCOMPARE(grt::units::quantityOfFileUnit(u""_s), Quantity::None);
    QCOMPARE(QString::fromLatin1(grt::units::systemUnit(Quantity::Length, System::Metric).id), u"mm"_s);
    QCOMPARE(QString::fromLatin1(grt::units::systemUnit(Quantity::Length, System::Imperial).id), u"in"_s);
    QCOMPARE(QString::fromLatin1(grt::units::systemUnit(Quantity::BulletMass, System::Original).id), u"grain"_s);
    QCOMPARE(QString::fromLatin1(grt::units::systemUnit(Quantity::Pressure, System::Original).id), u"bar"_s);
    QCOMPARE(QString::fromLatin1(grt::units::systemUnit(Quantity::Volume, System::Imperial).id), u"grh2o"_s);
    QVERIFY(grt::units::unitsOf(Quantity::None).empty());

    // typing "196" while the unit is grain stores the metric value; both decimal separators are accepted
    const auto *grain = grt::units::findUnit(Quantity::BulletMass, u"grain"_s);
    QVERIFY(grain);
    QVERIFY(std::abs(*grt::units::parseToBase(u"196"_s, *grain) - 12.700586359951) < 1e-9);
    QVERIFY(std::abs(*grt::units::parseToBase(u"0.5"_s, *grain) - 0.5 * 0.06479890999975) < 1e-12);
    QVERIFY(!grt::units::parseToBase(u"abc"_s, *grain));
    QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));
    QCOMPARE(grt::units::format(8.2, *grt::units::findUnit(Quantity::Length, u"mm"_s)), u"8.20"_s);
    QCOMPARE(grt::units::format(25.4, *grt::units::findUnit(Quantity::Length, u"in"_s)), u"1.000"_s);
}

namespace
{

// The 24 bytes of a PNG that the report engine reads: signature, IHDR chunk length and tag, width, height.
QByteArray fakePng(int w, int h)
{
    QByteArray b("\x89PNG\r\n\x1a\n\0\0\0\x0dIHDR", 16);
    for (int v : {w, h})
    {
        b.append(static_cast<char>((v >> 24) & 0xff));
        b.append(static_cast<char>((v >> 16) & 0xff));
        b.append(static_cast<char>((v >> 8) & 0xff));
        b.append(static_cast<char>(v & 0xff));
    }
    return b;
}

struct Records
{
    grt::Database db;
    std::optional<QVariantMap> cal, prj, pro;
    grt::LoadFile load;
};

// An 8x57 IS load with a Sellier-Bellot bullet and N150, charge 2.72 g, built from the database.
bool makeBaseLoad(Records &r)
{
    const QString dbPath = QStringLiteral(GRT_SOURCE_DIR "/tools/GordonsReloadingTool.plain.db");
    if (!QFile::exists(dbPath) || !r.db.open(dbPath)) return false;
    r.cal = findRecord(r.db, grt::Table::Caliber, u"cipname = '8 x 57 IS'"_s);
    r.prj = findRecord(r.db, grt::Table::Projectile, u"mname = 'Sellier-Bellot' AND pname = 'FMJ-BT 2910'"_s);
    r.pro = findRecord(r.db, grt::Table::Propellant, u"mname = 'Vihtavuori' AND pname = 'N150' AND lotid = '2021-11-13'"_s);
    if (!r.cal || !r.prj || !r.pro) return false;
    r.load = grt::buildLoad(*r.cal, *r.prj, *r.pro,
                            {.charge_g = 2.5,
                             .barrelLength_mm = 650.0,
                             .seatingDepth_mm = 9.0,
                             .twistLength_mm = 240.0,
                             .gunWeight_kg = 3.7,
                             .powderTemperature_C = 15.0},
                            r.db.columns(grt::Table::Caliber));
    return true;
}

}  // namespace

void CoreTest::replacingThePowderKeepsTheRest()
{
    Records r;
    if (!makeBaseLoad(r)) QSKIP("plain database not available");
    const auto n340 = findRecord(r.db, grt::Table::Propellant, u"mname = 'Vihtavuori' AND pname = 'N340'"_s);
    QVERIFY(n340);
    grt::LoadFile lf = r.load;
    grt::applyPropellant(lf, *n340);
    QCOMPARE(lf.propellant.text(u"pname"), u"N340"_s);
    QCOMPARE(lf.propellant.number(u"Ba"), n340->value(u"Ba"_s).toDouble());
    QVERIFY(lf.propellant.number(u"Ba") != r.load.propellant.number(u"Ba"));
    QCOMPARE(lf.propellant.number(u"mc"), 2.5);      // the charge is kept
    QCOMPARE(lf.propellant.number(u"pt"), 15.0);     // and the temperature
    QCOMPARE(lf.gun.number(u"xe"), 650.0);           // gun data untouched
    QCOMPARE(lf.projectile.number(u"gdepth"), 9.0);  // so is the bullet
    QCOMPARE(lf.projectile.text(u"pname"), r.load.projectile.text(u"pname"));
    QVERIFY(lf.title.contains(u"N340"_s) && !lf.title.contains(u"N150"_s));
    QCOMPARE(lf.propellant.entries().size(), r.load.propellant.entries().size());  // same fields, same order
    QVERIFY(grt::simulate(grt::SimInput::fromLoad(lf)).ok());

    // asking for the defaults of the new powder resets the charge
    grt::LoadFile lf2 = r.load;
    grt::applyPropellant(lf2, *n340, true, &*r.cal);
    QVERIFY(std::abs(lf2.propellant.number(u"mc") - grt::defaultCharge_g(*r.cal, *n340)) < 1e-12);
    QVERIFY(lf2.propellant.number(u"mc") != 2.5);
}

void CoreTest::replacingTheBulletUpdatesGeometry()
{
    Records r;
    if (!makeBaseLoad(r)) QSKIP("plain database not available");
    const auto other =
        findRecord(r.db, grt::Table::Projectile, u"caliber = '.323' AND gmass > 150 AND id <> %1"_s.arg(r.prj->value(u"id"_s).toInt()));
    QVERIFY(other);
    grt::LoadFile lf = r.load;
    grt::applyProjectile(lf, *other);
    QCOMPARE(lf.projectile.text(u"pname"), other->value(u"pname"_s).toString());
    QVERIFY(std::abs(lf.projectile.number(u"mp") - other->value(u"gmass"_s).toDouble() * grt::kGrainToGram) < 1e-9);
    QCOMPARE(lf.projectile.number(u"gdepth"), 9.0);  // seating depth kept
    QVERIFY(std::abs(lf.caliber.number(u"oal") - (57.0 + lf.projectile.number(u"glen") - 9.0)) < 1e-9);
    QCOMPARE(lf.caliber.number(u"ps"), lf.projectile.number(u"gpressure"));
    QVERIFY(lf.title.contains(other->value(u"pname"_s).toString()));
    QCOMPARE(lf.propellant.number(u"mc"), 2.5);

    grt::LoadFile lf2 = r.load;
    grt::applyProjectile(lf2, *other, true, &*r.cal);
    QVERIFY(std::abs(lf2.projectile.number(u"gdepth") - grt::defaultSeatingDepth_mm(*r.cal, *other)) < 1e-9);
}

void CoreTest::replacingTheCaliberCanResetDependentValues()
{
    Records r;
    if (!makeBaseLoad(r)) QSKIP("plain database not available");
    const auto auto45 = findRecord(r.db, grt::Table::Caliber, u"cipname = '.45 Auto'"_s);
    QVERIFY(auto45);
    grt::LoadFile kept = r.load;
    grt::applyCaliber(kept, *auto45, r.db.columns(grt::Table::Caliber));
    QCOMPARE(kept.caliber.text(u"CaliberName"), u".45 Auto (.45 ACP)"_s);
    QCOMPARE(kept.caliberFile.text(u"cipname"), u".45 Auto"_s);
    QCOMPARE(kept.caliber.number(u"caselen"), auto45->value(u"L3"_s).toDouble());
    QCOMPARE(kept.gun.number(u"xe"), 650.0);  // nothing else changes without the reset
    QCOMPARE(kept.propellant.number(u"mc"), 2.5);
    QCOMPARE(kept.projectile.number(u"gdepth"), 9.0);
    QVERIFY(kept.title.startsWith(u".45 Auto"_s));

    grt::LoadFile reset = r.load;
    grt::applyCaliber(reset, *auto45, r.db.columns(grt::Table::Caliber), true, &*r.prj, &*r.pro);
    QCOMPARE(reset.gun.number(u"xe"), 150.0);  // pistol barrel
    QVERIFY(std::abs(reset.projectile.number(u"gdepth") - grt::defaultSeatingDepth_mm(*auto45, *r.prj)) < 1e-9);
    QVERIFY(std::abs(reset.propellant.number(u"mc") - grt::defaultCharge_g(*auto45, *r.pro)) < 1e-12);
    QCOMPARE(reset.caliberFile.entries().size(), r.load.caliberFile.entries().size());
}

void CoreTest::reportNumberFormats_data()
{
    QTest::addColumn<double>("value");
    QTest::addColumn<QString>("format");
    QTest::addColumn<QString>("expected");
    const auto row = [](const char *name, double v, const char *f, const char *e)
    { QTest::newRow(name) << v << QString::fromUtf8(f) << QString::fromUtf8(e); };
    // the examples of the original's manual
    row("manual: #.000", 51.78, "#.000", "51.780");
    row("manual: with unit text", 51.78, "#.000 mm²", "51.780 mm²");
    // rounding and placeholders
    row("round half away", 2.5, "0", "3");
    row("round half away neg", -2.5, "0", "-3");
    row("0.005 -> 0.01", 0.005, "0.00", "0.01");
    row("# drops trailing zeros", 1.5, "#.##", "1.5");
    row("# drops all decimals", 2.0, "#.##", "2");
    row("0 keeps zeros", 2.0, "0.00", "2.00");
    row("mixed 0 and #", 2.5, "0.0#", "2.5");
    row("mixed 0 and # long", 2.125, "0.0#", "2.13");
    row("leading zeros", 3.14, "00.0", "03.1");
    row("# shows no integer zero", 0.5, "#.00", ".50");
    row("0 shows integer zero", 0.5, "0.00", "0.50");
    row("zero", 0.0, "0.00", "0.00");
    row("no negative zero", -0.001, "0.00", "0.00");
    // thousands, percent, parentheses, signs
    row("thousands", 1234567.891, "#,##0.00", "1,234,567.89");
    row("thousands small", 12.0, "#,##0", "12");
    row("percent", 0.7104, "0.0%", "71.0%");
    row("percent suffix text", 0.5, "0 %", "50 %");
    row("parentheses", 2.0, "(0.0)", "(2.0)");
    row("plus positive", 3.0, "+0.0", "+3.0");
    row("plus negative", -3.0, "+0.0", "-3.0");
    row("minus negative only", -3.0, "-0.0", "-3.0");
    row("minus positive", 3.0, "-0.0", "3.0");
    row("negative default", -3.25, "0.00", "-3.25");
    row("negative with parentheses", -2.0, "(0.0)", "(-2.0)");
    // scientific notation
    row("scientific", 12345.0, "0.00E+00", "1.23E+04");
    row("scientific small", 0.00123, "0.00E+00", "1.23E-03");
    row("scientific no plus", 12345.0, "0.0E00", "1.2E04");
    row("scientific carry", 9.999, "0.00E+00", "1.00E+01");
    row("scientific zero", 0.0, "0.0E+00", "0.0E+00");
    row("scientific negative", -1500.0, "0.0E+00", "-1.5E+03");
    // text without a pattern keeps the text
    row("only unit text", 4.0, " bar", " bar4");
    row("empty format", 1.25, "", "1.25");
}

void CoreTest::reportNumberFormats()
{
    QFETCH(double, value);
    QFETCH(QString, format);
    QFETCH(QString, expected);
    QLocale::setDefault(QLocale(QLocale::German, QLocale::Germany));  // output must not depend on the locale
    QCOMPARE(grt::report::formatNumber(value, format), expected);
    QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));
}

void CoreTest::wikiBasicFormatting()
{
    using grt::report::wikiToHtml;
    QVERIFY(wikiToHtml(u"====== A ======"_s).contains(u"<h1>A</h1>"_s));
    QVERIFY(wikiToHtml(u"===== B ====="_s).contains(u"<h2>B</h2>"_s));
    QVERIFY(wikiToHtml(u"==== C ===="_s).contains(u"<h3>C</h3>"_s));
    QVERIFY(wikiToHtml(u"== E =="_s).contains(u"<h5>E</h5>"_s));
    const QString f = wikiToHtml(u"**a** //b// __c__ ''d''"_s);
    QVERIFY(f.contains(u"<b>a</b>"_s) && f.contains(u"<i>b</i>"_s) && f.contains(u"<u>c</u>"_s) && f.contains(u"<tt>d</tt>"_s));
    const QString combined = wikiToHtml(u"**__//x//__**"_s);
    QVERIFY(combined.contains(u"<b>"_s) && combined.contains(u"<u>"_s) && combined.contains(u"<i>x</i>"_s));
    // paragraphs from blank lines, a forced break only with two backslashes
    QCOMPARE(wikiToHtml(u"one\n\ntwo"_s).count(u"<p>"_s), 2);
    QVERIFY(wikiToHtml(u"line1\\\\ line2"_s).contains(u"<br>"_s));
    QVERIFY(!wikiToHtml(u"line1\nline2"_s).contains(u"<br>"_s));
    // typography
    const QString t = wikiToHtml(u"a -> b => c (c) (tm) -- d"_s);
    QVERIFY(t.contains(u"→"_s) && t.contains(u"⇒"_s) && t.contains(u"©"_s) && t.contains(u"™"_s) && t.contains(u"–"_s));
    // an italic marker inside a URL is not italics
    QVERIFY(wikiToHtml(u"see http://example.org/x"_s).contains(u"http://example.org/x"_s));
}

void CoreTest::wikiBlocks()
{
    using grt::report::wikiToHtml;
    QVERIFY(wikiToHtml(u"text\n\n----\n\nmore"_s).contains(u"<hr>"_s));
    // nested list
    const QString list = wikiToHtml(u"  * a\n  * b\n    * c\n  * d"_s);
    QCOMPARE(list.count(u"<ul>"_s), 2);
    QCOMPARE(list.count(u"</ul>"_s), 2);
    QCOMPARE(list.count(u"<li>"_s), 4);
    // a code block is literal: no list, no formatting
    const QString code = wikiToHtml(u"<code>\n  * not a list\n**not bold**\n</code>"_s);
    QVERIFY(code.contains(u"<pre>"_s) && code.contains(u"* not a list"_s) && code.contains(u"**not bold**"_s));
    QVERIFY(!code.contains(u"<ul>"_s) && !code.contains(u"<b>"_s));
    // an inline code tag on one line
    QVERIFY(wikiToHtml(u"<code>x = 1</code>"_s).contains(u"<pre>x = 1</pre>"_s));
}

void CoreTest::wikiTables()
{
    using grt::report::wikiToHtml;
    const QString t = wikiToHtml(u"^ H1 ^ H2 ^\n| a | b |\n| c | d |"_s);
    QCOMPARE(t.count(u"<table"_s), 1);
    QVERIFY(t.contains(u"<th>H1</th>"_s) && t.contains(u"<th>H2</th>"_s));
    QVERIFY(t.contains(u"<td>a</td>"_s) && t.contains(u"<td>d</td>"_s));
    QCOMPARE(t.count(u"<tr>"_s), 3);
    // an empty cell extends the one before it (column span)
    const QString span = wikiToHtml(u"| x | y ||\n| 1 | 2 | 3 |"_s);
    QVERIFY(span.contains(u"colspan=\"2\""_s));
    // the separator before a cell decides header or normal cell (vertical titles)
    const QString vert = wikiToHtml(u"|   ^ Head ^\n^ Side | cell |"_s);
    QVERIFY(vert.contains(u"<th>Head</th>"_s) && vert.contains(u"<th>Side</th>"_s) && vert.contains(u"<td>cell</td>"_s));
    // formatting works inside cells, and two tables are two tables
    QVERIFY(wikiToHtml(u"| **bold** |"_s).contains(u"<td><b>bold</b></td>"_s));
    QCOMPARE(wikiToHtml(u"| a |\n\ntext\n\n| b |"_s).count(u"<table"_s), 2);
}

void CoreTest::wikiImagesLinksAndTags()
{
    using grt::report::wikiToHtml;
    // known.png is 400 x 200 pixels
    const auto resolver = [](const QString &name) -> grt::report::ImageSource
    { return name == u"known.png"_s ? grt::report::ImageSource(u"data:image/png;base64,AAA"_s, 400, 200) : grt::report::ImageSource(); };
    // no size given: natural size
    QVERIFY(
        wikiToHtml(u"{{known.png}}"_s, resolver, 1000).contains(u"<img src=\"data:image/png;base64,AAA\" width=\"400\" height=\"200\">"_s));
    // a width in pixels keeps the aspect ratio
    const QString w80 = wikiToHtml(u"{{known.png?80}}"_s, resolver, 1000);
    QVERIFY(w80.contains(u"width=\"80\""_s) && w80.contains(u"height=\"40\""_s));
    // a percentage is turned into pixels of the page width (Qt rich text has no percentages)
    const QString pct = wikiToHtml(u"{{known.png?15%}}"_s, resolver, 1000);
    QVERIFY(pct.contains(u"width=\"150\""_s) && pct.contains(u"height=\"75\""_s));
    QVERIFY(!pct.contains(u"%\""_s));
    const QString full = wikiToHtml(u"{{known.png?100%}}"_s, resolver, 600);
    QVERIFY(full.contains(u"width=\"600\""_s) && full.contains(u"height=\"300\""_s));
    // explicit width and height
    const QString wh = wikiToHtml(u"{{known.png?100x50}}"_s, resolver, 1000);
    QVERIFY(wh.contains(u"width=\"100\""_s) && wh.contains(u"height=\"50\""_s));
    // an image is never wider than the page
    const QString wide = wikiToHtml(u"{{known.png}}"_s, resolver, 300);
    QVERIFY(wide.contains(u"width=\"300\""_s) && wide.contains(u"height=\"150\""_s));
    QVERIFY(wikiToHtml(u"{{known.png?900}}"_s, resolver, 300).contains(u"width=\"300\""_s));
    QVERIFY(wikiToHtml(u"{{known.png?100  }}"_s, resolver, 1000).contains(u"align=\"left\""_s));
    QVERIFY(wikiToHtml(u"{{  known.png?100}}"_s, resolver, 1000).contains(u"align=\"right\""_s));
    QVERIFY(wikiToHtml(u"{{known.png?100&link=https://example.org}}"_s, resolver).contains(u"<a href=\"https://example.org\">"_s));
    // an image of unknown size still gets the requested width
    const auto unknownSize = [](const QString &) { return grt::report::ImageSource(u"data:x"_s); };
    QVERIFY(wikiToHtml(u"{{a.png?50%}}"_s, unknownSize, 400).contains(u"width=\"200\""_s));
    // an image that cannot be resolved shows its name instead of a broken picture
    QVERIFY(wikiToHtml(u"{{missing.png}}"_s, resolver).contains(u"[missing.png]"_s));
    QVERIFY(!wikiToHtml(u"{{missing.png}}"_s, resolver).contains(u"<img"_s));
    QVERIFY(wikiToHtml(u"{{known.png}}"_s).contains(u"[known.png]"_s));  // no resolver at all

    QVERIFY(wikiToHtml(u"[[http://x.org|Site]]"_s).contains(u"<a href=\"http://x.org\">Site</a>"_s));
    QVERIFY(wikiToHtml(u"[[http://x.org]]"_s).contains(u">http://x.org</a>"_s));
    QVERIFY(wikiToHtml(u"[[me@example.org]]"_s).contains(u"mailto:me@example.org"_s));

    QVERIFY(wikiToHtml(u"<color red>x</color>"_s).contains(u"<span style=\"color:#e02020\">x</span>"_s));
    QVERIFY(wikiToHtml(u"<color #80ff80>x</color>"_s).contains(u"color:#80ff80"_s));
    QVERIFY(wikiToHtml(u"<color color_diagram_pressure>x</color>"_s).contains(u"color:#"_s));
    QVERIFY(wikiToHtml(u"<color nosuchcolor>x</color>"_s).contains(u"<span>x</span>"_s));  // unknown: plain
    QVERIFY(wikiToHtml(u"<size 150%>x</size>"_s).contains(u"font-size:15pt"_s));
    QVERIFY(wikiToHtml(u"<size=14>x</size>"_s).contains(u"font-size:14pt"_s));
}

void CoreTest::wikiEscapesHostileInput()
{
    using grt::report::wikiToHtml;
    const QString h = wikiToHtml(u"<script>alert(1)</script> & <img src=x onerror=y> **ok**"_s);
    QVERIFY(!h.contains(u"<script>"_s));
    QVERIFY(!h.contains(u"<img"_s));
    QVERIFY(h.contains(u"&lt;script&gt;"_s) && h.contains(u"&amp;"_s));
    QVERIFY(h.contains(u"<b>ok</b>"_s));
    // quotes in a link target cannot break out of the attribute
    const QString l = wikiToHtml(u"[[x\" onclick=\"y|t]]"_s);
    QVERIFY(!l.contains(u"onclick=\"y"_s));
    // an image name cannot inject either
    const auto resolver = [](const QString &) { return QString(); };
    QVERIFY(!wikiToHtml(u"{{a\"><script>.png}}"_s, resolver).contains(u"<script>"_s));
}

void CoreTest::wikiPageBreak()
{
    using grt::report::wikiToHtml;
    const QString h = wikiToHtml(u"before\n\n<pagebreak>\n\nafter"_s);
    QVERIFY(h.contains(u"page-break-before:always"_s));
    QVERIFY(h.indexOf(u"before"_s) < h.indexOf(u"page-break-before"_s) && h.indexOf(u"page-break-before"_s) < h.indexOf(u"after"_s));
    QVERIFY(!h.contains(u"&lt;pagebreak"_s));
    // inside a sentence it is just text
    QVERIFY(wikiToHtml(u"use <pagebreak> here"_s).contains(u"&lt;pagebreak&gt;"_s));
}

void CoreTest::reportPlaceholders()
{
    using namespace grt::report;
    Data d;
    d.set(u"work.Aeff"_s, Value::ofNumber(51.78, grt::units::Quantity::Area));
    d.set(u"work.CaliberName"_s, Value::ofText(u"8 x 57 IS"_s));
    d.set(u"result.LoadRatio"_s, Value::ofFraction(0.7104));
    d.set(u"work.k"_s, Value::ofNumber(1.2214));
    d.set(u"result.diagramm.png"_s, Value::ofImage(fakePng(800, 400)));
    // text, numbers with a format string (in the file's own unit), case-insensitive names
    QCOMPARE(expandPragmas(u"~~work.CaliberName~~"_s, d), u"8 x 57 IS"_s);
    QCOMPARE(expandPragmas(u"~~WORK.caliberNAME~~"_s, d), u"8 x 57 IS"_s);
    QCOMPARE(expandPragmas(u"~~work.Aeff(\"#.000\")~~"_s, d), u"51.780"_s);
    QCOMPARE(expandPragmas(u"~~work.Aeff(\"#.000 mm²\")~~"_s, d), u"51.780 mm²"_s);
    QCOMPARE(expandPragmas(u"~~ work.Aeff ( \"0.0\" ) ~~"_s, d), u"51.8"_s);  // spaces are tolerated
    // fractions are shown as percent, plain numbers with up to four decimals
    QCOMPARE(expandPragmas(u"~~result.LoadRatio~~"_s, d), u"71.0 %"_s);
    QCOMPARE(expandPragmas(u"~~result.LoadRatio(\"0.00\")~~"_s, d), u"0.71"_s);
    QCOMPARE(expandPragmas(u"~~work.k~~"_s, d), u"1.2214"_s);
    d.set(u"work.half"_s, Value::ofNumber(0.5));
    QCOMPARE(expandPragmas(u"~~work.half~~"_s, d), u"0.5"_s);  // automatic format keeps the leading zero
    // several in one text, surrounding text untouched
    QCOMPARE(expandPragmas(u"A=~~work.k(\"0.0\")~~ / B=~~work.k(\"0.00\")~~."_s, d), u"A=1.2 / B=1.22."_s);
    // images become wiki image syntax, the format string is their size
    QCOMPARE(expandPragmas(u"~~result.diagramm.png~~"_s, d), u"{{result.diagramm.png}}"_s);
    QCOMPARE(expandPragmas(u"~~result.diagramm.png(\"?40%\")~~"_s, d), u"{{result.diagramm.png?40%}}"_s);
    // an unknown value is visible in red, not silently empty
    QCOMPARE(expandPragmas(u"~~work.nothing~~"_s, d), u"<color red>[work.nothing?]</color>"_s);
    // text that only looks similar stays
    QCOMPARE(expandPragmas(u"a ~ b ~~ c"_s, d), u"a ~ b ~~ c"_s);
    // the rendered report embeds the image and shows the missing value in red
    const QString html = renderReport(u"~~work.CaliberName~~ {{result.diagramm.png}} ~~work.nothing~~"_s, d);
    QVERIFY(html.contains(u"8 x 57 IS"_s));
    QVERIFY(html.contains(u"data:image/png;base64,"_s));
    QVERIFY(html.contains(u"[work.nothing?]"_s) && html.contains(u"color:#e02020"_s));
}

void CoreTest::reportUnitsInPlaceholders()
{
    using namespace grt::report;
    using grt::units::Quantity;
    Data d;
    d.set(u"result.MaxPressure"_s, Value::ofNumber(2287.0, Quantity::Pressure));
    d.set(u"work.mp"_s, Value::ofNumber(12.7005863599, Quantity::BulletMass));
    d.set(u"work.pt"_s, Value::ofNumber(21.0, Quantity::Temperature));
    // default: the base unit
    QCOMPARE(expandPragmas(u"~~result.MaxPressure~~"_s, d), u"2287 bar"_s);
    // with the user's units: psi, grains, Fahrenheit (decimals of the unit, '.' as the decimal point)
    RenderOptions imperial;
    imperial.unitFor = [](Quantity q) -> const grt::units::Unit & { return grt::units::systemUnit(q, grt::units::System::Imperial); };
    QLocale::setDefault(QLocale(QLocale::German, QLocale::Germany));
    QCOMPARE(expandPragmas(u"~~result.MaxPressure~~"_s, d, imperial), u"33170 psi"_s);
    QCOMPARE(expandPragmas(u"~~work.mp~~"_s, d, imperial), u"196.00 grain"_s);
    QCOMPARE(expandPragmas(u"~~work.pt~~"_s, d, imperial), u"70 °F"_s);
    QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));
    // an explicit format always means the file's base unit, so the template reads the same in every system
    QCOMPARE(expandPragmas(u"~~result.MaxPressure(\"0 bar\")~~"_s, d, imperial), u"2287 bar"_s);
    QCOMPARE(expandPragmas(u"~~work.mp(\"0.00 g\")~~"_s, d, imperial), u"12.70 g"_s);
}

void CoreTest::reportDataFromALoad()
{
    auto lf = grt::LoadFile::read(QStringLiteral(GRT_FIXTURE_DIR "/8x57is.grtload"));
    QVERIFY(lf);
    const grt::SimInput in = grt::SimInput::fromLoad(*lf);
    const grt::SimResult r = grt::simulate(in);
    QVERIFY(r.ok());
    const grt::report::Data d = grt::report::buildData(*lf, in, r, {QStringLiteral("/some/dir/my load.grtload")});
    const auto number = [&](const char *key)
    {
        const auto *v = d.find(QLatin1String(key));
        return v ? v->number : -1e99;
    };
    const auto text = [&](const char *key)
    {
        const auto *v = d.find(QLatin1String(key));
        return v ? v->text : QString();
    };

    QCOMPARE(text("file.caption"), u"my load"_s);
    QCOMPARE(text("file.name"), u"my load.grtload"_s);
    QCOMPARE(text("work.CaliberName"), u"8 x 57 IS (8 x 57 JS)"_s);
    QCOMPARE(text("work.PropellantName"), u"Vihtavuori N150"_s);
    QCOMPARE(number("work.Dbul"), 8.2);
    QCOMPARE(number("work.xe"), 600.0);
    QCOMPARE(number("work.xeEff"), 600.0 - 46.6);  // the projectile path
    QCOMPARE(text("work.gtailtype"), u"boattail"_s);
    QVERIFY(std::abs(number("work.gdepthc") - (10.4 - 5.1)) < 1e-9);  // guided seating depth: depth minus the boat tail
    QVERIFY(std::abs(number("work.Vb") - r.effectiveCaseVolume_cm3) < 1e-12);
    QVERIFY(std::abs(number("work.caliber") - 0.3228) < 0.001);  // inches
    // results come straight from the simulation
    QCOMPARE(number("result.MaxPressure"), r.maxPressure_bar);
    QCOMPARE(number("result.MuzzleVelocity"), r.muzzleVelocity_ms);
    QCOMPARE(number("result.LoadRatio"), r.loadRatio);
    QVERIFY(d.find(u"result.LoadRatio"_s)->percent);
    QVERIFY(number("result.OutflowVelocity") > r.muzzleVelocity_ms);  // the gases leave faster than the bullet
    QVERIFY(number("result.RecoilEnergy") > 0.0);                     // the sample has a gun weight
    // work.mc is the charge of the curve that is shown
    QCOMPARE(number("work.mc"), in.charge_g);

    // an unsaved load has no file name, but a caption
    const auto unsaved = grt::report::buildData(*lf, in, r, {});
    QCOMPARE(unsaved.find(u"file.caption"_s)->text, lf->title);
    QVERIFY(unsaved.find(u"file.name"_s)->text.isEmpty());
    // a load that cannot be simulated still has its inputs, but no results
    lf->propellant.set(u"mc", 50.0);
    const grt::SimInput bad = grt::SimInput::fromLoad(*lf);
    const grt::SimResult none = grt::simulate(bad);
    QVERIFY(!none.ok());
    const auto noResults = grt::report::buildData(*lf, bad, none, {});
    QVERIFY(noResults.find(u"work.mc"_s));
    QVERIFY(!noResults.find(u"result.MaxPressure"_s));
}

void CoreTest::defaultReportRendersEverything()
{
    auto lf = grt::LoadFile::read(QStringLiteral(GRT_FIXTURE_DIR "/8x57is.grtload"));
    QVERIFY(lf);
    const grt::SimInput in = grt::SimInput::fromLoad(*lf);
    const grt::SimResult r = grt::simulate(in);
    grt::report::Data d = grt::report::buildData(*lf, in, r, {QStringLiteral("/x/sample.grtload")});
    d.set(u"result.diagramm.png"_s, grt::report::Value::ofImage(fakePng(800, 400)));
    d.set(u"caliber.drawing.png"_s, grt::report::Value::ofImage(fakePng(330, 760)));
    d.set(u"caliber.chamber.png"_s, grt::report::Value::ofImage(fakePng(330, 760)));
    d.set(u"result.diagramCurves.png"_s, grt::report::Value::ofImage(fakePng(1100, 560)));
    d.set(u"result.diagramPropellant.png"_s, grt::report::Value::ofImage(fakePng(1100, 300)));
    const QString html = grt::report::renderReport(grt::report::defaultTemplate(), d);
    QVERIFY(!html.contains(u"[work."_s) && !html.contains(u"[result."_s));  // no unresolved placeholder in red
    QVERIFY(!html.contains(u"~~"_s));
    QVERIFY(html.contains(u"<h1>sample</h1>"_s));
    QVERIFY(html.contains(u"8 x 57 IS"_s) && html.contains(u"N150"_s));
    QVERIFY(html.contains(u" bar"_s) && html.contains(u" m/s"_s));
    QCOMPARE(html.count(u"<img"_s), 4);  // chart, burn rate, cartridge and chamber drawing
    QVERIFY(html.count(u"<table"_s) >= 3);
    QVERIFY(html.contains(u"Use at your own risk"_s));
    QVERIFY(html.contains(u"page-break-before"_s));  // the diagrams start on a new page
}

// A caliber record without Sebert factor (the project's records) gives 0.5 in the load, not 0; the physical model treats a 0 of older
// loads as 0.5, the Original model keeps it (the original program computes with 0).
void CoreTest::sebertDefault()
{
    const QString dbPath = QStringLiteral(GRT_SOURCE_DIR "/tools/GordonsReloadingTool.plain.db");
    if (!QFile::exists(dbPath)) QSKIP("plain database not available");
    grt::Database db;
    QVERIFY(db.open(dbPath));
    auto cal = findRecord(db, grt::Table::Caliber, u"cipname = '8 x 57 IS'"_s);
    const auto prj = findRecord(db, grt::Table::Projectile, u"mname = 'Sellier-Bellot' AND pname = 'FMJ-BT 2910' AND caliber = '.323'"_s);
    const auto pro = findRecord(db, grt::Table::Propellant, u"mname = 'Vihtavuori' AND pname = 'N150' AND lotid = '2021-11-13'"_s);
    QVERIFY(cal && prj && pro);
    cal->insert(u"sebert"_s, 0.0);
    QCOMPARE(grt::buildLoad(*cal, *prj, *pro, {.charge_g = 2.7}).caliber.number(u"sebert"), 0.5);
    cal->remove(u"sebert"_s);
    QCOMPARE(grt::buildLoad(*cal, *prj, *pro, {.charge_g = 2.7}).caliber.number(u"sebert"), 0.5);

    grt::SimInput zero = grt::SimInput::fromLoad(grt::buildLoad(*cal, *prj, *pro, {.charge_g = 2.7}));
    const grt::SimInput half = zero;
    zero.sebert = 0.0;
    const grt::Calibration physical = grt::calibrationFor(grt::CalibrationProfile::Physical);
    QCOMPARE(grt::simulate(zero, physical).maxPressure_bar, grt::simulate(half, physical).maxPressure_bar);
    const grt::Calibration original = grt::calibrationFor(grt::CalibrationProfile::Original);
    QVERIFY(grt::simulate(zero, original).maxPressure_bar != grt::simulate(half, original).maxPressure_bar);
}

// An own database of an older version (caliber table without the SAAMI pressure) gets the column when it is opened; the project's
// records carry it next to the C.I.P. Pmax.
void CoreTest::saamiColumnMigration()
{
    QTemporaryDir dir;
    const QString own = dir.filePath(u"old.db"_s);
    {
        QSqlDatabase old = QSqlDatabase::addDatabase(u"QSQLITE"_s, u"old"_s);
        old.setDatabaseName(own);
        QVERIFY(old.open());
        QSqlQuery q(old);
        QVERIFY(q.exec(u"CREATE TABLE caliber (id INTEGER PRIMARY KEY AUTOINCREMENT, cipname TEXT NOT NULL, Pmax FLOAT NOT NULL, "
                       "PK FLOAT NOT NULL)"_s));
        QVERIFY(q.exec(u"INSERT INTO caliber (id, cipname, Pmax, PK) VALUES (10000001, 'Old', 3500, 4000)"_s));
        old.close();
    }
    QSqlDatabase::removeDatabase(u"old"_s);
    grt::Database db;
    QVERIFY(db.open({}, own));
    const QStringList cols = db.columns(grt::Table::Caliber);
    QCOMPARE(cols.indexOf(u"PmaxSAAMI"_s), cols.indexOf(u"Pmax"_s) + 1);  // shown next to the C.I.P. value, not at the end
    const auto rec = db.record(grt::Table::Caliber, 10000001);
    QVERIFY(rec);
    QCOMPARE(rec->value(u"PmaxSAAMI"_s).toDouble(), 0.0);  // no SAAMI value known

    grt::Database fresh;
    QVERIFY(fresh.open({}, dir.filePath(u"new.db"_s)));
    const QStringList freshCols = fresh.columns(grt::Table::Caliber);
    QCOMPARE(freshCols.indexOf(u"PmaxSAAMI"_s), freshCols.indexOf(u"Pmax"_s) + 1);
}

// Measurements fitted for both models: the original's Ba and the physical values (velocities simulated with the physical model);
// applied together, the physical values refer to the new Ba and still apply. A load without physical values gets them.
void CoreTest::powderFitPhysicalValues()
{
    REQUIRE_PLAIN_DB();
    auto lf = grt::LoadFile::read(QStringLiteral(GRT_FIXTURE_DIR "/8x57is.grtload"));
    QVERIFY(lf);
    grt::writePhysicalBurn(lf->propellant, grt::PhysicalBurn{.Ba = 1.3, .a0 = 0.5, .z1 = 0.3, .z2 = 0.7});
    grt::SimInput truth = grt::SimInput::fromLoad(*lf);
    QVERIFY(truth.physicalBurn);
    const grt::Calibration physical = grt::calibrationFor(grt::CalibrationProfile::Physical);
    grt::Measurement m{.title = u"M"_s, .charges = {}, .attributes = {}};
    for (const double g : {2.4, 2.6, 2.75})
    {
        truth.charge_g = g;
        const grt::SimResult r = grt::simulate(truth, physical);
        QVERIFY(r.ok());
        grt::MeasuredCharge c = grt::MeasuredCharge::create(u"c"_s, g);
        c.shots.push_back({.velocity_ms = r.muzzleVelocity_ms});
        m.charges.push_back(c);
    }
    lf->appendixXml = grt::writeMeasurement(lf->appendixXml, m);
    grt::writePhysicalBurn(lf->propellant, grt::PhysicalBurn{.Ba = 1.1, .a0 = 0.5, .z1 = 0.3, .z2 = 0.7});  // the start value
    const double Ba = lf->propellant.number(u"Ba");

    QStringList warnings;
    const auto samples = grt::powderfit::samples(*lf, u"8x57"_s, &warnings);
    grt::setCalibrationProfile(grt::CalibrationProfile::Original);  // each model is fitted with its own constants, whatever is active
    const auto results =
        grt::powderfit::fitAll(samples, {.params = {grt::powderfit::Param::Ba}, .pressureWeight = 1.0, .maxIterations = 60});
    QCOMPARE(results.size(), std::size_t{2});
    QCOMPARE(results[0].model, grt::powderfit::Model::Original);
    const auto &res = results[1];
    QCOMPARE(res.model, grt::powderfit::Model::Physical);
    QVERIFY(res.improved);
    QCOMPARE_LT(std::abs(res.after[0] / 1.3 - 1.0), 0.003);
    grt::powderfit::apply(lf->propellant, results);  // the original's Ba first, then the physical values referring to it
    const auto burn = grt::readPhysicalBurn(lf->propellant);
    QVERIFY(burn);  // still valid: physRef is the new Ba
    QCOMPARE_LT(std::abs(burn->Ba / 1.3 - 1.0), 0.003);
    QCOMPARE(burn->z2, 0.7);  // not fitted: kept
    if (results[0].improved) QVERIFY(lf->propellant.number(u"Ba") != Ba);

    // a load without physical values (own or GRT powder) gets them from the physical fit
    grt::writePhysicalBurn(lf->propellant, std::nullopt);
    const auto fresh = grt::powderfit::samples(*lf, u"8x57"_s, &warnings);
    const auto physOnly = grt::powderfit::fit(
        fresh,
        {.params = {grt::powderfit::Param::Ba}, .pressureWeight = 1.0, .maxIterations = 60, .model = grt::powderfit::Model::Physical});
    QVERIFY(physOnly.improved);
    grt::powderfit::apply(lf->propellant, physOnly);
    QVERIFY(grt::readPhysicalBurn(lf->propellant));
    QCOMPARE_LT(physOnly.velocityRmsAfter, 0.01);  // another form function (the load's a0, z1, z2): Ba alone gets close, not exact
}

QTEST_APPLESS_MAIN(CoreTest)
#include "tst_core.moc"
