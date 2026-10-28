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

#include "propertypanel.h"

#include "fieldnames.h"
#include "primer.h"

#include "shotparts.h"
#include "unitprefs.h"

#include <QComboBox>
#include <QEvent>
#include <QHeaderView>
#include <QLineEdit>
#include <QSettings>
#include <QStyledItemDelegate>
#include <QTableWidget>
#include <QVBoxLayout>

#include <algorithm>
#include <array>

using namespace Qt::StringLiterals;

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

// The choices of a part of a shotshell: "not specified" (the model's average) and the groups of similar parts (low, medium and high pressure);
// the tooltip of a group lists the brands (the individual parts) it stands for.
const QList<Choice> &shotPartChoices(csr::ShotPart part)
{
    static const std::array<QList<Choice>, 3> lists = []
    {
        std::array<QList<Choice>, 3> all;
        for (const csr::ShotPart p : csr::kShotParts)
        {
            auto &list = all[static_cast<std::size_t>(p)];
            list.push_back({QString(), PropertyPanel::tr("not specified (average)"), {}});
            for (const csr::ShotPartInfo &info : csr::shotPartGroups(p))
            {
                const QString text = info.rank == u"low"    ? PropertyPanel::tr("Low pressure")
                                     : info.rank == u"high" ? PropertyPanel::tr("High pressure")
                                                            : PropertyPanel::tr("Medium pressure");
                list.push_back({info.id, text, info.members.join(u'\n')});
            }
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
    static const QList<Choice> yesNo{{u"false"_s, PropertyPanel::tr("no")}, {u"true"_s, PropertyPanel::tr("yes")}};
    static const QList<Choice> ladder = []
    {
        QList<Choice> l;
        for (int n = 1; n <= 10; ++n) l.push_back({QString::number(n), QString::number(n)});  // 1: no ladder
        return l;
    }();
    // the primer type of a metallic cartridge (primer.h): its start pressure plus the additional one is the load's start pressure.
    // Each entry shows its start pressure in the unit of the additional start pressure (built anew: the unit can change).
    if (key == u"primer")
    {
        static QList<Choice> primer;
        const csr::units::Unit &u = UnitPreferences::instance().unitForField(u"caliber/psadd"_s, csr::units::Quantity::Pressure);
        const auto withStart = [&](const QString &name, const QString &code) {
            return u"%1 (%2 %3)"_s.arg(name, csr::units::format(csr::primerStartPressure_bar(code, 0.0), u, 0),
                                       QString::fromUtf8(u.symbol));
        };
        primer = {{QString(), withStart(PropertyPanel::tr("not specified"), QString()), {}}};
        for (const csr::PrimerType &t : csr::kPrimerTypes)
        {
            const QString code = QString::fromLatin1(t.code);
            primer.push_back({code, withStart(csr::primerName(code), code), {}});
        }
        return &primer;
    }
    if (key == u"laddercnt") return &ladder;
    if (key == u"gTailType") return &tail;
    if (key == u"shotshell") return &yesNo;
    // the shell, wad and primer of a shotshell load: the parts of the data file (their factors correct the model), or none
    for (const csr::ShotPart part : csr::kShotParts)
        if (key == csr::shotPartKey(part)) return &shotPartChoices(part);
    return nullptr;
}

// Fields of the file that are not shown: "gjumpUnlock" only locks the jump distance in the original's input form,
// the original uses the distance either way (measured, batch 10).
bool hiddenField(const QString &key)
{
    return key == u"gjumpUnlock" || key == u"gfrictionUnlock" ||
           key == u"physRef";  // gfrictionUnlock: the friction always applies; physRef: the burn values the physical ones belong to
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
            box->setAutoFillBackground(true);
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
        const auto q = static_cast<csr::units::Quantity>(index.data(PropertyPanel::QuantityRole).toInt());
        const auto list = csr::units::unitsOf(q);
        if (list.empty()) return nullptr;
        auto *box = new QComboBox(parent);
        box->setAutoFillBackground(true);  // macOS draws the box transparent: the cell's own text shows through
        for (const csr::units::Unit &u : list) box->addItem(QString::fromUtf8(u.symbol), QString::fromLatin1(u.id));
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
        const auto q = static_cast<csr::units::Quantity>(index.data(PropertyPanel::QuantityRole).toInt());
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

// Follows the table's own viewport (the panel's resize event arrives before the table has its new size, which left
// the property column as wide as the whole table so the values had to be scrolled into view).
bool PropertyPanel::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_table->viewport() && event->type() == QEvent::Resize && !m_userSizedColumns) fitColumns();
    return QWidget::eventFilter(watched, event);
}

void PropertyPanel::fitColumns()
{
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
    m_table->viewport()->installEventFilter(this);
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

// the category rows carry a brush of the palette at the time they were built: renew it when the theme changes
void PropertyPanel::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() != QEvent::PaletteChange || !m_table) return;
    for (int r = 0; r < m_table->rowCount(); ++r)
        if (m_table->columnSpan(r, 0) > 1)
            if (auto *item = m_table->item(r, 0)) item->setBackground(palette().alternateBase());
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

        // propellant: the charge and the ladder come right below the powder's name, before its burn data
        std::vector<const csr::Entry *> order;
        for (const csr::Entry &e : s.entries->entries()) order.push_back(&e);
        if (s.key == u"propellant")
        {
            const auto charge = [](const csr::Entry *e) { return e->name == u"mc" || e->name == u"laddercnt" || e->name == u"laddermc"; };
            const auto name = std::ranges::find_if(order, [](const csr::Entry *e) { return e->name == u"pname"; });
            if (name != order.end())
            {
                std::stable_partition(name + 1, order.end(), charge);
                // stable_partition keeps the relative order; the three fields now follow pname
            }
        }
        for (const csr::Entry *entry : order)
        {
            const csr::Entry &e = *entry;
            if (hiddenField(e.name)) continue;  // kept in the file, written back unchanged
            // the start pressure of a load with a primer is primer + additional start pressure (shown); a shotshell has its own primer
            if (e.name == u"ps" && s.entries->find(u"psadd") && !s.entries->flag(u"shotshell")) continue;
            if ((e.name == u"primer" || e.name == u"psadd") && s.entries->flag(u"shotshell")) continue;
            const int r = m_table->rowCount();
            m_table->insertRow(r);
            // a translated description is the finished label; the English one is tidied up here
            const QString translated = csr::translatedDescription(e.descr);
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
            const auto q = csr::units::quantityOfFileUnit(e.unit);
            unit->setData(QuantityRole, static_cast<int>(q));
            unit->setData(FieldRole, s.key + u'/' + e.name);
            if (q == csr::units::Quantity::None)
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
                for (const csr::Entry &e : s.entries->entries())
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
        const auto q = static_cast<csr::units::Quantity>(val->data(QuantityRole).toInt());
        const QString base = val->data(BaseRole).toString();
        if (const auto *choices = val->data(ChoiceRole).toBool() ? choicesFor(val->data(KeyRole).toString()) : nullptr)
        {
            val->setText(labelFor(*choices, base));
            const QString tip = tipFor(*choices, base);
            const bool isPart =
                std::ranges::any_of(csr::kShotParts, [&](csr::ShotPart p) { return val->data(KeyRole).toString() == csr::shotPartKey(p); });
            val->setToolTip(tip.isEmpty() && !isPart ? tr("stored value: %1").arg(base) : tip);  // the parts: only the brands
            continue;
        }
        if (q == csr::units::Quantity::None)
        {
            val->setText(base);
            continue;
        }
        bool ok = false;
        const double b = base.toDouble(&ok);
        const csr::units::Unit &u = UnitPreferences::instance().unitForField(unit->data(FieldRole).toString(), q);
        unit->setText(QString::fromUtf8(u.symbol));
        val->setText(ok && !base.isEmpty() ? csr::units::format(b, u) : base);
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
    const auto q = static_cast<csr::units::Quantity>(item->data(QuantityRole).toInt());
    if (q != csr::units::Quantity::None)
    {
        const QString field = m_table->item(item->row(), 2)->data(FieldRole).toString();
        const csr::units::Unit &u = UnitPreferences::instance().unitForField(field, q);
        const auto parsed = csr::units::parseToBase(base, u);
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
