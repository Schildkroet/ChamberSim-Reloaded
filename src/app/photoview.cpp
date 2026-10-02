#include "photoview.h"

#include <QButtonGroup>
#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QScrollArea>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>

PhotoView::PhotoView(QWidget *parent)
    : QWidget(parent)
    , m_overviewButton(new QToolButton(this))
    , m_detailButton(new QToolButton(this))
{
    m_overviewButton->setText(tr("Overview"));
    m_detailButton->setText(tr("Detail"));
    m_overviewButton->setToolTip(tr("Photo of the powder grains"));
    m_detailButton->setToolTip(tr("Close-up photo of a grain"));
    auto *group = new QButtonGroup(this);
    group->setExclusive(true);
    for (QToolButton *b : {m_overviewButton, m_detailButton})
    {
        b->setCheckable(true);
        b->setAutoRaise(true);
        group->addButton(b);
    }
    connect(m_overviewButton, &QToolButton::clicked, this, [this] { select(0); });
    connect(m_detailButton, &QToolButton::clicked, this, [this] { select(1); });
    auto *bar = new QHBoxLayout;
    bar->addWidget(m_overviewButton);
    bar->addWidget(m_detailButton);
    bar->addStretch(1);
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addLayout(bar);
    layout->addStretch(1);
    setToolTip(tr("Double-click to enlarge"));
}

void PhotoView::setPhotos(const Photo &overview, const Photo &detail, const QString &key)
{
    m_key = key;
    m_photos[0] = overview;
    m_photos[1] = detail;
    m_overviewButton->setEnabled(!overview.image.isNull());
    m_detailButton->setEnabled(!detail.image.isNull());
    select(!overview.image.isNull() ? 0 : !detail.image.isNull() ? 1 : -1);
}

void PhotoView::clear()
{
    setPhotos({}, {}, {});
}

void PhotoView::show(int which)
{
    if (which >= 0 && which < 2 && !m_photos[which].image.isNull()) select(which);
}

void PhotoView::select(int which)
{
    m_current = which;
    m_overviewButton->setChecked(which == 0);
    m_detailButton->setChecked(which == 1);
    update();
}

const PhotoView::Photo *PhotoView::photo() const
{
    return m_current >= 0 ? &m_photos[m_current] : nullptr;
}

QRect PhotoView::imageRect(const QImage &img) const
{
    const int top = m_overviewButton->sizeHint().height() + 4;
    const QRect area(0, top, width(), std::max(1, height() - top));
    QSize s = img.size();
    s.scale(area.size(), Qt::KeepAspectRatio);
    return QRect(QPoint(area.left() + (area.width() - s.width()) / 2, area.top()), s);
}

double PhotoView::scaleBarMm(double pxPerMm, double imageWidthPx, double shownWidthPx)
{
    if (pxPerMm <= 0.0 || imageWidthPx <= 0.0 || shownWidthPx <= 0.0) return 0.0;
    const double shownPxPerMm = pxPerMm * shownWidthPx / imageWidthPx;
    const double target = 0.2 * shownWidthPx / shownPxPerMm;  // mm that fill a fifth of the picture
    const double decade = std::pow(10.0, std::floor(std::log10(target)));
    double best = decade;
    for (const double m : {1.0, 2.0, 5.0, 10.0})
        if (std::abs(m * decade - target) < std::abs(best - target)) best = m * decade;
    return best;
}

void PhotoView::paintEvent(QPaintEvent *)
{
    const Photo *p = photo();
    if (!p || p->image.isNull()) return;
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    const QRect r = imageRect(p->image);
    painter.drawImage(r, p->image);
    const double mm = scaleBarMm(p->pxPerMm, p->image.width(), r.width());
    if (mm <= 0.0) return;
    const double barPx = mm * p->pxPerMm * r.width() / p->image.width();
    const QString label = mm >= 1.0 ? tr("%1 mm").arg(mm) : tr("%1 mm").arg(mm, 0, 'f', 1);
    // white bar with a dark outline: readable on any photo
    const QRectF bar(r.left() + 10, r.bottom() - 18, barPx, 5);
    painter.setPen(QPen(Qt::black, 3));
    painter.drawLine(bar.bottomLeft() + QPointF(0, -2), bar.bottomRight() + QPointF(0, -2));
    painter.setPen(QPen(Qt::white, 1.5));
    painter.drawLine(bar.bottomLeft() + QPointF(0, -2), bar.bottomRight() + QPointF(0, -2));
    QFont f = font();
    f.setBold(true);
    painter.setFont(f);
    const QRect text(static_cast<int>(bar.left()), static_cast<int>(bar.top()) - 20, static_cast<int>(std::max(barPx, 60.0)), 18);
    painter.setPen(Qt::black);
    painter.drawText(text.translated(1, 1), Qt::AlignLeft | Qt::AlignVCenter, label);
    painter.setPen(Qt::white);
    painter.drawText(text, Qt::AlignLeft | Qt::AlignVCenter, label);
}

void PhotoView::mouseDoubleClickEvent(QMouseEvent *event)
{
    const Photo *p = photo();
    if (!p || p->image.isNull() || event->button() != Qt::LeftButton) return;
    QDialog dlg(this);
    dlg.setWindowTitle(p->name.isEmpty() ? tr("Photo") : p->name);
    auto *label = new QLabel;
    label->setPixmap(QPixmap::fromImage(p->image));
    auto *scroll = new QScrollArea;
    scroll->setWidget(label);
    auto *l = new QVBoxLayout(&dlg);
    l->setContentsMargins(0, 0, 0, 0);
    l->addWidget(scroll);
    dlg.resize(std::min(p->image.width() + 24, 1000), std::min(p->image.height() + 24, 800));
    dlg.exec();
}
