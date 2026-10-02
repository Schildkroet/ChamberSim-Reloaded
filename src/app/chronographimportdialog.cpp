#include "chronographimportdialog.h"

#include "measurement.h"
#include "unitprefs.h"

#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

using grt::units::Quantity;

namespace
{

enum Column
{
    Use,
    Name,
    Shots,
    Mean,
    Sd,
    Charge,
    ColumnCount
};

const grt::units::Unit &unitOf(Quantity q)
{
    return UnitPreferences::instance().unitFor(q);
}

QTableWidgetItem *readOnly(const QString &text)
{
    auto *item = new QTableWidgetItem(text);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    return item;
}

}  // namespace

ChronographImportDialog::ChronographImportDialog(grt::ChronographFile file, double defaultCharge_g, QWidget *parent)
    : QDialog(parent)
    , m_file(std::move(file))
    , m_default(defaultCharge_g)
    , m_table(new QTableWidget(static_cast<int>(m_file.series.size()), ColumnCount))
{
    setWindowTitle(tr("Import chronograph file"));
    const QString chargeUnit = QString::fromUtf8(unitOf(Quantity::BulletMass).symbol);
    m_table->setHorizontalHeaderLabels({tr("Import"), tr("Series"), tr("Shots"),
                                        tr("Velocity [%1]").arg(QString::fromUtf8(unitOf(Quantity::Velocity).symbol)), tr("SD"),
                                        tr("Charge [%1]").arg(chargeUnit)});
    m_table->horizontalHeaderItem(Charge)->setToolTip(tr("The powder charge of this series (the file does not contain it)"));
    m_table->verticalHeader()->hide();
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(Name, QHeaderView::Stretch);
    for (int row = 0; row < m_table->rowCount(); ++row)
    {
        const auto &s = m_file.series[static_cast<std::size_t>(row)];
        const grt::ShotStatistics st = grt::statistics(s.velocities_ms);
        auto *use = new QTableWidgetItem;
        use->setFlags((use->flags() | Qt::ItemIsUserCheckable) & ~Qt::ItemIsEditable);
        use->setCheckState(Qt::Checked);
        m_table->setItem(row, Use, use);
        m_table->setItem(row, Name, readOnly(s.name));
        m_table->item(row, Name)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_table->setItem(row, Shots, readOnly(QString::number(s.velocities_ms.size())));
        m_table->setItem(row, Mean, readOnly(grt::units::format(st.mean, unitOf(Quantity::Velocity), 1)));
        m_table->setItem(row, Sd, readOnly(st.count > 1 ? grt::units::format(st.sd, unitOf(Quantity::Velocity), 1) : QStringLiteral("–")));
        auto *charge = new QTableWidgetItem(grt::units::format(m_default, unitOf(Quantity::BulletMass)));
        charge->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(row, Charge, charge);
    }

    auto *intro = new QLabel(
        tr("%1 file with %n series. Every ticked series becomes a charge with its shots.", nullptr, static_cast<int>(m_file.series.size()))
            .arg(grt::formatName(m_file.format)));
    intro->setWordWrap(true);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Ok)->setText(tr("Import"));
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(intro);
    layout->addWidget(m_table, 1);
    layout->addWidget(buttons);
    resize(640, 360);
}

std::vector<ImportedCharge> ChronographImportDialog::selected() const
{
    std::vector<ImportedCharge> out;
    for (int row = 0; row < m_table->rowCount(); ++row)
    {
        if (m_table->item(row, Use)->checkState() != Qt::Checked) continue;
        // the shown text is rounded: an untouched charge keeps its exact value
        const QString text = m_table->item(row, Charge)->text();
        const auto g = text == grt::units::format(m_default, unitOf(Quantity::BulletMass))
                           ? std::nullopt
                           : grt::units::parseToBase(text, unitOf(Quantity::BulletMass));
        out.push_back({.series = m_file.series[static_cast<std::size_t>(row)], .charge_g = g && *g > 0.0 ? *g : m_default});
    }
    return out;
}

void ChronographImportDialog::setChecked(int row, bool on)
{
    m_table->item(row, Use)->setCheckState(on ? Qt::Checked : Qt::Unchecked);
}

void ChronographImportDialog::setChargeText(int row, const QString &text)
{
    m_table->item(row, Charge)->setText(text);
}
