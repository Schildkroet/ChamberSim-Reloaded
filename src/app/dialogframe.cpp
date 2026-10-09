#include "dialogframe.h"
#include "titlebar.h"
#include "windowborder.h"

#include <QDialog>
#include <QGuiApplication>
#include <QLibrary>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QSettings>
#include <QWindow>

#include <algorithm>
#include <cstdint>
#include <cstdlib>

namespace
{
constexpr int kBorder = 4;  // width of the frame (the border is drawn in its outer part) and of the resize edges

constexpr auto kRoundedKey = "ui/roundedDialogs";
std::optional<bool> g_forcedRounded;

// Whether a compositing window manager runs on X11: somebody owns the selection _NET_WM_CM_S<screen>. Asked through libxcb, loaded at
// run time (the program does not link it): the few structures of the protocol that are needed are declared here.
#if defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN) && !defined(Q_OS_ANDROID)
bool x11Compositing()
{
    struct Cookie
    {
        unsigned int sequence;
    };
    struct InternReply
    {
        std::uint8_t responseType, pad0;
        std::uint16_t sequence;
        std::uint32_t length, atom;
    };
    struct OwnerReply
    {
        std::uint8_t responseType, pad0;
        std::uint16_t sequence;
        std::uint32_t length, owner;
    };
    const auto *x11 = qGuiApp->nativeInterface<QNativeInterface::QX11Application>();
    if (!x11 || !x11->connection()) return true;
    QLibrary xcb(QStringLiteral("xcb"), 1);
    if (!xcb.load()) return true;  // cannot tell: round
    using InternFn = Cookie (*)(void *, std::uint8_t, std::uint16_t, const char *);
    using InternReplyFn = InternReply *(*)(void *, Cookie, void *);
    using OwnerFn = Cookie (*)(void *, std::uint32_t);
    using OwnerReplyFn = OwnerReply *(*)(void *, Cookie, void *);
    const auto intern = reinterpret_cast<InternFn>(xcb.resolve("xcb_intern_atom"));
    const auto internReply = reinterpret_cast<InternReplyFn>(xcb.resolve("xcb_intern_atom_reply"));
    const auto owner = reinterpret_cast<OwnerFn>(xcb.resolve("xcb_get_selection_owner"));
    const auto ownerReply = reinterpret_cast<OwnerReplyFn>(xcb.resolve("xcb_get_selection_owner_reply"));
    if (!intern || !internReply || !owner || !ownerReply) return true;
    void *connection = x11->connection();
    const QByteArray name = "_NET_WM_CM_S" + QByteArray::number(0);  // the first screen (Qt's primary one in practice)
    InternReply *atom = internReply(connection, intern(connection, 0, static_cast<std::uint16_t>(name.size()), name.constData()), nullptr);
    if (!atom) return true;
    const std::uint32_t selection = atom->atom;
    std::free(atom);
    OwnerReply *reply = ownerReply(connection, owner(connection, selection), nullptr);
    if (!reply) return true;
    const bool active = reply->owner != 0;
    std::free(reply);
    return active;
}
#else
bool x11Compositing()
{
    return true;  // no X11 on this platform
}
#endif

class DialogTitleBar : public QWidget
{
public:
    DialogTitleBar(QDialog *dialog, QLabel *title, int height)
        : QWidget(dialog)
        , m_dialog(dialog)
    {
        setObjectName(QStringLiteral("dialogTitleBar"));
        setAutoFillBackground(false);
        auto *icon = new QLabel;
        icon->setFixedSize(height, height);
        icon->setAlignment(Qt::AlignCenter);
        icon->setPixmap(dialog->windowIcon().pixmap(height - 14, height - 14));
        auto *close = new WindowButton(WindowButton::Kind::Close);
        close->setFixedSize(static_cast<int>(height * 1.35), height);
        close->setToolTip(tr("Close"));
        connect(close, &QAbstractButton::clicked, dialog, &QDialog::close);
        title->setTextFormat(Qt::PlainText);
        title->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);  // elided by the space left
        auto *layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->addWidget(icon);
        layout->addWidget(title, 1);
        layout->addWidget(close);
    }

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton && m_dialog->windowHandle()) m_dialog->windowHandle()->startSystemMove();
        QWidget::mousePressEvent(event);
    }
    void paintEvent(QPaintEvent *) override
    {
        // a little off the window's color, so that the bar is a bar, and a separating line
        QPainter p(this);
        const QColor window = palette().color(QPalette::Window);
        p.fillRect(rect(), window.lightness() < 128 ? window.lighter(125) : window.darker(106));
        p.setPen(palette().color(QPalette::PlaceholderText));
        p.drawLine(rect().bottomLeft(), rect().bottomRight());
    }

private:
    QDialog *m_dialog;
};
}  // namespace

bool DialogFrame::roundedCorners()
{
    if (g_forcedRounded) return *g_forcedRounded;
    return roundedCornersPossible() && !qEnvironmentVariableIsSet("CSR_SQUARE_CORNERS") && roundedStored();
}

bool DialogFrame::roundedStored()
{
    return QSettings().value(QLatin1String(kRoundedKey), true).toBool();
}

void DialogFrame::storeRounded(bool on)
{
    QSettings().setValue(QLatin1String(kRoundedKey), on);
}

bool DialogFrame::roundedCornersPossible()
{
    static const bool compositing = QGuiApplication::platformName() != QLatin1String("xcb") || x11Compositing();
    return compositing;
}

void DialogFrame::forceRoundedCorners(std::optional<bool> forced)
{
    g_forcedRounded = forced;
}

void DialogFrame::install(QDialog *dialog)
{
    if (!dialog->isWindow() || dialog->findChild<DialogFrame *>(QLatin1String(kName), Qt::FindDirectChildrenOnly)) return;
    new DialogFrame(dialog);
}

DialogFrame::DialogFrame(QDialog *dialog)
    : QWidget(dialog)
    , m_dialog(dialog)
{
    setObjectName(QLatin1String(kName));
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_NoSystemBackground);
    setFocusPolicy(Qt::NoFocus);
    if (TitleBar::stored())
    {
        dialog->setWindowFlag(Qt::FramelessWindowHint, true);
        m_radius = roundedCorners() ? kDialogCornerRadius : 0;
        if (m_radius > 0) dialog->setAttribute(Qt::WA_TranslucentBackground);  // the corners outside the rounded window stay transparent
        m_titleHeight = std::max(40, static_cast<int>(dialog->fontMetrics().height() * 1.8));
        m_title = new QLabel(dialog->windowTitle());
        QFont f = dialog->font();
        f.setBold(true);
        if (f.pointSizeF() > 0.0) f.setPointSizeF(f.pointSizeF() * 1.1);
        m_title->setFont(f);
        m_titleBar = new DialogTitleBar(dialog, m_title, m_titleHeight);
        dialog->setMouseTracking(true);
        dialog->setContentsMargins(kBorder, kBorder + m_titleHeight, kBorder, kBorder);
    }
    dialog->installEventFilter(this);
    layoutParts();
}

bool DialogFrame::resizable() const
{
    return m_dialog->minimumSize() != m_dialog->maximumSize();
}

void DialogFrame::layoutParts()
{
    setGeometry(m_dialog->rect());
    if (m_titleBar) m_titleBar->setGeometry(kBorder, kBorder, m_dialog->width() - 2 * kBorder, m_titleHeight);
    if (m_titleBar) m_titleBar->raise();
    raise();  // the border stays above later children
}

Qt::Edges DialogFrame::edgesAt(const QPoint &pos) const
{
    Qt::Edges edges;
    if (!m_titleBar || !resizable()) return edges;
    const int corner = 3 * kBorder;  // corners are easier to grab
    const int w = m_dialog->width(), h = m_dialog->height();
    if (pos.x() < kBorder || (pos.x() < corner && (pos.y() < kBorder || pos.y() >= h - kBorder))) edges |= Qt::LeftEdge;
    if (pos.x() >= w - kBorder || (pos.x() >= w - corner && (pos.y() < kBorder || pos.y() >= h - kBorder))) edges |= Qt::RightEdge;
    if (pos.y() < kBorder || (pos.y() < corner && (pos.x() < kBorder || pos.x() >= w - kBorder))) edges |= Qt::TopEdge;
    if (pos.y() >= h - kBorder || (pos.y() >= h - corner && (pos.x() < kBorder || pos.x() >= w - kBorder))) edges |= Qt::BottomEdge;
    return edges;
}

bool DialogFrame::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_dialog)
    {
        switch (event->type())
        {
        case QEvent::Paint:
            if (m_radius > 0)
            {
                // the window's rounded background, under the children (a translucent window has no background of its own)
                QPainter p(m_dialog);
                p.setRenderHint(QPainter::Antialiasing);
                p.setPen(Qt::NoPen);
                p.setBrush(m_dialog->palette().color(QPalette::Window));
                p.drawRoundedRect(QRectF(m_dialog->rect()), m_radius, m_radius);
            }
            break;
        case QEvent::Resize: layoutParts(); break;
        case QEvent::ChildAdded:
        case QEvent::Show: layoutParts(); break;
        case QEvent::WindowTitleChange:
            if (m_title) m_title->setText(m_dialog->windowTitle());
            break;
        case QEvent::MouseButtonPress:
        {
            auto *me = static_cast<QMouseEvent *>(event);
            const Qt::Edges edges = edgesAt(me->position().toPoint());
            if (edges && me->button() == Qt::LeftButton && m_dialog->windowHandle())
            {
                m_dialog->windowHandle()->startSystemResize(edges);
                return true;
            }
            break;
        }
        case QEvent::MouseMove:
        {
            const Qt::Edges e = edgesAt(static_cast<QMouseEvent *>(event)->position().toPoint());
            const bool diagonal1 = e == (Qt::LeftEdge | Qt::TopEdge) || e == (Qt::RightEdge | Qt::BottomEdge);
            const bool diagonal2 = e == (Qt::RightEdge | Qt::TopEdge) || e == (Qt::LeftEdge | Qt::BottomEdge);
            if (diagonal1) m_dialog->setCursor(Qt::SizeFDiagCursor);
            else if (diagonal2) m_dialog->setCursor(Qt::SizeBDiagCursor);
            else if (e & (Qt::LeftEdge | Qt::RightEdge)) m_dialog->setCursor(Qt::SizeHorCursor);
            else if (e & (Qt::TopEdge | Qt::BottomEdge)) m_dialog->setCursor(Qt::SizeVerCursor);
            else m_dialog->unsetCursor();
            break;
        }
        default: break;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void DialogFrame::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    paintWindowBorder(p, rect(), palette(), m_radius);
}
