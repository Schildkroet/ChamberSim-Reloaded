#include "settingsdialog.h"

#include "appfont.h"
#include "unitprefs.h"
#include "units.h"

#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include <algorithm>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , m_fontSize(new QSpinBox)
    , m_unitSystem(new QComboBox)
    , m_originalSize(appfont::storedSize())
    , m_originalSystem(static_cast<int>(UnitPreferences::instance().system()))
{
    setWindowTitle(tr("Settings"));
    m_fontSize->setRange(appfont::kMinSize, appfont::kMaxSize);
    m_fontSize->setSuffix(tr(" pt"));
    m_fontSize->setValue(m_originalSize);

    // Two groups with the same grid: label | control | reset button, so the fields line up.
    const auto makeForm = [](QGroupBox *box) {
        auto *grid = new QGridLayout(box);
        grid->setColumnStretch(1, 1);
        grid->setHorizontalSpacing(12);
        grid->setVerticalSpacing(8);
        return grid;
    };

    auto *appearance = new QGroupBox(tr("Appearance"));
    auto *appearanceGrid = makeForm(appearance);
    auto *fontLabel = new QLabel(tr("Font size:"));
    fontLabel->setBuddy(m_fontSize);
    auto *reset = new QPushButton(tr("Default (%1 pt)").arg(appfont::systemSize()));
    connect(reset, &QPushButton::clicked, this, [this] { m_fontSize->setValue(appfont::systemSize()); });
    appearanceGrid->addWidget(fontLabel, 0, 0);
    appearanceGrid->addWidget(m_fontSize, 0, 1);
    appearanceGrid->addWidget(reset, 0, 2);

    for (const grt::units::System sys : {grt::units::System::Original, grt::units::System::Metric, grt::units::System::Imperial})
        m_unitSystem->addItem(grt::units::systemName(sys), static_cast<int>(sys));
    m_unitSystem->setCurrentIndex(std::max(0, m_unitSystem->findData(m_originalSystem)));
    auto *units = new QGroupBox(tr("Units"));
    auto *unitGrid = makeForm(units);
    auto *systemLabel = new QLabel(tr("Unit system:"));
    systemLabel->setBuddy(m_unitSystem);
    auto *resetUnits = new QPushButton(tr("Reset per-field units"));
    resetUnits->setToolTip(tr("Forget the units chosen for single fields; every field follows the unit system again"));
    connect(resetUnits, &QPushButton::clicked, this, [] { UnitPreferences::instance().resetFieldUnits(); });
    unitGrid->addWidget(systemLabel, 0, 0);
    unitGrid->addWidget(m_unitSystem, 0, 1);
    unitGrid->addWidget(resetUnits, 0, 2);

    // the reset buttons share one width, the controls share the middle column
    const int buttonWidth = std::max(reset->sizeHint().width(), resetUnits->sizeHint().width());
    reset->setMinimumWidth(buttonWidth);
    resetUnits->setMinimumWidth(buttonWidth);
    m_fontSize->setMinimumWidth(110);
    m_unitSystem->setMinimumWidth(110);
    m_fontSize->setAlignment(Qt::AlignRight);
    const int labelWidth = std::max(fontLabel->sizeHint().width(), systemLabel->sizeHint().width());
    fontLabel->setMinimumWidth(labelWidth);
    systemLabel->setMinimumWidth(labelWidth);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel);
    connect(buttons->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &SettingsDialog::apply);
    connect(buttons, &QDialogButtonBox::accepted, this, [this] {
        apply();
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, [this] {
        // undo what Apply has done
        appfont::apply(*qobject_cast<QApplication *>(QCoreApplication::instance()), m_originalSize);
        appfont::storeSize(m_originalSize);
        UnitPreferences::instance().setSystem(static_cast<grt::units::System>(m_originalSystem));
        reject();
    });
    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(12);
    layout->addWidget(appearance);
    layout->addWidget(units);
    layout->addStretch(1);
    layout->addWidget(buttons);
    setMinimumWidth(460);
}

int SettingsDialog::fontSize() const
{
    return m_fontSize->value();
}

void SettingsDialog::apply()
{
    const int applied = appfont::apply(*qobject_cast<QApplication *>(QCoreApplication::instance()), m_fontSize->value());
    appfont::storeSize(applied);
    m_fontSize->setValue(applied);
    UnitPreferences::instance().setSystem(static_cast<grt::units::System>(m_unitSystem->currentData().toInt()));
}
