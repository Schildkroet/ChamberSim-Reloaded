#include "tablebrowser.h"

#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QSortFilterProxyModel>
#include <QSqlTableModel>
#include <QTableView>
#include <QVBoxLayout>

TableBrowser::TableBrowser(grt::Table table, QWidget *parent)
    : QWidget(parent)
    , m_table(table)
    , m_view(new QTableView)
    , m_filter(new QLineEdit)
    , m_count(new QLabel)
    , m_proxy(new QSortFilterProxyModel(this))
{
    m_filter->setPlaceholderText(tr("Filter (all columns)…"));
    m_filter->setClearButtonEnabled(true);

    m_proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxy->setFilterKeyColumn(-1);
    m_view->setModel(m_proxy);
    m_view->setSortingEnabled(true);
    m_view->setAlternatingRowColors(true);
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_view->setSelectionMode(QAbstractItemView::SingleSelection);
    m_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_view->horizontalHeader()->setStretchLastSection(false);
    m_view->verticalHeader()->setDefaultSectionSize(22);
    m_view->verticalHeader()->hide();

    auto *top = new QHBoxLayout;
    top->addWidget(m_filter, 1);
    top->addWidget(m_count);
    auto *layout = new QVBoxLayout(this);
    layout->addLayout(top);
    layout->addWidget(m_view, 1);

    connect(m_filter, &QLineEdit::textChanged, this, [this](const QString &t) {
        m_proxy->setFilterFixedString(t);
        updateCount();
    });
    connect(m_view->selectionModel(), &QItemSelectionModel::currentRowChanged, this,
            [this] { emit currentIdChanged(currentId()); });
}

TableBrowser::~TableBrowser() = default;

void TableBrowser::setDatabase(const grt::Database *db)
{
    m_proxy->setSourceModel(nullptr);
    delete std::exchange(m_model, nullptr);
    if (!db || !db->isOpen()) {
        updateCount();
        return;
    }
    m_model = new QSqlTableModel(this, db->handle());
    m_model->setTable(QLatin1String(grt::tableName(m_table)));
    m_model->setFilter(baseFilter());
    m_model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    m_model->select();
    while (m_model->canFetchMore()) m_model->fetchMore();  // the tables are small enough to load completely
    m_proxy->setSourceModel(m_model);
    m_proxy->sort(m_model->fieldIndex(QStringLiteral("id")), Qt::AscendingOrder);
    for (const char *hidden : {"geloescht"})
        m_view->setColumnHidden(m_model->fieldIndex(QLatin1String(hidden)), true);
    m_view->resizeColumnsToContents();
    for (int c = 0; c < m_model->columnCount(); ++c)
        m_view->setColumnWidth(c, std::min(m_view->columnWidth(c), 220));
    updateCount();
}

QString TableBrowser::baseFilter() const
{
    return m_rowFilter.isEmpty() ? QStringLiteral("geloescht = 0") : QStringLiteral("geloescht = 0 AND (%1)").arg(m_rowFilter);
}

void TableBrowser::setRowFilter(const QString &sqlCondition)
{
    if (m_rowFilter == sqlCondition) return;
    m_rowFilter = sqlCondition;
    if (!m_model) return;
    m_model->setFilter(baseFilter());
    m_model->select();
    while (m_model->canFetchMore()) m_model->fetchMore();
    updateCount();
}

bool TableBrowser::selectId(int id)
{
    if (!m_model) return false;
    const int col = m_model->fieldIndex(QStringLiteral("id"));
    for (int r = 0; r < m_proxy->rowCount(); ++r) {
        if (m_proxy->data(m_proxy->index(r, col)).toInt() == id) {
            m_view->setCurrentIndex(m_proxy->index(r, col));
            m_view->scrollTo(m_proxy->index(r, col));
            return true;
        }
    }
    return false;
}

int TableBrowser::currentId() const
{
    if (!m_model) return -1;
    const QModelIndex idx = m_view->currentIndex();
    if (!idx.isValid()) return -1;
    const QModelIndex src = m_proxy->mapToSource(idx);
    return m_model->data(m_model->index(src.row(), m_model->fieldIndex(QStringLiteral("id")))).toInt();
}

void TableBrowser::updateCount()
{
    const int shown = m_proxy->rowCount();
    const int total = m_model ? m_model->rowCount() : 0;
    m_count->setText(shown == total ? tr("%n entries", nullptr, total) : tr("%1 of %n entries", nullptr, total).arg(shown));
}
