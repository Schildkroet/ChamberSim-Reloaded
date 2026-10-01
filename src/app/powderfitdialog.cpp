#include "powderfitdialog.h"

#include "unitprefs.h"

#include <QApplication>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

using grt::units::Quantity;
using namespace Qt::StringLiterals;

namespace {

QString shown(double base, Quantity q, int decimals)
{
    const grt::units::Unit &u = UnitPreferences::instance().unitFor(q);
    return base > 0.0 ? grt::units::format(base, u, decimals) : QStringLiteral("–");
}

QString percent(double value, double reference)
{
    if (value <= 0.0 || reference <= 0.0) return QStringLiteral("–");
    return QStringLiteral("%1 %").arg(QLocale().toString(100.0 * (value / reference - 1.0), 'f', 1));
}

QTableWidgetItem *cell(const QString &text)
{
    auto *item = new QTableWidgetItem(text);
    item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    return item;
}

}  // namespace

PowderFitDialog::PowderFitDialog(std::vector<grt::powderfit::Sample> samples, QStringList warnings, QWidget *parent)
    : QDialog(parent)
    , m_samples(std::move(samples))
    , m_table(new QTableWidget(0, 9))
    , m_summary(new QLabel)
    , m_fit(new QPushButton(tr("Fit")))
    , m_apply(new QPushButton(tr("Apply to this load")))
    , m_saveButton(new QPushButton(tr("Save as own propellant…")))
{
    setWindowTitle(tr("Fit to measurements"));
    auto *intro = new QLabel(tr("Changes the propellant's model values until the simulation agrees with the measured "
                                "charges. Check the case volume (fired case, water), the bullet length and the cartridge "
                                "length first: wrong values there give a wrong model."));
    intro->setWordWrap(true);

    // the values to fit, preset for the amount of data
    auto *paramRow = new QHBoxLayout;
    paramRow->addWidget(new QLabel(tr("Values to fit:")));
    const std::vector<std::pair<grt::powderfit::Param, QString>> params{
        {grt::powderfit::Param::Ba, tr("Burn rate (Ba)")}, {grt::powderfit::Param::A0, tr("Form factor a0")},
        {grt::powderfit::Param::Z1, tr("Transition z1")},  {grt::powderfit::Param::Z2, tr("Transition z2")}};
    for (const auto &[p, label] : params) {
        auto *box = new QCheckBox(label);
        m_params[p] = box;
        paramRow->addWidget(box);
    }
    paramRow->addStretch(1);
    paramRow->addWidget(m_fit);
    setParams(grt::powderfit::defaultParams(m_samples));

    auto *warn = new QLabel;
    warn->setWordWrap(true);
    warn->setVisible(!warnings.isEmpty());
    QStringList escaped;
    for (const QString &w : warnings) escaped << w.toHtmlEscaped();
    warn->setText(u"<span style='color:#c0392b'>"_s + escaped.join(u"<br>"_s) + u"</span>"_s);

    const auto sym = [](Quantity q) { return QString::fromUtf8(UnitPreferences::instance().unitFor(q).symbol); };
    m_table->setHorizontalHeaderLabels({tr("Load"), tr("Charge [%1]").arg(sym(Quantity::BulletMass)), tr("Shots"),
                                        tr("Measured [%1]").arg(sym(Quantity::Velocity)), tr("Before"), tr("Δ"), tr("After"), tr("Δ"),
                                        tr("Pressure Δ after")});
    m_table->verticalHeader()->hide();
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_summary->setWordWrap(true);
    m_summary->setTextFormat(Qt::RichText);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close);
    buttons->addButton(m_apply, QDialogButtonBox::AcceptRole);
    buttons->addButton(m_saveButton, QDialogButtonBox::ActionRole);
    m_apply->setEnabled(false);
    m_saveButton->setEnabled(false);
    m_saveButton->setVisible(false);
    m_apply->setToolTip(tr("Put the fitted values into this load's propellant"));
    m_saveButton->setToolTip(tr("Add the propellant with the fitted values to your own records in the database"));
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_apply, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_saveButton, &QPushButton::clicked, this, [this] {
        if (m_result && m_save && m_save(*m_result)) m_saveButton->setText(tr("Saved"));
    });
    connect(m_fit, &QPushButton::clicked, this, &PowderFitDialog::runFit);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(intro);
    layout->addLayout(paramRow);
    layout->addWidget(warn);
    layout->addWidget(m_table, 1);
    layout->addWidget(m_summary);
    layout->addWidget(buttons);
    resize(900, 480);
    showResult();
}

void PowderFitDialog::setSaveHandler(std::function<bool(const grt::powderfit::Result &)> handler)
{
    m_save = std::move(handler);
    m_saveButton->setVisible(static_cast<bool>(m_save));
}

void PowderFitDialog::setParams(const std::vector<grt::powderfit::Param> &params)
{
    for (auto &[p, box] : m_params) box->setChecked(std::ranges::find(params, p) != params.end());
}

void PowderFitDialog::runFit()
{
    grt::powderfit::Options options;
    for (const auto &[p, box] : m_params)
        if (box->isChecked()) options.params.push_back(p);
    if (options.params.empty()) return;
    QApplication::setOverrideCursor(Qt::WaitCursor);
    m_result = grt::powderfit::fit(m_samples, options);
    QApplication::restoreOverrideCursor();
    m_saveButton->setText(tr("Save as own propellant…"));
    showResult();
}

void PowderFitDialog::showResult()
{
    m_table->setRowCount(static_cast<int>(m_samples.size()));
    for (int row = 0; row < m_table->rowCount(); ++row) {
        const grt::powderfit::Sample &s = m_samples[static_cast<std::size_t>(row)];
        auto *name = new QTableWidgetItem(s.source);
        name->setFlags(name->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(row, 0, name);
        m_table->setItem(row, 1, cell(shown(s.charge_g, Quantity::BulletMass, -1)));
        m_table->setItem(row, 2, cell(QString::number(s.shots)));
        m_table->setItem(row, 3, cell(shown(s.velocity_ms, Quantity::Velocity, 1)));
        const grt::powderfit::SampleResult *r = m_result ? &m_result->samples[static_cast<std::size_t>(row)] : nullptr;
        m_table->setItem(row, 4, cell(r ? shown(r->velocityBefore, Quantity::Velocity, 1) : QString()));
        m_table->setItem(row, 5, cell(r ? percent(r->velocityBefore, s.velocity_ms) : QString()));
        m_table->setItem(row, 6, cell(r ? shown(r->velocityAfter, Quantity::Velocity, 1) : QString()));
        m_table->setItem(row, 7, cell(r ? percent(r->velocityAfter, s.velocity_ms) : QString()));
        m_table->setItem(row, 8, cell(r && s.pressureShots > 0 ? percent(r->pressureAfter, s.pressure_bar) : QString()));
    }
    m_table->setColumnHidden(8, std::ranges::none_of(m_samples, [](const auto &s) { return s.pressureShots > 0; }));
    m_apply->setEnabled(m_result && m_result->improved);
    m_saveButton->setEnabled(m_result && m_result->improved);
    if (!m_result) {
        m_summary->setText(tr("Choose the values to fit and click <b>Fit</b>."));
        return;
    }
    QStringList values;
    for (std::size_t i = 0; i < m_result->params.size(); ++i)
        values << QStringLiteral("%1: %2 → <b>%3</b>")
                      .arg(grt::powderfit::name(m_result->params[i]), QLocale().toString(m_result->before[i], 'f', 4),
                           QLocale().toString(m_result->after[i], 'f', 4));
    QString text = values.join(u", &nbsp; "_s) + u"<br>"_s
                   + tr("Velocity error (rms): %1 % → <b>%2 %</b>")
                         .arg(QLocale().toString(100 * m_result->velocityRmsBefore, 'f', 2), QLocale().toString(100 * m_result->velocityRmsAfter, 'f', 2));
    if (m_result->pressureRmsBefore > 0.0)
        text += u", "_s + tr("pressure error: %1 % → %2 %").arg(QLocale().toString(100 * m_result->pressureRmsBefore, 'f', 1),
                                                               QLocale().toString(100 * m_result->pressureRmsAfter, 'f', 1));
    if (!m_result->improved) text += u"<br>"_s + tr("The values were not changed: the current ones fit best.");
    if (m_samples.size() < m_result->params.size())
        text += u"<br><span style='color:#c0392b'>"_s + tr("Fewer measured charges than values: the result is not well determined.") + u"</span>"_s;
    m_summary->setText(text);
}
