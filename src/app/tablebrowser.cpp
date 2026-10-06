#include "tablebrowser.h"
#include "log.h"
#include "qualitydelegate.h"

#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QSortFilterProxyModel>
#include <QSqlError>
#include <QSqlTableModel>
#include <QTableView>
#include <QVBoxLayout>

namespace
{

// Shows the "source" column (which part of the database a record comes from) as readable text.
class RecordModel : public QSqlTableModel
{
public:
    using QSqlTableModel::QSqlTableModel;

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (index.column() == sourceColumn() && (role == Qt::DisplayRole || role == Qt::ToolTipRole))
        {
            const QString code = QSqlTableModel::data(index, Qt::DisplayRole).toString();
            if (role == Qt::ToolTipRole)
            {
                if (code == QLatin1String(grt::Database::kSourceOwn)) return TableBrowser::tr("Your own record (can be changed)");
                if (code == QLatin1String(grt::Database::kSourceProject))
                    return TableBrowser::tr("Record of the project's database (read-only)");
                return TableBrowser::tr("Record of Gordon's Reloading Tool's database (read-only)");
            }
            if (code == QLatin1String(grt::Database::kSourceOwn)) return TableBrowser::tr("Personal");
            if (code == QLatin1String(grt::Database::kSourceProject)) return TableBrowser::tr("Project");
            return TableBrowser::tr("Original GRT");
        }
        return QSqlTableModel::data(index, role);
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override
    {
        if (orientation == Qt::Horizontal && section == sourceColumn() && role == Qt::DisplayRole) return TableBrowser::tr("Source");
        return QSqlTableModel::headerData(section, orientation, role);
    }

private:
    [[nodiscard]] int sourceColumn() const { return fieldIndex(QStringLiteral("source")); }
};

}  // namespace

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

    connect(m_filter, &QLineEdit::textChanged, this,
            [this](const QString &t)
            {
                m_proxy->setFilterFixedString(t);
                updateCount();
            });
    connect(m_view->selectionModel(), &QItemSelectionModel::currentRowChanged, this, [this] { emit currentIdChanged(currentId()); });
    connect(m_view, &QTableView::activated, this,
            [this]
            {
                if (const int id = currentId(); id >= 0) emit idActivated(id);
            });
}

TableBrowser::~TableBrowser() = default;

void TableBrowser::setDatabase(const grt::Database *db)
{
    m_proxy->setSourceModel(nullptr);
    delete std::exchange(m_model, nullptr);
    if (!db || !db->isOpen())
    {
        updateCount();
        return;
    }
    m_model = new RecordModel(this, db->handle());
    m_model->setTable(QLatin1String(grt::tableName(m_table)));
    m_model->setFilter(baseFilter());
    m_model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    selectAll();
    m_proxy->setSourceModel(m_model);
    const int idColumn = m_model->fieldIndex(QStringLiteral("id"));
    m_view->sortByColumn(idColumn, Qt::AscendingOrder);  // the view would sort by its first column (the source)
    for (const char *hidden : {"geloescht"}) m_view->setColumnHidden(m_model->fieldIndex(QLatin1String(hidden)), true);
    if (const int quality = m_model->fieldIndex(QStringLiteral("Qlty")); quality >= 0)  // propellants: the fit quality as a bar
        if (!dynamic_cast<QualityDelegate *>(m_view->itemDelegateForColumn(quality)))   // (a new database keeps the delegate)
            m_view->setItemDelegateForColumn(quality, new QualityDelegate(m_view));
    if (const int quality = m_model->fieldIndex(QStringLiteral("Qlty")), name = m_model->fieldIndex(QStringLiteral("pname"));
        quality >= 0 && name >= 0)  // propellants: the quality right behind the name, where it is seen without scrolling
    {
        auto *header = m_view->horizontalHeader();
        header->moveSection(header->visualIndex(quality),
                            header->visualIndex(name) + (header->visualIndex(quality) < header->visualIndex(name) ? 0 : 1));
    }
    m_view->resizeColumnsToContents();
    for (int c = 0; c < m_model->columnCount(); ++c) m_view->setColumnWidth(c, std::min(m_view->columnWidth(c), 220));
    // own records have 8-digit ids (only the visible rows were measured)
    m_view->setColumnWidth(
        idColumn, std::max(m_view->columnWidth(idColumn), m_view->fontMetrics().horizontalAdvance(QStringLiteral("00000000")) + 16));
    updateCount();
}

void TableBrowser::selectAll()
{
    if (!m_model->select())
    {
        LOG_Warn() << grt::tableName(m_table) << ":" << m_model->lastError().text();
        return;
    }
    while (m_model->canFetchMore()) m_model->fetchMore();
}

QString TableBrowser::baseFilter() const
{
    return m_rowFilter.isEmpty() ? QStringLiteral("geloescht = 0") : QStringLiteral("geloescht = 0 AND (%1)").arg(m_rowFilter);
}

void TableBrowser::refresh()
{
    if (!m_model) return;
    const int id = currentId();
    selectAll();
    updateCount();
    if (id >= 0) selectId(id);
}

void TableBrowser::setRowFilter(const QString &sqlCondition)
{
    if (m_rowFilter == sqlCondition) return;
    m_rowFilter = sqlCondition;
    if (!m_model) return;
    m_model->setFilter(baseFilter());
    selectAll();
    updateCount();
}

bool TableBrowser::selectId(int id)
{
    if (!m_model) return false;
    const int col = m_model->fieldIndex(QStringLiteral("id"));
    for (int r = 0; r < m_proxy->rowCount(); ++r)
    {
        if (m_proxy->data(m_proxy->index(r, col)).toInt() == id)
        {
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

double TableBrowser::currentNumber(const QString &column) const
{
    if (!m_model) return 0.0;
    const int col = m_model->fieldIndex(column);
    const QModelIndex idx = m_view->currentIndex();
    if (col < 0 || !idx.isValid()) return 0.0;
    return m_model->data(m_model->index(m_proxy->mapToSource(idx).row(), col)).toDouble();
}

void TableBrowser::updateCount()
{
    const int shown = m_proxy->rowCount();
    const int total = m_model ? m_model->rowCount() : 0;
    m_count->setText(shown == total ? tr("%n entries", nullptr, total) : tr("%1 of %n entries", nullptr, total).arg(shown));
}
