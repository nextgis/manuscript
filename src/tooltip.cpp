/******************************************************************************
*  Project: Manuscript
*  Purpose: GUI interface to prepare Sphinx documentation.
*  Author:  Dmitry Baryshnikov, dmitry.baryshnikov@nextgis.com
*******************************************************************************
*  Copyright (C) 2018 NextGIS, <info@nextgis.com>
*
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 2 of the License, or
*   (at your option) any later version.
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "tooltip.h"

#include <QApplication>
#include <QColor>
#include <QDesktopWidget>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionToolBar>
#include <QStylePainter>
#include <QVariant>
#include <QWidget>

static inline int screenNumber(const QPoint &pos, QWidget *w)
{
    if (QApplication::desktop()->isVirtualDesktop())
        return QApplication::desktop()->screenNumber(pos);
    else
        return QApplication::desktop()->screenNumber(w);
}

static inline QRect screenGeometry(const QPoint &pos, QWidget *w)
{
#ifdef Q_OS_MAC
    return QApplication::desktop()->availableGeometry(screenNumber(pos, w));
#else
    return QApplication::desktop()->screenGeometry(screenNumber(pos, w));
#endif // Q_OS_MAC
}

ToolTip::ToolTip() : QObject(), m_tip(nullptr), m_widget(nullptr)
{
    connect(&m_showTimer, &QTimer::timeout, this, &ToolTip::hideTipImmediately);
    connect(&m_hideDelayTimer, &QTimer::timeout, this, &ToolTip::hideTipImmediately);
}

ToolTip::~ToolTip()
{
    m_tip = nullptr;
}

ToolTip *ToolTip::instance()
{
    static ToolTip tooltip;
    return &tooltip;
}

void ToolTip::show(const QPoint &pos, const QString &content, QWidget *w,
                   const QString &helpId, const QRect &rect)
{
    if (content.isEmpty())
        instance()->hideTipWithDelay();
    else
        instance()->showInternal(pos, QVariant(content), TextContent, w, helpId, rect);
}

void ToolTip::show(const QPoint &pos, const QColor &color, QWidget *w,
                   const QString &helpId, const QRect &rect)
{
    if (!color.isValid())
        instance()->hideTipWithDelay();
    else
        instance()->showInternal(pos, QVariant(color), ColorContent, w, helpId, rect);
}

void ToolTip::show(const QPoint &pos, QWidget *content, QWidget *w,
                   const QString &helpId, const QRect &rect)
{
    if (!content)
        instance()->hideTipWithDelay();
    else
        instance()->showInternal(pos, QVariant::fromValue(content),
                                 WidgetContent, w, helpId, rect);
}

void ToolTip::show(const QPoint &pos, QLayout *content, QWidget *w,
                   const QString &helpId, const QRect &rect)
{
    if (content && content->count()) {
        auto tooltipWidget = new FakeToolTip;
        tooltipWidget->setLayout(content);
        instance()->showInternal(pos, QVariant::fromValue(tooltipWidget),
                                 WidgetContent, w, helpId, rect);
    } else {
        instance()->hideTipWithDelay();
    }
}

void ToolTip::move(const QPoint &pos, QWidget *w)
{
    if (isVisible())
        instance()->placeTip(pos, w);
}

bool ToolTip::pinToolTip(QWidget *w, QWidget *parent)
{
    Q_ASSERT(w);
    // Find the parent WidgetTip, tell it to pin/release the
    // widget and close.
    for (QWidget *p = w->parentWidget(); p ; p = p->parentWidget()) {
        if (WidgetTip *wt = qobject_cast<WidgetTip *>(p)) {
            wt->pinToolTipWidget(parent);
            ToolTip::hide();
            return true;
        }
    }
    return false;
}

QString ToolTip::contextHelpId()
{
    return instance()->m_tip ? instance()->m_tip->helpId() : QString();
}

bool ToolTip::acceptShow(const QVariant &content,
                         int typeId,
                         const QPoint &pos,
                         QWidget *w, const QString &helpId,
                         const QRect &rect)
{
    if (isVisible()) {
        if (m_tip->canHandleContentReplacement(typeId)) {
            // Reuse current tip.
            QPoint localPos = pos;
            if (w)
                localPos = w->mapFromGlobal(pos);
            if (tipChanged(localPos, content, typeId, w, helpId)) {
                m_tip->setContent(content);
                m_tip->setHelpId(helpId);
                setUp(pos, w, rect);
            }
            return false;
        }
        hideTipImmediately();
    }
#if !defined(QT_NO_EFFECTS) && !defined(Q_OS_MAC)
    // While the effect takes places it might be that although the widget is actually on
    // screen the isVisible function doesn't return true.
    else if (m_tip
             && (QApplication::isEffectEnabled(Qt::UI_FadeTooltip)
                 || QApplication::isEffectEnabled(Qt::UI_AnimateTooltip))) {
        hideTipImmediately();
    }
#endif
    return true;
}

void ToolTip::setUp(const QPoint &pos, QWidget *w, const QRect &rect)
{
    m_tip->configure(pos, w);

    placeTip(pos, w);
    setTipRect(w, rect);

    if (m_hideDelayTimer.isActive())
        m_hideDelayTimer.stop();
    m_showTimer.start(m_tip->showTime());
}

bool ToolTip::tipChanged(const QPoint &pos, const QVariant &content, int typeId,
                         QWidget *w, const QString &helpId) const
{
    if (!m_tip->equals(typeId, content, helpId) || m_widget != w)
        return true;
    if (!m_rect.isNull())
        return !m_rect.contains(pos);
    return false;
}

void ToolTip::setTipRect(QWidget *w, const QRect &rect)
{
    if (!m_rect.isNull() && !w) {
        qWarning("ToolTip::show: Cannot pass null widget if rect is set");
    } else {
        m_widget = w;
        m_rect = rect;
    }
}

bool ToolTip::isVisible()
{
    ToolTip *t = instance();
    return t->m_tip && t->m_tip->isVisible();
}

QPoint ToolTip::offsetFromPosition()
{
    return QPoint(2, 16);
}

void ToolTip::showTip()
{
    m_tip->show();
}

void ToolTip::hide()
{
    instance()->hideTipWithDelay();
}

void ToolTip::hideImmediately()
{
    instance()->hideTipImmediately();
}

void ToolTip::hideTipWithDelay()
{
    if (!m_hideDelayTimer.isActive())
        m_hideDelayTimer.start(300);
}

void ToolTip::hideTipImmediately()
{
    if (m_tip) {
        m_tip->close();
        m_tip->deleteLater();
        m_tip = nullptr;
    }
    m_showTimer.stop();
    m_hideDelayTimer.stop();

    qApp->removeEventFilter(this);
    emit hidden();
}

void ToolTip::showInternal(const QPoint &pos, const QVariant &content,
                           int typeId, QWidget *w, const QString &helpId, const QRect &rect)
{
    if (acceptShow(content, typeId, pos, w, helpId, rect)) {
        QWidget *target = w;

        switch (typeId) {
            case ColorContent:
                m_tip = new ColorTip(target);
                break;
            case TextContent:
                m_tip = new TextTip(target);
                break;
            case WidgetContent:
                m_tip = new WidgetTip(target);
                break;
        }
        m_tip->setContent(content);
        m_tip->setHelpId(helpId);
        setUp(pos, w, rect);
        qApp->installEventFilter(this);
        showTip();
    }
    emit shown();
}

void ToolTip::placeTip(const QPoint &pos, QWidget *w)
{
    QRect screen = screenGeometry(pos, w);
    QPoint p = pos;
    p += offsetFromPosition();
    if (p.x() + m_tip->width() > screen.x() + screen.width())
        p.rx() -= 4 + m_tip->width();
    if (p.y() + m_tip->height() > screen.y() + screen.height())
        p.ry() -= 24 + m_tip->height();
    if (p.y() < screen.y())
        p.setY(screen.y());
    if (p.x() + m_tip->width() > screen.x() + screen.width())
        p.setX(screen.x() + screen.width() - m_tip->width());
    if (p.x() < screen.x())
        p.setX(screen.x());
    if (p.y() + m_tip->height() > screen.y() + screen.height())
        p.setY(screen.y() + screen.height() - m_tip->height());

    m_tip->move(p);
}

bool ToolTip::eventFilter(QObject *o, QEvent *event)
{
    if (m_tip && event->type() == QEvent::ApplicationStateChange
            && QGuiApplication::applicationState() != Qt::ApplicationActive) {
        hideTipImmediately();
    }

    if (!o->isWidgetType())
        return false;

    switch (event->type()) {
    case QEvent::KeyPress:
    case QEvent::KeyRelease: {
        int key = static_cast<QKeyEvent *>(event)->key();
        if (key == Qt::Key_Escape)
            hideTipImmediately();
#ifdef Q_OS_MAC
        Qt::KeyboardModifiers mody = static_cast<QKeyEvent *>(event)->modifiers();
        if (!(mody & Qt::KeyboardModifierMask)
            && key != Qt::Key_Shift && key != Qt::Key_Control
            && key != Qt::Key_Alt && key != Qt::Key_Meta)
            hideTipWithDelay();
#endif
        break;
    }
    case QEvent::Leave:
        if (o == m_tip && !m_tip->isAncestorOf(QApplication::focusWidget()))
            hideTipWithDelay();
        break;
    case QEvent::Enter:
        // User moved cursor into tip and wants to interact.
        if (m_tip && m_tip->isInteractive() && o == m_tip) {
            if (m_hideDelayTimer.isActive())
                m_hideDelayTimer.stop();
        }
        break;
    case QEvent::WindowActivate:
    case QEvent::WindowDeactivate:
    case QEvent::FocusOut:
    case QEvent::FocusIn:
        if (m_tip && !m_tip->isInteractive()) // Windows: A sequence of those occurs when interacting
            hideTipImmediately();
        break;
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::Wheel:
        if (m_tip) {
            if (m_tip->isInteractive()) { // Do not close on interaction with the tooltip
                if (o != m_tip && !m_tip->isAncestorOf(static_cast<QWidget *>(o)))
                    hideTipImmediately();
            } else {
                hideTipImmediately();
            }
        }
        break;
    case QEvent::MouseMove:
        if (o == m_widget &&
            !m_rect.isNull() &&
            !m_rect.contains(static_cast<QMouseEvent*>(event)->pos())) {
            hideTipWithDelay();
        }
        break;
    default:
        break;
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////
/// QTipLabel
////////////////////////////////////////////////////////////////////////////////


QTipLabel::QTipLabel(QWidget *parent) :
    QLabel(parent, Qt::ToolTip | Qt::BypassGraphicsProxyWidget)
{

}

void QTipLabel::setHelpId(const QString &id)
{
    m_helpId = id;
    update();
}

QString QTipLabel::helpId() const
{
    return m_helpId;
}

////////////////////////////////////////////////////////////////////////////////
/// ColorTip
////////////////////////////////////////////////////////////////////////////////

ColorTip::ColorTip(QWidget *parent)
    : QTipLabel(parent)
{
    resize(40, 40);
}

void ColorTip::setContent(const QVariant &content)
{
    m_color = content.value<QColor>();

    const int size = 10;
    m_tilePixmap = QPixmap(size * 2, size * 2);
    m_tilePixmap.fill(Qt::white);
    QPainter tilePainter(&m_tilePixmap);
    QColor col(220, 220, 220);
    tilePainter.fillRect(0, 0, size, size, col);
    tilePainter.fillRect(size, size, size, size, col);
}

void ColorTip::configure(const QPoint &pos, QWidget *w)
{
    Q_UNUSED(pos)
    Q_UNUSED(w)

    update();
}

bool ColorTip::canHandleContentReplacement(int typeId) const
{
    return typeId == ToolTip::ColorContent;
}

bool ColorTip::equals(int typeId, const QVariant &other, const QString &otherHelpId) const
{
    return typeId == ToolTip::ColorContent && otherHelpId == helpId() && other == m_color;
}

void ColorTip::paintEvent(QPaintEvent *event)
{
    QTipLabel::paintEvent(event);

    QPainter painter(this);
    painter.setBrush(m_color);
    painter.drawTiledPixmap(rect(), m_tilePixmap);

    QPen pen;
    pen.setColor(m_color.value() > 100 ? m_color.darker() : m_color.lighter());
    pen.setJoinStyle(Qt::MiterJoin);
    const QRectF borderRect = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    painter.setPen(pen);
    painter.drawRect(borderRect);
}

////////////////////////////////////////////////////////////////////////////////
/// TextTip
////////////////////////////////////////////////////////////////////////////////

TextTip::TextTip(QWidget *parent) : QTipLabel(parent)
{
    setForegroundRole(QPalette::ToolTipText);
    setBackgroundRole(QPalette::ToolTipBase);
    ensurePolished();
    setMargin(1 + style()->pixelMetric(QStyle::PM_ToolTipLabelFrameWidth,
                                       nullptr, this));
    setFrameStyle(QFrame::NoFrame);
    setAlignment(Qt::AlignLeft);
    setIndent(1);
    setWindowOpacity(style()->styleHint(QStyle::SH_ToolTipLabel_Opacity,
                                        nullptr, this) / 255.0);
}

static bool likelyContainsLink(const QString &s)
{
    return s.contains(QLatin1String("href"), Qt::CaseInsensitive);
}

void TextTip::setContent(const QVariant &content)
{
    m_text = content.toString();
    bool containsLink = likelyContainsLink(m_text);
    setOpenExternalLinks(containsLink);
}

bool TextTip::isInteractive() const
{
    return likelyContainsLink(m_text);
}

void TextTip::configure(const QPoint &pos, QWidget *w)
{
    if (helpId().isEmpty())
        setText(m_text);
    else
        setText(QString::fromLatin1("<table><tr><td valign=middle>%1</td><td>&nbsp;&nbsp;"
                                    "<img src=\":/icons/info-icon.svg\"></td>"
                                    "</tr></table>").arg(m_text));

    // Make it look good with the default ToolTip font on Mac, which has a small descent.
    QFontMetrics fm(font());
    int extraHeight = 0;
    if (fm.descent() == 2 && fm.ascent() >= 11)
        ++extraHeight;

    // Try to find a nice width without unnecessary wrapping.
    setWordWrap(false);
    int tipWidth = sizeHint().width();
    const int screenWidth = screenGeometry(pos, w).width();
    const int maxDesiredWidth = int(screenWidth * .5);
    if (tipWidth > maxDesiredWidth) {
        setWordWrap(true);
        tipWidth = maxDesiredWidth;
    }

    resize(tipWidth, heightForWidth(tipWidth) + extraHeight);
}

bool TextTip::canHandleContentReplacement(int typeId) const
{
    return typeId == ToolTip::TextContent;
}

int TextTip::showTime() const
{
    return 10000 + 40 * qMax(0, m_text.size() - 100);
}

bool TextTip::equals(int typeId, const QVariant &other, const QString &otherHelpId) const
{
    return typeId == ToolTip::TextContent && otherHelpId == helpId() && other.toString() == m_text;
}

void TextTip::paintEvent(QPaintEvent *event)
{
    QStylePainter p(this);
    QStyleOptionFrame opt;
    opt.init(this);
    p.drawPrimitive(QStyle::PE_PanelTipLabel, opt);
    p.end();

    QLabel::paintEvent(event);
}

void TextTip::resizeEvent(QResizeEvent *event)
{
    QStyleHintReturnMask frameMask;
    QStyleOption option;
    option.init(this);
    if (style()->styleHint(QStyle::SH_ToolTip_Mask, &option, this, &frameMask))
        setMask(frameMask.region);

    QLabel::resizeEvent(event);
}

////////////////////////////////////////////////////////////////////////////////
/// WidgetTip
////////////////////////////////////////////////////////////////////////////////

WidgetTip::WidgetTip(QWidget *parent) :
    QTipLabel(parent), m_layout(new QVBoxLayout)
{
    m_layout->setContentsMargins(0, 0, 0, 0);
    setLayout(m_layout);
}

void WidgetTip::setContent(const QVariant &content)
{
    m_widget = content.value<QWidget *>();
}

void WidgetTip::configure(const QPoint &pos, QWidget *)
{
    Q_ASSERT(m_widget && m_layout->count() == 0);

    move(pos);
    m_layout->addWidget(m_widget);
    m_layout->setSizeConstraint(QLayout::SetFixedSize);
    adjustSize();
}

void WidgetTip::pinToolTipWidget(QWidget *parent)
{
    Q_ASSERT(m_layout->count());

    // Pin the content widget: Rip the widget out of the layout
    // and re-show as a tooltip, with delete on close.
    const QPoint screenPos = mapToGlobal(QPoint(0, 0));
    // Remove widget from layout
    if (!m_layout->count())
        return;

    QLayoutItem *item = m_layout->takeAt(0);
    QWidget *widget = item->widget();
    delete item;
    if (!widget)
        return;

    widget->setParent(parent, Qt::Tool|Qt::FramelessWindowHint);
    widget->move(screenPos);
    widget->show();
    widget->setAttribute(Qt::WA_DeleteOnClose);
}

bool WidgetTip::canHandleContentReplacement(int typeId) const
{
    // Always create a new widget.
    Q_UNUSED(typeId);
    return false;
}

bool WidgetTip::equals(int typeId, const QVariant &other,
                       const QString &otherHelpId) const
{
    return typeId == ToolTip::WidgetContent && otherHelpId == helpId()
            && other.value<QWidget *>() == m_widget;
}

////////////////////////////////////////////////////////////////////////////////
/// FakeToolTip
////////////////////////////////////////////////////////////////////////////////

FakeToolTip::FakeToolTip(QWidget *parent) :
    QWidget(parent, Qt::ToolTip | Qt::WindowStaysOnTopHint)
{
    setFocusPolicy(Qt::NoFocus);
    setAttribute(Qt::WA_DeleteOnClose);

    // Set the window and button text to the tooltip text color, since this
    // widget draws the background as a tooltip.
    QPalette p = palette();
    const QColor toolTipTextColor = p.color(QPalette::Inactive, QPalette::ToolTipText);
    p.setColor(QPalette::Inactive, QPalette::WindowText, toolTipTextColor);
    p.setColor(QPalette::Inactive, QPalette::ButtonText, toolTipTextColor);
    setPalette(p);

    const int margin = 1 + style()->pixelMetric(QStyle::PM_ToolTipLabelFrameWidth,
                                                nullptr, this);
    setContentsMargins(margin + 1, margin, margin, margin);
    setWindowOpacity(style()->styleHint(QStyle::SH_ToolTipLabel_Opacity,
                                        nullptr, this) / 255.0);
}

void FakeToolTip::paintEvent(QPaintEvent *)
{
    QStylePainter p(this);
    QStyleOptionFrame opt;
    opt.init(this);
    p.drawPrimitive(QStyle::PE_PanelTipLabel, opt);
    p.end();
}

void FakeToolTip::resizeEvent(QResizeEvent *)
{
    QStyleHintReturnMask frameMask;
    QStyleOption option;
    option.init(this);
    if (style()->styleHint(QStyle::SH_ToolTip_Mask, &option, this, &frameMask))
        setMask(frameMask.region);
}
