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

#include "loadview.h"
#include "log.h"
#include "photoview.h"
#include "physicalpowders.h"
#include "primer.h"
#include "shotparts.h"
#include "solverprefs.h"

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
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTabWidget>
#include <QScrollArea>
#include <QSettings>
#include <QSqlQuery>
#include "layoutstate.h"
#include <QSplitter>
#include <QTimer>
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
QString shown(double base, csr::units::Quantity q, int decimals = -1)
{
    const csr::units::Unit &u = UnitPreferences::instance().unitFor(q);
    return csr::units::format(base, u, decimals) + u' ' + QString::fromUtf8(u.symbol);
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

// A rounded panel painted from the current palette (a style sheet with palette() would not follow a theme change).
class ResultCard : public QFrame
{
public:
    using QFrame::QFrame;

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setBrush(palette().brush(QPalette::AlternateBase));
        p.setPen(palette().color(QPalette::Mid));
        p.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), 6, 6);
    }
};

// A notice above the results in fixed colours (the same in every theme): an icon and a rich-text label; hidden until shown.
void setupBanner(QWidget *banner, QLabel *text, const QColor &background, const QColor &foreground, QStyle::StandardPixmap icon)
{
    banner->setAutoFillBackground(true);
    QPalette pal = banner->palette();
    pal.setColor(QPalette::Window, background);
    pal.setColor(QPalette::WindowText, foreground);
    banner->setPalette(pal);
    auto *iconLabel = new QLabel;
    const int size = banner->style()->pixelMetric(QStyle::PM_MessageBoxIconSize);
    iconLabel->setPixmap(banner->style()->standardIcon(icon).pixmap(size, size));
    iconLabel->setAlignment(Qt::AlignTop);
    text->setTextFormat(Qt::RichText);
    text->setWordWrap(true);
    auto *row = new QHBoxLayout(banner);
    row->setContentsMargins(8, 6, 8, 6);
    row->addWidget(iconLabel);
    row->addWidget(text, 1);
    banner->hide();
}

// The title of the dialog that replaces a component (also the name of the undo step).
QString selectTitle(LoadView::Component component)
{
    const QString titles[] = {LoadView::tr("Select Caliber"), LoadView::tr("Select Projectile"), LoadView::tr("Select Propellant")};
    return titles[static_cast<std::size_t>(component)];
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
    , m_overpressure(new QWidget)
    , m_overpressureText(new QLabel)
    , m_burnNotice(new QWidget)
    , m_burnNoticeText(new QLabel)
    , m_outdatedNotice(new QWidget)
    , m_outdatedNoticeText(new QLabel)
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

    // The four key results as large cards, the rest in three captioned groups (index = position in m_values).
    struct Entry
    {
        QString label;
        QString tip;
    };
    const std::array<Entry, 4> cards = {Entry{tr("Maximum pressure"), {}}, Entry{tr("Muzzle velocity"), {}},
                                        Entry{tr("Projectile energy"), {}}, Entry{tr("Barrel time (BT)"), {}}};
    struct Group
    {
        QString title;
        std::vector<QString> labels;
    };
    const std::array<Group, 3> groups = {
        Group{tr("Barrel"), {tr("Muzzle pressure"), tr("Burnout pressure"), tr("Bullet lead time (BLT)"), tr("Optimal barrel time (OBT)")}},
        Group{tr("Projectile"), {tr("Projectile speed"), tr("Recoil energy"), tr("IPSC / MIP factor")}},
        Group{tr("Propellant"),
              {tr("Load ratio"), tr("Burnt propellant"), tr("Effective efficiency"), tr("Heat / friction loss"),
               tr("Gas outflow velocity")}}};

    auto *cardRow = new QHBoxLayout;
    cardRow->setSpacing(8);
    for (std::size_t i = 0; i < cards.size(); ++i)
    {
        auto *card = new ResultCard;
        auto *caption = new QLabel(cards[i].label);
        QLabel *value = valueLabel();
        value->setTextFormat(Qt::RichText);
        auto *cl = new QVBoxLayout(card);
        cl->setContentsMargins(10, 6, 10, 8);
        cl->setSpacing(2);
        cl->addWidget(caption);
        cl->addWidget(value);
        m_values[i] = value;
        cardRow->addWidget(card, 1);
    }

    auto *details = new QHBoxLayout;
    details->setSpacing(28);
    std::size_t next = cards.size();
    for (const Group &g : groups)
    {
        auto *col = new QVBoxLayout;
        col->setSpacing(4);
        auto *title = new QLabel(g.title.toUpper());
        QFont tf = title->font();
        tf.setBold(true);
        tf.setPointSizeF(tf.pointSizeF() * 0.85);
        tf.setLetterSpacing(QFont::PercentageSpacing, 108);
        title->setFont(tf);
        auto *line = new QFrame;
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Plain);
        line->setForegroundRole(QPalette::Mid);
        line->setFixedHeight(2);
        auto *form = new QFormLayout;
        form->setLabelAlignment(Qt::AlignLeft);
        form->setHorizontalSpacing(14);
        form->setVerticalSpacing(5);
        for (const QString &label : g.labels)
        {
            auto *name = new QLabel(label);
            QLabel *value = valueLabel();
            m_values[next++] = value;
            form->addRow(name, value);
        }
        col->addWidget(title);
        col->addWidget(line);
        col->addLayout(form);
        col->addStretch(1);
        details->addLayout(col);
    }
    details->addStretch(1);
    auto *results = new QVBoxLayout;
    results->setSpacing(10);
    results->addLayout(cardRow);
    results->addLayout(details);

    auto *infoCol = new QVBoxLayout;
    infoCol->addWidget(m_heading);
    // the load (heading) above, the results below
    auto *separator = new QFrame;
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Plain);
    separator->setForegroundRole(QPalette::PlaceholderText);  // visible in the light and the dark theme
    infoCol->addWidget(separator);
    infoCol->addWidget(m_overpressure);  // the warning stands above the results it is about
    infoCol->addLayout(results);
    // white on red: the maximum pressure is above the permitted one
    m_overpressure->setObjectName(u"overpressureWarning"_s);
    setupBanner(m_overpressure, m_overpressureText, QColor(0xc4, 0x2b, 0x1c), Qt::white, QStyle::SP_MessageBoxWarning);
    // dark on orange: the Physical Model runs without the powder's physical burn values
    m_burnNotice->setObjectName(u"burnValuesNotice"_s);
    m_burnNoticeText->setObjectName(u"burnValuesNoticeText"_s);
    setupBanner(m_burnNotice, m_burnNoticeText, QColor(0xff, 0xb3, 0x47), QColor(0x2b, 0x1a, 0x00), QStyle::SP_MessageBoxInformation);
    m_burnNoticeText->setTextInteractionFlags(Qt::LinksAccessibleByMouse | Qt::TextSelectableByMouse);
    connect(m_burnNoticeText, &QLabel::linkActivated, this, [this] { offerPhysicalBurn(); });
    m_outdatedNotice->setObjectName(u"outdatedPowderNotice"_s);
    m_outdatedNoticeText->setObjectName(u"outdatedPowderNoticeText"_s);
    setupBanner(m_outdatedNotice, m_outdatedNoticeText, QColor(0xff, 0xb3, 0x47), QColor(0x2b, 0x1a, 0x00),
                QStyle::SP_MessageBoxInformation);
    m_outdatedNoticeText->setTextInteractionFlags(Qt::LinksAccessibleByMouse | Qt::TextSelectableByMouse);
    connect(m_outdatedNoticeText, &QLabel::linkActivated, this, [this] { offerPowderUpdate(); });
    infoCol->addWidget(m_burnNotice);
    infoCol->addWidget(m_outdatedNotice);
    infoCol->addWidget(m_status);
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
    right->setSizes({520, 380});
    rememberSplitter(right, QStringLiteral("results"));

    // input tabs: each page shows the current item with a button to pick another record from the database
    const std::array<QString, 3> tabTitles{tr("Cartridge && gun"), tr("Projectile"), tr("Propellant")};
    const std::array<QString, 3> selectTexts{tr("Select Caliber"), tr("Select Projectile"), tr("Select Propellant")};
    const std::array<PropertyPanel *, 3> panels{m_cartridgePanel, m_projectilePanel, m_propellantPanel};
    for (PropertyPanel *p : panels) p->rememberColumns(QStringLiteral("properties"));  // one width for all three tabs
    for (std::size_t i = 0; i < 3; ++i)
    {
        m_itemLabels[i] = new QLabel;
        m_itemLabels[i]->setWordWrap(true);
        QFont f = m_itemLabels[i]->font();
        f.setBold(true);
        m_itemLabels[i]->setFont(f);
        m_selectButtons[i] = new QPushButton(selectTexts[i]);
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
    connect(&SolverPreferences::instance(), &SolverPreferences::changed, this, &LoadView::recompute);
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

csr::EntryList &LoadView::section(int id)
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
    auto lf = csr::LoadFile::read(path, error);
    if (!lf)
    {
        LOG_Warn() << "Cannot read load" << path << ":" << (error ? *error : QString());
        return false;
    }
    LOG_Info() << "Load opened:" << path;
    m_file = std::move(*lf);
    m_path = path;
    // a load saved before the physical burn values existed gets those of its (unchanged) project powder; they are saved with it next time
    if (csr::attachPhysicalBurn(m_file.propellant)) LOG_Info() << "Physical burn values added to" << path;
    csr::ensureShotParts(m_file.caliber);  // a shotshell load of an older file: the entries for shell, wad and primer (not chosen)
    if (csr::ensureSebert(m_file)) LOG_Info() << "Sebert factor of" << path << "outside 0..1, set to 0.5";
    // a load without the caliber's record (from another tool): the record of the caliber it names
    if (m_db && m_db->isOpen() && m_file.caliberFile.entries().empty() && !m_file.caliber.flag(u"shotshell"))
        if (const auto rec = m_db->caliberByName(m_file.caliber.text(u"CaliberName")); rec && csr::attachCaliberRecord(m_file, *rec))
            LOG_Info() << "Caliber record of" << m_file.caliber.text(u"CaliberName") << "added to" << path;
    // a load of an older version or of the original: the primer and the additional start pressure (its start pressure is kept exactly)
    if (csr::ensurePrimer(m_file))
    {
        // the caliber's record names its usual primer (the file's copy of the record is older than that column)
        if (const auto primer = m_db ? m_db->caliberPrimer(m_file.caliberFile.text(u"cipname")) : std::nullopt;
            primer && csr::isPrimerCode(*primer) && !m_file.caliber.flag(u"shotshell"))
            m_file.caliber.set(u"primer", *primer);
        LOG_Info() << "Primer" << m_file.caliber.text(u"primer") << "added to" << path;
    }
    rebuildPanels();
    loadMeasurement();
    recompute();
    clearUndo();
    setModified(false);
    return true;
}

void LoadView::setNewLoad(csr::LoadFile file)
{
    m_file = std::move(file);
    csr::ensureShotParts(m_file.caliber);
    csr::ensurePrimer(m_file);
    m_path.clear();
    rebuildPanels();
    loadMeasurement();
    recompute();
    clearUndo();
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
    const bool wasGrtFile = isGrtFile();
    m_path = path;
    m_cleanDepth = static_cast<int>(m_undo.size());
    setModified(false);
    if (isGrtFile() != wasGrtFile && SolverPreferences::instance().originalForGrtFiles())
        recompute();  // the model may change with the format
    return true;
}

void LoadView::setReference(std::optional<csr::ReferenceRun> ref)
{
    LOG_Debug() << "Reference overlay" << (ref ? "set" : "cleared");
    m_reference = std::move(ref);
    m_chart->setReference(m_reference ? std::span<const csr::SimPoint>(m_reference->points) : std::span<const csr::SimPoint>{});
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
QVariantMap recordOf(const csr::EntryList &list)
{
    QVariantMap m;
    for (const csr::Entry &e : list.entries()) m.insert(e.name, e.value);
    return m;
}

}  // namespace

QVariantMap LoadView::caliberRecord() const
{
    return recordOf(m_file.caliberFile);
}

void LoadView::setDatabase(const csr::Database *db)
{
    m_db = db;
    m_photo->clear();  // the pictures come from the database
    updatePhoto();
    updateOutdatedNotice();  // the load may have been shown before the database was attached
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
    const auto rec = m_db->record(csr::kRecordTables[static_cast<std::size_t>(component)], id);
    if (!rec) return false;
    const QVariantMap cal = caliberRecord();
    pushUndo(selectTitle(component));
    switch (component)
    {
    case Component::Caliber:
    {
        const QVariantMap prj = recordOf(m_file.projectile), pro = recordOf(m_file.propellant);
        csr::applyCaliber(m_file, *rec, m_db->columns(csr::Table::Caliber), resetDependent, &prj, &pro);
        break;
    }
    case Component::Projectile: csr::applyProjectile(m_file, *rec, resetDependent, &cal); break;
    case Component::Propellant: csr::applyPropellant(m_file, *rec, resetDependent, &cal); break;
    }
    rebuildPanels();
    recompute();
    setModified(true);
    return true;
}

void LoadView::chooseRecord(Component component)
{
    if (!m_db || !m_db->isOpen()) return;
    SelectRecordDialog dlg(*m_db, csr::kRecordTables[static_cast<std::size_t>(component)], selectTitle(component), this);
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
    pushUndo(tr("Edit value"));
    const double previousDiameter = m_file.projectile.number(u"Dbul");
    const double previousCaseLength = m_file.caliber.number(u"caselen");
    section(sectionId).set(key, text);
    // a cartridge that becomes a shotshell gets the choices of its shell, wad and primer (and loses the metallic primer's rows)
    if (sectionId == Caliber && key == u"shotshell")
    {
        csr::ensureShotParts(m_file.caliber);
        QTimer::singleShot(0, this, &LoadView::rebuildPanels);  // after the edit is finished
    }
    // The cartridge overall length is the case length plus the projectile length minus the seating depth; the solver uses the seating
    // depth. Editing either of them (or the case or projectile length) keeps the other one in step.
    csr::EntryList &cal = m_file.caliber, &prj = m_file.projectile;
    if (sectionId == Caliber && key == u"oal") prj.set(u"gdepth", cal.number(u"caselen") + prj.number(u"glen") - cal.number(u"oal"));
    else if (sectionId == Projectile && (key == u"gdepth" || key == u"glen"))
        cal.set(u"oal", cal.number(u"caselen") + prj.number(u"glen") - prj.number(u"gdepth"));
    // a new case length: the bullet stays (overall length kept), seating depth and case volume follow
    else if (sectionId == Caliber && key == u"caselen") csr::followCaseLength(cal, prj, previousCaseLength);
    // The initial gas pressure is in both sections (the solver reads the caliber's, the projectile window of the original shows the
    // projectile's). With a primer it is 250 bar plus the additional one: editing the total sets the additional one.
    if (cal.find(u"psadd"))
    {
        if (sectionId == Caliber && key == u"ps") cal.set(u"psadd", cal.number(u"ps") - csr::kReferenceStartPressure_bar);
        else if (sectionId == Projectile && key == u"gpressure")
            cal.set(u"psadd", prj.number(u"gpressure") - csr::kReferenceStartPressure_bar);
        csr::updateStartPressure(m_file);
    }
    else if (sectionId == Caliber && key == u"ps") prj.set(u"gpressure", cal.number(u"ps"));
    else if (sectionId == Projectile && key == u"gpressure") cal.set(u"ps", prj.number(u"gpressure"));
    if (sectionId == Caliber && key == u"sebert") cal.set(u"sebert", std::clamp(cal.number(u"sebert"), 0.0, 1.0));  // 0..1 are valid
    // a boat tail or hollow base without tail dimensions would have no effect: typical ones are filled in (editable)
    if (sectionId == Projectile && key == u"gTailType") csr::completeTail(prj);
    else if (sectionId == Projectile && key == u"Dbul") csr::completeTail(prj, previousDiameter);
    m_cartridgePanel->syncValues();
    m_projectilePanel->syncValues();
    setModified(true);
    recompute();
}

void LoadView::recompute()
{
    m_input = csr::SimInput::fromLoad(m_file);
    m_ladder = csr::simulateLadder(m_input, calibration());
    m_focus = csr::toIntClamped(m_file.config.number(u"CurveFocusIdx", 0.0), 0, static_cast<int>(m_ladder.size()) - 1);
    m_result = m_ladder[static_cast<std::size_t>(m_focus)].result;
    LOG_Debug() << "Simulated" << title() << ":" << m_ladder.size() << "ladder step(s), focus" << m_focus << ", pmax"
                << m_result.maxPressure_bar << "bar, v0" << m_result.muzzleVelocity_ms << "m/s";
    if (!m_result.ok()) LOG_Warn() << "Simulation of" << title() << "failed (charge too high, input out of range?)";
    m_focusInput = m_input;
    m_focusInput.charge_g = m_ladder[static_cast<std::size_t>(m_focus)].charge_g;

    m_chart->setResult(m_result, m_input.maxPressure_bar);
    m_chart->setLadder(otherLadderResults());
    m_cartridge->setLoad(m_file, m_focusInput);
    m_chamber->setLoad(m_file, m_focusInput);
    if (m_input.vivacityBa > 0.0) m_vivacity->setCurve(csr::vivacityCurve(m_input, calibration()), m_input.z1, m_input.z2);
    else m_vivacity->clear();
    m_measurements->setPredictor(
        [in = m_input, cal = &calibration()](double charge_g)
        {
            csr::SimInput i = in;
            i.charge_g = charge_g;
            return csr::simulate(i, *cal);
        });
    std::vector<double> charges{m_focusInput.charge_g};
    for (const csr::LadderStep &step : m_ladder)
        if (std::abs(step.charge_g - m_focusInput.charge_g) > 1e-9) charges.push_back(step.charge_g);
    m_measurements->setSuggestedCharges(std::move(charges));
    updateLadderUi();
    showResult();
    m_report->invalidate();
}

bool LoadView::isGrtFile() const
{
    return !m_path.isEmpty() && csr::formatForPath(m_path) == csr::FileFormat::Original;
}

const csr::Calibration &LoadView::calibration() const
{
    return SolverPreferences::instance().calibrationFor(isGrtFile());
}

std::vector<csr::SimResult> LoadView::otherLadderResults() const
{
    std::vector<csr::SimResult> others;
    for (std::size_t i = 0; i < m_ladder.size(); ++i)
        if (static_cast<int>(i) != m_focus) others.push_back(m_ladder[i].result);
    return others;
}

QString LoadView::propellantName() const
{
    return (m_file.propellant.text(u"mname") + u' ' + m_file.propellant.text(u"pname")).trimmed();
}

void LoadView::fitPowder()
{
    QStringList warnings;
    const QString source = m_path.isEmpty() ? title() : QFileInfo(m_path).fileName();
    const std::vector<csr::powderfit::Sample> samples = csr::powderfit::samples(m_file, source, &warnings);
    if (samples.empty())
    {
        QMessageBox::information(this, tr("Fit to measurements"), tr("Enter the measured velocity of the shots first."));
        return;
    }
    PowderFitDialog dlg(samples, warnings, this);
    if (m_saveRecord && m_db && m_db->isOpen())
    {
        dlg.setSaveHandler(
            [this, &samples](const csr::powderfit::Result &r)
            {
                QVariantMap record = csr::powderfit::toRecord(m_file.propellant, r, m_db->schema(csr::Table::Propellant),
                                                              csr::powderfit::describe(r, samples));
                record.insert(u"pname"_s, record.value(u"pname"_s).toString() + tr(" (fitted)"));
                return m_saveRecord(csr::Table::Propellant, record) >= 0;
            });
    }
    dlg.runFit();
    if (dlg.exec() != QDialog::Accepted || !dlg.improved()) return;
    LOG_Info() << "Powder fit applied to" << title();
    pushUndo(tr("Fit to measurements"));
    csr::powderfit::apply(m_file.propellant, dlg.results());  // both models: Ba ... and the physical values
    rebuildPanels();
    recompute();
    setModified(true);
}

// The database record of the load's powder (same maker and name; with the Physical Model only the project's records) closest to the
// load's values; none when the database has no such powder or one of its records equals the load's values.
void LoadView::updateOutdatedNotice()
{
    m_outdatedNotice->hide();
    m_outdatedRecordId = 0;
    if (!m_db || !m_db->isOpen() || m_input.shotshell) return;
    QSqlQuery q(m_db->handle());
    q.prepare(u"SELECT id FROM propellant WHERE geloescht = 0 AND mname = ? AND pname = ? ORDER BY id DESC"_s);
    q.addBindValue(m_file.propellant.text(u"mname"));
    q.addBindValue(m_file.propellant.text(u"pname"));
    if (!q.exec()) return;
    // the Physical Model's powder values come from the project's records: the original's and the own ones do not count for it
    const bool projectOnly = SolverPreferences::instance().profileFor(isGrtFile()) == csr::CalibrationProfile::Physical;
    qsizetype fewest = 0;
    int recordId = 0;
    while (q.next())
    {
        const int id = q.value(0).toInt();
        const auto rec = m_db->record(csr::Table::Propellant, id);
        if (!rec || (projectOnly && rec->value(u"source"_s).toString() != QLatin1String(csr::Database::kSourceProject))) continue;
        const QStringList d = csr::propellantDifferences(m_file.propellant, *rec);
        if (d.isEmpty()) return;             // the load has the values of a current record
        if (!recordId || d.size() < fewest)  // the record closest to the load
        {
            recordId = id;
            fewest = d.size();
        }
    }
    if (!recordId) return;
    m_outdatedRecordId = recordId;
    QString text = tr("<b>Powder values differ from the database.</b> The load uses its own values of %1. "
                      "<a href=\"powder\">Use the database values…</a>")
                       .arg(propellantName().toHtmlEscaped());
    text.replace(u"<a href=\"powder\">"_s, u"<a href=\"powder\" style=\"color:#2b1a00\">"_s);
    m_outdatedNoticeText->setText(text);
    m_outdatedNotice->show();
}

void LoadView::offerPowderUpdate()
{
    if (!m_db || !m_outdatedRecordId) return;
    const auto rec = m_db->record(csr::Table::Propellant, m_outdatedRecordId);
    if (!rec) return;
    const auto answer = QMessageBox::question(this, tr("Powder values"),
                                              tr("Replace the powder values of this load by the current ones of %1 in the database?\n\n"
                                                 "The charge and all other settings stay as they are.")
                                                  .arg(propellantName()));
    if (answer != QMessageBox::Yes) return;
    LOG_Info() << "Powder values updated from the database in" << title();
    pushUndo(tr("Powder values"));
    csr::applyPropellant(m_file, *rec);
    rebuildPanels();
    recompute();
    setModified(true);
}

void LoadView::offerPhysicalBurn()
{
    const auto burn = csr::findPhysicalBurn(m_file.propellant.text(u"mname"), m_file.propellant.text(u"pname"));
    if (!burn) return;
    const auto answer = QMessageBox::question(
        this, tr("Physical burn values"),
        tr("Use the Physical Model's burn values of %1 for this load?\n\n"
           "Burn rate %2, form function a0 %3, z1 %4, z2 %5. They replace the scaled burn rate of the Physical Model. The burn values "
           "of the load itself (Ba, a0, z1, z2 of the original's model) stay as they are; the physical ones apply while those are "
           "unchanged.")
            .arg(propellantName(), num(burn->Ba, 4), num(burn->a0, 4), num(burn->z1, 4), num(burn->z2, 4)));
    if (answer != QMessageBox::Yes) return;
    LOG_Info() << "Physical burn values applied to" << title();
    pushUndo(tr("Physical burn values"));
    csr::writePhysicalBurn(m_file.propellant, burn);
    rebuildPanels();
    recompute();
    setModified(true);
}

void LoadView::loadMeasurement(int series)
{
    auto measurements = csr::readMeasurements(m_file.appendixXml);
    m_hasMeasurement = !measurements.empty();
    m_measurements->setMeasurements(std::move(measurements), series);
}

// The measurement tab edits the appendix of the file; a load without measurements gets a block only once there is
// something in it.
void LoadView::storeMeasurement()
{
    const auto &series = m_measurements->allMeasurements();
    const bool empty = std::ranges::all_of(series, [](const csr::Measurement &m) { return m.charges.empty(); });
    if (!m_hasMeasurement && empty) return;
    pushUndo(tr("Edit measurements"));
    m_file.appendixXml = csr::writeMeasurements(m_file.appendixXml, series);
    m_hasMeasurement = true;
    setModified(true);
    m_report->invalidate();
}

// Selecting another ladder curve: remembered in the file (CurveFocusIdx) like the original does.
void LoadView::setFocus(int index)
{
    if (index < 0 || index >= static_cast<int>(m_ladder.size()) || index == m_focus) return;
    m_file.config.set(u"CurveFocusIdx", QString::number(index));
    if (csr::Entry *e = m_file.config.find(u"CurveFocusIdx"); e && e->type.isEmpty())
    {  // a file without the entry: typed like the original writes it
        e->type = u"integer"_s;
        e->descr = u"selected ladder curve for display"_s;
    }
    setModified(true);
    recompute();
}

void LoadView::updateLadderUi()
{
    using csr::units::Quantity;
    const bool ladder = m_ladder.size() >= 2;
    m_ladderTabs->setVisible(ladder);
    m_ladderTable->setVisible(ladder);
    if (!ladder) return;

    QSignalBlocker block(m_ladderTabs);
    while (m_ladderTabs->count() > 0) m_ladderTabs->removeTab(0);
    m_ladderTable->setRowCount(0);
    for (std::size_t i = 0; i < m_ladder.size(); ++i)
    {
        const csr::LadderStep &s = m_ladder[i];
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
            if (m_input.maxPressure_bar > 0.0 && s.result.maxPressure_bar > m_input.maxPressure_bar)
            {
                QTableWidgetItem *item = m_ladderTable->item(row, 1);
                item->setForeground(Qt::white);
                item->setBackground(QColor(0xc4, 0x2b, 0x1c));
                item->setText(u"\u26a0 "_s + item->text());
                item->setToolTip(tr("Above the permitted maximum pressure %1").arg(shown(m_input.maxPressure_bar, Quantity::Pressure)));
            }
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

csr::report::Data LoadView::reportData() const
{
    using csr::report::Value;
    csr::report::Data d = csr::report::buildData(m_file, m_focusInput, m_result, {m_path});
    if (!m_result.ok()) return d;

    // diagrams, rendered like the on-screen ones but on white paper
    const UnitPreferences &u = UnitPreferences::instance();
    using csr::units::Quantity;
    CurveChart curves;
    curves.setUnits(u.unitFor(Quantity::Pressure), u.unitFor(Quantity::Energy), u.unitFor(Quantity::Velocity), u.unitFor(Quantity::Length));
    curves.setResult(m_result, m_input.maxPressure_bar);
    curves.setLadder(otherLadderResults());
    const QImage curvesImage = renderOffscreen(curves, {1100, 560});

    VivacityChart burn;
    if (m_input.vivacityBa > 0.0) burn.setCurve(csr::vivacityCurve(m_input, calibration()), m_input.z1, m_input.z2);
    const QImage burnImage = renderOffscreen(burn, {1100, 600});

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
void LoadView::addMeasurementReport(csr::report::Data &d) const
{
    using csr::report::Value;
    using csr::units::Quantity;
    const csr::Measurement &m = m_measurements->measurement();
    if (m.charges.empty()) return;
    const UnitPreferences &u = UnitPreferences::instance();
    const auto sym = [&](Quantity q) { return QString::fromUtf8(u.unitFor(q).symbol); };
    const auto val = [&](double base, Quantity q, int decimals) { return csr::units::format(base, u.unitFor(q), decimals); };
    const auto cell = [](QString text) { return text.replace(u'|', u'/').replace(u'^', u' '); };

    std::vector<MeasurementChart::Prediction> curve;
    QString table = tr("^ Charge ^ Name ^ Shots ^ Velocity ^ SD ^ ES ^ Simulated ^ Δ ^ Pressure ^ Simulated ^ Δ ^") + u'\n';
    for (const csr::MeasuredCharge &c : m.charges)
    {
        csr::SimInput in = m_input;
        in.charge_g = c.charge_g();
        const csr::SimResult r = csr::simulate(in, calibration());
        if (r.ok()) curve.push_back({.charge_g = c.charge_g(), .velocity_ms = r.muzzleVelocity_ms, .pressure_bar = r.maxPressure_bar});
        const csr::ShotStatistics v = csr::velocityStatistics(c);
        const csr::ShotStatistics p = csr::pressureStatistics(c);
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
    using csr::units::Quantity;
    const UnitPreferences &u = UnitPreferences::instance();
    m_chart->setUnits(u.unitFor(Quantity::Pressure), u.unitFor(Quantity::Energy), u.unitFor(Quantity::Velocity),
                      u.unitFor(Quantity::Length));
}

void LoadView::pushUndo(const QString &text)
{
    m_undo.push_back({m_file, text});
    if (m_undo.size() > kMaxUndoSteps)
    {
        m_undo.erase(m_undo.begin());
        if (m_cleanDepth >= 0) --m_cleanDepth;  // below 0: the saved state fell off the stack
    }
    if (!m_redo.empty())
    {  // a new edit ends the redo branch; a saved state in it is gone
        if (m_cleanDepth > static_cast<int>(m_undo.size()) - 1) m_cleanDepth = -1;
        m_redo.clear();
    }
    emit undoStateChanged();
}

void LoadView::clearUndo()
{
    m_undo.clear();
    m_redo.clear();
    m_cleanDepth = 0;
    emit undoStateChanged();
}

// Puts a remembered file back and shows it; the modified flag follows the position in the history.
void LoadView::restore(const csr::LoadFile &file)
{
    m_file = file;
    rebuildPanels();
    loadMeasurement(m_measurements->currentSeries());
    recompute();
    setModified(m_cleanDepth != static_cast<int>(m_undo.size()));
    emit undoStateChanged();
}

void LoadView::undo()
{
    if (m_undo.empty()) return;
    UndoStep step = std::move(m_undo.back());
    m_undo.pop_back();
    m_redo.push_back({m_file, step.text});
    LOG_Debug() << "Undo:" << step.text;
    restore(step.file);
}

void LoadView::redo()
{
    if (m_redo.empty()) return;
    UndoStep step = std::move(m_redo.back());
    m_redo.pop_back();
    m_undo.push_back({m_file, step.text});
    LOG_Debug() << "Redo:" << step.text;
    restore(step.file);
}

void LoadView::setModified(bool m)
{
    if (m_modified == m) return;
    m_modified = m;
    emit modifiedChanged(m);
}

QString LoadView::headingHtml() const
{
    using csr::units::Quantity;
    const csr::SimInput &in = m_input;
    const auto &p = m_file.propellant;
    // title, then the load settings as a table: label/value pairs aligned in columns
    const auto cell = [](const QString &label, const QString &value, int span = 1)
    {
        return u"<td style='padding-right:4px; white-space:nowrap'><b>%1:</b></td>"
               u"<td colspan='%2' style='padding-right:20px; white-space:nowrap'><i>%3</i></td>"_s.arg(label)
                   .arg(span)
                   .arg(value);
    };
    const QString propellant = p.text(u"mname").toHtmlEscaped() + u' ' + p.text(u"pname").toHtmlEscaped();
    const QStringList rows = {
        cell(tr("Projectile"), m_file.projectile.text(u"ProjectileName").toHtmlEscaped(), 5),
        cell(tr("Propellant"), propellant) + cell(tr("Charge"), shown(m_focusInput.charge_g, Quantity::BulletMass)) +
            cell(tr("Case volume"), shown(in.caseVolume_cm3, Quantity::Volume)),
        cell(tr("L3/CL"), shown(in.caseLength_mm, Quantity::Length)) +
            cell(tr("L6/OAL"), shown(m_file.caliber.number(u"oal"), Quantity::Length)) +
            cell(tr("Seating depth"), shown(in.seatingDepth_mm, Quantity::Length)),
        cell(tr("Projectile length"), shown(in.bulletLength_mm, Quantity::Length)),
    };
    QString html = u"<span style='font-size:large'><b>%1</b></span><table cellspacing='0' cellpadding='1'>"_s.arg(
        m_file.caliber.text(u"CaliberName").toHtmlEscaped());
    for (const QString &row : rows) html += u"<tr>"_s + row + u"</tr>"_s;
    return html + u"</table>"_s;
}

bool LoadView::overpressureShown() const
{
    return !m_overpressure->isHidden();
}

void LoadView::showResult()
{
    m_heading->setText(headingHtml());
    m_comparison->clear();
    m_overpressure->hide();
    m_burnNotice->hide();
    updateOutdatedNotice();
    m_values[0]->setStyleSheet({});
    const auto clearAll = [&]
    {
        for (QLabel *l : m_values) l->setText(QStringLiteral("–"));
    };
    if (!m_result.ok())
    {
        m_status->setText(m_result.status == csr::SimStatus::ChargeTooHigh
                              ? tr("<b>Charge too high:</b> the propellant charge does not fit into the case behind the bullet.")
                          : m_result.status == csr::SimStatus::BulletStuck
                              ? tr("<b>Bullet stuck:</b> the gas cannot push the bullet out of the barrel (too little or too slow "
                                   "powder for this bullet and barrel).")
                              : tr("<b>Invalid input:</b> the simulation cannot run with these values."));
        clearAll();
        return;
    }
    // the model the results come from (Settings > Solver)
    const auto profile = SolverPreferences::instance().profileFor(isGrtFile());
    QString status = tr("Model: %1").arg(SolverPreferences::name(profile));
    if (profile != SolverPreferences::instance().profile()) status += tr(" (original GRT file, see Settings > Solver)");
    // the Physical Model without the powder's physical burn values (a powder it has none for, or edited burn values): the load's own
    // values with a scaled burn rate
    if (profile == csr::CalibrationProfile::Physical && !m_input.shotshell && !m_input.physicalBurn)
    {
        QString text =
            csr::findPhysicalBurn(m_file.propellant.text(u"mname"), m_file.propellant.text(u"pname"))
                ? tr("<b>No physical burn values in this load.</b> The Physical Model has values for %1, but this load's burn values are "
                     "not those of the project's record (the original program's, or edited), so its burn rate is scaled and the results "
                     "are less accurate. <a href=\"physical\">Use the physical values…</a>")
                      .arg(propellantName().toHtmlEscaped())
                : tr("<b>No physical burn values</b> for this propellant (or its burn values were changed): the load's burn rate is "
                     "scaled, "
                     "the results are less accurate than for the powders of the Physical Model.");
        text.replace(u"<a href=\"physical\">"_s, u"<a href=\"physical\" style=\"color:#2b1a00\">"_s);  // dark on the orange in every theme
        m_burnNoticeText->setText(text);
        m_burnNotice->show();
    }
    // shotshells: an own model, fitted to published shotgun loads (not to the original program)
    if (m_input.shotshell)
        status += u"<br>"_s +
                  tr("<b>Shotshell:</b> wad and shot column as the projectile (shot + wad mass, column length, "
                     "seated inside the hull, start pressure = crimp release about 20 bar). Fitted to published "
                     "shotgun loads: velocity typically 5-7 % low; pressure right on average but up to 30 % off depending on the powder - "
                     "check against the powder maker's data.");
    m_status->setText(status);

    const csr::SimResult &r = m_result;
    using csr::units::Quantity;
    // the cards (0-3) show the unit small behind the number
    const auto put = [this](int i, const QString &text)
    {
        if (i >= 4) return m_values[static_cast<std::size_t>(i)]->setText(text);
        const qsizetype sp = text.lastIndexOf(u' ');
        m_values[static_cast<std::size_t>(i)]->setText(
            sp < 0 ? u"<span style='font-size:xx-large'>%1</span>"_s.arg(text.toHtmlEscaped())
                   : u"<span style='font-size:xx-large'>%1</span> <span style='font-size:large; font-weight:normal'>%2</span>"_s.arg(
                         text.left(sp).toHtmlEscaped(), text.mid(sp + 1).toHtmlEscaped()));
    };
    put(0, shown(r.maxPressure_bar, Quantity::Pressure));
    if (const double permitted = m_input.maxPressure_bar; permitted > 0.0 && r.maxPressure_bar > permitted)
    {
        m_overpressureText->setText(tr("<b>Overpressure!</b> The maximum pressure %1 is %2 % above the permitted %3. "
                                       "Do not load this charge.")
                                        .arg(shown(r.maxPressure_bar, Quantity::Pressure),
                                             num(100.0 * (r.maxPressure_bar / permitted - 1.0), 1), shown(permitted, Quantity::Pressure)));
        m_overpressure->show();
        m_values[0]->setStyleSheet(u"color: #e0453a;"_s);
    }
    put(4, shown(r.muzzlePressure_bar, Quantity::Pressure));
    put(2, shown(r.muzzleEnergy_J, Quantity::Energy));
    put(1, shown(r.muzzleVelocity_ms, Quantity::Velocity));
    put(3, tr("%1 ms").arg(num(r.muzzleTime_ms, 4)));
    put(5, r.burnoutInBarrel ? shown(r.burnoutPressure_bar, Quantity::Pressure) : tr("not burnt out"));
    // mean velocity of the powder gases leaving the muzzle (gas impulse / charge mass), as in the report
    put(15, m_input.charge_g > 0.0 ? shown(r.gasImpulse_Ns / (m_input.charge_g * 1e-3), Quantity::Velocity) : QStringLiteral("–"));
    put(8, r.muzzleRpm > 0 ? tr("%1 k/min").arg(num(r.muzzleRpm / 1000.0, 1)) : QStringLiteral("–"));
    put(9, r.recoilEnergy_J > 0 ? shown(r.recoilEnergy_J, Quantity::Energy) : tr("– (gun weight 0)"));
    put(10, QStringLiteral("%1, %2").arg(num(r.ipscFactor, 0), num(r.mipFactor, 0)));
    put(11, QStringLiteral("%1 %").arg(num(100.0 * r.loadRatio, 1)));
    put(12, QStringLiteral("%1 %").arg(num(100.0 * r.burntFraction, 1)));
    put(13, QStringLiteral("%1 %").arg(num(100.0 * r.efficiency, 1)));
    // the physical model computes both; the original's model has no heat loss and its friction is only an estimate
    put(14, r.heatLoss_J > 0.0
                ? QStringLiteral("%1, %2").arg(shown(r.heatLoss_J, Quantity::Energy), shown(r.muzzleFrictionEnergy_J, Quantity::Energy))
                : tr("–, %1").arg(shown(r.muzzleFrictionEnergy_J, Quantity::Energy)));
    put(6, tr("%1 ms").arg(num(r.bulletLeadTime_ms, 4)));
    if (r.optimalBarrelNode >= 0.0)
    {
        const int whole = static_cast<int>(r.optimalBarrelNode);
        const QString node = r.optimalBarrelNode > whole ? tr("#%1 ½").arg(whole) : tr("#%1").arg(whole);
        put(7, tr("%1 ms, node %2").arg(num(r.optimalBarrelTime_ms, 4), node));
    }
    else put(7, QStringLiteral("–"));

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
        Row{tr("Bullet lead time"), r.bulletLeadTime_ms, Quantity::None, tr("ms"), 3, "BulletLeadTime10Pmax"},
        Row{tr("Optimal barrel time"), r.optimalBarrelTime_ms, Quantity::None, tr("ms"), 3, "OptimalBarrelTime"},
        Row{tr("Recoil impulse"), r.recoilImpulse_Ns, Quantity::Impulse, {}, -1, "RecoilImpulse"},
    };
    QString html = tr("<table cellspacing='3'><tr><th></th><th align='right'>this</th><th align='right'>original</th>"
                      "<th align='right'>Δ</th><th></th></tr>");
    for (const Row &row : rows)
    {
        const double ref = m_reference->number(QString::fromLatin1(row.refKey), std::nan(""));
        const csr::units::Unit *unit = row.quantity == Quantity::None ? nullptr : &UnitPreferences::instance().unitFor(row.quantity);
        const auto text = [&](double v) { return unit ? csr::units::format(v, *unit, row.decimals) : num(v, row.decimals); };
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
