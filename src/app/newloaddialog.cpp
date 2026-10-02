#include "newloaddialog.h"

#include "buttonstyle.h"
#include "unitprefs.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>

namespace
{

using grt::units::Quantity;

// Spin box in the user's unit for `q`; values are exchanged in base units via setBase()/base().
QDoubleSpinBox *spin(Quantity q, double minBase, double maxBase, QWidget *parent)
{
    const grt::units::Unit &u = UnitPreferences::instance().unitFor(q);
    auto *s = new QDoubleSpinBox(parent);
    s->setProperty("quantity", static_cast<int>(q));
    s->setProperty("unit", QString::fromLatin1(u.id));
    s->setDecimals(u.decimals);
    s->setSuffix(QStringLiteral(" ") + QString::fromUtf8(u.symbol));
    s->setRange(grt::units::toDisplay(minBase, u), grt::units::toDisplay(maxBase, u));
    s->setGroupSeparatorShown(false);
    return s;
}

const grt::units::Unit &unitOf(const QDoubleSpinBox *s)
{
    return *grt::units::findUnit(static_cast<Quantity>(s->property("quantity").toInt()), s->property("unit").toString());
}

double base(const QDoubleSpinBox *s)
{
    return grt::units::fromDisplay(s->value(), unitOf(s));
}
void setBase(QDoubleSpinBox *s, double b)
{
    s->setValue(grt::units::toDisplay(b, unitOf(s)));
}

QString describe(const std::optional<QVariantMap> &r, const QString &a, const QString &b, const QString &placeholder)
{
    if (!r) return placeholder;
    return QStringLiteral("%1 %2").arg(r->value(a).toString(), r->value(b).toString()).trimmed();
}

}  // namespace

NewLoadDialog::NewLoadDialog(const grt::Database &db, QWidget *parent)
    : QDialog(parent)
    , m_db(db)
    , m_tabs(new QTabWidget)
    , m_matchDiameter(new QCheckBox(tr("Show only bullets that fit the caliber")))
    , m_charge(spin(Quantity::BulletMass, 0.0, 100.0, this))
    , m_barrel(spin(Quantity::Length, 0.0, 2000.0, this))
    , m_depth(spin(Quantity::Length, 0.0, 100.0, this))
    , m_twist(spin(Quantity::Length, 0.0, 2000.0, this))
    , m_temperature(spin(Quantity::Temperature, -40.0, 80.0, this))
    , m_summary(new QLabel)
    , m_chargeGrains(new QLabel)
    , m_buttons(new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel))
{
    setWindowTitle(tr("New load"));
    resize(1100, 640);

    constexpr std::array tables{grt::Table::Caliber, grt::Table::Projectile, grt::Table::Propellant};
    const std::array titles{tr("1. Caliber"), tr("2. Projectile"), tr("3. Propellant")};
    for (std::size_t i = 0; i < tables.size(); ++i)
    {
        m_browsers[i] = new TableBrowser(tables[i]);
        m_browsers[i]->setDatabase(&m_db);
        if (i == 1)
        {
            auto *page = new QWidget;
            auto *l = new QVBoxLayout(page);
            l->setContentsMargins(0, 0, 0, 0);
            m_matchDiameter->setChecked(true);
            l->addWidget(m_matchDiameter);
            l->addWidget(m_browsers[i], 1);
            m_tabs->addTab(page, titles[i]);
        }
        else
        {
            m_tabs->addTab(m_browsers[i], titles[i]);
        }
        connect(m_browsers[i], &TableBrowser::currentIdChanged, this, &NewLoadDialog::onSelectionChanged);
    }
    connect(m_matchDiameter, &QCheckBox::toggled, this, &NewLoadDialog::onSelectionChanged);

    auto *params = new QGroupBox(tr("Load data"));
    auto *form = new QFormLayout(params);
    form->addRow(tr("Propellant charge"), m_charge);
    form->addRow(QString(), m_chargeGrains);
    form->addRow(tr("Barrel length"), m_barrel);
    form->addRow(tr("Bullet seating depth"), m_depth);
    form->addRow(tr("Twist length"), m_twist);
    form->addRow(tr("Propellant temperature"), m_temperature);
    auto *reset = new QPushButton(tr("Reset to defaults"));
    form->addRow(reset);
    connect(reset, &QPushButton::clicked, this, &NewLoadDialog::applyDefaults);
    connect(m_charge, &QDoubleSpinBox::valueChanged, this,
            [this]
            {
                // show the charge in the "other" mass unit as a convenience
                const bool inGrain = unitOf(m_charge).id == QLatin1String("grain");
                m_chargeGrains->setText(inGrain ? tr("= %1 g").arg(base(m_charge), 0, 'f', 3)
                                                : tr("= %1 grains").arg(base(m_charge) / grt::kGrainToGram, 0, 'f', 1));
            });

    m_summary->setWordWrap(true);
    m_summary->setTextFormat(Qt::RichText);
    auto *right = new QVBoxLayout;
    right->addWidget(m_summary);
    right->addWidget(params);
    right->addStretch(1);
    auto *rightWidget = new QWidget;
    rightWidget->setLayout(right);
    rightWidget->setFixedWidth(320);

    auto *main = new QVBoxLayout(this);
    auto *row = new QHBoxLayout;
    row->addWidget(m_tabs, 1);
    row->addWidget(rightWidget);
    main->addLayout(row, 1);
    main->addWidget(m_buttons);
    connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    m_buttons->button(QDialogButtonBox::Ok)->setText(tr("Create load"));
    markPrimary(m_buttons->button(QDialogButtonBox::Ok));
    m_buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
    m_charge->valueChanged(0.0);
    updateSummary();
}

void NewLoadDialog::onSelectionChanged()
{
    const auto fetch = [&](grt::Table t, std::size_t idx) -> std::optional<QVariantMap>
    {
        const int id = m_browsers[idx]->currentId();
        return id < 0 ? std::nullopt : m_db.record(t, id);
    };
    const auto oldCaliber = m_caliber;
    m_caliber = fetch(grt::Table::Caliber, 0);
    m_projectile = fetch(grt::Table::Projectile, 1);
    m_propellant = fetch(grt::Table::Propellant, 2);

    // narrow the bullet list to the caliber's groove diameter
    if (m_caliber && m_matchDiameter->isChecked())
    {
        const double z = m_caliber->value(QStringLiteral("c_Z")).toDouble();
        m_browsers[1]->setRowFilter(z > 0 ? QStringLiteral("ABS(gdia - %1) < 0.25").arg(z) : QString());
    }
    else
    {
        m_browsers[1]->setRowFilter({});
    }
    if (m_caliber != oldCaliber) m_projectile = fetch(grt::Table::Projectile, 1);

    applyDefaults();
    updateSummary();
    m_buttons->button(QDialogButtonBox::Ok)->setEnabled(m_caliber && m_projectile && m_propellant);
}

void NewLoadDialog::applyDefaults()
{
    if (m_caliber)
    {
        setBase(m_barrel, grt::defaultBarrelLength_mm(*m_caliber));
        if (m_projectile) setBase(m_depth, grt::defaultSeatingDepth_mm(*m_caliber, *m_projectile));
        if (m_propellant) setBase(m_charge, grt::defaultCharge_g(*m_caliber, *m_propellant));
    }
    if (m_twist->value() == 0.0) setBase(m_twist, 240.0);
    if (!m_temperatureSet)
    {
        setBase(m_temperature, 21.0);
        m_temperatureSet = true;
    }
}

void NewLoadDialog::updateSummary()
{
    const QString none = tr("<i>not selected</i>");
    m_summary->setText(tr("<b>Caliber:</b> %1<br><b>Projectile:</b> %2<br><b>Propellant:</b> %3")
                           .arg(describe(m_caliber, QStringLiteral("cipname"), QStringLiteral("altname"), none),
                                describe(m_projectile, QStringLiteral("mname"), QStringLiteral("pname"), none),
                                describe(m_propellant, QStringLiteral("mname"), QStringLiteral("pname"), none)));
}

bool NewLoadDialog::select(int caliberId, int projectileId, int propellantId)
{
    bool ok = m_browsers[0]->selectId(caliberId);  // narrows the bullet list
    ok = m_browsers[1]->selectId(projectileId) && ok;
    ok = m_browsers[2]->selectId(propellantId) && ok;
    return ok;
}

grt::LoadFile NewLoadDialog::result() const
{
    if (!(m_caliber && m_projectile && m_propellant)) return {};
    grt::LoadParams p;
    p.charge_g = base(m_charge);
    p.barrelLength_mm = base(m_barrel);
    p.seatingDepth_mm = base(m_depth);
    p.twistLength_mm = base(m_twist);
    p.powderTemperature_C = base(m_temperature);
    return grt::buildLoad(*m_caliber, *m_projectile, *m_propellant, p, m_db.columns(grt::Table::Caliber));
}
