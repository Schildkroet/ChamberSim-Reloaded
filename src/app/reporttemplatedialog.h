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

#include "report.h"

#include <QDialog>

class QMenu;
class QPlainTextEdit;
class QTextBrowser;
class QTimer;

// Edit the report template with a live preview and a menu that inserts values.
class ReportTemplateDialog : public QDialog
{
    Q_OBJECT
public:
    ReportTemplateDialog(csr::report::Data data, const QString &templateText, QWidget *parent = nullptr);

    [[nodiscard]] QString templateText() const;
    void setTemplateText(const QString &text);

private:
    void updatePreview();
    void insertAtCursor(const QString &text);

    csr::report::Data m_data;
    QPlainTextEdit *m_editor;
    QTextBrowser *m_preview;
    QTimer *m_timer;
};
