#include "reporttemplatedialog.h"

#include "unitprefs.h"

#include <QDialogButtonBox>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QTextBrowser>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>

ReportTemplateDialog::ReportTemplateDialog(grt::report::Data data, const QString &templateText, QWidget *parent)
    : QDialog(parent)
    , m_data(std::move(data))
    , m_editor(new QPlainTextEdit)
    , m_preview(new QTextBrowser)
    , m_timer(new QTimer(this))
{
    setWindowTitle(tr("Edit report template"));
    resize(1200, 760);
    m_editor->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_editor->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_editor->setPlainText(templateText);
    m_preview->setStyleSheet(QStringLiteral("QTextBrowser { background: white; color: black; }"));

    // "Insert value": every available source.property, grouped by source
    auto *insert = new QToolButton;
    insert->setText(tr("Insert value"));
    insert->setPopupMode(QToolButton::InstantPopup);
    auto *menu = new QMenu(insert);
    QStringList keys = m_data.keys();
    keys.sort();
    QMap<QString, QMenu *> groups;
    for (const QString &key : keys)
    {
        const QString source = key.section(u'.', 0, 0);
        if (!groups.contains(source)) groups.insert(source, menu->addMenu(source));
        const bool image = m_data.find(key)->kind == grt::report::Value::Kind::Image;
        QAction *a = groups[source]->addAction(key.section(u'.', 1));
        connect(a, &QAction::triggered, this,
                [this, key, image] { insertAtCursor(image ? QStringLiteral("~~%1~~").arg(key) : QStringLiteral("~~%1~~").arg(key)); });
    }
    insert->setMenu(menu);

    auto *reset = new QPushButton(tr("Reset to default"));
    connect(reset, &QPushButton::clicked, this, [this] { setTemplateText(grt::report::defaultTemplate()); });
    auto *help = new QLabel(tr("Placeholders: <tt>~~source.property~~</tt> or <tt>~~source.property(\"#.00\")~~</tt> "
                               "(a format string uses the file's units: mm, g, bar, m/s). Sources: file, work, result, caliber."));
    help->setWordWrap(true);

    auto *bar = new QHBoxLayout;
    bar->addWidget(insert);
    bar->addWidget(reset);
    bar->addWidget(help, 1);

    auto *split = new QSplitter(Qt::Horizontal);
    split->addWidget(m_editor);
    split->addWidget(m_preview);
    split->setSizes({560, 640});

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    auto *layout = new QVBoxLayout(this);
    layout->addLayout(bar);
    layout->addWidget(split, 1);
    layout->addWidget(buttons);

    m_timer->setSingleShot(true);
    m_timer->setInterval(250);  // render after the typing pauses
    connect(m_timer, &QTimer::timeout, this, &ReportTemplateDialog::updatePreview);
    connect(m_editor, &QPlainTextEdit::textChanged, m_timer, qOverload<>(&QTimer::start));
    updatePreview();
}

QString ReportTemplateDialog::templateText() const
{
    return m_editor->toPlainText();
}

void ReportTemplateDialog::setTemplateText(const QString &text)
{
    m_editor->setPlainText(text);
}

void ReportTemplateDialog::insertAtCursor(const QString &text)
{
    m_editor->insertPlainText(text);
    m_editor->setFocus();
}

void ReportTemplateDialog::updatePreview()
{
    grt::report::RenderOptions options;
    options.unitFor = [](grt::units::Quantity q) -> const grt::units::Unit & { return UnitPreferences::instance().unitFor(q); };
    m_preview->setHtml(grt::report::renderReport(m_editor->toPlainText(), m_data, options));
}
