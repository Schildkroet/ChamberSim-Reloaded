#include "unitconverterdialog.h"

#include "screenfit.h"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QSettings>
#include <QTableWidget>
#include <QVBoxLayout>

#include <array>

namespace
{
using grt::units::Quantity;

struct QuantityName
{
    Quantity quantity;
    const char *name;
};

// the quantities that can be converted (Quantity::None has no units)
constexpr std::array kQuantities{QuantityName{Quantity::Length, QT_TRANSLATE_NOOP("UnitConverterDialog", "Length")},
                                 QuantityName{Quantity::BulletMass, QT_TRANSLATE_NOOP("UnitConverterDialog", "Bullet and charge mass")},
                                 QuantityName{Quantity::GunMass, QT_TRANSLATE_NOOP("UnitConverterDialog", "Gun mass")},
                                 QuantityName{Quantity::Pressure, QT_TRANSLATE_NOOP("UnitConverterDialog", "Pressure")},
                                 QuantityName{Quantity::Velocity, QT_TRANSLATE_NOOP("UnitConverterDialog", "Velocity")},
                                 QuantityName{Quantity::Energy, QT_TRANSLATE_NOOP("UnitConverterDialog", "Energy")},
                                 QuantityName{Quantity::Force, QT_TRANSLATE_NOOP("UnitConverterDialog", "Force")},
                                 QuantityName{Quantity::Impulse, QT_TRANSLATE_NOOP("UnitConverterDialog", "Impulse")},
                                 QuantityName{Quantity::Area, QT_TRANSLATE_NOOP("UnitConverterDialog", "Area")},
                                 QuantityName{Quantity::Volume, QT_TRANSLATE_NOOP("UnitConverterDialog", "Volume")},
                                 QuantityName{Quantity::Density, QT_TRANSLATE_NOOP("UnitConverterDialog", "Density")},
                                 QuantityName{Quantity::Temperature, QT_TRANSLATE_NOOP("UnitConverterDialog", "Temperature")},
                                 QuantityName{Quantity::Time, QT_TRANSLATE_NOOP("UnitConverterDialog", "Time")}};
constexpr auto kQuantityKey = "unitConverter/quantity";
constexpr auto kUnitKey = "unitConverter/unit";
}  // namespace

UnitConverterDialog::UnitConverterDialog(QWidget *parent)
    : QDialog(parent)
    , m_quantity(new QComboBox)
    , m_input(new QLineEdit(QStringLiteral("1")))
    , m_from(new QComboBox)
    , m_table(new QTableWidget(0, 2))
{
    setObjectName(QStringLiteral("unitConverterDialog"));
    setWindowTitle(tr("Unit converter"));
    resizeWithinScreen(this, {420, 420});

    for (const auto &q : kQuantities) m_quantity->addItem(tr(q.name), static_cast<int>(q.quantity));
    m_input->setClearButtonEnabled(true);
    m_input->setPlaceholderText(tr("Value"));
    m_table->setHorizontalHeaderLabels({tr("Unit"), tr("Value")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->hide();
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setAlternatingRowColors(true);

    auto *form = new QFormLayout;
    form->addRow(tr("Quantity"), m_quantity);
    auto *valueRow = new QHBoxLayout;
    valueRow->addWidget(m_input, 1);
    valueRow->addWidget(m_from);
    form->addRow(tr("Convert"), valueRow);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::accept);
    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(m_table, 1);
    layout->addWidget(buttons);

    const QSettings s;
    const int stored = s.value(QLatin1String(kQuantityKey), static_cast<int>(Quantity::Pressure)).toInt();
    if (const int i = m_quantity->findData(stored); i >= 0) m_quantity->setCurrentIndex(i);
    fillUnits();
    if (const int i = m_from->findData(s.value(QLatin1String(kUnitKey)).toString()); i >= 0) m_from->setCurrentIndex(i);
    convert();

    connect(m_quantity, &QComboBox::currentIndexChanged, this,
            [this]
            {
                fillUnits();
                convert();
            });
    connect(m_from, &QComboBox::currentIndexChanged, this, &UnitConverterDialog::convert);
    connect(m_input, &QLineEdit::textChanged, this, &UnitConverterDialog::convert);
    connect(this, &QDialog::finished, this,
            [this]
            {
                QSettings s;
                s.setValue(QLatin1String(kQuantityKey), m_quantity->currentData().toInt());
                s.setValue(QLatin1String(kUnitKey), m_from->currentData().toString());
            });
}

Quantity UnitConverterDialog::quantity() const
{
    return static_cast<Quantity>(m_quantity->currentData().toInt());
}

void UnitConverterDialog::setQuantity(Quantity quantity)
{
    if (const int i = m_quantity->findData(static_cast<int>(quantity)); i >= 0) m_quantity->setCurrentIndex(i);
}

void UnitConverterDialog::setFromUnit(const QString &unitId)
{
    if (const int i = m_from->findData(unitId); i >= 0) m_from->setCurrentIndex(i);
}

void UnitConverterDialog::setInput(const QString &text)
{
    m_input->setText(text);
}

int UnitConverterDialog::rowCount() const
{
    return m_table->rowCount();
}

QString UnitConverterDialog::symbolAt(int row) const
{
    return m_table->item(row, 0)->text();
}

QString UnitConverterDialog::valueAt(int row) const
{
    return m_table->item(row, 1)->text();
}

void UnitConverterDialog::fillUnits()
{
    const QSignalBlocker block(m_from);
    m_from->clear();
    for (const auto &u : grt::units::unitsOf(quantity())) m_from->addItem(QString::fromUtf8(u.symbol), QString::fromLatin1(u.id));
}

void UnitConverterDialog::convert()
{
    const auto units = grt::units::unitsOf(quantity());
    const int from = m_from->currentIndex();
    m_table->setRowCount(static_cast<int>(units.size()));
    const auto base = from >= 0 ? grt::units::parseToBase(m_input->text(), units[static_cast<std::size_t>(from)]) : std::nullopt;
    for (int row = 0; row < static_cast<int>(units.size()); ++row)
    {
        const auto &u = units[static_cast<std::size_t>(row)];
        auto *symbol = new QTableWidgetItem(QString::fromUtf8(u.symbol));
        // more digits than the program's fields show: a converter is used for checking numbers
        auto *value = new QTableWidgetItem(base ? QLocale().toString(grt::units::toDisplay(*base, u), 'g', 7) : QString());
        value->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(row, 0, symbol);
        m_table->setItem(row, 1, value);
    }
}
