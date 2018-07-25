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

#ifndef MANUSCRIPT_HIGHLIGHTSCROLLBARCONTROLLER_H
#define MANUSCRIPT_HIGHLIGHTSCROLLBARCONTROLLER_H

#include "framework/style.h"

#include <QAbstractScrollArea>
#include <QPointer>
#include <QScrollBar>


struct Highlight
{
    enum Priority {
        Invalid = -1,
        LowPriority = 0,
        NormalPriority = 1,
        HighPriority = 2,
        HighestPriority = 3
    };

    enum Category {
        CurrentLine,
    };

    Highlight(int category, int position, NGTheme::Color color, Priority priority);
    Highlight() = default;

    int category;
    int position = -1;
    NGTheme::Color color = NGTheme::TextColorNormal;
    Priority priority = Invalid;
};

class HighlightScrollBarController;

class HighlightScrollBarOverlay : public QWidget
{
public:
    HighlightScrollBarOverlay(HighlightScrollBarController *scrollBarController);
    void doResize();
    void doMove();
    void scheduleUpdate();

protected:
    void paintEvent(QPaintEvent *paintEvent) override;
    bool eventFilter(QObject *object, QEvent *event) override;

private:
    void updateCache();
    QRect overlayRect() const;

    bool m_cacheUpdateScheduled = true;
    QMap<int, Highlight> m_cache;

    QScrollBar *m_scrollBar;
    HighlightScrollBarController *m_highlightController;
};

class HighlightScrollBarController
{
public:
    HighlightScrollBarController() = default;
    ~HighlightScrollBarController();

    QScrollBar *scrollBar() const;
    QAbstractScrollArea *scrollArea() const;
    void setScrollArea(QAbstractScrollArea *scrollArea);

    float visibleRange() const;
    void setVisibleRange(float visibleRange);

    float rangeOffset() const;
    void setRangeOffset(float offset);

    QHash<int, QVector<Highlight>> highlights() const;
    void addHighlight(Highlight highlight);

    void removeHighlights(int id);
    void removeAllHighlights();

private:
    QHash<int, QVector<Highlight> > m_highlights;
    float m_visibleRange = 0.0;
    float m_rangeOffset = 0.0;
    QAbstractScrollArea *m_scrollArea = nullptr;
    QPointer<HighlightScrollBarOverlay> m_overlay;
};

#endif // MANUSCRIPT_HIGHLIGHTSCROLLBARCONTROLLER_H
