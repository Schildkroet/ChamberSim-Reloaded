#include "settingsdialog.h"

#include "appfont.h"
#include "database.h"
#include "titlebar.h"
#include "unitprefs.h"
#include "units.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QLineEdit>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QUrl>
#include <QVBoxLayout>

#include <algorithm>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , m_fontSize(new QSpinBox)
    , m_titleBar(new QCheckBox)
    , m_originalTitleBar(TitleBar::stored())
    , m_unitSystem(new QComboBox)
    , m_logLevel(new QComboBox)
    , m_logConsole(new QCheckBox(tr("Write to the console (stderr)")))
    , m_logFile(new QCheckBox(tr("Write to the log file")))
    , m_originalLog(grt::log::config())
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

    for (const auto level : {grt::log::Level::Debug, grt::log::Level::Info, grt::log::Level::Warn, grt::log::Level::Error, grt::log::Level::Off})
        m_logLevel->addItem(level == grt::log::Level::Off ? tr("Off") : grt::log::levelName(level), static_cast<int>(level));
    m_logLevel->setCurrentIndex(std::max(0, m_logLevel->findData(static_cast<int>(m_originalLog.level))));
    m_logLevel->setToolTip(tr("Messages of this level and above are logged: Debug (everything), Info, Warning, Error"));
    m_logConsole->setChecked(m_originalLog.console);
    m_logFile->setChecked(m_originalLog.file);
    auto *logging = new QGroupBox(tr("Logging"));
    auto *logGrid = makeForm(logging);
    auto *logLabel = new QLabel(tr("Log level:"));
    logLabel->setBuddy(m_logLevel);
    auto *logPath = new QLabel(QStringLiteral("<small>%1</small>").arg(grt::log::activePath().toHtmlEscaped()));
    logPath->setTextInteractionFlags(Qt::TextSelectableByMouse);
    logPath->setWordWrap(true);
    auto *openLog = new QPushButton(tr("Open log file"));
    connect(openLog, &QPushButton::clicked, this, [] { QDesktopServices::openUrl(QUrl::fromLocalFile(grt::log::activePath())); });
    auto *clearLog = new QPushButton(tr("Clear log file"));
    connect(clearLog, &QPushButton::clicked, this, [] { grt::log::clearFile(); });
    logGrid->addWidget(logLabel, 0, 0);
    logGrid->addWidget(m_logLevel, 0, 1);
    logGrid->addWidget(m_logConsole, 1, 1, 1, 2);
    logGrid->addWidget(m_logFile, 2, 1);
    logGrid->addWidget(openLog, 2, 2);
    logGrid->addWidget(logPath, 3, 1);
    logGrid->addWidget(clearLog, 3, 2);

    // the buttons share one width, the controls share the middle column
    const int buttonWidth = std::max({reset->sizeHint().width(), resetUnits->sizeHint().width(), browse->sizeHint().width()});
    reset->setMinimumWidth(buttonWidth);
    resetUnits->setMinimumWidth(buttonWidth);
    browse->setMinimumWidth(buttonWidth);
    openLog->setMinimumWidth(buttonWidth);
    clearLog->setMinimumWidth(buttonWidth);
    m_logLevel->setMinimumWidth(110);
    m_fontSize->setMinimumWidth(110);
    m_unitSystem->setMinimumWidth(110);
    m_fontSize->setAlignment(Qt::AlignRight);
    const int labelWidth = std::max({fontLabel->sizeHint().width(), systemLabel->sizeHint().width(), databaseLabel->sizeHint().width(),
                                     titleBarLabel->sizeHint().width(), logLabel->sizeHint().width()}) + 36;
    // 48 px from the labels to the fields (the grid's 12 px plus this margin)
    for (QLabel *l : {fontLabel, titleBarLabel, systemLabel, databaseLabel, logLabel}) l->setContentsMargins(0, 0, 36, 0);
    fontLabel->setMinimumWidth(labelWidth);
    systemLabel->setMinimumWidth(labelWidth);
    databaseLabel->setMinimumWidth(labelWidth);
    logLabel->setMinimumWidth(labelWidth);

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
        grt::log::setConfig(m_originalLog);
        grt::log::storeConfig(m_originalLog);
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
    layout->addWidget(logging);
    layout->addStretch(1);
    layout->addWidget(buttons);
    setMinimumWidth(620);
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

grt::log::Config SettingsDialog::logConfig() const
{
    grt::log::Config c = grt::log::config();  // keeps a custom file path
    c.level = static_cast<grt::log::Level>(m_logLevel->currentData().toInt());
    c.console = m_logConsole->isChecked();
    c.file = m_logFile->isChecked();
    return c;
}

void SettingsDialog::apply()
{
    const grt::log::Config log = logConfig();
    grt::log::setConfig(log);
    grt::log::storeConfig(log);
    LOG_Info() << "Logging set to" << grt::log::levelName(log.level) << "console" << log.console << "file" << log.file;
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
