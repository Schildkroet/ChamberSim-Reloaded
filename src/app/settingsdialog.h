#pragma once

#include "log.h"

#include <QDialog>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QSpinBox;

// File > Settings: options that apply to the whole application. Changes take effect with OK or Apply.
class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr);

    [[nodiscard]] int fontSize() const;

    // The original program's database (read-only) and a summary of what is available.
    void setOriginalDatabase(const QString &path, const QString &summary);
    [[nodiscard]] QString originalDatabase() const;

signals:
    // OK or Apply with another original database file (empty: none).
    void originalDatabaseChosen(const QString &path);
    // OK, Apply or Cancel changed the choice of the program's own title bar.
    void customTitleBarChanged(bool on);

private:
    void apply();
    [[nodiscard]] grt::log::Config logConfig() const;

    QSpinBox *m_fontSize;
    QCheckBox *m_titleBar;
    QCheckBox *m_reopen;
    bool m_originalReopen;
    bool m_originalTitleBar;
    QComboBox *m_unitSystem;
    QComboBox *m_calibration;
    QLabel *m_calibrationInfo;
    QComboBox *m_logLevel;
    QCheckBox *m_logConsole;
    QCheckBox *m_logFile;
    grt::log::Config m_originalLog;
    QLineEdit *m_database;
    QLabel *m_databaseInfo;
    QString m_databasePath;
    int m_originalSize;
    int m_originalSystem;
};
