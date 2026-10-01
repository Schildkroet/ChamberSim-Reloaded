#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

#include "database.h"
#include "language.h"
#include "loadbuilder.h"
#include "loadfile.h"
#include "reference.h"
#include "report.h"
#include "reportformat.h"
#include "wiki.h"
#include "units.h"
#include "solver.h"

#include <algorithm>
#include <cmath>

using namespace Qt::StringLiterals;

class CoreTest : public QObject {
    Q_OBJECT

private slots:
    void loadFileReadsSections();
    void loadFileRoundTrip();
    void loadFileKeepsAppendix();
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
    void solverVersusReference_data();
    void solverVersusReference();
    void databaseOpensPlainFile();
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

void CoreTest::loadFileReadsSections()
{
    QString err;
    const auto lf = grt::LoadFile::read(QStringLiteral(GRT_TEST_DATA_DIR "/8x57is.grtload"), &err);
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
    const auto a = grt::LoadFile::read(QStringLiteral(GRT_TEST_DATA_DIR "/8x57is.grtload"));
    QVERIFY(a);
    const auto b = grt::LoadFile::parse(a->toXml());
    QVERIFY(b);
    QCOMPARE(b->title, a->title);
    QCOMPARE(b->caliberFile.entries().size(), a->caliberFile.entries().size());
    for (const auto sel : {&grt::LoadFile::caliber, &grt::LoadFile::gun, &grt::LoadFile::projectile,
                            &grt::LoadFile::propellant, &grt::LoadFile::caliberFile, &grt::LoadFile::config}) {
        const auto &ea = ((*a).*sel).entries();
        const auto &eb = ((*b).*sel).entries();
        QCOMPARE(eb.size(), ea.size());
        for (std::size_t i = 0; i < ea.size(); ++i) {
            QCOMPARE(eb[i].name, ea[i].name);
            QCOMPARE(eb[i].value, ea[i].value);
            QCOMPARE(eb[i].unit, ea[i].unit);
        }
    }
}

void CoreTest::loadFileKeepsAppendix()
{
    const auto a = grt::LoadFile::read(QStringLiteral(GRT_TEST_DATA_DIR "/POWDER-MEASURE-TEMPLATE.grtload"));
    QVERIFY(a);
    QVERIFY(a->appendixXml.contains(u"<Measurement"_s));
    QVERIFY(a->appendixXml.contains(u"<shot"_s));
    const auto b = grt::LoadFile::parse(a->toXml());
    QVERIFY(b);
    QCOMPARE(b->appendixXml.count(u"<shot "_s), a->appendixXml.count(u"<shot "_s));
    QCOMPARE(b->appendixXml.count(u"<charge "_s), 2);
}

void CoreTest::simInputFromLoad()
{
    const auto lf = grt::LoadFile::read(QStringLiteral(GRT_TEST_DATA_DIR "/8x57is.grtload"));
    QVERIFY(lf);
    const grt::SimInput in = grt::SimInput::fromLoad(*lf);
    QCOMPARE(in.startPosition_mm(), 46.6);                       // 57 - 10.4
    QCOMPARE(in.effectiveMass_g(), 12.700586359951799 + 0.5 * 2.721554219989671);
    QCOMPARE(in.vivacityBa, 0.5814);
    QCOMPARE(in.startPressure_bar, 250.0);
}

// Measured on the original: the default coefficients make cold powder weaker and hot powder stronger; explicit
// coefficients act linearly with their own sign (a positive tcc raises the burn rate when cold).
void CoreTest::temperatureCoefficients()
{
    const auto lf = grt::LoadFile::read(QStringLiteral(GRT_TEST_DATA_DIR "/8x57is.grtload"));
    QVERIFY(lf);
    const auto pmax = [&](double T, double tcc, double tch) {
        grt::SimInput in = grt::SimInput::fromLoad(*lf);
        in.temperature_C = T;
        in.coldCoeff = tcc;
        in.hotCoeff = tch;
        const grt::SimResult r = grt::simulate(in);
        return r.maxPressure_bar;
    };
    const double base = pmax(21, 0, 0);
    QVERIFY(pmax(-20, 0, 0) < base * 0.97);   // default: about -4 % when cold
    QVERIFY(pmax(60, 0, 0) > base * 1.05);    // and about +8 % when hot
    QVERIFY(pmax(-20, 0.0016, 0) > base);     // explicit positive tcc: slightly more pressure (the original does the same)
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
    const auto lf = grt::LoadFile::read(QStringLiteral(GRT_TEST_DATA_DIR "/8x57is.grtload"));
    QVERIFY(lf);
    const auto pmax = [&](double ba, double xe, double pc) {
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
    auto lf = grt::LoadFile::read(QStringLiteral(GRT_TEST_DATA_DIR "/8x57is.grtload"));
    QVERIFY(lf);
    lf->propellant.set(u"mc", 4.35);  // 1.06 g/cm3 > bulk density 0.89 g/cm3 (the original reports invalidData)
    const grt::SimResult r = grt::simulate(grt::SimInput::fromLoad(*lf));
    QCOMPARE(r.status, grt::SimStatus::ChargeTooHigh);
    QVERIFY(r.points.empty());
}

// The original's implicit momentum rule: 0.5*m'*(v1^2 - v0^2) = A*dx*p1, exactly, for every step.
void CoreTest::solverMatchesMomentumRule()
{
    const auto lf = grt::LoadFile::read(QStringLiteral(GRT_TEST_DATA_DIR "/8x57is.grtload"));
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
    for (std::size_t i = 1; i < r.points.size(); ++i) {
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
    auto lf = grt::LoadFile::read(QStringLiteral(GRT_TEST_DATA_DIR "/8x57is.grtload"));
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
    auto lf = grt::LoadFile::read(QStringLiteral(GRT_TEST_DATA_DIR "/8x57is.grtload"));
    QVERIFY(lf);
    const auto effective = [&](int type) {
        lf->projectile.set(u"gTailType", static_cast<double>(type));
        return grt::simulate(grt::SimInput::fromLoad(*lf)).effectiveCaseVolume_cm3;
    };
    const double flat = effective(0), hollow = effective(1), boat = effective(2);
    const double dBul = lf->projectile.number(u"Dbul"), depth = lf->projectile.number(u"gdepth");
    QVERIFY(std::abs(flat - (lf->caliber.number(u"casevol") - M_PI / 4 * dBul * dBul * depth * 1e-3)) < 1e-6);
    // cone between A = 8.2 mm and B = 6.8 mm over 5.1 mm
    const double R = 4.1, r = 3.4, h = 5.1;
    const double cone = M_PI * h / 3.0 * (R * R + R * r + r * r) * 1e-3;       // cm3
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
    auto lf = grt::LoadFile::read(QStringLiteral(GRT_TEST_DATA_DIR "/8x57is.grtload"));
    QVERIFY(lf);
    const double mc = lf->propellant.number(u"mc");
    lf->propellant.set(u"laddercnt", 4.0);
    lf->propellant.set(u"laddermc", 0.2);
    const auto ladder = grt::simulateLadder(grt::SimInput::fromLoad(*lf));
    QCOMPARE(ladder.size(), std::size_t{4});
    for (std::size_t i = 0; i < ladder.size(); ++i) {
        QVERIFY(ladder[i].result.ok());
        QVERIFY(std::abs(ladder[i].charge_g - (mc - 0.2 * static_cast<double>(i))) < 1e-12);
    }
    // less powder, less pressure and less speed
    for (std::size_t i = 1; i < ladder.size(); ++i) {
        QVERIFY(ladder[i].result.maxPressure_bar < ladder[i - 1].result.maxPressure_bar);
        QVERIFY(ladder[i].result.muzzleVelocity_ms < ladder[i - 1].result.muzzleVelocity_ms);
    }
    // step 0 is exactly the single simulation of the selected charge
    const grt::SimResult single = grt::simulate(grt::SimInput::fromLoad(*lf));
    QCOMPARE(ladder[0].result.maxPressure_bar, single.maxPressure_bar);
}

void CoreTest::ladderEdgeCases()
{
    auto lf = grt::LoadFile::read(QStringLiteral(GRT_TEST_DATA_DIR "/8x57is.grtload"));
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
}

// Every load the original rejected as "charge too high" is rejected here, and no accepted load is.
void CoreTest::chargeRuleMatchesTheOriginal()
{
    const QDir results(QStringLiteral(GRT_SOURCE_DIR "/reference/results"));
    const QDir jobs(QStringLiteral(GRT_SOURCE_DIR "/tools/GRTRef/jobs_all"));
    int rejected = 0, accepted = 0;
    for (const QFileInfo &fi : results.entryInfoList({u"exp_*.json"_s}, QDir::Files)) {
        const QString name = fi.completeBaseName().mid(4);
        const auto ref = grt::readReferenceRun(fi.absoluteFilePath());
        const auto lf = grt::LoadFile::read(jobs.filePath(u"exp_%1.grtload"_s.arg(name)));
        if (!ref || !lf || ref->stale) continue;
        const bool tooHigh = ref->summary.value(u"PropellantChargeTooHigh"_s).toString() == u"true"_s;
        if (ref->points.empty() && !tooHigh) continue;  // e.g. the first job of a batch, read too early
        const grt::SimResult r = grt::simulate(grt::SimInput::fromLoad(*lf));
        if (tooHigh) {
            QVERIFY2(r.status == grt::SimStatus::ChargeTooHigh, qPrintable(name));
            ++rejected;
        } else {
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
    QTest::addColumn<double>("pTol");   // relative tolerance on max pressure
    QTest::addColumn<double>("vTol");   // relative tolerance on muzzle velocity
    // Provisional model: tolerances are loose and meant to be tightened as the solver improves.
    for (const char *n : {"base", "mc_x0.8", "mc_x1.2", "mp_x0.5", "mp_x2.0", "sebert_0.0", "sebert_1.0", "xe_400", "xe_1000", "pw_562", "pw_599", "pw_675"})
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
    QVERIFY2(db.open(dbPath, true, &err), qPrintable(err));
    QVERIFY(db.count(grt::Table::Propellant) > 600);
    QVERIFY(db.count(grt::Table::Projectile) > 5000);
    QVERIFY(db.count(grt::Table::Caliber) > 500);
    const auto rec = db.record(grt::Table::Propellant, 1);
    QVERIFY(rec);
    QCOMPARE(rec->value(u"mname"_s).toString(), u"Accurate"_s);
    QVERIFY(!db.record(grt::Table::Propellant, -5));
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

namespace {

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
    const grt::LoadFile built = grt::buildLoad(*cal, *prj, *pro,
                                               {.charge_g = 2.721554219989671, .barrelLength_mm = 600.0, .seatingDepth_mm = 10.4,
                                                .twistLength_mm = 240.0, .gunWeight_kg = 3.7},
                                               db.columns(grt::Table::Caliber));
    const auto orig = grt::LoadFile::read(QStringLiteral(GRT_TEST_DATA_DIR "/8x57is.grtload"));
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
    QCOMPARE(QString(built.projectile.text(u"ProjectileName")).remove(u'#'), QString(orig->projectile.text(u"ProjectileName")).remove(u'#'));
    for (const char16_t *k : {u"Ba", u"Qex", u"k", u"a0", u"z1", u"z2", u"pc", u"pcd", u"mc", u"laddermc"})
        QCOMPARE(built.propellant.number(k), orig->propellant.number(k));

    // same set and order of fields as the original program writes
    for (const auto sel : {&grt::LoadFile::caliber, &grt::LoadFile::gun, &grt::LoadFile::projectile, &grt::LoadFile::propellant}) {
        const auto &a = (built.*sel).entries();
        const auto &b = ((*orig).*sel).entries();
        QCOMPARE(a.size(), b.size());
        for (std::size_t i = 0; i < a.size(); ++i) {
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
    const auto row = [](const char *name, Quantity q, const char *unit, double base, double shown) {
        QTest::newRow(name) << static_cast<int>(q) << QString::fromLatin1(unit) << base << shown;
    };
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

namespace {

// The 24 bytes of a PNG that the report engine reads: signature, IHDR chunk length and tag, width, height.
QByteArray fakePng(int w, int h)
{
    QByteArray b("\x89PNG\r\n\x1a\n\0\0\0\x0dIHDR", 16);
    for (int v : {w, h}) {
        b.append(static_cast<char>((v >> 24) & 0xff));
        b.append(static_cast<char>((v >> 16) & 0xff));
        b.append(static_cast<char>((v >> 8) & 0xff));
        b.append(static_cast<char>(v & 0xff));
    }
    return b;
}

struct Records {
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
    r.load = grt::buildLoad(*r.cal, *r.prj, *r.pro, {.charge_g = 2.5, .barrelLength_mm = 650.0, .seatingDepth_mm = 9.0, .twistLength_mm = 240.0,
                                                      .gunWeight_kg = 3.7, .powderTemperature_C = 15.0},
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
    QCOMPARE(lf.propellant.number(u"mc"), 2.5);            // the charge is kept
    QCOMPARE(lf.propellant.number(u"pt"), 15.0);           // and the temperature
    QCOMPARE(lf.gun.number(u"xe"), 650.0);                 // gun data untouched
    QCOMPARE(lf.projectile.number(u"gdepth"), 9.0);        // so is the bullet
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
    const auto other = findRecord(r.db, grt::Table::Projectile, u"caliber = '.323' AND gmass > 150 AND id <> %1"_s.arg(r.prj->value(u"id"_s).toInt()));
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
    QCOMPARE(kept.gun.number(u"xe"), 650.0);                // nothing else changes without the reset
    QCOMPARE(kept.propellant.number(u"mc"), 2.5);
    QCOMPARE(kept.projectile.number(u"gdepth"), 9.0);
    QVERIFY(kept.title.startsWith(u".45 Auto"_s));

    grt::LoadFile reset = r.load;
    grt::applyCaliber(reset, *auto45, r.db.columns(grt::Table::Caliber), true, &*r.prj, &*r.pro);
    QCOMPARE(reset.gun.number(u"xe"), 150.0);              // pistol barrel
    QVERIFY(std::abs(reset.projectile.number(u"gdepth") - grt::defaultSeatingDepth_mm(*auto45, *r.prj)) < 1e-9);
    QVERIFY(std::abs(reset.propellant.number(u"mc") - grt::defaultCharge_g(*auto45, *r.pro)) < 1e-12);
    QCOMPARE(reset.caliberFile.entries().size(), r.load.caliberFile.entries().size());
}

void CoreTest::reportNumberFormats_data()
{
    QTest::addColumn<double>("value");
    QTest::addColumn<QString>("format");
    QTest::addColumn<QString>("expected");
    const auto row = [](const char *name, double v, const char *f, const char *e) {
        QTest::newRow(name) << v << QString::fromUtf8(f) << QString::fromUtf8(e);
    };
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
    const auto resolver = [](const QString &name) -> grt::report::ImageSource {
        return name == u"known.png"_s ? grt::report::ImageSource(u"data:image/png;base64,AAA"_s, 400, 200) : grt::report::ImageSource();
    };
    // no size given: natural size
    QVERIFY(wikiToHtml(u"{{known.png}}"_s, resolver, 1000).contains(u"<img src=\"data:image/png;base64,AAA\" width=\"400\" height=\"200\">"_s));
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
    auto lf = grt::LoadFile::read(QStringLiteral(GRT_TEST_DATA_DIR "/8x57is.grtload"));
    QVERIFY(lf);
    const grt::SimInput in = grt::SimInput::fromLoad(*lf);
    const grt::SimResult r = grt::simulate(in);
    QVERIFY(r.ok());
    const grt::report::Data d = grt::report::buildData(*lf, in, r, {QStringLiteral("/some/dir/my load.grtload")});
    const auto number = [&](const char *key) { const auto *v = d.find(QLatin1String(key)); return v ? v->number : -1e99; };
    const auto text = [&](const char *key) { const auto *v = d.find(QLatin1String(key)); return v ? v->text : QString(); };

    QCOMPARE(text("file.caption"), u"my load"_s);
    QCOMPARE(text("file.name"), u"my load.grtload"_s);
    QCOMPARE(text("work.CaliberName"), u"8 x 57 IS (8 x 57 JS)"_s);
    QCOMPARE(text("work.PropellantName"), u"Vihtavuori N150"_s);
    QCOMPARE(number("work.Dbul"), 8.2);
    QCOMPARE(number("work.xe"), 600.0);
    QCOMPARE(number("work.xeEff"), 600.0 - 46.6);                  // the projectile path
    QCOMPARE(text("work.gtailtype"), u"boattail"_s);
    QVERIFY(std::abs(number("work.gdepthc") - (10.4 - 5.1)) < 1e-9);  // guided seating depth: depth minus the boat tail
    QVERIFY(std::abs(number("work.Vb") - r.effectiveCaseVolume_cm3) < 1e-12);
    QVERIFY(std::abs(number("work.caliber") - 0.3228) < 0.001);        // inches
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
    auto lf = grt::LoadFile::read(QStringLiteral(GRT_TEST_DATA_DIR "/8x57is.grtload"));
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

QTEST_APPLESS_MAIN(CoreTest)
#include "tst_core.moc"
