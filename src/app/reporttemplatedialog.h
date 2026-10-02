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
    ReportTemplateDialog(grt::report::Data data, const QString &templateText, QWidget *parent = nullptr);

    [[nodiscard]] QString templateText() const;
    void setTemplateText(const QString &text);

private:
    void updatePreview();
    void insertAtCursor(const QString &text);

    grt::report::Data m_data;
    QPlainTextEdit *m_editor;
    QTextBrowser *m_preview;
    QTimer *m_timer;
};
