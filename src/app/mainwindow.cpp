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

#include "mainwindow.h"
#include "burnratedialog.h"
#include "recordviewdialog.h"
#include "session.h"
#include "unitconverterdialog.h"
#include "windowborder.h"
#include "./ui_mainwindow.h"

#include "about.h"
#include "log.h"
#include "buttonstyle.h"
#include "dbfields.h"
#include "flags.h"
#include "projectdata.h"
#include "loadview.h"
#include "newloaddialog.h"
#include "reportview.h"
#include "settingsdialog.h"
#include "theme.h"
#include "titlebar.h"
#include "translation.h"
#include "recentfiles.h"
#include "unitprefs.h"

#include <QAction>
#include <QApplication>
#include <QActionGroup>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>
#include <QMenuBar>
#include <QMouseEvent>
#include <QPainter>
#include <QWindow>
#include <QMessageBox>
#include <QScopeGuard>
#include <QSettings>
#include <QSqlQuery>
#include <QStatusBar>
#include <QTabBar>
#include <QTabWidget>
#include <QUrl>

using namespace Qt::StringLiterals;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(std::make_unique<Ui::MainWindow>())
    , m_menuBar(new QMenuBar)
    , m_tabs(new QTabWidget)
    , m_dbTabs(new QTabWidget)
{
    ui->setupUi(this);
    // the menu bar sits in the (own) title bar row; with the system's title bar the row shows just the menu
    m_titleBar = new TitleBar(m_menuBar, this);
    setMenuWidget(m_titleBar);

    const std::array titles{tr("Calibers"), tr("Projectiles"), tr("Propellants")};
    for (std::size_t i = 0; i < csr::kRecordTables.size(); ++i)
    {
        m_browsers[i] = new TableBrowser(csr::kRecordTables[i]);
        m_dbTabs->addTab(m_browsers[i], titles[i]);
        connect(m_browsers[i], &TableBrowser::currentIdChanged, this, &MainWindow::updateRecordButtons);
        connect(m_browsers[i], &TableBrowser::idActivated, this, [this](int id) { viewRecord(id); });
    }
    connect(m_dbTabs, &QTabWidget::currentChanged, this, &MainWindow::updateRecordButtons);

    // records: new ones (and copies of the original's) go into the own database, only those can be changed
    m_newRecord = new QPushButton(style()->standardIcon(QStyle::SP_FileIcon), tr("New"));
    m_copyRecord = new QPushButton(tr("Copy"));
    m_editRecord = new QPushButton(tr("Edit"));
    m_deleteRecord = new QPushButton(tr("Delete"));
    m_exportRecord = new QPushButton(tr("Export"));
    m_exportRecord->setToolTip(tr("Save the selected record as a file to contribute it to the project's database "
                                  "(see CONTRIBUTING.md)"));
    m_importBullets = new QPushButton(tr("Import bullets"));
    m_importBullets->setToolTip(tr("Add the bullets of a manufacturer's bullet file (*.%1) to your own records")
                                    .arg(QLatin1String(csr::projectdata::kBulletFileSuffix)));
    m_newRecord->setToolTip(tr("Add a new record to the database"));
    m_copyRecord->setToolTip(tr("Add a changed copy of the selected record (a double click shows a record and offers this)"));
    connect(m_newRecord, &QPushButton::clicked, this, [this] { editRecord(RecordDialog::Mode::New); });
    connect(m_copyRecord, &QPushButton::clicked, this, [this] { editRecord(RecordDialog::Mode::Copy); });
    connect(m_editRecord, &QPushButton::clicked, this, [this] { editRecord(RecordDialog::Mode::Edit); });
    connect(m_deleteRecord, &QPushButton::clicked, this, [this] { deleteRecord(); });
    connect(m_exportRecord, &QPushButton::clicked, this, [this] { exportRecord(); });
    connect(m_importBullets, &QPushButton::clicked, this, [this] { importBullets(); });
    auto *recordBar = new QHBoxLayout;
    for (QPushButton *b : {m_newRecord, m_copyRecord, m_editRecord, m_deleteRecord}) recordBar->addWidget(b);
    recordBar->addStretch(1);
    recordBar->addWidget(m_importBullets);
    recordBar->addWidget(m_exportRecord);
    // the first thing to do at the start: a new load (also in the File menu) or an existing one
    m_newLoad = new QPushButton(style()->standardIcon(QStyle::SP_FileIcon), tr("New load"));
    m_newLoad->setObjectName(QStringLiteral("newLoadButton"));
    m_newLoad->setToolTip(tr("Create a load from a caliber, a projectile and a propellant (%1)")
                              .arg(QKeySequence(QKeySequence::New).toString(QKeySequence::NativeText)));
    markPrimary(m_newLoad);
    auto *openLoadButton = new QPushButton(style()->standardIcon(QStyle::SP_DialogOpenButton), tr("Open load"));
    openLoadButton->setObjectName(QStringLiteral("openLoadButton"));
    for (QPushButton *b : {m_newLoad, openLoadButton})  // the first steps: larger than the record buttons
    {
        b->setMinimumHeight(46);
        b->setIconSize(QSize(24, 24));
    }
    openLoadButton->setToolTip(tr("Open a load file (%1)").arg(QKeySequence(QKeySequence::Open).toString(QKeySequence::NativeText)));
    connect(m_newLoad, &QPushButton::clicked, this, [this] { newLoad(); });
    connect(openLoadButton, &QPushButton::clicked, this, [this] { openLoadDialog(); });
    auto *loadBar = new QHBoxLayout;
    loadBar->addWidget(m_newLoad);
    loadBar->addWidget(openLoadButton);
    loadBar->addStretch(1);
    auto *dbPage = new QWidget;
    auto *dbLayout = new QVBoxLayout(dbPage);
    dbLayout->setContentsMargins(4, 4, 4, 0);
    dbLayout->addLayout(loadBar);
    dbLayout->addLayout(recordBar);
    dbLayout->addWidget(m_dbTabs, 1);

    m_tabs->setTabsClosable(true);
    m_tabs->setDocumentMode(true);
    m_tabs->addTab(dbPage, tr("Database"));
    m_tabs->tabBar()->setTabButton(0, QTabBar::RightSide, nullptr);  // the database tab stays
    setCentralWidget(m_tabs);

    connect(m_tabs, &QTabWidget::tabCloseRequested, this,
            [this](int i)
            {
                auto *view = qobject_cast<LoadView *>(m_tabs->widget(i));
                if (!view || !maybeSave(view)) return;
                m_tabs->removeTab(i);
                view->deleteLater();
            });

    createActions();
    setCustomTitleBar(TitleBar::stored());
    connect(m_tabs, &QTabWidget::currentChanged, this, &MainWindow::updateActions);
    updateActions();
    resize(1280, 800);
    restoreGeometry(QSettings().value(QStringLiteral("window/geometry")).toByteArray());

    // the own database is always there; the original one is chosen in the settings
    openDatabase(QSettings().value(QStringLiteral("database/path")).toString(), false);
}

// Hands the database to every open load (or takes it away while it is being replaced).
void MainWindow::setLoadViewDatabases(bool available)
{
    for (int i = 0; i < m_tabs->count(); ++i)
        if (auto *v = qobject_cast<LoadView *>(m_tabs->widget(i))) v->setDatabase(available ? &m_db : nullptr);
}

MainWindow::~MainWindow()
{
    // the table models must not outlive the database connection
    for (TableBrowser *b : m_browsers) b->setDatabase(nullptr);
}

void MainWindow::createActions()
{
    QMenu *file = m_menuBar->addMenu(tr("&File"));
    auto add = [&](const QString &text, const QKeySequence &key, auto slot)
    {
        QAction *a = file->addAction(text, this, slot);
        a->setShortcut(key);
        return a;
    };
    m_newLoadAction = add(tr("&New load…"), QKeySequence::New, [this] { newLoad(); });
    add(tr("&Open load…"), QKeySequence::Open, [this] { openLoadDialog(); });
    m_saveAction = add(tr("&Save"), QKeySequence::Save,
                       [this]
                       {
                           if (auto *v = currentLoad()) saveLoad(v, false);
                       });
    m_loadActions.push_back(add(tr("Save &as…"), QKeySequence::SaveAs,
                                [this]
                                {
                                    if (auto *v = currentLoad()) saveLoad(v, true);
                                }));
    // last used loads: a submenu next to "Open load", empty = disabled
    m_recentMenu = file->addMenu(tr("Open &recent"));
    for (int i = 0; i < recentfiles::kMaxFiles; ++i)
    {
        QAction *a = m_recentMenu->addAction(QString());
        connect(a, &QAction::triggered, this,
                [this, a]
                {
                    const QString path = a->data().toString();
                    if (!openLoad(path))
                    {  // gone or unreadable: forget it
                        QStringList files = recentfiles::load();
                        files.removeAll(path);
                        recentfiles::store(files);
                        updateRecentActions();
                    }
                });
        m_recentActions.push_back(a);
    }
    updateRecentActions();
    file->addSeparator();
    m_loadActions.push_back(add(tr("Print pre&view…"), {},
                                [this]
                                {
                                    if (auto *v = currentLoad()) v->report()->showPrintPreview(this);
                                }));
    m_loadActions.push_back(add(tr("&Print…"), QKeySequence::Print,
                                [this]
                                {
                                    if (auto *v = currentLoad()) v->report()->showPrintDialog(this);
                                }));
    m_loadActions.push_back(add(tr("Export report as P&DF…"), {},
                                [this]
                                {
                                    auto *v = currentLoad();
                                    if (!v) return;
                                    const QString path =
                                        QFileDialog::getSaveFileName(this, tr("Export PDF"), {}, tr("PDF documents (*.pdf)"));
                                    if (path.isEmpty()) return;
                                    if (v->report()->exportPdf(path))
                                    {
                                        LOG_Info() << "Report exported:" << path;
                                    }
                                    else
                                    {
                                        LOG_Error() << "Cannot write PDF" << path;
                                        QMessageBox::warning(this, tr("Export PDF"), tr("Cannot write %1").arg(path));
                                    }
                                }));
    file->addSeparator();
    m_loadActions.push_back(add(tr("Overlay &reference result…"), QKeySequence(Qt::CTRL | Qt::Key_R),
                                [this]
                                {
                                    const QString f =
                                        QFileDialog::getOpenFileName(this, tr("Reference result"), {}, tr("Reference results (*.json)"));
                                    if (!f.isEmpty()) overlayReference(f);
                                }));
    m_clearReferenceAction = add(tr("&Clear reference overlay"), {},
                                 [this]
                                 {
                                     if (auto *v = currentLoad()) v->setReference(std::nullopt);
                                     updateActions();
                                 });
    file->addSeparator();
    add(tr("&Settings…"), QKeySequence(Qt::CTRL | Qt::Key_Comma), [this] { showSettings(); });
    file->addSeparator();
    add(tr("&Quit"), QKeySequence::Quit, [this] { close(); });

    // Edit: undo / redo of the load that is shown
    QMenu *edit = m_menuBar->addMenu(tr("&Edit"));
    m_undoAction = edit->addAction(tr("&Undo"), this,
                                   [this]
                                   {
                                       if (auto *v = currentLoad()) v->undo();
                                   });
    m_undoAction->setShortcut(QKeySequence::Undo);
    m_redoAction = edit->addAction(tr("&Redo"), this,
                                   [this]
                                   {
                                       if (auto *v = currentLoad()) v->redo();
                                   });
    m_redoAction->setShortcuts({QKeySequence::Redo, QKeySequence(Qt::CTRL | Qt::Key_Y)});

    // View > Theme
    QMenu *view = m_menuBar->addMenu(tr("&View"));
    QMenu *themeMenu = view->addMenu(tr("&Theme"));
    auto *themeGroup = new QActionGroup(this);
    const theme::Mode currentTheme = theme::storedMode();
    const std::array<std::pair<QString, theme::Mode>, 3> themes{
        {{tr("System default"), theme::Mode::System}, {tr("Light"), theme::Mode::Light}, {tr("Dark"), theme::Mode::Dark}}};
    for (const auto &[label, mode] : themes)
    {
        QAction *a = themeMenu->addAction(label);
        a->setCheckable(true);
        a->setChecked(mode == currentTheme);
        themeGroup->addAction(a);
        connect(a, &QAction::triggered, this,
                [mode]
                {
                    theme::storeMode(mode);
                    theme::apply(*qobject_cast<QApplication *>(QCoreApplication::instance()), mode);
                });
    }

    // View > Language: "System default" plus every language that ships a translation
    QMenu *lang = view->addMenu(tr("&Language"));
    auto *group = new QActionGroup(this);
    const QString stored = translation::storedLanguage();
    auto addLanguage = [&](const QString &text, const QString &value)
    {
        QAction *a = lang->addAction(flags::forLanguage(value), text);
        a->setCheckable(true);
        // with an icon the style shows the check only as a faint frame: the chosen language is bold as well
        connect(a, &QAction::toggled, a,
                [a](bool on)
                {
                    QFont f = a->font();
                    f.setBold(on);
                    a->setFont(f);
                });
        a->setChecked(stored == value);
        group->addAction(a);
        connect(a, &QAction::triggered, this,
                [this, value]
                {
                    LOG_Info() << "Language chosen:" << (value.isEmpty() ? QStringLiteral("system default") : value);
                    translation::storeLanguage(value);
                    QMessageBox::information(this, tr("Language"),
                                             tr("The language change takes effect after restarting the application."));
                });
    };
    addLanguage(tr("System default"), {});
    for (const QString &l : translation::availableLanguages()) addLanguage(translation::displayName(l), l);

    // Tools
    QMenu *tools = m_menuBar->addMenu(tr("&Tools"));
    m_burnRateAction = tools->addAction(tr("&Burn rate table…"), this,
                                        [this]
                                        {
                                            if (!m_db.isOpen()) return;
                                            BurnRateDialog dialog(m_db, this);
                                            dialog.exec();
                                        });
    tools->addAction(tr("&Unit converter…"), this,
                     [this]
                     {
                         UnitConverterDialog dialog(this);
                         dialog.exec();
                     });

    // Help
    QMenu *help = m_menuBar->addMenu(tr("&Help"));
    help->addAction(tr("&User manual"), QKeySequence::HelpContents, this, [this] { openManual(); });
    help->addAction(tr("&About ChamberSim Reloaded…"), this,
                    [this]
                    {
                        std::optional<about::DatabaseInfo> info;
                        if (m_db.isOpen())
                        {
                            // the original's records: all minus the project's and the own ones
                            const auto counts = [&](auto count)
                            {
                                return about::Counts{.calibers = count(csr::Table::Caliber),
                                                     .projectiles = count(csr::Table::Projectile),
                                                     .propellants = count(csr::Table::Propellant)};
                            };
                            info = about::DatabaseInfo{
                                .project = counts([&](csr::Table t) { return m_db.projectCount(t); }),
                                .own = counts([&](csr::Table t) { return m_db.ownCount(t); }),
                                .original = counts([&](csr::Table t) { return m_db.count(t) - m_db.projectCount(t) - m_db.ownCount(t); }),
                                .originalPath = m_db.path()};
                        }
                        about::show(this, info);
                    });
}

LoadView *MainWindow::currentLoad() const
{
    return qobject_cast<LoadView *>(m_tabs->currentWidget());
}

// The manual PDF is installed next to the executable (Windows) or under share/ChamberSimReloaded (AppImage, packages).
void MainWindow::openManual()
{
    const QString dir = QCoreApplication::applicationDirPath();
    for (const QString &candidate : {dir + QStringLiteral("/manual.pdf"), dir + QStringLiteral("/../share/ChamberSimReloaded/manual.pdf")})
    {
        if (QFileInfo::exists(candidate))
        {
            if (!QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(candidate).canonicalFilePath())))
                QMessageBox::warning(this, tr("User manual"), tr("No program was found to open the PDF file."));
            return;
        }
    }
    QMessageBox::information(this, tr("User manual"), tr("The user manual is not part of this installation."));
}

void MainWindow::addRecentFile(const QString &path)
{
    recentfiles::store(recentfiles::pushed(recentfiles::load(), path));
    updateRecentActions();
}

// File menu entries that need a load work only while one is shown (and Save only with something to save).
void MainWindow::updateActions()
{
    LoadView *view = currentLoad();
    // the title names the load that is shown (it appears in the title bar and the task switcher)
    const QString app = tr("ChamberSim Reloaded");
    setWindowTitle(view ? QStringLiteral("%1 – %2").arg(m_tabs->tabText(m_tabs->currentIndex()), app) : app);
    for (QAction *a : m_loadActions) a->setEnabled(view != nullptr);
    if (m_saveAction) m_saveAction->setEnabled(view && (view->isModified() || view->path().isEmpty()));
    if (m_clearReferenceAction) m_clearReferenceAction->setEnabled(view && view->hasReference());
    // "Undo Select Caliber": the menu names the edit
    if (m_undoAction)
    {
        m_undoAction->setEnabled(view && view->canUndo());
        m_undoAction->setText(view && view->canUndo() ? tr("&Undo %1").arg(view->undoText()) : tr("&Undo"));
    }
    if (m_redoAction)
    {
        m_redoAction->setEnabled(view && view->canRedo());
        m_redoAction->setText(view && view->canRedo() ? tr("&Redo %1").arg(view->redoText()) : tr("&Redo"));
    }
}

void MainWindow::updateRecentActions()
{
    const QStringList files = recentfiles::load();
    for (std::size_t i = 0; i < m_recentActions.size(); ++i)
    {
        QAction *a = m_recentActions[i];
        const bool used = static_cast<qsizetype>(i) < files.size();
        a->setVisible(used);
        if (!used) continue;
        const QFileInfo info(files[static_cast<qsizetype>(i)]);
        // "&1 name.grtload" (numbers 10-12 without accelerator); the tooltip/status tip shows the full path
        a->setText((i < 9 ? QStringLiteral("&%1 ").arg(i + 1) : QStringLiteral("%1 ").arg(i + 1)) +
                   info.fileName().replace(u'&', QStringLiteral("&&")));
        a->setData(files[static_cast<qsizetype>(i)]);
        a->setStatusTip(files[static_cast<qsizetype>(i)]);
        a->setToolTip(files[static_cast<qsizetype>(i)]);
    }
    m_recentMenu->setEnabled(!files.isEmpty());
}

bool MainWindow::openLoad(const QString &path)
{
    for (int i = 0; i < m_tabs->count(); ++i)
    {  // already open: just activate
        if (auto *v = qobject_cast<LoadView *>(m_tabs->widget(i)); v && v->path() == path)
        {
            m_tabs->setCurrentIndex(i);
            addRecentFile(path);
            return true;
        }
    }
    auto owned = std::make_unique<LoadView>();
    QString error;
    if (!owned->load(path, &error))
    {
        QMessageBox::warning(this, tr("Open load"), tr("Cannot read %1:\n%2").arg(path, error));
        return false;
    }
    addLoadTab(owned.release());  // the tab widget owns it now
    addRecentFile(path);
    return true;
}

bool MainWindow::openDatabase(const QString &originalPath, bool complain)
{
    for (TableBrowser *b : m_browsers) b->setDatabase(nullptr);  // release the old connection first
    setLoadViewDatabases(false);
    QString error;
    const bool ok = m_db.open(originalPath, {}, &error);
    if (!m_db.isOpen())
    {  // not even the own database: nothing to show
        LOG_Error() << "Cannot open the program's database" << csr::Database::defaultOwnPath() << ":" << error;
        QMessageBox::warning(this, tr("Database"), tr("Cannot open the database %1:\n%2").arg(csr::Database::defaultOwnPath(), error));
        return false;
    }
    for (TableBrowser *b : m_browsers) b->setDatabase(&m_db);
    setLoadViewDatabases(true);
    updateRecordButtons();
    if (ok) LOG_Info() << "Database opened:" << (originalPath.isEmpty() ? QStringLiteral("(own records only)") : originalPath);
    else LOG_Warn() << "Original database not usable:" << originalPath << ":" << error;
    for (const QString &problem : m_db.projectErrors()) LOG_Warn() << "project data:" << problem;
    if (!ok && complain)
        QMessageBox::warning(this, tr("Original database"),
                             tr("Cannot use %1:\n%2\n\nThe original GordonsReloadingTool.db is encrypted; "
                                "choose a decrypted copy (GordonsReloadingTool.plain.db).")
                                 .arg(originalPath, error));
    if (ok || originalPath.isEmpty()) QSettings().setValue(QStringLiteral("database/path"), originalPath);
    statusBar()->showMessage(databaseSummary());
    return ok;
}

QString MainWindow::databaseSummary() const
{
    const QString counts = tr("%1 calibers, %2 projectiles, %3 propellants")
                               .arg(m_db.count(csr::Table::Caliber))
                               .arg(m_db.count(csr::Table::Projectile))
                               .arg(m_db.count(csr::Table::Propellant));
    return m_db.hasOriginal() ? tr("%1: %2").arg(QFileInfo(m_db.path()).fileName(), counts)
                              : tr("No original database (choose it in File ▸ Settings…): %1").arg(counts);
}

namespace
{
constexpr int kBorder = 5;  // resize border of the frameless window
}

void MainWindow::setCustomTitleBar(bool on)
{
    if (on == m_customTitleBar && m_titleBar->isDecorated() == on) return;
    LOG_Info() << "Custom title bar" << (on ? "on" : "off");
    m_customTitleBar = on;
    const bool visible = isVisible();
    setWindowFlag(Qt::FramelessWindowHint, on);  // hides the window; shown again below
    m_titleBar->setDecorated(on);
    setMouseTracking(on);
    updateFrame();
    if (visible) show();
}

void MainWindow::updateFrame()
{
    const int b = m_customTitleBar && !isMaximized() && !isFullScreen() ? kBorder : 0;
    setContentsMargins(b, b, b, b);
    update();
}

void MainWindow::changeEvent(QEvent *event)
{
    QMainWindow::changeEvent(event);
    if (event->type() == QEvent::WindowStateChange) updateFrame();
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    QMainWindow::paintEvent(event);
    if (!m_customTitleBar || contentsMargins().left() == 0) return;
    QPainter p(this);
    paintWindowBorder(p, rect(), palette());
}

Qt::Edges MainWindow::edgesAt(const QPoint &pos) const
{
    Qt::Edges edges;
    if (!m_customTitleBar || contentsMargins().left() == 0) return edges;
    const int corner = 3 * kBorder;  // corners are easier to grab
    if (pos.x() < kBorder || (pos.x() < corner && (pos.y() < kBorder || pos.y() >= height() - kBorder))) edges |= Qt::LeftEdge;
    if (pos.x() >= width() - kBorder || (pos.x() >= width() - corner && (pos.y() < kBorder || pos.y() >= height() - kBorder)))
        edges |= Qt::RightEdge;
    if (pos.y() < kBorder || (pos.y() < corner && (pos.x() < kBorder || pos.x() >= width() - kBorder))) edges |= Qt::TopEdge;
    if (pos.y() >= height() - kBorder || (pos.y() >= height() - corner && (pos.x() < kBorder || pos.x() >= width() - kBorder)))
        edges |= Qt::BottomEdge;
    return edges;
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    const Qt::Edges edges = edgesAt(event->position().toPoint());
    if (edges && event->button() == Qt::LeftButton && windowHandle())
    {
        windowHandle()->startSystemResize(edges);
        return;
    }
    QMainWindow::mousePressEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    const Qt::Edges e = edgesAt(event->position().toPoint());
    const bool diagonal1 = e == (Qt::LeftEdge | Qt::TopEdge) || e == (Qt::RightEdge | Qt::BottomEdge);
    const bool diagonal2 = e == (Qt::RightEdge | Qt::TopEdge) || e == (Qt::LeftEdge | Qt::BottomEdge);
    if (diagonal1) setCursor(Qt::SizeFDiagCursor);
    else if (diagonal2) setCursor(Qt::SizeBDiagCursor);
    else if (e & (Qt::LeftEdge | Qt::RightEdge)) setCursor(Qt::SizeHorCursor);
    else if (e & (Qt::TopEdge | Qt::BottomEdge)) setCursor(Qt::SizeVerCursor);
    else unsetCursor();
    QMainWindow::mouseMoveEvent(event);
}

void MainWindow::leaveEvent(QEvent *event)
{
    unsetCursor();
    QMainWindow::leaveEvent(event);
}

TableBrowser *MainWindow::currentBrowser() const
{
    return m_browsers[static_cast<std::size_t>(std::clamp(m_dbTabs->currentIndex(), 0, 2))];
}

csr::Table MainWindow::currentTable() const
{
    return csr::kRecordTables[static_cast<std::size_t>(std::clamp(m_dbTabs->currentIndex(), 0, 2))];
}

void MainWindow::updateRecordButtons()
{
    if (!m_newRecord) return;
    const int id = currentBrowser()->currentId();
    const bool own = csr::Database::isOwnId(id);
    m_newRecord->setEnabled(m_db.isOpen());
    m_newLoad->setEnabled(m_db.isOpen());
    m_copyRecord->setEnabled(id >= 0);
    m_editRecord->setEnabled(own);
    m_deleteRecord->setEnabled(own);
    m_exportRecord->setEnabled(own);
    m_importBullets->setVisible(currentTable() == csr::Table::Projectile);  // only the projectile tab has bullet files
    m_importBullets->setEnabled(m_db.isOpen());
    const QString locked = tr("Only your own records can be changed: make a copy");
    m_editRecord->setToolTip(id >= 0 && !own ? locked : tr("Change the selected record"));
    m_deleteRecord->setToolTip(id >= 0 && !own ? locked : tr("Delete the selected record"));
}

// A double click shows all data of the record; the viewer offers to edit (own records) or to copy it.
void MainWindow::viewRecord(int id)
{
    if (!m_db.isOpen()) return;
    const csr::Table table = currentTable();
    const auto record = m_db.record(table, id);
    if (!record) return;
    RecordViewDialog dlg(m_db, table, id, *record, this);
    const int result = dlg.exec();
    if (result == RecordViewDialog::EditRequested) editRecord(RecordDialog::Mode::Edit);
    else if (result == RecordViewDialog::CopyRequested) editRecord(RecordDialog::Mode::Copy);
}

void MainWindow::editRecord(RecordDialog::Mode mode)
{
    if (!m_db.isOpen()) return;
    const csr::Table table = currentTable();
    const int id = currentBrowser()->currentId();
    QVariantMap record;
    if (mode != RecordDialog::Mode::New)
    {
        const auto r = m_db.record(table, id);
        if (!r) return;
        record = *r;
        if (mode == RecordDialog::Mode::Edit && !csr::Database::isOwnId(id)) mode = RecordDialog::Mode::Copy;
    }
    RecordDialog dlg(m_db, table, mode, record, this);
    if (dlg.exec() == QDialog::Accepted) storeRecord(table, mode, id, dlg.values(), dlg.pendingImages());
}

int MainWindow::storeRecord(csr::Table table, RecordDialog::Mode mode, int id, const QVariantMap &values,
                            const std::vector<csr::Database::Image> &images)
{
    QString error;
    int stored = -1;
    for (const csr::Database::Image &image : images)
    {  // the record refers to them
        if (!m_db.addImage(image, &error))
        {
            QMessageBox::warning(this, tr("Database"), tr("The photo could not be stored:\n%1").arg(error));
            return -1;
        }
    }
    if (mode == RecordDialog::Mode::Edit)
    {
        if (m_db.update(table, id, values, &error)) stored = id;
    }
    else if (const auto newId = m_db.insert(table, values, &error))
    {
        stored = *newId;
    }
    if (stored < 0)
    {
        QMessageBox::warning(this, tr("Database"), tr("The record could not be stored:\n%1").arg(error));
        return -1;
    }
    TableBrowser *browser = m_browsers[static_cast<std::size_t>(table)];
    browser->refresh();
    browser->selectId(stored);
    updateRecordButtons();
    statusBar()->showMessage(databaseSummary());
    return stored;
}

bool MainWindow::deleteRecord(bool ask)
{
    const csr::Table table = currentTable();
    const int id = currentBrowser()->currentId();
    if (!csr::Database::isOwnId(id)) return false;
    const auto record = m_db.record(table, id);
    if (!record) return false;
    if (ask && QMessageBox::question(
                   this, tr("Delete record"),
                   tr("Delete \"%1\"?\n\nLoad files that use it keep their values.").arg(csr::dbfields::recordTitle(table, *record))) !=
                   QMessageBox::Yes)
        return false;
    QString error;
    if (!m_db.remove(table, id, &error))
    {
        QMessageBox::warning(this, tr("Database"), tr("The record could not be deleted:\n%1").arg(error));
        return false;
    }
    currentBrowser()->refresh();
    updateRecordButtons();
    statusBar()->showMessage(databaseSummary());
    return true;
}

bool MainWindow::exportRecord(const QString &path)
{
    const csr::Table table = currentTable();
    const int id = currentBrowser()->currentId();
    if (!csr::Database::isOwnId(id)) return false;
    const auto record = m_db.record(table, id);
    if (!record) return false;
    const auto schema = m_db.schema(table);
    // the file gets the schema's columns without id and geloescht (toJson): check the record as it is written, not the database row
    QVariantMap content = *record;
    for (const QString &column : {QStringLiteral("id"), QStringLiteral("geloescht"), QStringLiteral("source")}) content.remove(column);
    if (const QStringList problems = csr::projectdata::check(table, content, schema); !problems.isEmpty() && path.isEmpty())
    {
        if (QMessageBox::warning(
                this, tr("Export record"),
                tr("This record is not complete for the project's database:\n%1\n\nExport it anyway?").arg(problems.join(u'\n')),
                QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
            return false;
    }
    QString file = path;
    if (file.isEmpty())
        file = QFileDialog::getSaveFileName(this, tr("Export record"), csr::projectdata::fileName(table, *record),
                                            tr("Record files (*.json)"));
    if (file.isEmpty()) return false;
    QFile out(file);
    if (!out.open(QIODevice::WriteOnly) || out.write(csr::projectdata::toJson(*record, schema)) < 0)
    {
        QMessageBox::warning(this, tr("Export record"), tr("Cannot write %1").arg(file));
        return false;
    }
    statusBar()->showMessage(tr("Saved %1: add it to data/%2/ in the project to contribute it.")
                                 .arg(QFileInfo(file).fileName(), csr::projectdata::folderName(table)));
    return true;
}

int MainWindow::importBullets(const QString &path)
{
    if (!m_db.isOpen()) return 0;
    QSettings settings;
    QString file = path;
    if (file.isEmpty())
        file = QFileDialog::getOpenFileName(
            this, tr("Import bullets"), settings.value(u"files/importDir"_s).toString(),
            tr("Bullet files (*.%1 *.json);;All files (*)").arg(QLatin1String(csr::projectdata::kBulletFileSuffix)));
    if (file.isEmpty()) return 0;
    settings.setValue(u"files/importDir"_s, QFileInfo(file).absolutePath());
    QFile in(file);
    if (!in.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(this, tr("Import bullets"), tr("Cannot read %1").arg(QDir::toNativeSeparators(file)));
        return 0;
    }
    const csr::projectdata::BulletFile bullets = csr::projectdata::readBulletFile(in.readAll(), m_db.schema(csr::Table::Projectile));
    // bullets the database has already (same manufacturer, name and lot) are left out
    std::vector<QVariantMap> fresh;
    int known = 0;
    for (const QVariantMap &b : bullets.bullets)
    {
        QSqlQuery q(m_db.handle());
        q.prepare(u"SELECT count(*) FROM projectile WHERE geloescht = 0 AND mname = ? AND pname = ? AND lotid = ?"_s);
        q.addBindValue(b.value(u"mname"_s));
        q.addBindValue(b.value(u"pname"_s));
        q.addBindValue(b.value(u"lotid"_s).toString());
        if (q.exec() && q.next() && q.value(0).toInt() > 0) ++known;
        else fresh.push_back(b);
    }
    QString text = tr("%n bullet(s) can be added", nullptr, static_cast<int>(fresh.size()));
    if (!bullets.manufacturer.isEmpty()) text += tr(" (manufacturer: %1)").arg(bullets.manufacturer);
    text += u'.';
    if (!bullets.source.isEmpty()) text += u"\n"_s + tr("Source: %1").arg(bullets.source);
    if (known > 0) text += u"\n"_s + tr("%n bullet(s) are already in the database and are left out.", nullptr, known);
    if (!bullets.errors.isEmpty())
        text += u"\n\n"_s + tr("%n problem(s) in the file, those bullets are left out:", nullptr, static_cast<int>(bullets.errors.size())) +
                u"\n"_s + bullets.errors.mid(0, 8).join(u'\n') + (bullets.errors.size() > 8 ? u"\n…"_s : QString());
    if (fresh.empty())
    {
        QMessageBox::information(this, tr("Import bullets"), text);
        return 0;
    }
    if (QMessageBox::question(this, tr("Import bullets"), text + u"\n\n"_s + tr("Add them to your own records?")) != QMessageBox::Yes)
        return 0;
    int added = 0;
    QString error;
    for (const QVariantMap &b : fresh)
    {
        if (m_db.insert(csr::Table::Projectile, b, &error)) ++added;
        else
        {
            QMessageBox::warning(this, tr("Import bullets"), tr("A bullet could not be stored:\n%1").arg(error));
            break;
        }
    }
    currentBrowser()->refresh();
    updateRecordButtons();
    statusBar()->showMessage(tr("%n bullet(s) imported.", nullptr, added));
    return added;
}

void MainWindow::showSettings()
{
    SettingsDialog dlg(this);
    dlg.setOriginalDatabase(m_db.path(), databaseSummary());
    connect(&dlg, &SettingsDialog::customTitleBarChanged, this, &MainWindow::setCustomTitleBar);
    connect(&dlg, &SettingsDialog::originalDatabaseChosen, this,
            [this, &dlg](const QString &path)
            {
                openDatabase(path);
                dlg.setOriginalDatabase(m_db.path(), databaseSummary());
            });
    dlg.exec();
}

bool MainWindow::overlayReference(const QString &jsonPath)
{
    LoadView *view = currentLoad();
    if (!view)
    {
        QMessageBox::information(this, tr("Reference"), tr("Open a load first."));
        return false;
    }
    QString error;
    auto ref = csr::readReferenceRun(jsonPath, &error);
    if (!ref)
    {
        LOG_Warn() << "Cannot read reference" << jsonPath << ":" << error;
        QMessageBox::warning(this, tr("Reference"), tr("Cannot read %1:\n%2").arg(jsonPath, error));
        return false;
    }
    LOG_Info() << "Reference overlay:" << jsonPath << "(" << ref->points.size() << "points, stale" << ref->stale << ")";
    if (ref->stale) QMessageBox::warning(this, tr("Reference"), tr("This reference run is marked stale (the wrong tab was read)."));
    view->setReference(std::move(ref));
    updateActions();
    return true;
}

// The database, the tab title and the actions follow the load; records it creates (a fitted propellant) go into
// the own database and show up in the Database tab.
void MainWindow::setupLoadView(LoadView *view)
{
    view->setDatabase(&m_db);
    view->setRecordSaver(
        [this](csr::Table table, const QVariantMap &values)
        {
            const int id = storeRecord(table, RecordDialog::Mode::New, -1, values);
            if (id >= 0) statusBar()->showMessage(tr("Saved as your own record: %1").arg(csr::dbfields::recordTitle(table, values)));
            return id;
        });
    connect(view, &LoadView::undoStateChanged, this, &MainWindow::updateActions);
    connect(view, &LoadView::modifiedChanged, this,
            [this, view]
            {
                updateTabTitle(view);
                updateActions();
            });
}

void MainWindow::openLoadDialog()
{
    const QStringList files = QFileDialog::getOpenFileNames(
        this, tr("Open load"), {},
        tr("Loads (*.csrload *.grtload);;ChamberSim Reloaded loads (*.csrload);;Original GRT loads (*.grtload);;All files (*)"));
    for (const QString &f : files) openLoad(f);
}

void MainWindow::newLoad()
{
    if (!m_db.isOpen())
    {
        QMessageBox::information(this, tr("New load"), tr("The database cannot be opened."));
        return;
    }
    NewLoadDialog dlg(m_db, this);
    if (dlg.exec() != QDialog::Accepted) return;
    auto *view = new LoadView;
    view->setNewLoad(dlg.result());
    LOG_Info() << "New load created:" << view->title();
    view->setProperty("untitledName", tr("Untitled %1").arg(++m_untitled));
    addLoadTab(view);
}

void MainWindow::addLoadTab(LoadView *view)
{
    setupLoadView(view);
    m_tabs->setCurrentIndex(m_tabs->addTab(view, QString()));
    updateTabTitle(view);
}

// The file name of the load ("Untitled 2" for a new load that has none yet).
QString MainWindow::loadName(const LoadView *view, bool withSuffix)
{
    if (view->path().isEmpty()) return view->property("untitledName").toString();
    const QFileInfo info(view->path());
    return withSuffix ? info.fileName() : info.completeBaseName();
}

void MainWindow::showTab(int index)
{
    if (index >= 0 && index < m_tabs->count()) m_tabs->setCurrentIndex(index);
}

void MainWindow::showResultTab(int index)
{
    if (LoadView *view = currentLoad()) view->showResultTab(index);
}

void MainWindow::showInputTab(int index)
{
    if (LoadView *view = currentLoad()) view->showInputTab(index);
}

void MainWindow::updateTabTitle(LoadView *view)
{
    const auto refreshTitle = qScopeGuard([this] { updateActions(); });
    const int i = m_tabs->indexOf(view);
    if (i < 0) return;
    QString name = loadName(view, false);
    if (view->isModified()) name += QStringLiteral(" *");
    m_tabs->setTabText(i, name);
    m_tabs->setTabToolTip(i, view->title());
}

bool MainWindow::saveLoad(LoadView *view, bool saveAs)
{
    QString path = view->path();
    // a load opened from the original's *.grtload is saved in this program's format (Save as ... *.grtload exports it)
    if (!path.isEmpty() && csr::formatForPath(path) == csr::FileFormat::Original && !saveAs) saveAs = true;
    if (saveAs || path.isEmpty())
    {
        if (!path.isEmpty() && csr::formatForPath(path) == csr::FileFormat::Original)
            path = QFileInfo(path).absolutePath() + QLatin1Char('/') + QFileInfo(path).completeBaseName() + QStringLiteral(".csrload");
        QString selected;
        path = QFileDialog::getSaveFileName(
            this, tr("Save load"), path,
            tr("ChamberSim Reloaded loads (*.csrload);;Original GRT loads, without the data of ChamberSim Reloaded (*.grtload)"),
            &selected);
        if (path.isEmpty()) return false;
        if (QFileInfo(path).suffix().isEmpty() && selected.contains(QStringLiteral("*.grtload"))) path += QStringLiteral(".grtload");
        path = withLoadExtension(path);
    }
    QString error;
    if (!view->save(path, &error))
    {
        QMessageBox::warning(this, tr("Save load"), tr("Cannot write %1:\n%2").arg(path, error));
        return false;
    }
    updateTabTitle(view);
    addRecentFile(path);
    return true;
}

bool MainWindow::maybeSave(LoadView *view)
{
    if (!view->isModified()) return true;
    const auto answer = QMessageBox::question(this, tr("Unsaved changes"), tr("Save changes to %1?").arg(loadName(view, true)),
                                              QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (answer == QMessageBox::Cancel) return false;
    return answer == QMessageBox::Discard || saveLoad(view, false);
}

void MainWindow::saveSession() const
{
    QStringList files;
    int current = -1;
    for (int i = 0; i < m_tabs->count(); ++i)
        if (const auto *v = qobject_cast<LoadView *>(m_tabs->widget(i)); v && !v->path().isEmpty())
        {
            if (i == m_tabs->currentIndex()) current = static_cast<int>(files.size());
            files << v->path();
        }
    session::store(files, current);
}

void MainWindow::restoreSession()
{
    const QStringList files = session::loads();
    for (const QString &f : files)
        if (QFileInfo::exists(f)) openLoad(f);
    if (const int current = session::current(); current >= 0 && current < files.size())
        for (int i = 0; i < m_tabs->count(); ++i)
            if (const auto *v = qobject_cast<LoadView *>(m_tabs->widget(i)); v && v->path() == files[current]) m_tabs->setCurrentIndex(i);
    LOG_Info() << "Last session: reopened" << files.size() << "load(s)";
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    for (int i = 0; i < m_tabs->count(); ++i)
    {
        if (auto *v = qobject_cast<LoadView *>(m_tabs->widget(i)); v && !maybeSave(v))
        {
            event->ignore();
            return;
        }
    }
    QSettings().setValue(QStringLiteral("window/geometry"), saveGeometry());
    saveSession();
    LOG_Info() << "ChamberSim Reloaded closed";
    event->accept();
}
