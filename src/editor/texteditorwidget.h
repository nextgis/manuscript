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

#ifndef MANUSCRIPT_TEXTEDITORWIDGET_H
#define MANUSCRIPT_TEXTEDITORWIDGET_H

#include "highlightscrollbarcontroller.h"

#include <QPlainTextEdit>
#include <QTextBlock>
#include <QTimer>

/**
 * @brief The MSLink class
 */
class MSLink
{
public:
    enum Type {
        Unknown,
        Ref,
        Numref,
        Term,
        Link,
        LocalLink
    };
public:
    MSLink(int begin = -1, int end = -1);
    MSLink(const QString &link, enum Type type, int begin = -1, int end = -1);
    int begin() const;
    int end() const;
    bool isValid() const;
    bool operator ==(const MSLink &other);
    enum Type type() const;
    QString link() const;

private:
    int m_begin, m_end;
    QString m_link;
    enum Type m_type;
};

class MSTextEditorWidget;
class TextEditExtraArea : public QWidget
{
public:
    TextEditExtraArea(MSTextEditorWidget *edit);
    virtual ~TextEditExtraArea() = default;

protected:
    QSize sizeHint() const;
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void leaveEvent(QEvent *event);
//    void contextMenuEvent(QContextMenuEvent *event) {
//        m_textEdit->extraAreaContextMenuEvent(event);
//    }

    void wheelEvent(QWheelEvent *event);

private:
    MSTextEditorWidget *m_textEdit;
};

struct ExtraAreaPaintEventData;

/**
 * @brief The MSTextEditorWidget class
 */
class MSTextEditorWidget : public QPlainTextEdit
{
    friend class TextEditExtraArea;
    friend struct ExtraAreaPaintEventData;
    Q_OBJECT
public:
    enum TextFormat {
        Bold = 1,
        Italic = 2
    };
    enum NoteType {
        Note = 1,
        Tip,
        Warning
    };
public:
    MSTextEditorWidget(QWidget *parent = nullptr);
    virtual ~MSTextEditorWidget() override;
    void scrollToLine(int line);
    void formatText(enum TextFormat format);
    void addNote(enum NoteType format);
    void addHeader(int level);

    // QWidget interface
    void setFilePath(const QString &filePath);
    QString filePath() const;
    bool hasSelection() const;
    int line() const;

protected:
    virtual void mouseMoveEvent(QMouseEvent *event) override;
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;

protected:
    void requestUpdateLink(QMouseEvent *event, bool immediate = false);
    void showLink(const MSLink &link);
    void clearLink();
    void updateLink();
    MSLink findLinkAt(const QTextCursor &cursor);
    bool openLink(const MSLink &link);
    void paintRightMarginLine(QPaintEvent *event, QPainter &painter) const;
    int lineNumberDigits() const;
    int extraAreaWidth(int *markWidthPtr = nullptr) const;
    void extraAreaPaintEvent(QPaintEvent *event);
    void extraAreaLeaveEvent(QEvent *event);
    void extraAreaMouseEvent(QMouseEvent *event);
    void paintLineNumbers(QPainter &painter, const ExtraAreaPaintEventData &data,
                          const QRectF &blockBoundingRect) const;
    void paintTextMarks(QPainter &painter, const ExtraAreaPaintEventData &data,
                        const QRectF &blockBoundingRect) const;
    void adjustScrollBarRanges();
    void updateCurrentLineInScrollbar();
    void updateHighlightScrollBarNow();
    void scheduleUpdateHighlightScrollBar();

private:
    MSLink m_currentLink;
    QTextCharFormat m_linkFormat, m_currentLineNumberFormat;
    QColor m_marginColor, m_lineNumberColor;
    QTextCursor m_pendingLinkUpdate, m_lastLinkUpdate;
    bool m_linkPressed;
    QString m_filePath;
    int m_visibleWrapColumn;
    bool m_lineNumbersVisible, m_marksVisible;
    TextEditExtraArea *m_extraArea;
    int m_extraAreaPreviousMarkTooltipRequestedLine;
    QTimer m_scrollBarUpdateTimer;
    HighlightScrollBarController *m_highlightScrollBarController;
    bool m_scrollBarUpdateScheduled;

    // QWidget interface
protected:
    virtual void paintEvent(QPaintEvent *event) override;
    virtual void resizeEvent(QResizeEvent *event) override;
    virtual void changeEvent(QEvent *event) override;    
    virtual void keyPressEvent(QKeyEvent *event) override;

signals:
    void cursorPositionChanged(int pos, int col, int chars, int lines);

protected slots:
    void onCursorPositionChanged();
    void onUpdateExtraAreaWidth();
    void onUpdateRequest(const QRect &r, int dy);

};

struct ExtraAreaPaintEventData
{
    ExtraAreaPaintEventData(const MSTextEditorWidget *editor)
        : doc(editor->document())
        , selectionStart(editor->textCursor().selectionStart())
        , selectionEnd(editor->textCursor().selectionEnd())
        , fontMetrics(editor->m_extraArea->font())
        , lineSpacing(fontMetrics.lineSpacing())
        , markWidth(editor->m_marksVisible ? lineSpacing : 0)
        , extraAreaWidth(editor->m_extraArea->width())
        , palette(editor->m_extraArea->palette())
    {
        palette.setCurrentColorGroup(QPalette::Active);
    }
    QTextBlock block;
    const QTextDocument *doc;
    const int selectionStart;
    const int selectionEnd;
    const QFontMetrics fontMetrics;
    const int lineSpacing;
    const int markWidth;
    const int extraAreaWidth;
    QPalette palette;
};

#endif // MANUSCRIPT_TEXTEDITORWIDGET_H
