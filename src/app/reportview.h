#pragma once

#include "report.h"

#include <QWidget>

#include <functional>
#include <memory>

class QPrinter;
class QSpinBox;
class QTextBrowser;
class QTextDocument;

// The "Report" tab: the load rendered through the report template, with printing and PDF export.
// The template (wiki-like text with ~~source.property~~ placeholders) is the user's own and is stored in
// the settings; without one the default report is used.
class ReportView : public QWidget
{
    Q_OBJECT
public:
    explicit ReportView(QWidget *parent = nullptr);

    // Where the values (and images) come from. Called whenever the report is rebuilt.
    void setDataProvider(std::function<grt::report::Data()> provider);
    // The data changed: the report is rebuilt as soon as it is visible.
    void invalidate();
    // Rebuilds now.
    void refresh();

    // The current report as HTML (rebuilt first if the data changed since it was last shown).
    [[nodiscard]] QString html();
    [[nodiscard]] static QString storedTemplate();   // the user's template, or the default
    static void storeTemplate(const QString &text);  // empty: back to the default
    [[nodiscard]] static bool hasCustomTemplate();

    // Scale of the report in percent: how large text, tables and images are relative to the page. For
    // output a larger scale gives bigger content on more pages, a smaller one fits more on a page.
    static constexpr int kMinScale = 50;
    static constexpr int kMaxScale = 200;
    [[nodiscard]] static int clampScale(int percent);
    [[nodiscard]] static int storedScale();  // 100 if none was stored
    static void storeScale(int percent);     // clamped; 100 removes the setting
    void setScale(int percent);              // stores, updates the control and the report
    [[nodiscard]] int scale() const { return storedScale(); }
    // Number of printed pages of the current report on a page of this size (in points) at the current scale.
    [[nodiscard]] int printedPageCount(const QSizeF &pagePoints);

    // Output
    bool exportPdf(const QString &path);
    void print(QPrinter *printer);
    void showPrintPreview(QWidget *parent);
    void showPrintDialog(QWidget *parent);
    void editTemplate(QWidget *parent);

protected:
    void showEvent(QShowEvent *event) override;

private:
    std::function<grt::report::Data()> m_provider;
    [[nodiscard]] std::unique_ptr<QTextDocument> makePrintDocument(const QSizeF &pagePoints);
    QTextBrowser *m_browser;
    QSpinBox *m_scale;
    bool m_dirty{true};
    double m_baseFontSize{0.0};
};
