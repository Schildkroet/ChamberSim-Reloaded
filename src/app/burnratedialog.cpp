#include "burnratedialog.h"
#include "searchfilter.h"

#include "screenfit.h"
#include "burnfactors.h"
#include "qualitydelegate.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QSettings>
#include <QSortFilterProxyModel>
#include <QSqlQuery>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QTableView>
#include <QVBoxLayout>

#include <algorithm>
#include <array>
#include <map>

namespace
{
enum Column
{
    Manufacturer,
    Name,
    Lot,
    Ba,
    Br,
    Bp,
    Brp,
    Quality,
    Search,
    ColumnCount
};
constexpr int kValueRole = Qt::UserRole;
constexpr int kShareRole = Qt::UserRole + 1;  // value / largest value of the column: the length of the bar

// A number with a bar behind it (the share of the largest value in the column).
class BarDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        const double share = std::clamp(index.data(kShareRole).toDouble(), 0.0, 1.0);
        if (share > 0.0)
        {
            QColor bar = option.palette.color(QPalette::Highlight);
            bar.setAlpha(option.state & QStyle::State_Selected ? 90 : 70);
            QRect r = option.rect.adjusted(1, 2, -1, -2);
            r.setWidth(static_cast<int>(r.width() * share));
            painter->fillRect(r, bar);
        }
        QStyledItemDelegate::paint(painter, option, index);
    }
};

[[nodiscard]] double number(const QSqlQuery &q, int i)
{
    return q.value(i).toDouble();
}
}  // namespace

BurnRateDialog::BurnRateDialog(const grt::Database &db, QWidget *parent)
    : QDialog(parent)
    , m_model(new QStandardItemModel(0, ColumnCount, this))
    , m_proxy(new SearchFilterProxy(this))
    , m_view(new QTableView)
    , m_search(new QLineEdit)
    , m_allLots(new QCheckBox(tr("All lots")))
    , m_buttons(new QButtonGroup(this))
    , m_count(new QLabel)
    , m_db(db)
{
    setWindowTitle(tr("Burn rate table"));
    resizeWithinScreen(this, QSettings().value(QStringLiteral("burnrate/size"), QSize(940, 640)).toSize());

    m_model->setHorizontalHeaderLabels({tr("Manufacturer"), tr("Name"), tr("Lot"), tr("Burn rate (Ba)"), tr("Brisance (Br)"),
                                        tr("Progressivity (Bp)"), tr("Combined (Brp)"), tr("Quality"), QString()});
    m_proxy->setSourceModel(m_model);
    m_proxy->setSortRole(kValueRole);
    m_proxy->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_proxy->setFilterKeyColumn(Search);
    m_proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);

    m_view->setModel(m_proxy);
    m_view->setColumnHidden(Search, true);
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_view->setSelectionMode(QAbstractItemView::SingleSelection);
    m_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_view->setAlternatingRowColors(true);
    m_view->setSortingEnabled(true);
    m_view->verticalHeader()->setDefaultSectionSize(fontMetrics().height() + 8);
    m_view->horizontalHeader()->setStretchLastSection(false);
    m_view->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    auto *bars = new BarDelegate(m_view);
    for (const int c : {Ba, Br, Bp, Brp}) m_view->setItemDelegateForColumn(c, bars);
    m_view->setItemDelegateForColumn(Quality, new QualityDelegate(m_view, kValueRole));

    // the three sorts of the original: progressivity, brisance, both combined
    auto *top = new QHBoxLayout;
    top->addWidget(new QLabel(tr("Sort by:")));
    const std::array<std::pair<Factor, QString>, 3> sorts{
        {{Progressivity, tr("Progressivity")}, {Brisance, tr("Brisance")}, {Combined, tr("Brisance && progressivity")}}};
    for (const auto &[factor, text] : sorts)
    {
        auto *b = new QPushButton(text);
        b->setCheckable(true);
        m_buttons->addButton(b, factor);
        top->addWidget(b);
    }
    m_buttons->setExclusive(true);
    top->addStretch(1);
    m_search->setPlaceholderText(tr("Search manufacturer or name"));
    m_search->setClearButtonEnabled(true);
    m_search->setMinimumWidth(220);
    top->addWidget(m_search);

    auto *note =
        new QLabel(tr("The table is a rough overview of how the powders of different manufacturers compare. It says nothing about how "
                      "a powder behaves in the pressure curve of a particular load: powders at the same level are not 1:1 substitutes."));
    note->setWordWrap(true);
    note->setForegroundRole(QPalette::PlaceholderText);

    auto *bottom = new QHBoxLayout;
    bottom->addWidget(m_allLots);
    bottom->addStretch(1);
    bottom->addWidget(m_count);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(top);
    layout->addWidget(m_view, 1);
    layout->addLayout(bottom);
    layout->addWidget(note);

    connect(m_buttons, &QButtonGroup::idClicked, this, [this](int id) { setFactor(static_cast<Factor>(id)); });
    connect(m_search, &QLineEdit::textChanged, this, [this] { applyFilter(); });
    connect(m_allLots, &QCheckBox::toggled, this,
            [this](bool on)
            {
                QSettings().setValue(QStringLiteral("burnrate/allLots"), on);
                load(m_db);
            });
    connect(m_view->horizontalHeader(), &QHeaderView::sortIndicatorChanged, this,
            [this](int column, Qt::SortOrder)
            {
                if (column >= Br && column <= Brp)  // clicking a factor column selects that sort
                {
                    m_factor = column == Br ? Brisance : column == Bp ? Progressivity : Combined;
                    if (auto *b = m_buttons->button(m_factor)) b->setChecked(true);
                }
                else
                {
                    m_buttons->setExclusive(false);
                    if (auto *b = m_buttons->checkedButton()) b->setChecked(false);
                    m_buttons->setExclusive(true);
                }
            });

    const QSettings settings;
    m_allLots->blockSignals(true);
    m_allLots->setChecked(settings.value(QStringLiteral("burnrate/allLots"), false).toBool());
    m_allLots->blockSignals(false);
    load(db);
    m_view->setColumnWidth(Manufacturer, 140);
    m_view->setColumnWidth(Name, 140);
    m_view->setColumnWidth(Lot, 90);
    for (const int c : {Ba, Br, Bp, Brp}) m_view->setColumnWidth(c, 115);
    m_view->setColumnWidth(Quality, 100);
    setFactor(static_cast<Factor>(settings.value(QStringLiteral("burnrate/factor"), Progressivity).toInt()));
}

BurnRateDialog::~BurnRateDialog()
{
    QSettings().setValue(QStringLiteral("burnrate/size"), size());
}

void BurnRateDialog::load(const grt::Database &db)
{
    struct Row
    {
        QString maker, name, lot;
        double ba, br, bp, brp, quality;
    };
    std::vector<Row> rows;
    QSqlQuery q(db.handle());
    if (q.exec(QStringLiteral(
            "SELECT mname, pname, lotid, Ba, Br, Bp, Brp, Qex, k, a0, z1, z2, Qlty FROM propellant WHERE geloescht = 0 AND Ba > 0")))
        while (q.next())
        {
            Row r{q.value(0).toString(), q.value(1).toString(), q.value(2).toString(), number(q, 3),
                  number(q, 4),          number(q, 5),          number(q, 6),          number(q, 12)};
            if (r.br <= 0.0)  // records without stored factors: derived from the model values
            {
                const grt::BurnFactors f = grt::burnFactors(r.ba, number(q, 7), number(q, 8), number(q, 9), number(q, 10), number(q, 11));
                r.br = f.Br;
                r.bp = f.Bp;
                r.brp = f.Brp;
            }
            if (r.br > 0.0) rows.push_back(std::move(r));
        }
    if (!m_allLots->isChecked())  // one row per powder: the newest lot
    {
        std::map<QString, std::size_t> newest;
        for (std::size_t i = 0; i < rows.size(); ++i)
        {
            const QString key = rows[i].maker.toLower() + u'\n' + rows[i].name.toLower();
            const auto [it, added] = newest.try_emplace(key, i);
            if (!added && rows[i].lot > rows[it->second].lot) it->second = i;
        }
        std::vector<Row> kept;
        for (const auto &[key, i] : newest) kept.push_back(rows[i]);
        rows = std::move(kept);
    }

    double largest[ColumnCount]{};
    for (const Row &r : rows)
    {
        largest[Ba] = std::max(largest[Ba], r.ba);
        largest[Br] = std::max(largest[Br], r.br);
        largest[Bp] = std::max(largest[Bp], r.bp);
        largest[Brp] = std::max(largest[Brp], r.brp);
    }
    const bool sorted = m_view->isSortingEnabled();
    m_view->setSortingEnabled(false);
    m_model->removeRows(0, m_model->rowCount());
    for (const Row &r : rows)
    {
        QList<QStandardItem *> items;
        const auto text = [](const QString &t)
        {
            auto *item = new QStandardItem(t);
            item->setData(t.toLower(), kValueRole);
            return item;
        };
        const auto quality = [](double v)  // the model quality 0..1 of the record (0: unknown)
        {
            auto *item = new QStandardItem(v > 0.0 ? QLocale().toString(v, 'f', 2) : QStringLiteral("-"));
            item->setData(v, kValueRole);
            item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            return item;
        };
        const auto value = [&](double v, int column)
        {
            auto *item = new QStandardItem(QLocale().toString(v, 'f', 4));
            item->setData(v, kValueRole);
            item->setData(largest[column] > 0.0 ? v / largest[column] : 0.0, kShareRole);
            item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            return item;
        };
        items << text(r.maker) << text(r.name) << text(r.lot) << value(r.ba, Ba) << value(r.br, Br) << value(r.bp, Bp) << value(r.brp, Brp)
              << quality(r.quality) << text(r.maker + u' ' + r.name);
        m_model->appendRow(items);
    }
    m_view->setSortingEnabled(sorted);
    if (sorted) setFactor(m_factor);
    applyFilter();
}

void BurnRateDialog::setFactor(Factor factor)
{
    m_factor = factor;
    QSettings().setValue(QStringLiteral("burnrate/factor"), static_cast<int>(factor));
    if (auto *b = m_buttons->button(factor)) b->setChecked(true);
    const int column = factor == Brisance ? Br : factor == Progressivity ? Bp : Brp;
    m_view->sortByColumn(column, Qt::DescendingOrder);  // the fastest powders first
}

void BurnRateDialog::applyFilter()
{
    m_proxy->setQuery(m_search->text());
    m_count->setText(tr("%n propellants", nullptr, m_proxy->rowCount()));
}

int BurnRateDialog::rowCount() const
{
    return m_proxy->rowCount();
}

QString BurnRateDialog::powderAt(int row) const
{
    return m_proxy->index(row, Search).data().toString();
}

double BurnRateDialog::valueAt(int row, Factor factor) const
{
    return m_proxy->index(row, factor == Brisance ? Br : factor == Progressivity ? Bp : Brp).data(kValueRole).toDouble();
}

void BurnRateDialog::setSearch(const QString &text)
{
    m_search->setText(text);
}
