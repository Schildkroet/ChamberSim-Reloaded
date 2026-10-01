#pragma once

#include "database.h"
#include "recorddialog.h"
#include "tablebrowser.h"

#include <QMainWindow>

#include <array>
#include <vector>
#include <memory>

class LoadView;
class QAction;
class QMenuBar;
class TitleBar;
class QPushButton;
class QTabWidget;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    bool openLoad(const QString &path);
    // The menu bar (it sits in the title bar row; use this instead of QMainWindow::menuBar()).
    [[nodiscard]] QMenuBar *mainMenu() const noexcept { return m_menuBar; }
    // Draws the window's own title bar and border (frameless window) instead of the system's.
    void setCustomTitleBar(bool on);
    [[nodiscard]] bool customTitleBar() const noexcept { return m_customTitleBar; }
    // Opens the own database and attaches the original one (empty: none); `complain` shows a message on errors.
    bool openDatabase(const QString &originalPath, bool complain = true);
    // Applies a reference-runner result (*.json) to the current load.
    bool overlayReference(const QString &jsonPath);
    void showTab(int index);
    void showSettings();
    void showResultTab(int index);
    // Opens the "New load" dialog (needs an open database).
    void newLoad();

    // Database tab: the table shown and its selected record.
    [[nodiscard]] grt::Table currentTable() const;
    // Opens the record dialog (New: empty, Copy/Edit: the selected record) and stores the result.
    void editRecord(RecordDialog::Mode mode);
    // Stores a new (New, Copy) or changed (Edit, `id`) record in the own database and shows it; returns its id or -1.
    int storeRecord(grt::Table table, RecordDialog::Mode mode, int id, const QVariantMap &values);
    // Deletes the selected own record after asking (`ask` false: without asking).
    bool deleteRecord(bool ask = true);
    // Writes the selected own record as a project data file (to contribute it); empty `path`: asks for one.
    bool exportRecord(const QString &path = {});

protected:
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    [[nodiscard]] QString databaseSummary() const;
    void updateRecordButtons();
    [[nodiscard]] TableBrowser *currentBrowser() const;
    void createActions();
    [[nodiscard]] LoadView *currentLoad() const;
    [[nodiscard]] bool maybeSave(LoadView *view);
    void updateTabTitle(LoadView *view);
    void setLoadViewDatabases(bool available);
    void addRecentFile(const QString &path);
    void updateRecentActions();
    void updateActions();
    void setupLoadView(LoadView *view);
    void updateFrame();
    [[nodiscard]] Qt::Edges edgesAt(const QPoint &pos) const;
    bool saveLoad(LoadView *view, bool saveAs);

    std::unique_ptr<Ui::MainWindow> ui;
    QMenuBar *m_menuBar;
    TitleBar *m_titleBar;
    bool m_customTitleBar{false};
    QTabWidget *m_tabs;
    QTabWidget *m_dbTabs;
    QPushButton *m_newRecord{nullptr};
    QPushButton *m_copyRecord{nullptr};
    QPushButton *m_editRecord{nullptr};
    QPushButton *m_deleteRecord{nullptr};
    QPushButton *m_exportRecord{nullptr};
    std::array<TableBrowser *, 3> m_browsers{};
    grt::Database m_db;
    int m_untitled{0};
    QAction *m_newLoadAction{nullptr};
    QAction *m_saveAction{nullptr};
    QAction *m_clearReferenceAction{nullptr};
    std::vector<QAction *> m_loadActions;  // enabled while a load is shown
    std::vector<QAction *> m_recentActions;  // File menu: last used loads (File > Open recent)
    QMenu *m_recentMenu{nullptr};
};
