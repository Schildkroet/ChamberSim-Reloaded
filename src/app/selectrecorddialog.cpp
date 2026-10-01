#include "selectrecorddialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

SelectRecordDialog::SelectRecordDialog(const grt::Database &db, grt::Table table, const QString &title, QWidget *parent)
    : QDialog(parent)
    , m_browser(new TableBrowser(table))
    , m_buttons(new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel))
{
    setWindowTitle(title);
    resize(980, 600);
    m_browser->setDatabase(&db);
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_browser, 1);
    layout->addWidget(m_buttons);
    connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_browser, &TableBrowser::currentIdChanged, this, [this] { updateOk(); });
    updateOk();
}

void SelectRecordDialog::setRestriction(const QString &sqlCondition, const QString &checkBoxText)
{
    m_condition = sqlCondition;
    if (!m_restrict) {
        m_restrict = new QCheckBox(checkBoxText);
        m_restrict->setChecked(true);
        static_cast<QVBoxLayout *>(layout())->insertWidget(0, m_restrict);
        connect(m_restrict, &QCheckBox::toggled, this, [this](bool on) { m_browser->setRowFilter(on ? m_condition : QString()); });
    }
    m_restrict->setText(checkBoxText);
    m_browser->setRowFilter(m_restrict->isChecked() ? m_condition : QString());
}

void SelectRecordDialog::setResetOption(const QString &text, bool checked)
{
    if (!m_reset) {
        m_reset = new QCheckBox;
        static_cast<QVBoxLayout *>(layout())->insertWidget(layout()->count() - 1, m_reset);
    }
    m_reset->setText(text);
    m_reset->setChecked(checked);
}

bool SelectRecordDialog::resetChecked() const
{
    return m_reset && m_reset->isChecked();
}

bool SelectRecordDialog::select(int id)
{
    return m_browser->selectId(id);
}

void SelectRecordDialog::updateOk()
{
    m_buttons->button(QDialogButtonBox::Ok)->setEnabled(m_browser->currentId() >= 0);
}
