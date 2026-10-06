#include "qualityrow.h"

#include "qualitydelegate.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>

#include <algorithm>

QualityRow::QualityRow(QWidget *parent)
    : QWidget(parent)
    , m_value(new QLabel)
    , m_bar(makeQualityBar(0.0))
{
    auto *row = new QHBoxLayout(this);
    row->setContentsMargins(0, 0, 0, 0);
    row->addWidget(new QLabel(tr("Model quality:")));
    m_value->setMinimumWidth(m_value->fontMetrics().horizontalAdvance(tr("unknown")) + 8);
    row->addWidget(m_value);
    row->addWidget(m_bar, 1);
    setToolTip(tr("Quality of the fit of the powder model: 1 = very good, 0 = unknown"));
    setQuality(0.0, false);
}

void QualityRow::setQuality(double quality, bool known)
{
    quality = known ? std::clamp(quality, 0.0, 1.0) : 0.0;
    m_value->setText(quality > 0.0 ? QLocale().toString(quality, 'f', 2) : tr("unknown"));
    setQualityBar(m_bar, quality);
}
