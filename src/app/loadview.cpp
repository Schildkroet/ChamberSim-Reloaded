#include "loadview.h"
#include "log.h"
#include "photoview.h"

#include "cartridgeview.h"
#include "chamberview.h"
#include "curvechart.h"
#include "loadbuilder.h"
#include "numeric.h"
#include "measurementchart.h"
#include "measurementview.h"
#include "powderfitdialog.h"
#include "buttonstyle.h"
#include "propertypanel.h"
#include "reportview.h"
#include "selectrecorddialog.h"
#include "unitprefs.h"
#include "vivacitychart.h"

#include <QApplication>
#include <QBuffer>
#include <QComboBox>
#include <QPainter>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTabWidget>
#include <QScrollArea>
#include <QSettings>
#include "layoutstate.h"
#include <QSplitter>
#include <QStyle>
#include <QTabBar>
#include <QTableWidget>
#include <QHeaderView>
#include <QVBoxLayout>

#include <cmath>

using namespace Qt::StringLiterals;

namespace
{

QLabel *valueLabel()
{
    auto *l = new QLabel(QStringLiteral("–"));
    l->setTextInteractionFlags(Qt::TextSelectableByMouse);
    QFont f = l->font();
    f.setBold(true);
    l->setFont(f);
    return l;
}

void setRow(QFormLayout *form, int row, const QString &text)
{
    if (auto *item = form->itemAt(row, QFormLayout::FieldRole))
        if (auto *l = qobject_cast<QLabel *>(item->widget())) l->setText(text);
}

void setTip(QFormLayout *form, int row, const QString &tip)
{
    if (auto *item = form->itemAt(row, QFormLayout::FieldRole)) item->widget()->setToolTip(tip);
}

QString num(double v, int decimals)
{
    return QLocale().toString(v, 'f', decimals);
}

// "value symbol" in the user's unit for the quantity.
QString shown(double base, grt::units::Quantity q, int decimals = -1)
{
    const grt::units::Unit &u = UnitPreferences::instance().unitFor(q);
    return grt::units::format(base, u, decimals) + u' ' + QString::fromUtf8(u.symbol);
}

// A light, paper-like palette: reports and printouts must not depend on the dark theme.
QPalette printPalette()
{
    QPalette p;
    p.setColor(QPalette::Window, Qt::white);
    p.setColor(QPalette::Base, Qt::white);
    p.setColor(QPalette::AlternateBase, QColor(0xf0, 0xf0, 0xf0));
    p.setColor(QPalette::WindowText, Qt::black);
    p.setColor(QPalette::Text, Qt::black);
    p.setColor(QPalette::PlaceholderText, QColor(0x70, 0x70, 0x70));
    return p;
}

// Renders a widget offscreen (it is never shown on screen) and returns it as an image.
QImage renderOffscreen(QWidget &w, QSize size)
{
    w.setPalette(printPalette());
    w.setAttribute(Qt::WA_DontShowOnScreen);
    w.resize(size);
    w.show();
    QApplication::processEvents();
    const QImage img = w.grab().toImage();
    w.hide();
    return img;
}

QByteArray pngBytes(const QImage &img)
{
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    img.save(&buffer, "PNG");
    return bytes;
}

}  // namespace

LoadView::LoadView(QWidget *parent)
    : QWidget(parent)
    , m_inputTabs(new QTabWidget)
    , m_cartridgePanel(new PropertyPanel)
    , m_resultTabs(new QTabWidget)
    , m_report(new ReportView)
    , m_measurements(new MeasurementView)
    , m_drawings(new QTabWidget)
    , m_cartridge(new CartridgeView)
    , m_chamber(new ChamberView)
    , m_projectilePanel(new PropertyPanel)
    , m_propellantPanel(new PropertyPanel)
    , m_photo(new PhotoView)
    , m_heading(new QLabel)
    , m_status(new QLabel)
    , m_leftResults(new QFormLayout)
    , m_rightResults(new QFormLayout)
    , m_comparison(new QLabel)
    , m_vivacity(new VivacityChart)
    , m_chart(new CurveChart)
    , m_xAxis(new QComboBox)
    , m_ladderTabs(new QTabBar)
    , m_ladderTable(new QTableWidget(0, 5))
{
    m_heading->setTextFormat(Qt::RichText);
    m_heading->setWordWrap(true);
    m_status->setWordWrap(true);
    m_comparison->setTextFormat(Qt::RichText);

    for (const QString &label : {tr("Maximum pressure"), tr("Muzzle pressure"), tr("Projectile energy"), tr("Muzzle velocity"),
                                 tr("Barrel time (BT)"), tr("Burnout pressure")})
        m_leftResults->addRow(label + u':', valueLabel());
    for (const QString &label : {tr("Projectile speed"), tr("Recoil energy"), tr("IPSC / MIP factor"), tr("Load ratio"),
                                 tr("Burnt propellant"), tr("Effective efficiency")})
        m_rightResults->addRow(label + u':', valueLabel());
    m_leftResults->setLabelAlignment(Qt::AlignLeft);
    m_rightResults->setLabelAlignment(Qt::AlignLeft);

    auto *results = new QHBoxLayout;
    results->addLayout(m_leftResults);
    results->addSpacing(24);
    results->addLayout(m_rightResults);
    results->addStretch(1);

    auto *infoCol = new QVBoxLayout;
    infoCol->addWidget(m_heading);
    infoCol->addWidget(m_status);
    infoCol->addLayout(results);
    m_ladderTable->setHorizontalHeaderLabels({tr("Charge"), tr("Max. pressure"), tr("Velocity"), tr("Energy"), tr("Load ratio")});
    m_ladderTable->verticalHeader()->hide();
    m_ladderTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_ladderTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_ladderTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_ladderTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_ladderTable->setMaximumHeight(170);
    m_ladderTable->hide();
    infoCol->addWidget(m_ladderTable);
    infoCol->addWidget(m_comparison);
    infoCol->addStretch(1);

    auto *top = new QHBoxLayout;
    top->addLayout(infoCol, 1);
    top->addWidget(m_vivacity);

    m_xAxis->addItem(tr("Time axis"), static_cast<int>(CurveChart::XAxis::Time));
    m_xAxis->addItem(tr("Position axis"), static_cast<int>(CurveChart::XAxis::Position));
    auto *chartBar = new QHBoxLayout;
    m_ladderTabs->setExpanding(false);
    m_ladderTabs->setDrawBase(false);
    m_ladderTabs->setToolTip(tr("Load development ladder: click a charge to select its curve"));
    m_ladderTabs->hide();
    chartBar->addWidget(m_ladderTabs);
    chartBar->addStretch(1);
    chartBar->addWidget(m_xAxis);

    auto *info = new QWidget;
    info->setLayout(top);
    // larger fonts need more room: the info area scrolls instead of clipping its content
    auto *infoScroll = new QScrollArea;
    infoScroll->setWidgetResizable(true);
    infoScroll->setFrameShape(QFrame::NoFrame);
    infoScroll->setWidget(info);
    auto *chartArea = new QWidget;
    auto *cl = new QVBoxLayout(chartArea);
    cl->setContentsMargins(0, 0, 0, 0);
    cl->addLayout(chartBar);
    cl->addWidget(m_chart, 1);
    auto *right = new QSplitter(Qt::Vertical);
    right->addWidget(infoScroll);
    right->addWidget(chartArea);
    right->setStretchFactor(0, 0);
    right->setStretchFactor(1, 1);
    right->setSizes({430, 440});
    rememberSplitter(right, QStringLiteral("results"));

    // input tabs: each page shows the current item with a button to pick another record from the database
    const std::array<QString, 3> tabTitles{tr("Cartridge && gun"), tr("Projectile"), tr("Propellant")};
    const std::array<PropertyPanel *, 3> panels{m_cartridgePanel, m_projectilePanel, m_propellantPanel};
    for (PropertyPanel *p : panels) p->rememberColumns(QStringLiteral("properties"));  // one width for all three tabs
    for (std::size_t i = 0; i < 3; ++i)
    {
        m_itemLabels[i] = new QLabel;
        m_itemLabels[i]->setWordWrap(true);
        QFont f = m_itemLabels[i]->font();
        f.setBold(true);
        m_itemLabels[i]->setFont(f);
        m_selectButtons[i] = new QPushButton(tr("Select from database…"));
        m_selectButtons[i]->setIcon(style()->standardIcon(QStyle::SP_FileDialogContentsView));
        markPrimary(m_selectButtons[i]);
        m_selectButtons[i]->setEnabled(false);
        m_selectButtons[i]->setToolTip(tr("No database available"));
        auto *page = new QWidget;
        auto *pl = new QVBoxLayout(page);
        pl->setContentsMargins(0, 4, 0, 0);
        auto *head = new QHBoxLayout;
        head->addWidget(m_itemLabels[i], 1);
        head->addWidget(m_selectButtons[i]);
        pl->addLayout(head);
        pl->addWidget(panels[i], 1);
        m_inputTabs->addTab(page, tabTitles[i]);
        connect(m_selectButtons[i], &QPushButton::clicked, this, [this, i] { chooseRecord(static_cast<Component>(i)); });
    }

    auto *leftColumn = new QSplitter(Qt::Vertical);
    leftColumn->addWidget(m_inputTabs);
    m_drawings->setDocumentMode(true);
    m_drawings->addTab(m_cartridge, tr("Cartridge"));
    m_drawings->addTab(m_chamber, tr("Chamber"));
    m_photoTab = m_drawings->addTab(m_photo, tr("Powder"));  // shown while the propellant has photos
    m_drawings->setTabVisible(m_photoTab, false);
    leftColumn->addWidget(m_drawings);
    leftColumn->setStretchFactor(0, 3);
    leftColumn->setStretchFactor(1, 2);
    leftColumn->setChildrenCollapsible(false);
    leftColumn->setMinimumWidth(360);
    rememberSplitter(leftColumn, QStringLiteral("left"));

    auto *split = new QSplitter(Qt::Horizontal);
    split->addWidget(leftColumn);
    m_resultTabs->setDocumentMode(true);
    m_resultTabs->addTab(right, tr("Results"));
    m_resultTabs->addTab(m_measurements, tr("Measurements"));
    m_resultTabs->addTab(m_report, tr("Report"));
    m_resultTabs->setCurrentIndex(QSettings().value(u"layout/resultTab"_s, 0).toInt());
    connect(m_resultTabs, &QTabWidget::currentChanged, this, [](int i) { QSettings().setValue(u"layout/resultTab"_s, i); });
    split->addWidget(m_resultTabs);
    split->setStretchFactor(0, 0);
    split->setStretchFactor(1, 1);
    split->setSizes({420, 1200});
    rememberSplitter(split, QStringLiteral("main"));
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(split);

    for (PropertyPanel *p : {m_cartridgePanel, m_projectilePanel, m_propellantPanel})
        connect(p, &PropertyPanel::valueEdited, this, &LoadView::onValueEdited);
    connect(&UnitPreferences::instance(), &UnitPreferences::changed, this,
            [this]
            {
                m_cartridge->update();
                m_chamber->update();
                applyChartUnits();
                updateLadderUi();
                showResult();
            });
    applyChartUnits();
    m_report->setDataProvider([this] { return reportData(); });
    connect(m_measurements, &MeasurementView::edited, this, &LoadView::storeMeasurement);
    connect(m_measurements, &MeasurementView::seriesChanged, this, [this] { m_report->invalidate(); });
    connect(m_measurements, &MeasurementView::fitRequested, this, &LoadView::fitPowder);
    connect(m_ladderTabs, &QTabBar::currentChanged, this, [this](int i) { setFocus(i); });
    connect(m_ladderTable, &QTableWidget::cellClicked, this, [this](int row, int) { setFocus(row); });
    connect(m_xAxis, &QComboBox::currentIndexChanged, this,
            [this] { m_chart->setXAxis(static_cast<CurveChart::XAxis>(m_xAxis->currentData().toInt())); });
}

void LoadView::showInputTab(int index)
{
    if (index >= 0 && index < m_inputTabs->count()) m_inputTabs->setCurrentIndex(index);
}

void LoadView::showResultTab(int index)
{
    if (index >= 0 && index < m_resultTabs->count()) m_resultTabs->setCurrentIndex(index);
}

QString LoadView::title() const
{
    if (!m_file.title.isEmpty()) return m_file.title;
    return QFileInfo(m_path).completeBaseName();
}

grt::EntryList &LoadView::section(int id)
{
    switch (id)
    {
    case Caliber: return m_file.caliber;
    case Gun: return m_file.gun;
    case Projectile: return m_file.projectile;
    default: return m_file.propellant;
    }
}

bool LoadView::load(const QString &path, QString *error)
{
    auto lf = grt::LoadFile::read(path, error);
    if (!lf)
    {
        LOG_Warn() << "Cannot read load" << path << ":" << (error ? *error : QString());
        return false;
    }
    LOG_Info() << "Load opened:" << path;
    m_file = std::move(*lf);
    m_path = path;
    rebuildPanels();
    loadMeasurement();
    recompute();
    setModified(false);
    return true;
}

void LoadView::setNewLoad(grt::LoadFile file)
{
    m_file = std::move(file);
    m_path.clear();
    rebuildPanels();
    loadMeasurement();
    recompute();
    setModified(true);
}

bool LoadView::save(const QString &path, QString *error)
{
    if (!m_file.write(path, error))
    {
        LOG_Error() << "Cannot write load" << path << ":" << (error ? *error : QString());
        return false;
    }
    LOG_Info() << "Load saved:" << path;
    m_path = path;
    setModified(false);
    return true;
}

void LoadView::setReference(std::optional<grt::ReferenceRun> ref)
{
    LOG_Debug() << "Reference overlay" << (ref ? "set" : "cleared");
    m_reference = std::move(ref);
    m_chart->setReference(m_reference ? std::span<const grt::SimPoint>(m_reference->points) : std::span<const grt::SimPoint>{});
    showResult();
}

void LoadView::rebuildPanels()
{
    updateHeaders();
    m_cartridgePanel->setSections(
        {{Caliber, u"caliber"_s, tr("Caliber / cartridge"), &m_file.caliber}, {Gun, u"gun"_s, tr("Gun"), &m_file.gun}});
    m_projectilePanel->setSections({{Projectile, u"projectile"_s, tr("Projectile"), &m_file.projectile}});
    m_propellantPanel->setSections({{Propellant, u"propellant"_s, tr("Propellant"), &m_file.propellant}});
}

namespace
{

// A record-like map from a section's entries (names match the database columns where it matters:
// glen, pcd, ...), so defaults can be computed for the load's current components.
QVariantMap recordOf(const grt::EntryList &list)
{
    QVariantMap m;
    for (const grt::Entry &e : list.entries()) m.insert(e.name, e.value);
    return m;
}

}  // namespace

QVariantMap LoadView::caliberRecord() const
{
    return recordOf(m_file.caliberFile);
}

void LoadView::setDatabase(const grt::Database *db)
{
    m_db = db;
    m_photo->clear();  // the pictures come from the database
    updatePhoto();
    const bool on = db && db->isOpen();
    for (QPushButton *b : m_selectButtons)
    {
        b->setEnabled(on);
        b->setToolTip(on ? tr("Replace this component by another record of the database") : tr("No database available"));
    }
}

void LoadView::updateHeaders()
{
    m_itemLabels[0]->setText(m_file.caliber.text(u"CaliberName"));
    m_itemLabels[1]->setText(m_file.projectile.text(u"ProjectileName"));
    const auto &p = m_file.propellant;
    QString lot = p.text(u"lotid");
    m_itemLabels[2]->setText(
        QStringLiteral("%1 %2%3").arg(p.text(u"mname"), p.text(u"pname"), lot.isEmpty() ? QString() : QStringLiteral(" (%1)").arg(lot)));
    updatePhoto();
}

// The propellant's photos (imageUuid / imageDetailUuid of its record) from the image table of the database.
void LoadView::updatePhoto()
{
    const QString overview = m_file.propellant.text(u"imageUuid"), detail = m_file.propellant.text(u"imageDetailUuid");
    const QString key = overview + u'|' + detail;
    const auto showTab = [this] { m_drawings->setTabVisible(m_photoTab, m_photo->hasPhotos()); };
    if (!m_db || !m_db->isOpen() || (overview.isEmpty() && detail.isEmpty()))
    {
        m_photo->clear();
        showTab();
        return;
    }
    if (key == m_photo->key()) return;
    const auto load = [this](const QString &uuid)
    {
        PhotoView::Photo p;
        if (const auto img = m_db->image(uuid))
        {
            p.image = QImage::fromData(img->data);
            p.name = img->name;
            if (img->scaleUnit == u"px/mm") p.pxPerMm = img->scale;
        }
        return p;
    };
    m_photo->setPhotos(load(overview), load(detail), key);
    showTab();
}

bool LoadView::applyRecord(Component component, int id, bool resetDependent)
{
    if (!m_db || !m_db->isOpen()) return false;
    constexpr std::array tables{grt::Table::Caliber, grt::Table::Projectile, grt::Table::Propellant};
    const auto rec = m_db->record(tables[static_cast<std::size_t>(component)], id);
    if (!rec) return false;
    const QVariantMap cal = caliberRecord();
    switch (component)
    {
    case Component::Caliber:
    {
        const QVariantMap prj = recordOf(m_file.projectile), pro = recordOf(m_file.propellant);
        grt::applyCaliber(m_file, *rec, m_db->columns(grt::Table::Caliber), resetDependent, &prj, &pro);
        break;
    }
    case Component::Projectile: grt::applyProjectile(m_file, *rec, resetDependent, &cal); break;
    case Component::Propellant: grt::applyPropellant(m_file, *rec, resetDependent, &cal); break;
    }
    rebuildPanels();
    recompute();
    setModified(true);
    return true;
}

void LoadView::chooseRecord(Component component)
{
    if (!m_db || !m_db->isOpen()) return;
    constexpr std::array tables{grt::Table::Caliber, grt::Table::Projectile, grt::Table::Propellant};
    const QString titles[] = {tr("Select caliber"), tr("Select projectile"), tr("Select propellant")};
    const auto idx = static_cast<std::size_t>(component);
    SelectRecordDialog dlg(*m_db, tables[idx], titles[idx], this);
    switch (component)
    {
    case Component::Caliber:
        dlg.setResetOption(tr("Also set barrel length, seating depth and charge to the defaults of the new caliber"), true);
        break;
    case Component::Projectile:
    {
        const double dz = m_file.caliber.number(u"Dz");
        if (dz > 0.0) dlg.setRestriction(QStringLiteral("ABS(gdia - %1) < 0.25").arg(dz), tr("Show only bullets that fit the caliber"));
        dlg.setResetOption(tr("Also set the seating depth to the default (maximum cartridge length)"), false);
        break;
    }
    case Component::Propellant: dlg.setResetOption(tr("Also set the charge to the default (70 % of the case capacity)"), false); break;
    }
    if (dlg.exec() != QDialog::Accepted || dlg.selectedId() < 0) return;
    if (!applyRecord(component, dlg.selectedId(), dlg.resetChecked()))
        QMessageBox::warning(this, tr("Select"), tr("The record could not be read from the database."));
}

void LoadView::onValueEdited(int sectionId, const QString &key, const QString &text)
{
    section(sectionId).set(key, text);
    setModified(true);
    recompute();
}

void LoadView::recompute()
{
    m_input = grt::SimInput::fromLoad(m_file);
    m_ladder = grt::simulateLadder(m_input);
    m_focus = grt::toIntClamped(m_file.config.number(u"CurveFocusIdx", 0.0), 0, static_cast<int>(m_ladder.size()) - 1);
    m_result = m_ladder[static_cast<std::size_t>(m_focus)].result;
    LOG_Debug() << "Simulated" << title() << ":" << m_ladder.size() << "ladder step(s), focus" << m_focus << ", pmax"
                << m_result.maxPressure_bar << "bar, v0" << m_result.muzzleVelocity_ms << "m/s";
    if (!m_result.ok()) LOG_Warn() << "Simulation of" << title() << "failed (charge too high, input out of range?)";
    m_focusInput = m_input;
    m_focusInput.charge_g = m_ladder[static_cast<std::size_t>(m_focus)].charge_g;

    m_chart->setResult(m_result, m_input.maxPressure_bar);
    std::vector<grt::SimResult> others;
    for (std::size_t i = 0; i < m_ladder.size(); ++i)
        if (static_cast<int>(i) != m_focus) others.push_back(m_ladder[i].result);
    m_chart->setLadder(std::move(others));
    m_cartridge->setLoad(m_file, m_focusInput);
    m_chamber->setLoad(m_file, m_focusInput);
    if (m_input.vivacityBa > 0.0) m_vivacity->setCurve(grt::vivacityCurve(m_input), m_input.z1, m_input.z2);
    else m_vivacity->clear();
    m_measurements->setPredictor(
        [in = m_input](double charge_g)
        {
            grt::SimInput i = in;
            i.charge_g = charge_g;
            return grt::simulate(i);
        });
    std::vector<double> charges{m_focusInput.charge_g};
    for (const grt::LadderStep &step : m_ladder)
        if (std::abs(step.charge_g - m_focusInput.charge_g) > 1e-9) charges.push_back(step.charge_g);
    m_measurements->setSuggestedCharges(std::move(charges));
    updateLadderUi();
    showResult();
    m_report->invalidate();
}

void LoadView::fitPowder()
{
    QStringList warnings;
    const QString source = m_path.isEmpty() ? title() : QFileInfo(m_path).fileName();
    const std::vector<grt::powderfit::Sample> samples = grt::powderfit::samples(m_file, source, &warnings);
    if (samples.empty())
    {
        QMessageBox::information(this, tr("Fit to measurements"), tr("Enter the measured velocity of the shots first."));
        return;
    }
    PowderFitDialog dlg(samples, warnings, this);
    if (m_saveRecord && m_db && m_db->isOpen())
    {
        dlg.setSaveHandler(
            [this, &samples](const grt::powderfit::Result &r)
            {
                QVariantMap record = grt::powderfit::toRecord(m_file.propellant, r, m_db->schema(grt::Table::Propellant),
                                                              grt::powderfit::describe(r, samples));
                record.insert(u"pname"_s, record.value(u"pname"_s).toString() + tr(" (fitted)"));
                return m_saveRecord(grt::Table::Propellant, record) >= 0;
            });
    }
    dlg.runFit();
    if (dlg.exec() != QDialog::Accepted || !dlg.result() || !dlg.result()->improved) return;
    LOG_Info() << "Powder fit applied to" << title();
    grt::powderfit::apply(m_file.propellant, *dlg.result());
    rebuildPanels();
    recompute();
    setModified(true);
}

void LoadView::loadMeasurement()
{
    auto series = grt::readMeasurements(m_file.appendixXml);
    m_hasMeasurement = !series.empty();
    m_measurements->setMeasurements(std::move(series));
}

// The measurement tab edits the appendix of the file; a load without measurements gets a block only once there is
// something in it.
void LoadView::storeMeasurement()
{
    const auto &series = m_measurements->allMeasurements();
    const bool empty = std::ranges::all_of(series, [](const grt::Measurement &m) { return m.charges.empty(); });
    if (!m_hasMeasurement && empty) return;
    m_file.appendixXml = grt::writeMeasurements(m_file.appendixXml, series);
    m_hasMeasurement = true;
    setModified(true);
    m_report->invalidate();
}

// Selecting another ladder curve: remembered in the file (CurveFocusIdx) like the original does.
void LoadView::setFocus(int index)
{
    if (index < 0 || index >= static_cast<int>(m_ladder.size()) || index == m_focus) return;
    m_file.config.set(u"CurveFocusIdx", static_cast<double>(index));
    if (m_file.config.find(u"CurveFocusIdx") && m_file.config.find(u"CurveFocusIdx")->type.isEmpty())
    {
        for (grt::Entry &e : m_file.config.entries())
            if (e.name == u"CurveFocusIdx")
            {
                e.type = u"integer"_s;
                e.descr = u"selected ladder curve for display"_s;
                e.value = QString::number(index);
            }
    }
    setModified(true);
    recompute();
}

void LoadView::updateLadderUi()
{
    using grt::units::Quantity;
    const bool ladder = m_ladder.size() >= 2;
    m_ladderTabs->setVisible(ladder);
    m_ladderTable->setVisible(ladder);
    if (!ladder) return;

    QSignalBlocker block(m_ladderTabs);
    while (m_ladderTabs->count() > 0) m_ladderTabs->removeTab(0);
    m_ladderTable->setRowCount(0);
    for (std::size_t i = 0; i < m_ladder.size(); ++i)
    {
        const grt::LadderStep &s = m_ladder[i];
        m_ladderTabs->addTab(shown(s.charge_g, Quantity::BulletMass));
        const int row = m_ladderTable->rowCount();
        m_ladderTable->insertRow(row);
        const auto cell = [&](int col, const QString &text)
        {
            auto *item = new QTableWidgetItem(text);
            item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            m_ladderTable->setItem(row, col, item);
        };
        cell(0, shown(s.charge_g, Quantity::BulletMass));
        if (s.result.ok())
        {
            cell(1, shown(s.result.maxPressure_bar, Quantity::Pressure));
            cell(2, shown(s.result.muzzleVelocity_ms, Quantity::Velocity));
            cell(3, shown(s.result.muzzleEnergy_J, Quantity::Energy));
            cell(4, QStringLiteral("%1 %").arg(num(100.0 * s.result.loadRatio, 1)));
        }
        else
        {
            cell(1, tr("charge too high"));
            for (int c = 2; c < 5; ++c) cell(c, QStringLiteral("–"));
        }
    }
    m_ladderTabs->setCurrentIndex(m_focus);
    m_ladderTable->selectRow(m_focus);
}

grt::report::Data LoadView::reportData() const
{
    using grt::report::Value;
    grt::report::Data d = grt::report::buildData(m_file, m_focusInput, m_result, {m_path});
    if (!m_result.ok()) return d;

    // diagrams, rendered like the on-screen ones but on white paper
    const UnitPreferences &u = UnitPreferences::instance();
    using grt::units::Quantity;
    CurveChart curves;
    curves.setUnits(u.unitFor(Quantity::Pressure), u.unitFor(Quantity::Energy), u.unitFor(Quantity::Velocity), u.unitFor(Quantity::Length));
    curves.setResult(m_result, m_input.maxPressure_bar);
    std::vector<grt::SimResult> others;
    for (std::size_t i = 0; i < m_ladder.size(); ++i)
        if (static_cast<int>(i) != m_focus) others.push_back(m_ladder[i].result);
    curves.setLadder(std::move(others));
    const QImage curvesImage = renderOffscreen(curves, {1100, 560});

    VivacityChart burn;
    if (m_input.vivacityBa > 0.0) burn.setCurve(grt::vivacityCurve(m_input), m_input.z1, m_input.z2);
    const QImage burnImage = renderOffscreen(burn, {1100, 300});

    QImage stacked(1100, curvesImage.height() + burnImage.height(), QImage::Format_ARGB32);
    stacked.fill(Qt::white);
    QPainter p(&stacked);
    p.drawImage(0, 0, curvesImage);
    p.drawImage(0, curvesImage.height(), burnImage);
    p.end();
    d.set(u"result.diagramm.png"_s, Value::ofImage(pngBytes(stacked)));
    d.set(u"result.diagramCurves.png"_s, Value::ofImage(pngBytes(curvesImage)));
    d.set(u"result.diagramPropellant.png"_s, Value::ofImage(pngBytes(burnImage)));

    CartridgeView cartridge;
    cartridge.setTableVisible(false);
    cartridge.setLoad(m_file, m_focusInput);
    d.set(u"caliber.drawing.png"_s, Value::ofImage(pngBytes(renderOffscreen(cartridge, {420, 760}))));
    ChamberView chamber;
    chamber.setTableVisible(false);
    chamber.setLoad(m_file, m_focusInput);
    d.set(u"caliber.chamber.png"_s, Value::ofImage(pngBytes(renderOffscreen(chamber, {420, 760}))));
    addMeasurementReport(d);
    return d;
}

// "measurement.section": heading, a table of the measured charges against the simulation and the chart, as report
// markup in the user's units; "measurement.chart.png" alone for custom templates.
void LoadView::addMeasurementReport(grt::report::Data &d) const
{
    using grt::report::Value;
    using grt::units::Quantity;
    const grt::Measurement &m = m_measurements->measurement();
    if (m.charges.empty()) return;
    const UnitPreferences &u = UnitPreferences::instance();
    const auto sym = [&](Quantity q) { return QString::fromUtf8(u.unitFor(q).symbol); };
    const auto val = [&](double base, Quantity q, int decimals) { return grt::units::format(base, u.unitFor(q), decimals); };
    const auto cell = [](QString text) { return text.replace(u'|', u'/').replace(u'^', u' '); };

    std::vector<MeasurementChart::Prediction> curve;
    QString table = tr("^ Charge ^ Name ^ Shots ^ Velocity ^ SD ^ ES ^ Simulated ^ Δ ^ Pressure ^ Simulated ^ Δ ^") + u'\n';
    for (const grt::MeasuredCharge &c : m.charges)
    {
        grt::SimInput in = m_input;
        in.charge_g = c.charge_g();
        const grt::SimResult r = grt::simulate(in);
        if (r.ok()) curve.push_back({.charge_g = c.charge_g(), .velocity_ms = r.muzzleVelocity_ms, .pressure_bar = r.maxPressure_bar});
        const grt::ShotStatistics v = grt::velocityStatistics(c);
        const grt::ShotStatistics p = grt::pressureStatistics(c);
        const auto delta = [](double sim, double meas) {
            return sim > 0.0 && meas > 0.0 ? QStringLiteral("%1 %").arg(QLocale().toString(100.0 * (sim / meas - 1.0), 'f', 1)) : QString();
        };
        table += QStringLiteral("| %1 %2 | %3 | %4 | %5 | %6 | %7 | %8 | %9 |")
                     .arg(val(c.charge_g(), Quantity::BulletMass, -1), sym(Quantity::BulletMass), cell(c.name), QString::number(v.count),
                          v.count ? val(v.mean, Quantity::Velocity, 1) + u' ' + sym(Quantity::Velocity) : QString(),
                          v.count > 1 ? val(v.sd, Quantity::Velocity, 1) : QString(),
                          v.count > 1 ? val(v.extremeSpread(), Quantity::Velocity, 1) : QString(),
                          r.ok() ? val(r.muzzleVelocity_ms, Quantity::Velocity, 1) : QString(), delta(r.muzzleVelocity_ms, v.mean)) +
                 QStringLiteral(" %1 | %2 | %3 |\n")
                     .arg(p.count ? val(p.mean, Quantity::Pressure, 0) + u' ' + sym(Quantity::Pressure) : QString(),
                          r.ok() && p.count ? val(r.maxPressure_bar, Quantity::Pressure, 0) : QString(),
                          p.count ? delta(r.maxPressure_bar, p.mean) : QString());
    }
    std::ranges::sort(curve, {}, &MeasurementChart::Prediction::charge_g);
    MeasurementChart chart;
    chart.setUnits(u.unitFor(Quantity::BulletMass), u.unitFor(Quantity::Velocity), u.unitFor(Quantity::Pressure));
    chart.setData(m, curve);
    d.set(u"measurement.chart.png"_s, Value::ofImage(pngBytes(renderOffscreen(chart, {1100, 480}))));
    d.set(u"measurement.title"_s, Value::ofText(m.title));
    const QString title = m.title.isEmpty() ? tr("Measurements") : cell(m.title);
    d.set(u"measurement.section"_s,
          Value::ofText(QStringLiteral("===== %1 =====\n%2\n{{measurement.chart.png?100%}}\n").arg(title, table)));
}

void LoadView::applyChartUnits()
{
    using grt::units::Quantity;
    const UnitPreferences &u = UnitPreferences::instance();
    m_chart->setUnits(u.unitFor(Quantity::Pressure), u.unitFor(Quantity::Energy), u.unitFor(Quantity::Velocity),
                      u.unitFor(Quantity::Length));
}

void LoadView::setModified(bool m)
{
    if (m_modified == m) return;
    m_modified = m;
    emit modifiedChanged(m);
}

QString LoadView::headingHtml() const
{
    using grt::units::Quantity;
    const grt::SimInput &in = m_input;
    const auto &p = m_file.propellant;
    return tr("<span style='font-size:large'><b>%1</b></span><br>"
              "<b>Projectile:</b> <i>%2</i>, <b>Propellant:</b> <i>%3 %4</i>, %5<br>"
              "<b>L3/CL:</b> %6, <b>L6/OAL:</b> %7, <b>Seating depth:</b> %8, <b>Projectile length:</b> %9, "
              "<b>Case volume:</b> %10")
        .arg(m_file.caliber.text(u"CaliberName").toHtmlEscaped(), m_file.projectile.text(u"ProjectileName").toHtmlEscaped(),
             p.text(u"mname").toHtmlEscaped(), p.text(u"pname").toHtmlEscaped(), shown(m_focusInput.charge_g, Quantity::BulletMass),
             shown(in.caseLength_mm, Quantity::Length), shown(m_file.caliber.number(u"oal"), Quantity::Length),
             shown(in.seatingDepth_mm, Quantity::Length))
        .arg(shown(in.bulletLength_mm, Quantity::Length), shown(in.caseVolume_cm3, Quantity::Volume));
}

void LoadView::showResult()
{
    m_heading->setText(headingHtml());
    m_comparison->clear();
    const auto clearAll = [&]
    {
        for (QFormLayout *f : {m_leftResults, m_rightResults})
            for (int r = 0; r < f->rowCount(); ++r) setRow(f, r, QStringLiteral("–"));
    };
    if (!m_result.ok())
    {
        m_status->setText(m_result.status == grt::SimStatus::ChargeTooHigh
                              ? tr("<b>Charge too high:</b> the propellant charge does not fit into the case behind the bullet.")
                              : tr("<b>Invalid input:</b> the simulation cannot run with these values."));
        clearAll();
        return;
    }
    QString status = tr("<i>Calibrated against runs of the original program and checked against published load data: typical "
                        "deviation about 1-3 % in velocity and 2-3 % in peak pressure; slow powders at low pressure and unusual "
                        "loads can deviate more.</i>");
    // shotshells: an own model, fitted to published shotgun loads (not to the original program)
    if (m_input.shotshell)
        status += u"<br>"_s + tr("<b>Shotshell:</b> wad and shot column as the projectile (shot + wad mass, column length, "
                                 "seated inside the hull, start pressure = crimp release about 20 bar). Fitted to published "
                                 "shotgun loads: typical deviation about 6 % in velocity.");
    m_status->setText(status);

    const grt::SimResult &r = m_result;
    using grt::units::Quantity;
    setRow(m_leftResults, 0, shown(r.maxPressure_bar, Quantity::Pressure));
    setRow(m_leftResults, 1, shown(r.muzzlePressure_bar, Quantity::Pressure));
    setRow(m_leftResults, 2, shown(r.muzzleEnergy_J, Quantity::Energy));
    setRow(m_leftResults, 3, shown(r.muzzleVelocity_ms, Quantity::Velocity));
    setRow(m_leftResults, 4, tr("%1 ms").arg(num(r.muzzleTime_ms, 4)));
    setRow(m_leftResults, 5, r.burnoutInBarrel ? shown(r.burnoutPressure_bar, Quantity::Pressure) : tr("not burnt out"));
    setRow(m_rightResults, 0, r.muzzleRpm > 0 ? tr("%1 k/min").arg(num(r.muzzleRpm / 1000.0, 1)) : QStringLiteral("–"));
    setRow(m_rightResults, 1, r.recoilEnergy_J > 0 ? shown(r.recoilEnergy_J, Quantity::Energy) : tr("– (gun weight 0)"));
    setRow(m_rightResults, 2, QStringLiteral("%1, %2").arg(num(r.ipscFactor, 0), num(r.mipFactor, 0)));
    setRow(m_rightResults, 3, QStringLiteral("%1 %").arg(num(100.0 * r.loadRatio, 1)));
    setRow(m_rightResults, 4, QStringLiteral("%1 %").arg(num(100.0 * r.burntFraction, 1)));
    setRow(m_rightResults, 5, QStringLiteral("%1 %").arg(num(100.0 * r.efficiency, 1)));

    if (!m_reference) return;
    struct Row
    {
        QString label;
        double value;
        Quantity quantity;  // None: shown as is, with `unit`
        QString unit;
        int decimals;
        const char *refKey;
    };
    const std::array rows{
        Row{tr("Maximum pressure"), r.maxPressure_bar, Quantity::Pressure, {}, -1, "MaxPressure"},
        Row{tr("Muzzle pressure"), r.muzzlePressure_bar, Quantity::Pressure, {}, -1, "MuzzlePressure"},
        Row{tr("Muzzle velocity"), r.muzzleVelocity_ms, Quantity::Velocity, {}, -1, "MuzzleVelocity"},
        Row{tr("Muzzle energy"), r.muzzleEnergy_J, Quantity::Energy, {}, -1, "MuzzleEnergy"},
        Row{tr("Bullet time"), r.muzzleTime_ms, Quantity::None, tr("ms"), 3, "MuzzleTime"},
        Row{tr("Recoil impulse"), r.recoilImpulse_Ns, Quantity::Impulse, {}, -1, "RecoilImpulse"},
    };
    QString html = tr("<table cellspacing='3'><tr><th></th><th align='right'>this</th><th align='right'>original</th>"
                      "<th align='right'>Δ</th><th></th></tr>");
    for (const Row &row : rows)
    {
        const double ref = m_reference->number(QString::fromLatin1(row.refKey), std::nan(""));
        const grt::units::Unit *unit = row.quantity == Quantity::None ? nullptr : &UnitPreferences::instance().unitFor(row.quantity);
        const auto text = [&](double v) { return unit ? grt::units::format(v, *unit, row.decimals) : num(v, row.decimals); };
        html += QStringLiteral("<tr><td>%1</td><td align='right'><b>%2</b></td>").arg(row.label, text(row.value));
        if (std::isnan(ref) || ref == 0.0) html += QStringLiteral("<td></td><td></td>");
        else
            html += QStringLiteral("<td align='right'>%1</td><td align='right'>%2 %</td>")
                        .arg(text(ref), num(100.0 * (row.value / ref - 1.0), 1));
        html += QStringLiteral("<td>%1</td></tr>").arg(unit ? QString::fromUtf8(unit->symbol) : row.unit);
    }
    html += QStringLiteral("</table>");
    m_comparison->setText(html);
}
