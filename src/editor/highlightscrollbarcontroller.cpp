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

#include "highlightscrollbarcontroller.h"

#include <QEvent>
#include <QMap>
#include <QPainter>
#include <QStyleOptionToolBar>
#include <QTimer>
#include <QWidget>

#include "framework/application.h"

HighlightScrollBarController::~HighlightScrollBarController()
{
    if (m_overlay)
        delete m_overlay;
}

QScrollBar *HighlightScrollBarController::scrollBar() const
{
    if (m_scrollArea)
        return m_scrollArea->verticalScrollBar();

    return nullptr;
}

QAbstractScrollArea *HighlightScrollBarController::scrollArea() const
{
    return m_scrollArea;
}

void HighlightScrollBarController::setScrollArea(QAbstractScrollArea *scrollArea)
{
    if (m_scrollArea == scrollArea) {
        return;
    }

    if (m_overlay) {
        delete m_overlay;
        m_overlay = nullptr;
    }

    m_scrollArea = scrollArea;

    if (m_scrollArea) {
        m_overlay = new HighlightScrollBarOverlay(this);
        m_overlay->scheduleUpdate();
    }
}

float HighlightScrollBarController::visibleRange() const
{
    return m_visibleRange;
}

void HighlightScrollBarController::setVisibleRange(float visibleRange)
{
    m_visibleRange = visibleRange;
}

float HighlightScrollBarController::rangeOffset() const
{
    return m_rangeOffset;
}

void HighlightScrollBarController::setRangeOffset(float offset)
{
    m_rangeOffset = offset;
}

QHash<int, QVector<Highlight>> HighlightScrollBarController::highlights() const
{
    return m_highlights;
}

void HighlightScrollBarController::addHighlight(Highlight highlight)
{
    if (!m_overlay)
        return;

    m_highlights[highlight.category] << highlight;
    m_overlay->scheduleUpdate();
}

void HighlightScrollBarController::removeHighlights(int category)
{
    if (!m_overlay)
        return;

    m_highlights.remove(category);
    m_overlay->scheduleUpdate();
}

void HighlightScrollBarController::removeAllHighlights()
{
    if (!m_overlay)
        return;

    m_highlights.clear();
    m_overlay->scheduleUpdate();
}


////////////////////////////////////////////////////////////////////////////////
/// HighlightScrollBarOverlay
////////////////////////////////////////////////////////////////////////////////
HighlightScrollBarOverlay::HighlightScrollBarOverlay(
        HighlightScrollBarController *scrollBarController) :
    QWidget(scrollBarController->scrollArea()),
    m_scrollBar(scrollBarController->scrollBar()),
    m_highlightController(scrollBarController)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    m_scrollBar->parentWidget()->installEventFilter(this);
    doResize();
    doMove();
    show();
}

void HighlightScrollBarOverlay::doResize()
{
    resize(m_scrollBar->size());
}

void HighlightScrollBarOverlay::doMove()
{
    move(parentWidget()->mapFromGlobal(m_scrollBar->mapToGlobal(m_scrollBar->pos())));
}

QRect HighlightScrollBarOverlay::overlayRect() const
{
    QStyleOptionSlider opt = qt_qscrollbarStyleOption(m_scrollBar);
    return m_scrollBar->style()->subControlRect(QStyle::CC_ScrollBar, &opt, QStyle::SC_ScrollBarGroove, m_scrollBar);
}

void HighlightScrollBarOverlay::scheduleUpdate()
{
    if (m_cacheUpdateScheduled)
        return;

    m_cacheUpdateScheduled = true;
    QTimer::singleShot(0, this, static_cast<void (QWidget::*)()>(&QWidget::update));
}

void HighlightScrollBarOverlay::paintEvent(QPaintEvent *paintEvent)
{
    QWidget::paintEvent(paintEvent);

    updateCache();

    if (m_cache.isEmpty())
        return;

    const QRect &rect = overlayRect();

    NGTheme::Color previousColor = NGTheme::TextColorNormal;
    Highlight::Priority previousPriority = Highlight::LowPriority;
    QRect *previousRect = nullptr;

    const int scrollbarRange = m_scrollBar->maximum() + m_scrollBar->pageStep();
    const int range = qMax(m_highlightController->visibleRange(), float(scrollbarRange));
    const int horizontalMargin = 3;
    const int resultWidth = rect.width() - 2 * horizontalMargin + 1;
    const int resultHeight = 4;
//            qMin(int(rect.height() / range) + 1, 4);
//    if(resultHeight < 4)
//        resultHeight = 4;
    const int offset = rect.height() / range * m_highlightController->rangeOffset();
    const int verticalMargin = ((rect.height() / range) - resultHeight) / 2;
    int previousBottom = -1;

    QHash<NGTheme::Color, QVector<QRect> > highlights;
    for (const Highlight &currentHighlight : m_cache.values()) {
        // Calculate top and bottom
        int top = rect.top() + offset + verticalMargin
                + float(currentHighlight.position) / range * rect.height();
        const int bottom = top + resultHeight;

        if (previousRect && previousColor == currentHighlight.color && previousBottom + 1 >= top) {
            // If the previous highlight has the same color and is directly prior to this highlight
            // we just extend the previous highlight.
            previousRect->setBottom(bottom - 1);

        } else { // create a new highlight
            if (previousRect && previousBottom >= top) {
                // make sure that highlights with higher priority are drawn on top of other highlights
                // when rectangles are overlapping

                if (previousPriority > currentHighlight.priority) {
                    // Moving the top of the current highlight when the previous
                    // highlight has a higher priority
                    top = previousBottom + 1;
                    if (top == bottom) // if this would result in an empty highlight just skip
                        continue;
                } else {
                    previousRect->setBottom(top - 1); // move the end of the last highlight
                    if (previousRect->height() == 0) // if the result is an empty rect, remove it.
                        highlights[previousColor].removeLast();
                }
            }
            highlights[currentHighlight.color] << QRect(rect.left() + horizontalMargin, top,
                                                        resultWidth, bottom - top);
            previousRect = &highlights[currentHighlight.color].last();
            previousColor = currentHighlight.color;
            previousPriority = currentHighlight.priority;
        }
        previousBottom = previousRect->bottom();
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);
    const auto highlightEnd = highlights.cend();
    for (auto highlightIt = highlights.cbegin(); highlightIt != highlightEnd; ++highlightIt) {
        const NGTheme *theme = NGGUIApplication::theme();
        const QColor &color = theme->color(highlightIt.key());
        for (const QRect &rect : highlightIt.value())
            painter.fillRect(rect, color);
    }
}

bool HighlightScrollBarOverlay::eventFilter(QObject *object, QEvent *event)
{
    switch (event->type()) {
    case QEvent::Move:
        doMove();
        break;
    case QEvent::Resize:
        doResize();
        break;
    case QEvent::ZOrderChange:
        raise();
        break;
    default:
        break;
    }
    return QWidget::eventFilter(object, event);
}

void HighlightScrollBarOverlay::updateCache()
{
    if (!m_cacheUpdateScheduled)
        return;

    m_cache.clear();
    const QHash<int, QVector<Highlight>> highlights = m_highlightController->highlights();
    const QList<int> &categories = highlights.keys();
    for (const int &category : categories) {
        for (const Highlight &highlight : highlights.value(category)) {
            const Highlight oldHighlight = m_cache.value(highlight.position);
            if (highlight.priority > oldHighlight.priority)
                m_cache[highlight.position] = highlight;
        }
    }
    m_cacheUpdateScheduled = false;
}

/////////////

Highlight::Highlight(int category_, int position_,
                     NGTheme::Color color_, Highlight::Priority priority_)
    : category(category_)
    , position(position_)
    , color(color_)
    , priority(priority_)
{
}

/////////////
