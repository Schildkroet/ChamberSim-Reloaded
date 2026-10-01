#include "recorddialog.h"

#include "numeric.h"
#include "unitprefs.h"

#include <QComboBox>
#include <QDate>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>
#include <map>

using namespace Qt::StringLiterals;
using grt::dbfields::Kind;

namespace {

const grt::units::Unit *unitOf(const grt::dbfields::Field &f)
{
    return f.quantity == grt::units::Quantity::None ? nullptr : &UnitPreferences::instance().unitFor(f.quantity);
}

QString labelText(const grt::dbfields::Field &f)
{
    QString unit = f.unitText;
    if (const auto *u = unitOf(f)) unit = QString::fromUtf8(u->symbol);
    return (unit.isEmpty() ? f.label : QStringLiteral("%1 [%2]").arg(f.label, unit)) + (f.required ? u" *"_s : u""_s) + u':';
}

// a stored number as shown to the user: the unit's usual decimals, more only where the value has them; no
// thousands separators in an input field
QString shownNumber(const grt::dbfields::Field &f, double stored)
{
    QLocale locale;
    locale.setNumberOptions(QLocale::OmitGroupSeparator);
    const auto *u = unitOf(f);
    const double v = u ? grt::units::toDisplay(stored * f.toBase, *u) : stored;
    for (int d = u ? u->decimals : 0; d < 8; ++d) {
        const double scale = std::pow(10.0, d);
        if (std::abs(std::round(v * scale) / scale - v) <= 1e-9 * std::max(1.0, std::abs(v))) return locale.toString(v, 'f', d);
    }
    return locale.toString(v, 'g', 10);
}

std::optional<double> parseNumber(const grt::dbfields::Field &f, const QString &text)
{
    const QString t = text.trimmed();
    if (t.isEmpty()) return 0.0;
    if (const auto *u = unitOf(f)) {
        const auto base = grt::units::parseToBase(t, *u);
        return base ? std::optional(*base / f.toBase) : std::nullopt;
    }
    return grt::parseFiniteLocale(t);
}

}  // namespace

RecordDialog::RecordDialog(const grt::Database &db, grt::Table table, Mode mode, const QVariantMap &record, QWidget *parent)
    : QDialog(parent)
    , m_table(table)
    , m_mode(mode)
    , m_record(record)
{
    const QString kind = table == grt::Table::Caliber      ? tr("caliber")
                         : table == grt::Table::Projectile ? tr("projectile")
                                                           : tr("propellant");
    const QString name = grt::dbfields::recordTitle(table, record);
    setWindowTitle(mode == Mode::New    ? tr("New %1").arg(kind)
                   : mode == Mode::Copy ? tr("Copy %1 – %2").arg(kind, name)
                                        : tr("Edit %1 – %2").arg(kind, name));

    // one group box per group; groups with many fields get two label/field pairs per row
    auto *content = new QWidget;
    auto *cl = new QVBoxLayout(content);
    std::map<QString, std::vector<grt::dbfields::Field>> byGroup;
    for (const auto &[column, type] : db.schema(table)) {
        const grt::dbfields::Field f = grt::dbfields::describe(table, column, type);
        if (f.kind != Kind::Hidden) byGroup[f.group].push_back(f);
    }
    for (const QString &group : grt::dbfields::groups(table)) {
        const auto it = byGroup.find(group);
        if (it == byGroup.end()) continue;
        auto *box = new QGroupBox(group);
        auto *grid = new QGridLayout(box);
        grid->setHorizontalSpacing(10);
        const int pairs = it->second.size() > 10 ? 2 : 1;
        for (int c = 0; c < pairs; ++c) grid->setColumnStretch(2 * c + 1, 1);
        int i = 0;
        for (const grt::dbfields::Field &f : it->second) {
            const QVariant stored = record.value(f.column);
            QWidget *w = nullptr;
            if (f.kind == Kind::BaseType) {
                auto *combo = new QComboBox;
                combo->addItem(tr("flat base"), 0);
                combo->addItem(tr("hollow base"), 1);
                combo->addItem(tr("boat tail"), 2);
                combo->setCurrentIndex(std::max(0, combo->findData(stored.toInt())));
                w = combo;
            } else if (f.kind == Kind::Choice) {
                // a fixed list: no free text, so the values stay consistent (the program reads some of them)
                auto *combo = new QComboBox;
                combo->addItem(tr("– not specified –"), QString());
                for (const auto &[value, text] : grt::dbfields::choices(table, f.column)) combo->addItem(text, value);
                const QString value = stored.toString();
                int index = combo->findData(value);
                for (int k = 0; index < 0 && k < combo->count(); ++k)  // "Wildcat" is "WildCat"
                    if (combo->itemData(k).toString().compare(value, Qt::CaseInsensitive) == 0) index = k;
                if (index < 0 && !value.isEmpty()) {  // an unusual value of an existing record is kept (e.g. "DE/AT")
                    combo->addItem(value, value);
                    index = combo->count() - 1;
                }
                combo->setCurrentIndex(std::max(0, index));
                combo->setMaxVisibleItems(20);
                w = combo;
            } else if (f.kind == Kind::LongText) {
                auto *edit = new QPlainTextEdit(stored.toString());
                edit->setMaximumHeight(90);
                w = edit;
            } else {
                auto *edit = new QLineEdit;
                if (f.kind == Kind::Number) {
                    edit->setText(stored.isValid() ? shownNumber(f, stored.toDouble()) : QString());
                    edit->setAlignment(Qt::AlignRight);
                } else {
                    edit->setText(stored.toString());
                }
                edit->setReadOnly(f.readOnly);
                if (f.readOnly) edit->setToolTip(tr("Set by the program"));
                w = edit;
            }
            w->setObjectName(f.column);
            auto *label = new QLabel(labelText(f));
            label->setBuddy(w);
            label->setToolTip(tr("Database column: %1").arg(f.column));
            const int span = f.kind == Kind::LongText ? 2 * pairs - 1 : 1;
            const int row = i / pairs, col = 2 * (i % pairs);
            grid->addWidget(label, row, col);
            grid->addWidget(w, row, col + 1, 1, span);
            i += f.kind == Kind::LongText ? pairs - i % pairs : 1;
            m_editors.push_back({f, w});
        }
        cl->addWidget(box);
    }
    cl->addStretch(1);

    // a copy is a new record: name it so that it can be told apart, and forget the program's dates
    if (mode == Mode::Copy) {
        const QString nameColumn = table == grt::Table::Caliber ? u"cipname"_s : u"pname"_s;
        setText(nameColumn, text(nameColumn) + tr(" (copy)"));
    }

    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setWidget(content);
    auto *hint = new QLabel(tr("<small>Fields marked * are required. Numbers are shown in your units "
                               "(File ▸ Settings).</small>"));
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &RecordDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &RecordDialog::reject);
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(scroll, 1);
    layout->addWidget(hint);
    layout->addWidget(buttons);
    resize(table == grt::Table::Caliber ? 900 : 620, 720);
}

const RecordDialog::Editor *RecordDialog::editor(const QString &column) const
{
    for (const Editor &e : m_editors)
        if (e.field.column == column) return &e;
    return nullptr;
}

QString RecordDialog::text(const QString &column) const
{
    const Editor *e = editor(column);
    if (!e) return {};
    if (auto *l = qobject_cast<QLineEdit *>(e->widget)) return l->text();
    if (auto *p = qobject_cast<QPlainTextEdit *>(e->widget)) return p->toPlainText();
    if (auto *c = qobject_cast<QComboBox *>(e->widget)) return e->field.kind == Kind::Choice ? c->currentData().toString() : c->currentText();
    return {};
}

void RecordDialog::setText(const QString &column, const QString &text)
{
    const Editor *e = editor(column);
    if (!e) return;
    if (auto *l = qobject_cast<QLineEdit *>(e->widget)) l->setText(text);
    else if (auto *p = qobject_cast<QPlainTextEdit *>(e->widget)) p->setPlainText(text);
    else if (auto *c = qobject_cast<QComboBox *>(e->widget)) {
        if (e->field.kind == Kind::Choice) {
            if (const int i = c->findData(text); i >= 0) c->setCurrentIndex(i);
        } else {
            c->setCurrentText(text);
        }
    }
}

std::optional<QVariant> RecordDialog::valueOf(const Editor &e) const
{
    if (auto *c = qobject_cast<QComboBox *>(e.widget)) return c->currentData();
    const QString t = text(e.field.column);
    if (e.field.kind != Kind::Number) return QVariant(t);
    const auto v = parseNumber(e.field, t);
    return v ? std::optional<QVariant>(*v) : std::nullopt;
}

QString RecordDialog::firstError() const
{
    for (const Editor &e : m_editors) {
        if (e.field.required && text(e.field.column).trimmed().isEmpty()) return e.field.label;
        if (!valueOf(e)) return e.field.label;
    }
    return {};
}

QVariantMap RecordDialog::values() const
{
    QVariantMap out;
    for (const Editor &e : m_editors)
        if (const auto v = valueOf(e)) out.insert(e.field.column, *v);
    const QString today = QDate::currentDate().toString(Qt::ISODate);
    if (m_mode == Mode::Edit) {
        out.insert(u"mdate"_s, today);
    } else {
        out.insert(u"cdate"_s, today);
        out.insert(u"mdate"_s, QString());
    }
    return out;
}

void RecordDialog::accept()
{
    const QString wrong = firstError();
    for (const Editor &e : m_editors) {
        const bool bad = (e.field.required && text(e.field.column).trimmed().isEmpty()) || !valueOf(e);
        e.widget->setStyleSheet(bad ? u"background: #f6c6c6; color: black;"_s : QString());
    }
    if (!wrong.isEmpty()) {
        QMessageBox::warning(this, windowTitle(), tr("Please check the field \"%1\".").arg(wrong));
        const auto it = std::ranges::find_if(m_editors, [&](const Editor &x) { return x.field.label == wrong; });
        if (it != m_editors.end()) it->widget->setFocus();
        return;
    }
    QDialog::accept();
}
