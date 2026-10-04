#include "propertypanel.h"

#include "fieldnames.h"

#include "shotparts.h"
#include "unitprefs.h"

using namespace Qt::StringLiterals;

#include <algorithm>
#include <array>
#include <QComboBox>
#include <QLineEdit>
#include <QHeaderView>
#include <QSettings>
#include <QStyledItemDelegate>
#include <QTableWidget>
#include <QVBoxLayout>

namespace
{

// Title case for the labels: the first letter of every word is upper case (the rest is left alone, so CIP stays CIP).
void capitalizeWords(QString &text)
{
    bool wordStart = true;
    for (QChar &c : text)
    {
        if (wordStart && c.isLetter()) c = c.toUpper();
        wordStart = c.isSpace() || c == u'(' || c == u'/' || c == u'-';
    }
}

struct Choice
{
    QString code;   // what is stored in the file
    QString label;  // what the user sees
    QString tip;    // explanation shown as a tooltip of the entry (empty: none)
};

// What a part or group does to the model: its factors, and for a group its members.
QString partTip(const grt::ShotPartInfo &info)
{
    const QString factors =
        PropertyPanel::tr("Powder space ×%1, gas energy ×%2").arg(info.capacityScale, 0, 'f', 2).arg(info.energyScale, 0, 'f', 2);
    if (info.group)
        return PropertyPanel::tr("A group of %n part(s) that give a similar pressure:", nullptr, static_cast<int>(info.members.size())) +
               u"\n  "_s + info.members.join(u"\n  "_s) + u"\n\n"_s + factors + u". "_s +
               PropertyPanel::tr("A larger powder space gives a lower pressure.");
    return factors +
           PropertyPanel::tr(" (fitted to %n published load(s)). A larger powder space gives a lower pressure.", nullptr, info.loads);
}

// The choices of a part of a shotshell: "not specified" (the model's average) and the parts of the data file; wads of other gauges than 12
// show their gauge.
const QList<Choice> &shotPartChoices(grt::ShotPart part)
{
    static const std::array<QList<Choice>, 3> lists = []
    {
        std::array<QList<Choice>, 3> all;
        for (const grt::ShotPart p : grt::kShotParts)
        {
            auto &list = all[static_cast<std::size_t>(p)];
            list.push_back({QString(), PropertyPanel::tr("not specified (average)"),
                            PropertyPanel::tr("No correction: the average of all published loads.")});
            // general entries first: groups of parts that give a similar pressure (for those who do not know the exact part)
            const QString kind = p == grt::ShotPart::Shell ? PropertyPanel::tr("shells")
                                 : p == grt::ShotPart::Wad ? PropertyPanel::tr("wads")
                                                           : PropertyPanel::tr("primers");
            for (const grt::ShotPartInfo &info : grt::shotPartGroups(p))
            {
                const QString text = info.rank == u"low"    ? PropertyPanel::tr("Low-pressure %1 (e.g. %2)")
                                     : info.rank == u"high" ? PropertyPanel::tr("High-pressure %1 (e.g. %2)")
                                                            : PropertyPanel::tr("Medium-pressure %1 (e.g. %2)");
                list.push_back({info.id, text.arg(kind, info.examples), partTip(info)});
            }
            for (const grt::ShotPartInfo &info : grt::shotParts(p))
                list.push_back(
                    {info.id,
                     info.gauge && info.gauge != 12 ? PropertyPanel::tr("%1 (%2 gauge)").arg(info.name).arg(info.gauge) : info.name,
                     partTip(info)});
        }
        return all;
    }();
    return lists[static_cast<std::size_t>(part)];
}

// Fields whose stored value is a code: shown as readable text and edited with a drop-down.
const QList<Choice> *choicesFor(const QString &key)
{
    static const QList<Choice> tail{
        {u"0"_s, PropertyPanel::tr("flat base")}, {u"1"_s, PropertyPanel::tr("hollow base")}, {u"2"_s, PropertyPanel::tr("boat tail")}};
    static const QList<Choice> lock{{u"false"_s, PropertyPanel::tr("locked")}, {u"true"_s, PropertyPanel::tr("unlocked")}};
    static const QList<Choice> yesNo{{u"false"_s, PropertyPanel::tr("no")}, {u"true"_s, PropertyPanel::tr("yes")}};
    static const QList<Choice> ladder = []
    {
        QList<Choice> l;
        for (int n = 1; n <= 10; ++n) l.push_back({QString::number(n), QString::number(n)});  // 1: no ladder
        return l;
    }();
    if (key == u"laddercnt") return &ladder;
    if (key == u"gTailType") return &tail;
    if (key == u"gfrictionUnlock") return &lock;
    if (key == u"shotshell") return &yesNo;
    // the shell, wad and primer of a shotshell load: the parts of the data file (their factors correct the model), or none
    for (const grt::ShotPart part : grt::kShotParts)
        if (key == grt::shotPartKey(part)) return &shotPartChoices(part);
    return nullptr;
}

// Fields of the file that are not shown: "gjumpUnlock" only locks the jump distance in the original's input form,
// the original uses the distance either way (measured, batch 10).
bool hiddenField(const QString &key)
{
    return key == u"gjumpUnlock" || key == u"physRef";  // physRef: the burn values the physical ones belong to
}

QString tipFor(const QList<Choice> &list, const QString &code)
{
    for (const Choice &c : list)
        if (c.code == code) return c.tip;
    return {};
}

QString labelFor(const QList<Choice> &list, const QString &code)
{
    for (const Choice &c : list)
        if (c.code == code) return c.label;
    if (!list.isEmpty() && list.front().code == list.front().label) return code;  // plain numbers: show what the file has
    if (!list.isEmpty() && list.front().code.isEmpty()) return code;  // names of parts: an id this version does not know stays visible
    return PropertyPanel::tr("type %1").arg(code);                    // a code we do not know: keep it visible
}

}  // namespace

// Value column: a drop-down for coded fields, the normal line edit for everything else.
class ValueDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        const QString key = index.data(PropertyPanel::KeyRole).toString();
        if (const auto *list = choicesFor(key))
        {
            auto *box = new QComboBox(parent);
            for (const Choice &c : *list)
            {
                box->addItem(c.label, c.code);
                if (!c.tip.isEmpty()) box->setItemData(box->count() - 1, c.tip, Qt::ToolTipRole);
            }
            connect(box, &QComboBox::activated, box, [box] { box->clearFocus(); });
            return box;
        }
        QWidget *editor = QStyledItemDelegate::createEditor(parent, option, index);
        // aligned like the shown value (right): the text does not jump to the left while it is edited
        if (auto *edit = qobject_cast<QLineEdit *>(editor))
            if (const QVariant a = index.data(Qt::TextAlignmentRole); a.isValid()) edit->setAlignment(Qt::Alignment(a.toInt()));
        return editor;
    }

    void setEditorData(QWidget *editor, const QModelIndex &index) const override
    {
        if (auto *box = qobject_cast<QComboBox *>(editor))
        {
            box->setCurrentIndex(std::max(0, box->findData(index.data(PropertyPanel::BaseRole).toString())));
            return;
        }
        QStyledItemDelegate::setEditorData(editor, index);
    }

    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override
    {
        if (auto *box = qobject_cast<QComboBox *>(editor))
        {
            model->setData(index, box->currentText(), Qt::DisplayRole);  // onItemChanged maps the label back to the code
            return;
        }
        QStyledItemDelegate::setModelData(editor, model, index);
    }
};

// Unit column: a drop-down with the alternative units of the field's quantity.
class UnitDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &index) const override
    {
        const auto q = static_cast<grt::units::Quantity>(index.data(PropertyPanel::QuantityRole).toInt());
        const auto list = grt::units::unitsOf(q);
        if (list.empty()) return nullptr;
        auto *box = new QComboBox(parent);
        for (const grt::units::Unit &u : list) box->addItem(QString::fromUtf8(u.symbol), QString::fromLatin1(u.id));
        connect(box, &QComboBox::activated, box, [box] { box->clearFocus(); });
        return box;
    }

    void setEditorData(QWidget *editor, const QModelIndex &index) const override
    {
        auto *box = static_cast<QComboBox *>(editor);
        box->setCurrentIndex(std::max(0, box->findText(index.data(Qt::DisplayRole).toString())));
    }

    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override
    {
        auto *box = static_cast<QComboBox *>(editor);
        const auto q = static_cast<grt::units::Quantity>(index.data(PropertyPanel::QuantityRole).toInt());
        const QString id = box->currentData().toString();
        const QString field = index.data(PropertyPanel::FieldRole).toString();
        // the system's own unit is stored as "no override" so that switching the system still works
        const bool isSystemUnit = id == QLatin1String(UnitPreferences::instance().unitFor(q).id);
        model->setData(index, box->currentText(), Qt::DisplayRole);
        UnitPreferences::instance().setFieldUnit(field, q, isSystemUnit ? QString() : id);
    }
};

// The widths of the property and value columns are remembered under `key` once the user has changed them.
void PropertyPanel::rememberColumns(const QString &key)
{
    m_settingsKey = key;
    const QList<QVariant> w = QSettings().value(QStringLiteral("layout/columns/") + key).toList();
    if (w.size() != 2) return;
    m_adjustingColumns = true;
    m_table->setColumnWidth(0, std::max(30, w[0].toInt()));
    m_table->setColumnWidth(1, std::max(30, w[1].toInt()));
    m_adjustingColumns = false;
    m_userSizedColumns = true;
}

void PropertyPanel::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (m_userSizedColumns) return;
    m_adjustingColumns = true;
    const int unitWidth = 60;
    m_table->setColumnWidth(0, std::max(80, m_table->viewport()->width() - m_table->columnWidth(1) - unitWidth));
    m_adjustingColumns = false;
}

PropertyPanel::PropertyPanel(QWidget *parent)
    : QWidget(parent)
    , m_table(new QTableWidget(0, 3))
{
    m_table->setHorizontalHeaderLabels({tr("Property"), tr("Value"), tr("Unit")});
    m_table->verticalHeader()->hide();
    m_table->horizontalHeader()->setStretchLastSection(true);
    // All borders can be dragged; the unit column takes what is left. Until the user drags a border, the
    // property column follows the panel width so the layout stays sensible.
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_table->horizontalHeader()->setMinimumSectionSize(30);
    m_table->setColumnWidth(1, 96);
    connect(m_table->horizontalHeader(), &QHeaderView::sectionResized, this,
            [this]
            {
                if (m_adjustingColumns) return;
                m_userSizedColumns = true;
                if (!m_settingsKey.isEmpty())
                    QSettings().setValue(QStringLiteral("layout/columns/") + m_settingsKey,
                                         QList<QVariant>{m_table->columnWidth(0), m_table->columnWidth(1)});
            });
    setMinimumWidth(300);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setAlternatingRowColors(true);
    m_table->setShowGrid(false);
    m_table->verticalHeader()->setDefaultSectionSize(24);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_table);
    m_table->setItemDelegateForColumn(1, new ValueDelegate(this));
    m_table->setItemDelegateForColumn(2, new UnitDelegate(this));
    m_table->setEditTriggers(QAbstractItemView::AllEditTriggers);
    connect(m_table, &QTableWidget::itemChanged, this, &PropertyPanel::onItemChanged);
    connect(&UnitPreferences::instance(), &UnitPreferences::changed, this, &PropertyPanel::refreshValues);
}

void PropertyPanel::setSections(const std::vector<Section> &sections)
{
    m_sections = sections;
    m_rebuilding = true;
    m_table->setRowCount(0);
    for (const Section &s : sections)
    {
        const int head = m_table->rowCount();
        m_table->insertRow(head);
        auto *h = new QTableWidgetItem(s.title);
        QFont f = h->font();
        f.setBold(true);
        h->setFont(f);
        h->setFlags(Qt::ItemIsEnabled);
        h->setBackground(palette().alternateBase());
        m_table->setItem(head, 0, h);
        m_table->setSpan(head, 0, 1, 3);

        for (const grt::Entry &e : s.entries->entries())
        {
            if (hiddenField(e.name)) continue;  // kept in the file, written back unchanged
            const int r = m_table->rowCount();
            m_table->insertRow(r);
            // a translated description is the finished label; the English one is tidied up here
            const QString translated = grt::translatedDescription(e.descr);
            const bool english = translated == e.descr;
            QString label = e.descr.isEmpty() ? e.name : translated;
            if (english && label.startsWith(u"true if "_s, Qt::CaseInsensitive)) label.remove(0, 8);  // "true if shotshell" -> "Shotshell"
            // explanations in brackets ("ladder steps (1 to 10, ...)") go to the tooltip, the label stays short
            QString hint;
            if (const qsizetype open = label.indexOf(u" ("); open > 0 && label.endsWith(u')'))
            {
                hint = label.mid(open + 2, label.size() - open - 3);
                label.truncate(open);
            }
            if (english) capitalizeWords(label);
            auto *name = new QTableWidgetItem(label);
            name->setToolTip(hint.isEmpty() ? e.name : QStringLiteral("%1\n%2").arg(e.name, hint));
            name->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            auto *val = new QTableWidgetItem;
            val->setData(SectionRole, s.id);
            val->setData(KeyRole, e.name);
            val->setData(BaseRole, e.value);
            val->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            auto *unit = new QTableWidgetItem;
            const auto q = grt::units::quantityOfFileUnit(e.unit);
            unit->setData(QuantityRole, static_cast<int>(q));
            unit->setData(FieldRole, s.key + u'/' + e.name);
            if (q == grt::units::Quantity::None)
            {
                unit->setText(e.unit);
                unit->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            }
            else
            {
                unit->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
                unit->setToolTip(tr("Click to choose another unit for this field"));
            }
            m_table->setItem(r, 0, name);
            m_table->setItem(r, 1, val);
            m_table->setItem(r, 2, unit);
            val->setData(QuantityRole, static_cast<int>(q));
            if (choicesFor(e.name)) val->setData(ChoiceRole, true);
        }
    }
    m_rebuilding = false;
    refreshValues();
}

void PropertyPanel::syncValues()
{
    m_rebuilding = true;
    for (int r = 0; r < m_table->rowCount(); ++r)
    {
        QTableWidgetItem *val = m_table->item(r, 1);
        if (!val || !val->data(SectionRole).isValid()) continue;
        const int id = val->data(SectionRole).toInt();
        const QString key = val->data(KeyRole).toString();
        for (const Section &s : m_sections)
            if (s.id == id)
                for (const grt::Entry &e : s.entries->entries())
                    if (e.name == key) val->setData(BaseRole, e.value);
    }
    m_rebuilding = false;
    refreshValues();
}

// Shows every value in its field's current unit.
void PropertyPanel::refreshValues()
{
    m_rebuilding = true;
    for (int r = 0; r < m_table->rowCount(); ++r)
    {
        QTableWidgetItem *val = m_table->item(r, 1);
        QTableWidgetItem *unit = m_table->item(r, 2);
        if (!val || !unit || !val->data(SectionRole).isValid()) continue;
        const auto q = static_cast<grt::units::Quantity>(val->data(QuantityRole).toInt());
        const QString base = val->data(BaseRole).toString();
        if (const auto *choices = val->data(ChoiceRole).toBool() ? choicesFor(val->data(KeyRole).toString()) : nullptr)
        {
            val->setText(labelFor(*choices, base));
            const QString tip = tipFor(*choices, base);
            val->setToolTip(tip.isEmpty() ? tr("stored value: %1").arg(base) : tip);
            continue;
        }
        if (q == grt::units::Quantity::None)
        {
            val->setText(base);
            continue;
        }
        bool ok = false;
        const double b = base.toDouble(&ok);
        const grt::units::Unit &u = UnitPreferences::instance().unitForField(unit->data(FieldRole).toString(), q);
        unit->setText(QString::fromUtf8(u.symbol));
        val->setText(ok && !base.isEmpty() ? grt::units::format(b, u) : base);
        val->setToolTip(ok && !base.isEmpty() ? tr("stored value: %1").arg(base) : QString());
    }
    m_rebuilding = false;
}

void PropertyPanel::onItemChanged(QTableWidgetItem *item)
{
    if (m_rebuilding || item->column() != 1) return;
    const QVariant s = item->data(SectionRole);
    if (!s.isValid()) return;
    QString base = item->text();
    if (item->data(ChoiceRole).toBool())
    {
        const auto *choices = choicesFor(item->data(KeyRole).toString());
        const auto it =
            choices ? std::ranges::find_if(*choices, [&](const Choice &c) { return c.label == base; }) : QList<Choice>::const_iterator{};
        if (!choices || it == choices->cend())
        {  // not one of the choices (e.g. "type 3"): keep the stored code
            refreshValues();
            return;
        }
        base = it->code;
    }
    const auto q = static_cast<grt::units::Quantity>(item->data(QuantityRole).toInt());
    if (q != grt::units::Quantity::None)
    {
        const QString field = m_table->item(item->row(), 2)->data(FieldRole).toString();
        const grt::units::Unit &u = UnitPreferences::instance().unitForField(field, q);
        const auto parsed = grt::units::parseToBase(base, u);
        if (!parsed)
        {  // not a number: put the old value back
            refreshValues();
            return;
        }
        base = QString::number(*parsed, 'g', 15);
    }
    m_rebuilding = true;  // setData() below must not re-enter this slot
    item->setData(BaseRole, base);
    m_rebuilding = false;
    emit valueEdited(s.toInt(), item->data(KeyRole).toString(), base);
    refreshValues();
}
