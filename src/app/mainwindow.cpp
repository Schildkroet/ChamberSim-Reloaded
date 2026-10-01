#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "about.h"
#include "loadview.h"
#include "newloaddialog.h"
#include "reportview.h"
#include "settingsdialog.h"
#include "theme.h"
#include "translation.h"
#include "recentfiles.h"
#include "unitprefs.h"

#include <QAction>
#include <QApplication>
#include <QActionGroup>
#include <QCloseEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QStatusBar>
#include <QTabBar>
#include <QTabWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(std::make_unique<Ui::MainWindow>())
    , m_tabs(new QTabWidget)
    , m_dbTabs(new QTabWidget)
{
    ui->setupUi(this);

    constexpr std::array tables{grt::Table::Caliber, grt::Table::Projectile, grt::Table::Propellant};
    const std::array titles{tr("Calibers"), tr("Projectiles"), tr("Propellants")};
    for (std::size_t i = 0; i < tables.size(); ++i) {
        m_browsers[i] = new TableBrowser(tables[i]);
        m_dbTabs->addTab(m_browsers[i], titles[i]);
    }

    m_tabs->setTabsClosable(true);
    m_tabs->setDocumentMode(true);
    m_tabs->addTab(m_dbTabs, tr("Database"));
    m_tabs->tabBar()->setTabButton(0, QTabBar::RightSide, nullptr);  // the database tab stays
    setCentralWidget(m_tabs);

    connect(m_tabs, &QTabWidget::tabCloseRequested, this, [this](int i) {
        auto *view = qobject_cast<LoadView *>(m_tabs->widget(i));
        if (!view || !maybeSave(view)) return;
        m_tabs->removeTab(i);
        view->deleteLater();
    });

    createActions();
    resize(1280, 800);
    restoreGeometry(QSettings().value(QStringLiteral("window/geometry")).toByteArray());

    const QString lastDb = QSettings().value(QStringLiteral("database/path")).toString();
    if (!lastDb.isEmpty() && QFileInfo::exists(lastDb))
        openDatabase(lastDb);
    else
        statusBar()->showMessage(tr("No database loaded: use File ▸ Open database…"));
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
    QMenu *file = menuBar()->addMenu(tr("&File"));
    auto add = [&](const QString &text, const QKeySequence &key, auto slot) {
        QAction *a = file->addAction(text, this, slot);
        a->setShortcut(key);
        return a;
    };
    m_newLoadAction = add(tr("&New load…"), QKeySequence::New, [this] { newLoad(); });
    add(tr("&Open load…"), QKeySequence::Open, [this] {
        const QStringList files = QFileDialog::getOpenFileNames(this, tr("Open load"), {}, tr("GRT loads (*.grtload);;All files (*)"));
        for (const QString &f : files) openLoad(f);
    });
    add(tr("&Save"), QKeySequence::Save, [this] { if (auto *v = currentLoad()) saveLoad(v, false); });
    add(tr("Save &as…"), QKeySequence::SaveAs, [this] { if (auto *v = currentLoad()) saveLoad(v, true); });
    // last used loads: a submenu next to "Open load", empty = disabled
    m_recentMenu = file->addMenu(tr("Open &recent"));
    for (int i = 0; i < recentfiles::kMaxFiles; ++i) {
        QAction *a = m_recentMenu->addAction(QString());
        connect(a, &QAction::triggered, this, [this, a] {
            const QString path = a->data().toString();
            if (!openLoad(path)) {  // gone or unreadable: forget it
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
    add(tr("Print pre&view…"), {}, [this] { if (auto *v = currentLoad()) v->report()->showPrintPreview(this); });
    add(tr("&Print…"), QKeySequence::Print, [this] { if (auto *v = currentLoad()) v->report()->showPrintDialog(this); });
    add(tr("Export report as P&DF…"), {}, [this] {
        auto *v = currentLoad();
        if (!v) return;
        const QString path = QFileDialog::getSaveFileName(this, tr("Export PDF"), {}, tr("PDF documents (*.pdf)"));
        if (!path.isEmpty() && !v->report()->exportPdf(path)) QMessageBox::warning(this, tr("Export PDF"), tr("Cannot write %1").arg(path));
    });
    file->addSeparator();
    add(tr("Open &database…"), {}, [this] {
        const QString f = QFileDialog::getOpenFileName(this, tr("Open database"), {}, tr("SQLite databases (*.db *.sqlite);;All files (*)"));
        if (!f.isEmpty()) openDatabase(f);
    });
    file->addSeparator();
    add(tr("Overlay &reference result…"), QKeySequence(Qt::CTRL | Qt::Key_R), [this] {
        const QString f = QFileDialog::getOpenFileName(this, tr("Reference result"), {}, tr("Reference results (*.json)"));
        if (!f.isEmpty()) overlayReference(f);
    });
    add(tr("&Clear reference overlay"), {}, [this] { if (auto *v = currentLoad()) v->setReference(std::nullopt); });
    file->addSeparator();
    add(tr("&Settings…"), QKeySequence(Qt::CTRL | Qt::Key_Comma), [this] { SettingsDialog(this).exec(); });
    file->addSeparator();
    add(tr("&Quit"), QKeySequence::Quit, [this] { close(); });

    // View > Theme
    QMenu *view = menuBar()->addMenu(tr("&View"));
    QMenu *themeMenu = view->addMenu(tr("&Theme"));
    auto *themeGroup = new QActionGroup(this);
    const theme::Mode currentTheme = theme::storedMode();
    const std::array<std::pair<QString, theme::Mode>, 3> themes{{{tr("System default"), theme::Mode::System},
                                                                  {tr("Light"), theme::Mode::Light},
                                                                  {tr("Dark"), theme::Mode::Dark}}};
    for (const auto &[label, mode] : themes) {
        QAction *a = themeMenu->addAction(label);
        a->setCheckable(true);
        a->setChecked(mode == currentTheme);
        themeGroup->addAction(a);
        connect(a, &QAction::triggered, this, [mode] {
            theme::storeMode(mode);
            theme::apply(*qobject_cast<QApplication *>(QCoreApplication::instance()), mode);
        });
    }

    // View > Language: "System default" plus every language that ships a translation
    QMenu *lang = view->addMenu(tr("&Language"));
    auto *group = new QActionGroup(this);
    const QString stored = translation::storedLanguage();
    auto addLanguage = [&](const QString &text, const QString &value) {
        QAction *a = lang->addAction(text);
        a->setCheckable(true);
        a->setChecked(stored == value);
        group->addAction(a);
        connect(a, &QAction::triggered, this, [this, value] {
            translation::storeLanguage(value);
            QMessageBox::information(this, tr("Language"), tr("The language change takes effect after restarting the application."));
        });
    };
    addLanguage(tr("System default"), {});
    for (const QString &l : translation::availableLanguages()) addLanguage(translation::displayName(l), l);

    // Help
    QMenu *help = menuBar()->addMenu(tr("&Help"));
    help->addAction(tr("&About GRT Reloaded…"), this, [this] {
        about::show(this, m_db.isOpen() ? m_db.path() : QString(), m_db.count(grt::Table::Caliber), m_db.count(grt::Table::Projectile),
                    m_db.count(grt::Table::Propellant));
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

void MainWindow::updateRecentActions()
{
    const QStringList files = recentfiles::load();
    for (std::size_t i = 0; i < m_recentActions.size(); ++i) {
        QAction *a = m_recentActions[i];
        const bool used = static_cast<qsizetype>(i) < files.size();
        a->setVisible(used);
        if (!used) continue;
        const QFileInfo info(files[static_cast<qsizetype>(i)]);
        // "&1 name.grtload" (numbers 10-12 without accelerator); the tooltip/status tip shows the full path
        a->setText((i < 9 ? QStringLiteral("&%1 ").arg(i + 1) : QStringLiteral("%1 ").arg(i + 1)) + info.fileName().replace(u'&', QStringLiteral("&&")));
        a->setData(files[static_cast<qsizetype>(i)]);
        a->setStatusTip(files[static_cast<qsizetype>(i)]);
        a->setToolTip(files[static_cast<qsizetype>(i)]);
    }
    m_recentMenu->setEnabled(!files.isEmpty());
}

bool MainWindow::openLoad(const QString &path)
{
    for (int i = 0; i < m_tabs->count(); ++i) {  // already open: just activate
        if (auto *v = qobject_cast<LoadView *>(m_tabs->widget(i)); v && v->path() == path) {
            m_tabs->setCurrentIndex(i);
            addRecentFile(path);
            return true;
        }
    }
    auto *view = new LoadView;
    QString error;
    if (!view->load(path, &error)) {
        delete view;
        QMessageBox::warning(this, tr("Open load"), tr("Cannot read %1:\n%2").arg(path, error));
        return false;
    }
    view->setDatabase(&m_db);
    connect(view, &LoadView::modifiedChanged, this, [this, view] { updateTabTitle(view); });
    const int idx = m_tabs->addTab(view, QString());
    m_tabs->setCurrentIndex(idx);
    updateTabTitle(view);
    addRecentFile(path);
    return true;
}

bool MainWindow::openDatabase(const QString &path)
{
    for (TableBrowser *b : m_browsers) b->setDatabase(nullptr);  // release the old connection first
    setLoadViewDatabases(false);
    QString error;
    if (!m_db.open(path, true, &error)) {
        QMessageBox::warning(this, tr("Open database"),
                             tr("Cannot open %1:\n%2\n\nThe original GordonsReloadingTool.db is encrypted; "
                                "use a decrypted copy (see tools/grt_decrypt_db.sh).").arg(path, error));
        return false;
    }
    for (TableBrowser *b : m_browsers) b->setDatabase(&m_db);
    setLoadViewDatabases(true);
    QSettings().setValue(QStringLiteral("database/path"), path);
    statusBar()->showMessage(tr("%1: %2 calibers, %3 projectiles, %4 propellants")
                                 .arg(QFileInfo(path).fileName())
                                 .arg(m_db.count(grt::Table::Caliber))
                                 .arg(m_db.count(grt::Table::Projectile))
                                 .arg(m_db.count(grt::Table::Propellant)));
    return true;
}

bool MainWindow::overlayReference(const QString &jsonPath)
{
    LoadView *view = currentLoad();
    if (!view) {
        QMessageBox::information(this, tr("Reference"), tr("Open a load first."));
        return false;
    }
    QString error;
    auto ref = grt::readReferenceRun(jsonPath, &error);
    if (!ref) {
        QMessageBox::warning(this, tr("Reference"), tr("Cannot read %1:\n%2").arg(jsonPath, error));
        return false;
    }
    if (ref->stale)
        QMessageBox::warning(this, tr("Reference"), tr("This reference run is marked stale (the wrong tab was read)."));
    view->setReference(std::move(ref));
    return true;
}

void MainWindow::newLoad()
{
    if (!m_db.isOpen()) {
        QMessageBox::information(this, tr("New load"), tr("Open a database first (File ▸ Open database…)."));
        return;
    }
    NewLoadDialog dlg(m_db, this);
    if (dlg.exec() != QDialog::Accepted) return;
    auto *view = new LoadView;
    view->setNewLoad(dlg.result());
    view->setProperty("untitledName", tr("Untitled %1").arg(++m_untitled));
    view->setDatabase(&m_db);
    connect(view, &LoadView::modifiedChanged, this, [this, view] { updateTabTitle(view); });
    const int idx = m_tabs->addTab(view, QString());
    m_tabs->setCurrentIndex(idx);
    updateTabTitle(view);
}

void MainWindow::showTab(int index)
{
    if (index >= 0 && index < m_tabs->count()) m_tabs->setCurrentIndex(index);
}

void MainWindow::updateTabTitle(LoadView *view)
{
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
    if (saveAs || path.isEmpty()) {
        path = QFileDialog::getSaveFileName(this, tr("Save load"), path, tr("GRT loads (*.grtload)"));
        if (path.isEmpty()) return false;
        path = withLoadExtension(path);
    }
    QString error;
    if (!view->save(path, &error)) {
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
    const auto answer = QMessageBox::question(this, tr("Unsaved changes"),
                                              tr("Save changes to %1?").arg(view->path().isEmpty() ? view->property("untitledName").toString() : QFileInfo(view->path()).fileName()),
                                              QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (answer == QMessageBox::Cancel) return false;
    return answer == QMessageBox::Discard || saveLoad(view, false);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    for (int i = 0; i < m_tabs->count(); ++i) {
        if (auto *v = qobject_cast<LoadView *>(m_tabs->widget(i)); v && !maybeSave(v)) {
            event->ignore();
            return;
        }
    }
    QSettings().setValue(QStringLiteral("window/geometry"), saveGeometry());
    event->accept();
}
