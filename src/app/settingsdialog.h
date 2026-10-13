/*
 * This file is part of ChamberSim Reloaded.
 *
 * Copyright (C) 2026 Schildkroet
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

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

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    // The wrapped labels (model description, database info) get the height their text needs at the current width; the layout
    // would give them less than that and cut the text off.
    void fitWrappedLabels();
    void apply();
    [[nodiscard]] csr::log::Config logConfig() const;

    QSpinBox *m_fontSize;
    QCheckBox *m_titleBar;
    QCheckBox *m_reopen;
    bool m_originalReopen;
    bool m_originalTitleBar;
    QCheckBox *m_rounded;
    bool m_originalRounded;
    QComboBox *m_unitSystem;
    QComboBox *m_calibration;
    QLabel *m_calibrationInfo;
    QCheckBox *m_originalForGrtFiles;
    QComboBox *m_logLevel;
    QCheckBox *m_logConsole;
    QCheckBox *m_logFile;
    csr::log::Config m_originalLog;
    QLineEdit *m_database;
    QLabel *m_databaseInfo;
    QString m_databasePath;
    int m_originalSize;
    int m_originalSystem;
};
