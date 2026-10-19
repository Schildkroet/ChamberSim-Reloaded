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

#include "reportview.h"
#include "log.h"

#include "buttonstyle.h"
#include "reporttemplatedialog.h"
#include "unitprefs.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPageLayout>
#include <QPageSize>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrinter>
#include <QPushButton>
#include <QSettings>
#include <QTimer>
#include <QSpinBox>
#include <QLabel>
#include <QTextBrowser>
#include <QTextDocument>
#include <QSignalBlocker>
#include <algorithm>
#include <QVBoxLayout>

namespace
{
constexpr auto kTemplateKey = "report/template";
constexpr auto kScaleKey = "report/scale";
}  // namespace

ReportView::ReportView(QWidget *parent)
    : QWidget(parent)
    , m_browser(new QTextBrowser)
    , m_scale(new QSpinBox)
{
    m_scale->setRange(kMinScale, kMaxScale);
    m_scale->setSingleStep(5);
    m_scale->setSuffix(QStringLiteral(" %"));
    m_scale->setValue(storedScale());
    m_scale->setToolTip(tr("Size of the report: scales text, tables and images (on screen and in print, preview and PDF)"));
    m_browser->setOpenExternalLinks(true);
    // the page looks like paper, whatever the theme is
    m_browser->setStyleSheet(QStringLiteral("QTextBrowser { background: white; color: black; }"));

    auto *edit = new QPushButton(tr("Edit template"));
    auto *preview = new QPushButton(tr("Print preview"));
    auto *print = new QPushButton(tr("Print"));
    auto *pdf = new QPushButton(tr("Export PDF"));
    for (QPushButton *b : {preview, print, pdf}) markPrimary(b);
    auto *bar = new QHBoxLayout;
    bar->addWidget(edit);
    bar->addWidget(new QLabel(tr("Scale")));
    bar->addWidget(m_scale);
    bar->addStretch(1);
    bar->addWidget(preview);
    bar->addWidget(print);
    bar->addWidget(pdf);
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addLayout(bar);
    layout->addWidget(m_browser, 1);

    connect(edit, &QPushButton::clicked, this, [this] { editTemplate(this); });
    connect(preview, &QPushButton::clicked, this, [this] { showPrintPreview(this); });
    connect(print, &QPushButton::clicked, this, [this] { showPrintDialog(this); });
    connect(pdf, &QPushButton::clicked, this,
            [this]
            {
                const QString path = QFileDialog::getSaveFileName(this, tr("Export PDF"), {}, tr("PDF documents (*.pdf)"));
                if (!path.isEmpty() && !exportPdf(path)) QMessageBox::warning(this, tr("Export PDF"), tr("Cannot write %1").arg(path));
            });
    connect(m_scale, &QSpinBox::valueChanged, this, [this](int v) { setScale(v); });
    connect(&UnitPreferences::instance(), &UnitPreferences::changed, this, &ReportView::invalidate);
}

void ReportView::setDataProvider(std::function<csr::report::Data()> provider)
{
    m_provider = std::move(provider);
    invalidate();
}

void ReportView::invalidate()
{
    m_dirty = true;
    if (isVisible()) refresh();
}

void ReportView::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    // not inside the show: the provider renders other widgets, and the window's children are still being shown
    if (m_dirty)
        QTimer::singleShot(0, this,
                           [this]
                           {
                               if (m_dirty && isVisible()) refresh();
                           });
}

void ReportView::refresh()
{
    m_dirty = false;
    if (!m_provider) return;
    csr::report::RenderOptions options;
    options.unitFor = [](csr::units::Quantity q) -> const csr::units::Unit & { return UnitPreferences::instance().unitFor(q); };
    QFont f = m_browser->document()->defaultFont();
    if (m_baseFontSize <= 0) m_baseFontSize = f.pointSizeF() > 0 ? f.pointSizeF() : 10.0;
    f.setPointSizeF(m_baseFontSize * storedScale() / 100.0);
    m_browser->document()->setDefaultFont(f);
    m_browser->setHtml(csr::report::renderReport(storedTemplate(), m_provider(), options));
}

QString ReportView::html()
{
    if (m_dirty) refresh();
    return m_browser->toHtml();
}

bool ReportView::hasCustomTemplate()
{
    return !QSettings().value(QLatin1String(kTemplateKey)).toString().isEmpty();
}

QString ReportView::storedTemplate()
{
    const QString t = QSettings().value(QLatin1String(kTemplateKey)).toString();
    return t.isEmpty() ? csr::report::defaultTemplate() : t;
}

void ReportView::storeTemplate(const QString &text)
{
    QSettings s;
    if (text.trimmed().isEmpty() || text == csr::report::defaultTemplate()) s.remove(QLatin1String(kTemplateKey));
    else s.setValue(QLatin1String(kTemplateKey), text);
}

int ReportView::clampScale(int percent)
{
    return std::clamp(percent, kMinScale, kMaxScale);
}

int ReportView::storedScale()
{
    QSettings s;
    if (!s.contains(QLatin1String(kScaleKey))) return 100;
    bool ok = false;
    const int v = s.value(QLatin1String(kScaleKey)).toInt(&ok);
    return ok ? clampScale(v) : 100;
}

void ReportView::storeScale(int percent)
{
    QSettings s;
    const int v = clampScale(percent);
    if (v == 100) s.remove(QLatin1String(kScaleKey));
    else s.setValue(QLatin1String(kScaleKey), v);
}

void ReportView::setScale(int percent)
{
    const int v = clampScale(percent);
    storeScale(v);
    QSignalBlocker block(m_scale);
    m_scale->setValue(v);
    refresh();
}

// The document laid out for printing: a page of `pagePoints / scale` units that is then mapped onto the
// real page, so a larger scale makes everything (text, tables, images) larger.
std::unique_ptr<QTextDocument> ReportView::makePrintDocument(const QSizeF &pagePoints)
{
    const double s = storedScale() / 100.0;
    const QSizeF page(pagePoints.width() / s, pagePoints.height() / s);
    csr::report::RenderOptions options;
    options.unitFor = [](csr::units::Quantity q) -> const csr::units::Unit & { return UnitPreferences::instance().unitFor(q); };
    options.pageWidth = static_cast<int>(page.width());
    auto doc = std::make_unique<QTextDocument>();
    doc->setHtml(csr::report::renderReport(storedTemplate(), m_provider ? m_provider() : csr::report::Data{}, options));
    doc->setPageSize(page);
    return doc;
}

int ReportView::printedPageCount(const QSizeF &pagePoints)
{
    return makePrintDocument(pagePoints)->pageCount();
}

void ReportView::print(QPrinter *printer)
{
    if (!m_provider) return;
    makePrintDocument(QSizeF(printer->pageRect(QPrinter::Point).size()))->print(printer);
}

bool ReportView::exportPdf(const QString &path)
{
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageMargins(QMarginsF(15, 15, 15, 15), QPageLayout::Millimeter);
    printer.setOutputFileName(path);
    print(&printer);
    const bool ok = QFileInfo(path).size() > 0;
    if (!ok) LOG_Warn() << "PDF export produced no file:" << path;
    return ok;
}

void ReportView::showPrintPreview(QWidget *parent)
{
    QPrinter printer(QPrinter::HighResolution);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageMargins(QMarginsF(15, 15, 15, 15), QPageLayout::Millimeter);
    QPrintPreviewDialog dlg(&printer, parent);
    dlg.resize(1000, 800);
    connect(&dlg, &QPrintPreviewDialog::paintRequested, this, [this](QPrinter *p) { print(p); });
    dlg.exec();
}

void ReportView::showPrintDialog(QWidget *parent)
{
    QPrinter printer(QPrinter::HighResolution);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageMargins(QMarginsF(15, 15, 15, 15), QPageLayout::Millimeter);
    QPrintDialog dlg(&printer, parent);
    if (dlg.exec() == QDialog::Accepted) print(&printer);
}

void ReportView::editTemplate(QWidget *parent)
{
    ReportTemplateDialog dlg(m_provider ? m_provider() : csr::report::Data{}, storedTemplate(), parent);
    if (dlg.exec() != QDialog::Accepted) return;
    storeTemplate(dlg.templateText());
    refresh();
}
