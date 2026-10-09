#include "recorddialog.h"

#include "screenfit.h"
#include "dates.h"
#include "barrel.h"
#include "numeric.h"
#include "recordfields.h"
#include "unitprefs.h"

#include <QBuffer>
#include <QComboBox>
#include <QDateEdit>
#include <QDate>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QImage>
#include <QPixmap>
#include <QUuid>
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
using csr::dbfields::Kind;

namespace
{

const QDate kNoDate(1800, 1, 1);  // the date editor's "– not set –"

const csr::units::Unit *unitOf(const csr::dbfields::Field &f)
{
    return recordfields::unitOf(f);
}

QString labelText(const csr::dbfields::Field &f)
{
    QString unit = f.unitText;
    if (const auto *u = unitOf(f)) unit = QString::fromUtf8(u->symbol);
    return (unit.isEmpty() ? f.label : QStringLiteral("%1 [%2]").arg(f.label, unit)) + (f.required ? u" *"_s : u""_s) + u':';
}

QString shownNumber(const csr::dbfields::Field &f, double stored)
{
    return recordfields::shownNumber(f, stored);
}

std::optional<double> parseNumber(const csr::dbfields::Field &f, const QString &text)
{
    const QString t = text.trimmed();
    if (t.isEmpty()) return 0.0;
    if (const auto *u = unitOf(f))
    {
        const auto base = csr::units::parseToBase(t, *u);
        return base ? std::optional(*base / f.toBase) : std::nullopt;
    }
    return csr::parseFiniteLocale(t);
}

}  // namespace

RecordDialog::RecordDialog(const csr::Database &db, csr::Table table, Mode mode, const QVariantMap &record, QWidget *parent)
    : QDialog(parent)
    , m_db(&db)
    , m_table(table)
    , m_mode(mode)
    , m_record(record)
{
    const QString kind = table == csr::Table::Caliber      ? tr("caliber")
                         : table == csr::Table::Projectile ? tr("projectile")
                                                           : tr("propellant");
    const QString name = csr::dbfields::recordTitle(table, record);
    setWindowTitle(mode == Mode::New    ? tr("New %1").arg(kind)
                   : mode == Mode::Copy ? tr("Copy %1 – %2").arg(kind, name)
                                        : tr("Edit %1 – %2").arg(kind, name));

    // one group box per group; groups with many fields get two label/field pairs per row
    auto *content = new QWidget;
    auto *cl = new QVBoxLayout(content);
    std::map<QString, std::vector<csr::dbfields::Field>> byGroup;
    for (const auto &[column, type] : db.schema(table))
    {
        const csr::dbfields::Field f = csr::dbfields::describe(table, column, type);
        if (table == csr::Table::Propellant && column.startsWith(u"image"_s)) continue;  // the photo editor below
        if (f.kind != Kind::Hidden) byGroup[f.group].push_back(f);
    }
    for (const QString &group : csr::dbfields::groups(table))
    {
        const auto it = byGroup.find(group);
        if (it == byGroup.end()) continue;
        auto *box = new QGroupBox(group);
        auto *grid = new QGridLayout(box);
        grid->setHorizontalSpacing(10);
        const int pairs = it->second.size() > 10 ? 2 : 1;
        for (int c = 0; c < pairs; ++c) grid->setColumnStretch(2 * c + 1, 1);
        int i = 0;
        for (const csr::dbfields::Field &f : it->second)
        {
            const QVariant stored = record.value(f.column);
            QWidget *w = nullptr;
            if (f.kind == Kind::BaseType)
            {
                auto *combo = new QComboBox;
                combo->addItem(tr("flat base"), 0);
                combo->addItem(tr("hollow base"), 1);
                combo->addItem(tr("boat tail"), 2);
                combo->setCurrentIndex(std::max(0, combo->findData(stored.toInt())));
                w = combo;
            }
            else if (f.kind == Kind::Choice)
            {
                // a fixed list: no free text, so the values stay consistent (the program reads some of them)
                auto *combo = new QComboBox;
                combo->addItem(tr("– not specified –"), QString());
                for (const auto &[value, text] : csr::dbfields::choices(table, f.column)) combo->addItem(text, value);
                const QString value = stored.toString();
                int index = combo->findData(value);
                for (int k = 0; index < 0 && k < combo->count(); ++k)  // "Wildcat" is "WildCat"
                    if (combo->itemData(k).toString().compare(value, Qt::CaseInsensitive) == 0) index = k;
                if (index < 0 && !value.isEmpty())
                {  // an unusual value of an existing record is kept (e.g. "DE/AT")
                    combo->addItem(value, value);
                    index = combo->count() - 1;
                }
                combo->setCurrentIndex(std::max(0, index));
                combo->setMaxVisibleItems(20);
                w = combo;
            }
            else if (f.kind == Kind::Date && !f.readOnly)
            {
                // a date picker: always yyyy-MM-dd; the lowest date stands for "no date"
                auto *edit = new QDateEdit;
                edit->setCalendarPopup(true);
                edit->setDisplayFormat(u"yyyy-MM-dd"_s);
                edit->setMinimumDate(kNoDate);
                edit->setSpecialValueText(tr("– not set –"));
                const auto date = csr::normalizedDate(stored);
                edit->setDate(date && !date->isEmpty() ? QDate::fromString(*date, Qt::ISODate) : kNoDate);
                w = edit;
            }
            else if (f.kind == Kind::LongText)
            {
                auto *edit = new QPlainTextEdit(stored.toString());
                edit->setMaximumHeight(90);
                w = edit;
            }
            else
            {
                auto *edit = new QLineEdit;
                if (f.kind == Kind::Date)  // set by the program: shown as yyyy-MM-dd
                {
                    const auto date = csr::normalizedDate(stored);
                    edit->setText(date ? *date : stored.toString());
                }
                else if (f.kind == Kind::Number)
                {
                    edit->setText(stored.isValid() ? shownNumber(f, stored.toDouble()) : QString());
                }
                else
                {
                    edit->setText(stored.toString());
                }
                if (f.alignRight) edit->setAlignment(Qt::AlignRight);
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
            if (table == csr::Table::Caliber && f.column == u"c_Q")
            {
                // the bore cross-section can be calculated from the bore and groove diameters, the number and the width of the grooves
                auto *calculate = new QPushButton(tr("Calculate"));
                calculate->setObjectName(u"calculateBoreArea"_s);
                calculate->setToolTip(tr("Q = π/4·F² + N·b·(Z − F)/2 from the bore diameter F, the groove diameter Z, the number N and "
                                         "the width b of the grooves"));
                connect(calculate, &QPushButton::clicked, this, &RecordDialog::calculateBoreCrossSection);
                auto *cell = new QWidget;
                auto *cellLayout = new QHBoxLayout(cell);
                cellLayout->setContentsMargins(0, 0, 0, 0);
                cellLayout->addWidget(w, 1);
                cellLayout->addWidget(calculate);
                grid->addWidget(cell, row, col + 1, 1, span);
            }
            else
            {
                grid->addWidget(w, row, col + 1, 1, span);
            }
            i += f.kind == Kind::LongText ? pairs - i % pairs : 1;
            m_editors.push_back({f, w});
        }
        cl->addWidget(box);
    }
    if (table == csr::Table::Propellant) addPhotoGroup(cl);
    cl->addStretch(1);

    // a new record or a copy is created today (the date can be changed)
    if (mode != Mode::Edit) setText(u"cdate"_s, QDate::currentDate().toString(Qt::ISODate));
    // a copy is a new record: name it so that it can be told apart
    if (mode == Mode::Copy)
    {
        const QString nameColumn = table == csr::Table::Caliber ? u"cipname"_s : u"pname"_s;
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
    resizeWithinScreen(this, {table == csr::Table::Caliber ? 900 : 860, 760});
}

void RecordDialog::calculateBoreCrossSection()
{
    const auto number = [this](const QString &column) -> std::optional<double>
    {
        const Editor *e = editor(column);
        return e ? parseNumber(e->field, text(column)) : std::nullopt;
    };
    const auto bore = number(u"c_F"_s), groove = number(u"c_Z"_s), count = number(u"c_N"_s), width = number(u"c_b_"_s);
    if (!bore || !groove || !count || !width || *bore <= 0.0)
    {
        QMessageBox::information(this, tr("Bore cross-section"),
                                 tr("Enter the bore diameter (F), the groove diameter (Z), the number of grooves (N) and the groove "
                                    "width (b) first."));
        return;
    }
    // the fields hold the stored (base) unit values of their columns: mm for the lengths, mm² for Q
    if (const Editor *q = editor(u"c_Q"_s))
        setText(u"c_Q"_s, shownNumber(q->field, csr::boreCrossSection_mm2(*bore, *groove, *count, *width)));
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
    if (auto *d = qobject_cast<QDateEdit *>(e->widget)) return d->date() == kNoDate ? QString() : d->date().toString(Qt::ISODate);
    if (auto *p = qobject_cast<QPlainTextEdit *>(e->widget)) return p->toPlainText();
    if (auto *c = qobject_cast<QComboBox *>(e->widget))
        return e->field.kind == Kind::Choice ? c->currentData().toString() : c->currentText();
    return {};
}

void RecordDialog::setText(const QString &column, const QString &text)
{
    const Editor *e = editor(column);
    if (!e) return;
    if (auto *l = qobject_cast<QLineEdit *>(e->widget)) l->setText(text);
    else if (auto *d = qobject_cast<QDateEdit *>(e->widget))
    {
        const auto date = csr::normalizedDate(text);
        d->setDate(date && !date->isEmpty() ? QDate::fromString(*date, Qt::ISODate) : kNoDate);
    }
    else if (auto *p = qobject_cast<QPlainTextEdit *>(e->widget)) p->setPlainText(text);
    else if (auto *c = qobject_cast<QComboBox *>(e->widget))
    {
        if (e->field.kind == Kind::Choice)
        {
            if (const int i = c->findData(text); i >= 0) c->setCurrentIndex(i);
        }
        else
        {
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

namespace
{

constexpr int kMaxPhotoSide = 1200;  // longer sides are scaled down: the photos live in the database

QImage thumbnail(const QImage &img)
{
    return img.scaled(112, 96, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

}  // namespace

void RecordDialog::addPhotoGroup(QVBoxLayout *layout)
{
    const std::array<QString, 2> columns{u"imageUuid"_s, u"imageDetailUuid"_s};
    const std::array<QString, 2> titles{tr("Overview photo:"), tr("Detail photo:")};
    auto *box = new QGroupBox(tr("Photos"));
    auto *grid = new QGridLayout(box);
    grid->setColumnStretch(2, 1);
    for (int i = 0; i < 2; ++i)
    {
        PhotoSlot &slot = m_photos[static_cast<std::size_t>(i)];
        slot.column = columns[static_cast<std::size_t>(i)];
        slot.uuid = m_record.value(slot.column).toString();
        slot.preview = new QLabel;
        slot.preview->setObjectName(slot.column + u"Preview"_s);
        slot.preview->setFixedSize(116, 100);
        slot.preview->setFrameShape(QFrame::StyledPanel);
        slot.preview->setAlignment(Qt::AlignCenter);
        slot.info = new QLabel;
        slot.info->setWordWrap(true);
        slot.scale = new QLineEdit;
        slot.scale->setObjectName(slot.column + u"Scale"_s);
        slot.scale->setAlignment(Qt::AlignRight);
        slot.scale->setMaximumWidth(90);
        slot.scale->setToolTip(tr("Pixels per millimeter of the new photo (for the scale bar); empty: unknown"));
        auto *choose = new QPushButton(tr("Choose file…"));
        auto *remove = new QPushButton(tr("Remove"));
        connect(choose, &QPushButton::clicked, this,
                [this, i]
                {
                    const QString path = QFileDialog::getOpenFileName(this, tr("Photo"), QString(),
                                                                      tr("Pictures (*.jpg *.jpeg *.png *.bmp);;All files (*)"));
                    if (path.isEmpty()) return;
                    const QImage img(path);
                    if (img.isNull())
                    {
                        QMessageBox::warning(this, windowTitle(),
                                             tr("%1 is not a picture this program can read.").arg(QDir::toNativeSeparators(path)));
                        return;
                    }
                    const auto scale = csr::parseFiniteLocale(m_photos[static_cast<std::size_t>(i)].scale->text());
                    setPhoto(i, img, scale && *scale > 0.0 ? *scale : 0.0, QFileInfo(path).fileName());
                });
        connect(remove, &QPushButton::clicked, this, [this, i] { removePhoto(i); });
        grid->addWidget(new QLabel(titles[static_cast<std::size_t>(i)]), 3 * i, 0, 1, 2);
        grid->addWidget(slot.preview, 3 * i + 1, 0, 2, 1);
        grid->addWidget(slot.info, 3 * i + 1, 1, 1, 3, Qt::AlignTop);
        auto *row = new QHBoxLayout;
        row->addWidget(choose);
        row->addWidget(remove);
        row->addWidget(new QLabel(tr("Scale [px/mm]:")));
        row->addWidget(slot.scale);
        row->addStretch(1);
        grid->addLayout(row, 3 * i + 2, 1, 1, 3);
        showPhoto(i);
    }
    layout->insertWidget(std::min(1, layout->count()), box);  // below the name
}

// The preview and the text of a slot: the pending picture, else the stored one, else "none".
void RecordDialog::showPhoto(int slotIndex)
{
    PhotoSlot &slot = m_photos[static_cast<std::size_t>(slotIndex)];
    std::optional<csr::Database::Image> img = slot.pending;
    if (!img && !slot.uuid.isEmpty()) img = m_db->image(slot.uuid);
    if (!img)
    {
        slot.preview->clear();
        slot.info->setText(slot.uuid.isEmpty() ? tr("No photo") : tr("The photo is not available in the database."));
        return;
    }
    slot.preview->setPixmap(QPixmap::fromImage(thumbnail(QImage::fromData(img->data))));
    slot.info->setText(img->scaleUnit == u"px/mm" && img->scale > 0.0
                           ? tr("%1 (%2 px/mm)").arg(img->name, QLocale().toString(img->scale, 'f', 1))
                           : img->name);
}

void RecordDialog::setPhoto(int slotIndex, const QImage &image, double pxPerMm, const QString &name)
{
    if (slotIndex < 0 || slotIndex > 1 || image.isNull()) return;
    QImage img = image;
    if (std::max(img.width(), img.height()) > kMaxPhotoSide)
    {
        img = img.scaled(kMaxPhotoSide, kMaxPhotoSide, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        if (pxPerMm > 0.0) pxPerMm *= static_cast<double>(img.width()) / image.width();
    }
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    if (!img.convertToFormat(QImage::Format_RGB32).save(&buffer, "JPG", 88)) return;
    PhotoSlot &slot = m_photos[static_cast<std::size_t>(slotIndex)];
    csr::Database::Image stored;
    stored.uuid = QUuid::createUuid().toString(QUuid::Id128);
    stored.name = name;
    stored.scale = pxPerMm;
    stored.scaleUnit = pxPerMm > 0.0 ? u"px/mm"_s : QString();
    stored.data = bytes;
    slot.uuid = stored.uuid;
    slot.pending = std::move(stored);
    showPhoto(slotIndex);
}

void RecordDialog::removePhoto(int slotIndex)
{
    if (slotIndex < 0 || slotIndex > 1) return;
    PhotoSlot &slot = m_photos[static_cast<std::size_t>(slotIndex)];
    slot.uuid.clear();
    slot.pending.reset();
    showPhoto(slotIndex);
}

std::vector<csr::Database::Image> RecordDialog::pendingImages() const
{
    std::vector<csr::Database::Image> out;
    for (const PhotoSlot &slot : m_photos)
        if (slot.pending) out.push_back(*slot.pending);
    return out;
}

QString RecordDialog::firstError() const
{
    for (const Editor &e : m_editors)
    {
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
    if (m_table == csr::Table::Propellant)
        for (const PhotoSlot &slot : m_photos) out.insert(slot.column, slot.uuid);
    // the creation date comes from its date picker (today for a new record or a copy); the change date is the program's
    out.insert(u"mdate"_s, m_mode == Mode::Edit ? QDate::currentDate().toString(Qt::ISODate) : QString());
    return out;
}

void RecordDialog::accept()
{
    const QString wrong = firstError();
    for (const Editor &e : m_editors)
    {
        const bool bad = (e.field.required && text(e.field.column).trimmed().isEmpty()) || !valueOf(e);
        e.widget->setStyleSheet(bad ? u"background: #f6c6c6; color: black;"_s : QString());
    }
    if (!wrong.isEmpty())
    {
        QMessageBox::warning(this, windowTitle(), tr("Please check the field \"%1\".").arg(wrong));
        const auto it = std::ranges::find_if(m_editors, [&](const Editor &x) { return x.field.label == wrong; });
        if (it != m_editors.end()) it->widget->setFocus();
        return;
    }
    QDialog::accept();
}
