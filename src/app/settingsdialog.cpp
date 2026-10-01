#include "settingsdialog.h"

#include "appfont.h"
#include "database.h"
#include "titlebar.h"
#include "unitprefs.h"
#include "units.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QLineEdit>
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
    , m_titleBar(new QCheckBox)
    , m_originalTitleBar(TitleBar::stored())
    , m_unitSystem(new QComboBox)
    , m_database(new QLineEdit)
    , m_databaseInfo(new QLabel)
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
    m_titleBar->setChecked(m_originalTitleBar);
    auto *titleBarLabel = new QLabel(tr("Custom Title Bar:"));
    titleBarLabel->setBuddy(m_titleBar);
    appearanceGrid->addWidget(titleBarLabel, 1, 0);
    appearanceGrid->addWidget(m_titleBar, 1, 1, 1, 2);

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

    auto *database = new QGroupBox(tr("Database"));
    auto *databaseGrid = makeForm(database);
    auto *databaseLabel = new QLabel(tr("Original database:"));
    databaseLabel->setBuddy(m_database);
    m_database->setObjectName(QStringLiteral("originalDatabase"));
    m_database->setClearButtonEnabled(true);
    m_database->setPlaceholderText(tr("GordonsReloadingTool.plain.db"));
    m_database->setToolTip(tr("The decrypted database of Gordon's Reloading Tool. It is only read; new records go "
                              "into the program's own database, and both appear as one database."));
    auto *browse = new QPushButton(tr("Browse…"));
    connect(browse, &QPushButton::clicked, this, [this] {
        const QString start = m_database->text().isEmpty() ? QString() : QFileInfo(m_database->text()).absolutePath();
        const QString f = QFileDialog::getOpenFileName(this, tr("Original database"), start,
                                                       tr("SQLite databases (*.db *.sqlite);;All files (*)"));
        if (!f.isEmpty()) m_database->setText(f);
    });
    m_databaseInfo->setWordWrap(true);
    m_databaseInfo->setTextInteractionFlags(Qt::TextSelectableByMouse);
    databaseGrid->addWidget(databaseLabel, 0, 0);
    databaseGrid->addWidget(m_database, 0, 1);
    databaseGrid->addWidget(browse, 0, 2);
    databaseGrid->addWidget(m_databaseInfo, 1, 1, 1, 2);

    // the buttons share one width, the controls share the middle column
    const int buttonWidth = std::max({reset->sizeHint().width(), resetUnits->sizeHint().width(), browse->sizeHint().width()});
    reset->setMinimumWidth(buttonWidth);
    resetUnits->setMinimumWidth(buttonWidth);
    browse->setMinimumWidth(buttonWidth);
    m_fontSize->setMinimumWidth(110);
    m_unitSystem->setMinimumWidth(110);
    m_fontSize->setAlignment(Qt::AlignRight);
    const int labelWidth = std::max({fontLabel->sizeHint().width(), systemLabel->sizeHint().width(), databaseLabel->sizeHint().width()});
    fontLabel->setMinimumWidth(labelWidth);
    systemLabel->setMinimumWidth(labelWidth);
    databaseLabel->setMinimumWidth(labelWidth);

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
        if (TitleBar::stored() != m_originalTitleBar) {
            TitleBar::store(m_originalTitleBar);
            emit customTitleBarChanged(m_originalTitleBar);
        }
        reject();
    });
    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(12);
    layout->addWidget(appearance);
    layout->addWidget(units);
    layout->addWidget(database);
    layout->addStretch(1);
    layout->addWidget(buttons);
    setMinimumWidth(560);
}

void SettingsDialog::setOriginalDatabase(const QString &path, const QString &summary)
{
    m_databasePath = path;
    m_database->setText(path);
    m_databaseInfo->setText(tr("<small>%1<br>New records are stored in the program's own database:<br>%2</small>")
                                .arg(summary.toHtmlEscaped(), grt::Database::defaultOwnPath().toHtmlEscaped()));
}

QString SettingsDialog::originalDatabase() const
{
    return m_database->text().trimmed();
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
    if (m_titleBar->isChecked() != TitleBar::stored()) {
        TitleBar::store(m_titleBar->isChecked());
        emit customTitleBarChanged(m_titleBar->isChecked());
    }
    if (originalDatabase() != m_databasePath) {
        m_databasePath = originalDatabase();
        emit originalDatabaseChosen(m_databasePath);
    }
}
