#include "mainwindow.h"
#include "burnratedialog.h"
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
#include <QStatusBar>
#include <QTabBar>
#include <QTabWidget>

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

    constexpr std::array tables{grt::Table::Caliber, grt::Table::Projectile, grt::Table::Propellant};
    const std::array titles{tr("Calibers"), tr("Projectiles"), tr("Propellants")};
    for (std::size_t i = 0; i < tables.size(); ++i)
    {
        m_browsers[i] = new TableBrowser(tables[i]);
        m_dbTabs->addTab(m_browsers[i], titles[i]);
        connect(m_browsers[i], &TableBrowser::currentIdChanged, this, &MainWindow::updateRecordButtons);
        connect(m_browsers[i], &TableBrowser::idActivated, this,
                [this](int id) { editRecord(grt::Database::isOwnId(id) ? RecordDialog::Mode::Edit : RecordDialog::Mode::Copy); });
    }
    connect(m_dbTabs, &QTabWidget::currentChanged, this, &MainWindow::updateRecordButtons);

    // records: new ones (and copies of the original's) go into the own database, only those can be changed
    m_newRecord = new QPushButton(style()->standardIcon(QStyle::SP_FileIcon), tr("New…"));
    m_copyRecord = new QPushButton(tr("Copy…"));
    m_editRecord = new QPushButton(tr("Edit…"));
    m_deleteRecord = new QPushButton(tr("Delete"));
    m_exportRecord = new QPushButton(tr("Export…"));
    m_exportRecord->setToolTip(tr("Save the selected record as a file to contribute it to the project's database "
                                  "(see CONTRIBUTING.md)"));
    markPrimary(m_newRecord);
    m_newRecord->setToolTip(tr("Add a new record to the database"));
    m_copyRecord->setToolTip(tr("Add a changed copy of the selected record (double click on a record of the original database)"));
    connect(m_newRecord, &QPushButton::clicked, this, [this] { editRecord(RecordDialog::Mode::New); });
    connect(m_copyRecord, &QPushButton::clicked, this, [this] { editRecord(RecordDialog::Mode::Copy); });
    connect(m_editRecord, &QPushButton::clicked, this, [this] { editRecord(RecordDialog::Mode::Edit); });
    connect(m_deleteRecord, &QPushButton::clicked, this, [this] { deleteRecord(); });
    connect(m_exportRecord, &QPushButton::clicked, this, [this] { exportRecord(); });
    auto *recordBar = new QHBoxLayout;
    for (QPushButton *b : {m_newRecord, m_copyRecord, m_editRecord, m_deleteRecord}) recordBar->addWidget(b);
    recordBar->addStretch(1);
    recordBar->addWidget(m_exportRecord);
    auto *dbPage = new QWidget;
    auto *dbLayout = new QVBoxLayout(dbPage);
    dbLayout->setContentsMargins(4, 4, 4, 0);
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
    add(tr("&Open load…"), QKeySequence::Open,
        [this]
        {
            const QStringList files = QFileDialog::getOpenFileNames(this, tr("Open load"), {}, tr("GRT loads (*.grtload);;All files (*)"));
            for (const QString &f : files) openLoad(f);
        });
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

    // Help
    QMenu *help = m_menuBar->addMenu(tr("&Help"));
    help->addAction(tr("&About GRT Reloaded…"), this,
                    [this]
                    {
                        std::optional<about::DatabaseInfo> info;
                        if (m_db.isOpen())
                        {
                            // the original's records: all minus the project's and the own ones
                            const auto counts = [&](auto count)
                            {
                                return about::Counts{.calibers = count(grt::Table::Caliber),
                                                     .projectiles = count(grt::Table::Projectile),
                                                     .propellants = count(grt::Table::Propellant)};
                            };
                            info = about::DatabaseInfo{
                                .project = counts([&](grt::Table t) { return m_db.projectCount(t); }),
                                .own = counts([&](grt::Table t) { return m_db.ownCount(t); }),
                                .original = counts([&](grt::Table t) { return m_db.count(t) - m_db.projectCount(t) - m_db.ownCount(t); }),
                                .originalPath = m_db.path()};
                        }
                        about::show(this, info);
                    });
}

LoadView *MainWindow::currentLoad() const
{
    return qobject_cast<LoadView *>(m_tabs->currentWidget());
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
    const QString app = tr("GRT Reloaded");
    setWindowTitle(view ? QStringLiteral("%1 – %2").arg(m_tabs->tabText(m_tabs->currentIndex()), app) : app);
    for (QAction *a : m_loadActions) a->setEnabled(view != nullptr);
    if (m_saveAction) m_saveAction->setEnabled(view && (view->isModified() || view->path().isEmpty()));
    if (m_clearReferenceAction) m_clearReferenceAction->setEnabled(view && view->hasReference());
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
    LoadView *view = owned.get();
    setupLoadView(view);
    const int idx = m_tabs->addTab(owned.release(), QString());  // the tab widget owns it now
    m_tabs->setCurrentIndex(idx);
    updateTabTitle(view);
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
        LOG_Error() << "Cannot open the program's database" << grt::Database::defaultOwnPath() << ":" << error;
        QMessageBox::warning(this, tr("Database"), tr("Cannot open the database %1:\n%2").arg(grt::Database::defaultOwnPath(), error));
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
                               .arg(m_db.count(grt::Table::Caliber))
                               .arg(m_db.count(grt::Table::Projectile))
                               .arg(m_db.count(grt::Table::Propellant));
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
    p.setPen(palette().color(QPalette::Mid));
    p.drawRect(rect().adjusted(0, 0, -1, -1));
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

grt::Table MainWindow::currentTable() const
{
    constexpr std::array tables{grt::Table::Caliber, grt::Table::Projectile, grt::Table::Propellant};
    return tables[static_cast<std::size_t>(std::clamp(m_dbTabs->currentIndex(), 0, 2))];
}

void MainWindow::updateRecordButtons()
{
    if (!m_newRecord) return;
    const int id = currentBrowser()->currentId();
    const bool own = grt::Database::isOwnId(id);
    m_newRecord->setEnabled(m_db.isOpen());
    m_copyRecord->setEnabled(id >= 0);
    m_editRecord->setEnabled(own);
    m_deleteRecord->setEnabled(own);
    m_exportRecord->setEnabled(own);
    const QString locked = tr("Only your own records can be changed: make a copy");
    m_editRecord->setToolTip(id >= 0 && !own ? locked : tr("Change the selected record"));
    m_deleteRecord->setToolTip(id >= 0 && !own ? locked : tr("Delete the selected record"));
}

void MainWindow::editRecord(RecordDialog::Mode mode)
{
    if (!m_db.isOpen()) return;
    const grt::Table table = currentTable();
    const int id = currentBrowser()->currentId();
    QVariantMap record;
    if (mode != RecordDialog::Mode::New)
    {
        const auto r = m_db.record(table, id);
        if (!r) return;
        record = *r;
        if (mode == RecordDialog::Mode::Edit && !grt::Database::isOwnId(id)) mode = RecordDialog::Mode::Copy;
    }
    RecordDialog dlg(m_db, table, mode, record, this);
    if (dlg.exec() == QDialog::Accepted) storeRecord(table, mode, id, dlg.values(), dlg.pendingImages());
}

int MainWindow::storeRecord(grt::Table table, RecordDialog::Mode mode, int id, const QVariantMap &values,
                            const std::vector<grt::Database::Image> &images)
{
    QString error;
    int stored = -1;
    for (const grt::Database::Image &image : images)
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
    const grt::Table table = currentTable();
    const int id = currentBrowser()->currentId();
    if (!grt::Database::isOwnId(id)) return false;
    const auto record = m_db.record(table, id);
    if (!record) return false;
    if (ask && QMessageBox::question(
                   this, tr("Delete record"),
                   tr("Delete \"%1\"?\n\nLoad files that use it keep their values.").arg(grt::dbfields::recordTitle(table, *record))) !=
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
    const grt::Table table = currentTable();
    const int id = currentBrowser()->currentId();
    if (!grt::Database::isOwnId(id)) return false;
    const auto record = m_db.record(table, id);
    if (!record) return false;
    const auto schema = m_db.schema(table);
    if (const QStringList problems = grt::projectdata::check(table, *record, schema); !problems.isEmpty() && path.isEmpty())
    {
        if (QMessageBox::warning(
                this, tr("Export record"),
                tr("This record is not complete for the project's database:\n%1\n\nExport it anyway?").arg(problems.join(u'\n')),
                QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
            return false;
    }
    QString file = path;
    if (file.isEmpty())
        file = QFileDialog::getSaveFileName(this, tr("Export record"), grt::projectdata::fileName(table, *record),
                                            tr("Record files (*.json)"));
    if (file.isEmpty()) return false;
    QFile out(file);
    if (!out.open(QIODevice::WriteOnly) || out.write(grt::projectdata::toJson(*record, schema)) < 0)
    {
        QMessageBox::warning(this, tr("Export record"), tr("Cannot write %1").arg(file));
        return false;
    }
    statusBar()->showMessage(tr("Saved %1: add it to data/%2/ in the project to contribute it.")
                                 .arg(QFileInfo(file).fileName(), grt::projectdata::folderName(table)));
    return true;
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
    auto ref = grt::readReferenceRun(jsonPath, &error);
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
        [this](grt::Table table, const QVariantMap &values)
        {
            const int id = storeRecord(table, RecordDialog::Mode::New, -1, values);
            if (id >= 0) statusBar()->showMessage(tr("Saved as your own record: %1").arg(grt::dbfields::recordTitle(table, values)));
            return id;
        });
    connect(view, &LoadView::modifiedChanged, this,
            [this, view]
            {
                updateTabTitle(view);
                updateActions();
            });
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
    setupLoadView(view);
    const int idx = m_tabs->addTab(view, QString());
    m_tabs->setCurrentIndex(idx);
    updateTabTitle(view);
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
    QString name = view->path().isEmpty() ? view->property("untitledName").toString() : QFileInfo(view->path()).completeBaseName();
    if (view->isModified()) name += QStringLiteral(" *");
    m_tabs->setTabText(i, name);
    m_tabs->setTabToolTip(i, view->title());
}

bool MainWindow::saveLoad(LoadView *view, bool saveAs)
{
    QString path = view->path();
    if (saveAs || path.isEmpty())
    {
        path = QFileDialog::getSaveFileName(this, tr("Save load"), path, tr("GRT loads (*.grtload)"));
        if (path.isEmpty()) return false;
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
    const auto answer = QMessageBox::question(
        this, tr("Unsaved changes"),
        tr("Save changes to %1?")
            .arg(view->path().isEmpty() ? view->property("untitledName").toString() : QFileInfo(view->path()).fileName()),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (answer == QMessageBox::Cancel) return false;
    return answer == QMessageBox::Discard || saveLoad(view, false);
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
    LOG_Info() << "GRT Reloaded closed";
    event->accept();
}
