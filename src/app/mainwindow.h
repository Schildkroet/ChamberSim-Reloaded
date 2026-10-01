#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "database.h"
#include "recorddialog.h"
#include "tablebrowser.h"

#include <QMainWindow>

#include <array>
#include <vector>
#include <memory>

class LoadView;
class QAction;
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

protected:
    void closeEvent(QCloseEvent *event) override;

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
    bool saveLoad(LoadView *view, bool saveAs);

    std::unique_ptr<Ui::MainWindow> ui;
    QTabWidget *m_tabs;
    QTabWidget *m_dbTabs;
    QPushButton *m_newRecord{nullptr};
    QPushButton *m_copyRecord{nullptr};
    QPushButton *m_editRecord{nullptr};
    QPushButton *m_deleteRecord{nullptr};
    std::array<TableBrowser *, 3> m_browsers{};
    grt::Database m_db;
    int m_untitled{0};
    QAction *m_newLoadAction{nullptr};
    std::vector<QAction *> m_recentActions;  // File menu: last used loads (File > Open recent)
    QMenu *m_recentMenu{nullptr};
};
#endif // MAINWINDOW_H
