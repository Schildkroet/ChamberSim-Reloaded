#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "database.h"
#include "tablebrowser.h"

#include <QMainWindow>

#include <array>
#include <vector>
#include <memory>

class LoadView;
class QAction;
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
    bool openDatabase(const QString &path);
    // Applies a reference-runner result (*.json) to the current load.
    bool overlayReference(const QString &jsonPath);
    void showTab(int index);
    // Opens the "New load" dialog (needs an open database).
    void newLoad();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
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
    std::array<TableBrowser *, 3> m_browsers{};
    grt::Database m_db;
    int m_untitled{0};
    QAction *m_newLoadAction{nullptr};
    std::vector<QAction *> m_recentActions;  // File menu: last used loads (File > Open recent)
    QMenu *m_recentMenu{nullptr};
};
#endif // MAINWINDOW_H
