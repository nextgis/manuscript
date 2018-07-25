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

#include "mainwindow.h"
#include "rsthighlighter.h"
#include "texteditorwidget.h"
#include "tooltip.h"

#include "framework/application.h"

#include <QDir>
#include <QDesktopServices>
#include <QFileInfo>
#include <QSettings>
#include <QTextDocumentFragment>
#include <QTextBlock>
#include <QTimer>
#include <QPainter>
#include <QGridLayout>

constexpr int tabSize = 3;
constexpr int indentSize = 3;

static QTextBlock nextVisibleBlock(const QTextBlock &block,
                                   const QTextDocument *doc)
{
    QTextBlock nextVisibleBlock = block.next();
    if (!nextVisibleBlock.isVisible()) {
        // invisible blocks do have zero line count
        nextVisibleBlock = doc->findBlockByLineNumber(nextVisibleBlock.firstLineNumber());
        // paranoia in case our code somewhere did not set the line count
        // of the invisible block to 0
        while (nextVisibleBlock.isValid() && !nextVisibleBlock.isVisible())
            nextVisibleBlock = nextVisibleBlock.next();
    }
    return nextVisibleBlock;
}

MSTextEditorWidget::MSTextEditorWidget(QWidget *parent) :
    QPlainTextEdit(parent),
    m_linkPressed(false),
    m_visibleWrapColumn(80),
    m_lineNumbersVisible(true),
    m_marksVisible(true),
    m_extraAreaPreviousMarkTooltipRequestedLine(-1),
    m_highlightScrollBarController(nullptr),
    m_scrollBarUpdateScheduled(false)
{
    viewport()->setMouseTracking(true);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setLayoutDirection(Qt::LeftToRight);

    QString themeName = NGGUIApplication::style();
    QString themeURI = QStringLiteral(":/highlighter/%1.theme").arg(themeName);
    QSettings themeSettings(themeURI, QSettings::IniFormat);
    QColor c = readColor(themeSettings.value("Colors/Link").toString());

    m_linkFormat.setForeground(c);
    m_linkFormat.setFontUnderline(true);

    c = readColor(themeSettings.value("Colors/LineNumberSelected").toString());
    m_currentLineNumberFormat.setForeground(c);

    m_marginColor = readColor(themeSettings.value("Colors/RightMargin").toString());
    m_lineNumberColor = readColor(themeSettings.value("Colors/LineNumber").toString());

    m_extraArea = new TextEditExtraArea(this);
    m_extraArea->setMouseTracking(true);

    connect(this, &QPlainTextEdit::cursorPositionChanged,
            this, &MSTextEditorWidget::onCursorPositionChanged);
    connect(this, &MSTextEditorWidget::cursorPositionChanged,
            MSMainWindow::instance(), &MSMainWindow::onCursorPositionChanged);
    connect(this, &QPlainTextEdit::blockCountChanged,
            this, &MSTextEditorWidget::onUpdateExtraAreaWidth);
    connect(this, &QPlainTextEdit::modificationChanged, m_extraArea,
            static_cast<void (QWidget::*)()>(&QWidget::update));
    connect(this, &QPlainTextEdit::updateRequest,
            this, &MSTextEditorWidget::onUpdateRequest);

    // Add shortcut for invoking quick fix options
//        QAction *quickFixAction = new QAction(tr("Trigger Refactoring Action"), this);
//        Command *quickFixCommand = ActionManager::registerAction(quickFixAction, Constants::QUICKFIX_THIS, context);
//        quickFixCommand->setDefaultKeySequence(QKeySequence(tr("Alt+Return")));
//        connect(quickFixAction, &QAction::triggered, []() {
//            if (BaseTextEditor *editor = BaseTextEditor::currentTextEditor())
//                editor->editorWidget()->invokeAssist(QuickFix);
//        });
//    QAction *showContextMenuAction = new QAction(tr("Show Context Menu"), this);
//        ActionManager::registerAction(showContextMenuAction,
//                                      Constants::SHOWCONTEXTMENU,
//                                      context);
//        connect(showContextMenuAction, &QAction::triggered, []() {
//            if (BaseTextEditor *editor = BaseTextEditor::currentTextEditor())
//                editor->editorWidget()->showContextMenu();
//        });


//    setCenterOnScroll(true);
    onUpdateExtraAreaWidth();

    m_highlightScrollBarController = new HighlightScrollBarController();
    m_highlightScrollBarController->setScrollArea(this);
    scheduleUpdateHighlightScrollBar();
}

MSTextEditorWidget::~MSTextEditorWidget()
{
    delete m_highlightScrollBarController;
}

void MSTextEditorWidget::scrollToLine(int line)
{
    m_extraArea->setFont(document()->defaultFont());
    scheduleUpdateHighlightScrollBar();
    QTextCursor cursor(document()->findBlockByLineNumber(line));
    moveCursor(QTextCursor::End);
    setTextCursor(cursor);
}

void MSTextEditorWidget::formatText(enum TextFormat format)
{
    QString insert;
    if(format == Bold) {
        insert = QLatin1String("**");
    }
    else if(format == Italic) {
        insert = QLatin1String("*");
    }
    QTextCursor cursor = textCursor();

    if(cursor.hasSelection()) {
        int start = cursor.selectionStart();
        int end = cursor.selectionEnd();
        cursor.setPosition(start);
        cursor.insertText(insert);
        cursor.setPosition(end + insert.length());
        cursor.insertText(insert);
        cursor.clearSelection();
        setTextCursor(cursor);
    }
    else {
        cursor.insertText(insert + insert);
    }
}

void MSTextEditorWidget::addNote(MSTextEditorWidget::NoteType format)
{
    QTextCursor cursor = textCursor();
//    cursor.insertBlock();
    QString text;
    if(format == Note) {
        text = QLatin1String("note");
    }
    else if(format == Tip) {
        text = QLatin1String("tip");
    }
    else if(format == Warning) {
        text = QLatin1String("warning");
    }
    int pos = cursor.position() + 10 + text.length();
    QString instruction = tr("<your text here>");
    QString insertText = QString("\n.. %1::\n   %2\n").arg(text).arg(instruction);
    cursor.insertText(insertText);
    cursor.setPosition(pos, QTextCursor::MoveAnchor);
    cursor.setPosition(pos + instruction.length(), QTextCursor::KeepAnchor);
    setTextCursor(cursor);
}


void MSTextEditorWidget::mouseMoveEvent(QMouseEvent *event)
{
    requestUpdateLink(event);
    QPlainTextEdit::mouseMoveEvent(event);
};


void MSTextEditorWidget::mousePressEvent(QMouseEvent *event)
{
    if(event->modifiers() & Qt::ControlModifier) {
        if(m_currentLink.isValid()) {
            m_linkPressed = true;
        }
    }
    QPlainTextEdit::mousePressEvent(event);
}

void MSTextEditorWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if(m_linkPressed) {
        if(openLink(m_currentLink)) {
            clearLink();
        }
    }
    QPlainTextEdit::mouseReleaseEvent(event);
}

void MSTextEditorWidget::requestUpdateLink(QMouseEvent *event, bool immediate)
{
    if (event->modifiers() & Qt::ControlModifier) {
        // Link emulation behaviour for 'go to definition'
        const QTextCursor cursor = cursorForPosition(event->pos());

        // Avoid updating the link we already found
        if (cursor.position() >= m_currentLink.begin()
                && cursor.position() <= m_currentLink.end()) {
            return;
        }

        // Check that the mouse was actually on the text somewhere
        bool onText = cursorRect(cursor).right() >= event->x();
        if (!onText) {
            QTextCursor nextPos = cursor;
            nextPos.movePosition(QTextCursor::Right);
            onText = cursorRect(nextPos).right() >= event->x();
        }

        if (onText) {
            m_pendingLinkUpdate = cursor;

            if (immediate) {
                updateLink();
            }
            else {
                QTimer::singleShot(0, this, &MSTextEditorWidget::updateLink);
            }
            return;
        }
    }

    clearLink();
}

void MSTextEditorWidget::showLink(const MSLink &link)
{
    if (m_currentLink == link) {
        return;
    }

    QTextEdit::ExtraSelection sel;
    sel.cursor = textCursor();
    sel.cursor.setPosition(link.begin());
    sel.cursor.setPosition(link.end(), QTextCursor::KeepAnchor);
    sel.format = m_linkFormat;
    setExtraSelections(QList<QTextEdit::ExtraSelection>() << sel);
    viewport()->setCursor(Qt::PointingHandCursor);
    m_currentLink = link;
}

void MSTextEditorWidget::clearLink()
{
    m_pendingLinkUpdate = QTextCursor();
    m_lastLinkUpdate = QTextCursor();
    if (!m_currentLink.isValid()) {
        return;
    }

    setExtraSelections(QList<QTextEdit::ExtraSelection>());
    viewport()->setCursor(Qt::IBeamCursor);
    m_currentLink = MSLink();
    m_linkPressed = false;
}

void MSTextEditorWidget::updateLink()
{
    if (m_pendingLinkUpdate.isNull()) {
        return;
    }
    if (m_pendingLinkUpdate == m_lastLinkUpdate) {
        return;
    }

    m_lastLinkUpdate = m_pendingLinkUpdate;
    const MSLink link = findLinkAt(m_pendingLinkUpdate);
    if (link.isValid()) {
        showLink(link);
    }
    else {
        clearLink();
    }
}

MSLink MSTextEditorWidget::findLinkAt(const QTextCursor &cursor)
{
    const QString blockText = cursor.block().text();
    int pos = -1;

    // Check TOC items
    MSProject &project = MSMainWindow::instance()->project();
    QString probableTocItem = blockText.trimmed();
    if(probableTocItem.indexOf(' ') == -1) {
        if(project.hasFile(probableTocItem)) {
            pos = blockText.indexOf(probableTocItem);
            return MSLink(probableTocItem, MSLink::LocalLink,
                          cursor.block().position() + pos,
                          cursor.block().position() + pos + probableTocItem.length());
        }
    }

    // Check include
    if(blockText.startsWith(".. include::")) {
        pos = 12;
        int end = blockText.length();
        QString linkText = blockText.mid(pos, end - pos);
        if(cursor.positionInBlock() >= pos && cursor.positionInBlock() <= end) {
            if(project.hasFile(probableTocItem)) {
                return MSLink(linkText, MSLink::LocalLink,
                              cursor.block().position() + pos,
                              cursor.block().position() + end);
            }
        }
    }

    // Check term
    int startSearch = 0;
    while((pos = blockText.indexOf(":term:`", startSearch)) != -1) {
        int end = blockText.indexOf('`', pos + 7);
        if(end == -1) {
//            end = blockText.length();
            break;
        }
        pos += 7;
        QString linkText = blockText.mid(pos, end - pos);
        linkText = MSProject::filterLinkText(linkText, pos, end);
        startSearch = end;

        if(cursor.positionInBlock() >= pos && cursor.positionInBlock() <= end) {
            return MSLink(linkText, MSLink::Term, cursor.block().position() + pos,
                          cursor.block().position() + end);
        }
    }

//    // Check reference
//    if((pos = blockText.indexOf(".. _")) != -1) {
//        int end = blockText.indexOf(':', pos + 4);
//        if(end == -1) {
//            end = blockText.length();
//        }
//        pos += 4;
//        QString linkText = blockText.mid(pos, end - pos);
//        if(cursor.positionInBlock() >= pos && cursor.positionInBlock() <= end) {
//            if(m_project->hasReference(linkText)) {
//                return MSLink(linkText, MSLink::Ref,
//                              cursor.block().position() + pos,
//                              cursor.block().position() + end);
//            }
//        }
//    }

    // Check reference2
    startSearch = 0;
    while((pos = blockText.indexOf(":ref:`", startSearch)) != -1) {
        int end = blockText.indexOf('`', pos + 6);
        if(end == -1) {
//            end = blockText.length();
            break;
        }
        pos += 6;
        QString linkText = blockText.mid(pos, end - pos);
        linkText = MSProject::filterLinkText(linkText, pos, end);

        startSearch = end;

        if(cursor.positionInBlock() >= pos && cursor.positionInBlock() <= end) {
            return MSLink(linkText, MSLink::Ref, cursor.block().position() + pos,
                          cursor.block().position() + end);
        }
    }


    // Check num reference
    startSearch = 0;
    while((pos = blockText.indexOf(":numref:`", startSearch)) != -1) {
        int end = blockText.indexOf('`', pos + 9);
        if(end == -1) {
//            end = blockText.length();
            break;
        }
        pos += 9;
        QString linkText = blockText.mid(pos, end - pos);
        linkText = MSProject::filterLinkText(linkText, pos, end);

        startSearch = end;

        if(cursor.positionInBlock() >= pos && cursor.positionInBlock() <= end) {
            return MSLink(linkText, MSLink::Numref, cursor.block().position() + pos,
                          cursor.block().position() + end);
        }
    }

    // Check doc
    startSearch = 0;
    while((pos = blockText.indexOf(":doc:`", startSearch)) != -1) {
        int end = blockText.indexOf('`', pos + 6);
        if(end == -1) {
//            end = blockText.length();
            break;
        }
        pos += 6;
        QString linkText = blockText.mid(pos, end - pos);
        linkText = MSProject::filterLinkText(linkText, pos, end);

        startSearch = end;

        if(cursor.positionInBlock() >= pos && cursor.positionInBlock() <= end) {
            return MSLink(linkText, MSLink::LocalLink, cursor.block().position() + pos,
                          cursor.block().position() + end);
        }
    }

    // Check external link
    startSearch = 0;
    while((pos = blockText.indexOf("http", startSearch, Qt::CaseInsensitive)) != -1) {
        int end = blockText.indexOf(' ', pos + 1);
        if(end == -1) {
            end = blockText.length();
//            break;
        }
        QString linkText = blockText.mid(pos, end - pos);
        int linkEnd = linkText.indexOf(">`_");
        if(linkEnd != -1) {
            linkText = linkText.mid(0, linkEnd);
            end = pos + linkEnd;
        }

        startSearch = end;

        if(cursor.positionInBlock() >= pos && cursor.positionInBlock() <= end) {
            if(linkText.startsWith("http://") || linkText.startsWith("https://")) {

                return MSLink(linkText, MSLink::Link,
                              cursor.block().position() + pos,
                              cursor.block().position() + end);
            }
        }
    }


    // Check unknown link
    if((pos = blockText.indexOf("<")) != -1) {
        int end = blockText.indexOf('>', pos + 1);
        if(end != -1) {
            pos += 1;
            QString linkText = blockText.mid(pos, end - pos);
            if(cursor.positionInBlock() >= pos && cursor.positionInBlock() <= end) {
                if(linkText.startsWith("http://") || linkText.startsWith("https://")) {
                    return MSLink(linkText, MSLink::Link, cursor.block().position() + pos,
                              cursor.block().position() + end);
                }
                else {
                    return MSLink(linkText, MSLink::Unknown, cursor.block().position() + pos,
                              cursor.block().position() + end);
                }
            }
        }
    }

    return MSLink();
}

bool MSTextEditorWidget::openLink(const MSLink &link)
{
    if (!link.isValid()) {
        return false;
    }

    if(link.type() == MSLink::Link) {
        return QDesktopServices::openUrl(link.link());
    }

    MSProject &project = MSMainWindow::instance()->project();

    if(link.type() == MSLink::LocalLink) {
        QFileInfo fileInfo(m_filePath);
        QString linkFilePath = project.getFileByName(
                    fileInfo.path(), link.link());
        if(!linkFilePath.isEmpty()) {
            rstItem item = {link.link(), "", linkFilePath, 0};
            MSMainWindow::instance()->navigateTo(&item);
            return true;
        }
    }

    if(link.type() == MSLink::Ref) {
        rstItem *item =project.bookmarksModel()->itemByName(link.link());
        if(item) {
            MSMainWindow::instance()->navigateTo(item);
            return true;
        }
    }

    if(link.type() == MSLink::Numref) {
        rstItem *item = project.picturesModel()->itemByName(link.link());
        if(item) {
            MSMainWindow::instance()->navigateTo(item);
            return true;
        }
    }

    if(link.type() == MSLink::Term) {
        rstItem *item = project.glossaryItem(link.link());
        if(item) {
            MSMainWindow::instance()->navigateTo(item);
            delete item;
            return true;
        }
    }

    return false;
}

void MSTextEditorWidget::setFilePath(const QString &filePath)
{
    m_filePath = filePath;
}

void MSTextEditorWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(viewport());
    paintRightMarginLine(event, painter);

    QPlainTextEdit::paintEvent(event);
}

void MSTextEditorWidget::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);
    QRect cr = rect();
    m_extraArea->setGeometry(
        QStyle::visualRect(layoutDirection(), cr,
                           QRect(cr.left() + frameWidth(), cr.top() + frameWidth(),
                                 extraAreaWidth(), cr.height() - 2 * frameWidth())));
    adjustScrollBarRanges();
    updateCurrentLineInScrollbar();
    onUpdateExtraAreaWidth();
}

void MSTextEditorWidget::changeEvent(QEvent *event)
{
    QPlainTextEdit::changeEvent(event);
    if (event->type() == QEvent::ApplicationFontChange ||
            event->type() == QEvent::FontChange) {
        if (m_extraArea) {
            QFont f = m_extraArea->font();
            f.setPointSizeF(font().pointSizeF());
            m_extraArea->setFont(f);
            onUpdateExtraAreaWidth();
            m_extraArea->update();
        }
    }
}

static int firstNonSpace(const QString &text)
{
    int i = 0;
    while (i < text.size()) {
        if (!text.at(i).isSpace())
            return i;
        ++i;
    }
    return i;
}

static QString indentationString(const QString &text)
{
    return text.left(firstNonSpace(text));
}

static int columnAt(const QString &text, int position)
{
    int column = 0;
    for (int i = 0; i < position; ++i) {
        if (text.at(i) == QLatin1Char('\t'))
            column = column - (column % tabSize) + tabSize;
        else
            ++column;
    }
    return column;
}

static int lineIndentPosition(const QString &text)
{
    int i = 0;
    while (i < text.size()) {
        if (!text.at(i).isSpace())
            break;
        ++i;
    }
    int column = columnAt(text, i);
    return i - (column % indentSize);
}

static int indentedColumn(int column, bool doIndent)
{
    int aligned = (column / indentSize) * indentSize;
    if (doIndent)
        return aligned + indentSize;
    if (aligned < column)
        return aligned;
    return qMax(0, aligned - indentSize);
}

static QString indentationString(int startColumn, int targetColumn, int padding,
                                 const QTextBlock &block)
{
    Q_UNUSED(padding)
    Q_UNUSED(block)
    targetColumn = qMax(startColumn, targetColumn);
    return QString(targetColumn - startColumn, QLatin1Char(' '));
}

static int positionAtColumn(const QString &text, int column, int *offset,
                            bool allowOverstep)
{
    int col = 0;
    int i = 0;
    int textSize = text.size();
    while ((i < textSize || allowOverstep) && col < column) {
        if (i < textSize && text.at(i) == QLatin1Char('\t'))
            col = col - (col % tabSize) + tabSize;
        else
            ++col;
        ++i;
    }
    if (offset)
        *offset = column - col;
    return i;
}

static int spacesLeftFromPosition(const QString &text, int position)
{
    if (position > text.size())
        return 0;
    int i = position;
    while (i > 0) {
        if (!text.at(i-1).isSpace())
            break;
        --i;
    }
    return position - i;
}

static QTextCursor indentOrUnindent(const QTextDocument *document,
                                    const QTextCursor &textCursor, bool doIndent,
                                    bool blockSelection, int columnIn, int *offset)
{
   QTextCursor cursor = textCursor;
   cursor.beginEditBlock();

   // Indent or unindent the selected lines
   int pos = cursor.position();
   int column = blockSelection ? columnIn
              : columnAt(cursor.block().text(), cursor.positionInBlock());
   int anchor = cursor.anchor();
   int start = qMin(anchor, pos);
   int end = qMax(anchor, pos);
   bool modified = true;

   QTextBlock startBlock = document->findBlock(start);
   QTextBlock endBlock = document->findBlock(blockSelection ? end : qMax(end - 1, 0)).next();
   const bool cursorAtBlockStart = (textCursor.position() == startBlock.position());
   const bool anchorAtBlockStart = (textCursor.anchor() == startBlock.position());
   const bool oneLinePartial = (startBlock.next() == endBlock)
                             && (start > startBlock.position() || end < endBlock.position() - 1);

   // Make sure one line selection will get processed in "for" loop
   if (startBlock == endBlock)
       endBlock = endBlock.next();

   if (cursor.hasSelection() && !blockSelection && !oneLinePartial) {
       for (QTextBlock block = startBlock; block != endBlock; block = block.next()) {
           const QString text = block.text();
           int indentPosition = lineIndentPosition(text);
           if (!doIndent && !indentPosition)
               indentPosition = firstNonSpace(text);
           int targetColumn = indentedColumn(columnAt(text, indentPosition), doIndent);
           cursor.setPosition(block.position() + indentPosition);
           cursor.insertText(indentationString(0, targetColumn, 0, block));
           cursor.setPosition(block.position());
           cursor.setPosition(block.position() + indentPosition, QTextCursor::KeepAnchor);
           cursor.removeSelectedText();
       }
       // make sure that selection that begins in first column stays at first column
       // even if we insert text at first column
       if (cursorAtBlockStart) {
           cursor = textCursor;
           cursor.setPosition(startBlock.position(), QTextCursor::KeepAnchor);
       } else if (anchorAtBlockStart) {
           cursor = textCursor;
           cursor.setPosition(startBlock.position(), QTextCursor::MoveAnchor);
           cursor.setPosition(textCursor.position(), QTextCursor::KeepAnchor);
       } else {
           modified = false;
       }
   } else if (cursor.hasSelection() && !blockSelection && oneLinePartial) {
       // Only one line partially selected.
       cursor.removeSelectedText();
   } else {
       // Indent or unindent at cursor position
       for (QTextBlock block = startBlock; block != endBlock; block = block.next()) {
           QString text = block.text();

           int blockColumn = columnAt(text, text.size());
           if (blockColumn < column) {
               cursor.setPosition(block.position() + text.size());
               cursor.insertText(indentationString(blockColumn, column, 0, block));
               text = block.text();
           }

           int indentPosition = positionAtColumn(text, column, nullptr, true);
           int spaces = spacesLeftFromPosition(text, indentPosition);
           int startColumn = columnAt(text, indentPosition - spaces);
           int targetColumn = indentedColumn(columnAt(text, indentPosition), doIndent);
           cursor.setPosition(block.position() + indentPosition);
           cursor.setPosition(block.position() + indentPosition - spaces, QTextCursor::KeepAnchor);
           cursor.removeSelectedText();
           cursor.insertText(indentationString(startColumn, targetColumn, 0, block));
       }
       // Preserve initial anchor of block selection
       if (blockSelection) {
           end = cursor.position();
           if (offset)
               *offset = columnAt(cursor.block().text(), cursor.positionInBlock()) - column;
           cursor.setPosition(start);
           cursor.setPosition(end, QTextCursor::KeepAnchor);
       }
   }

   cursor.endEditBlock();

   return modified ? cursor : textCursor;
}

void MSTextEditorWidget::keyPressEvent(QKeyEvent *event)
{
    if (event == QKeySequence::InsertParagraphSeparator
            || event == QKeySequence::InsertLineSeparator) {
        QTextCursor cursor = textCursor();
        if(cursor.hasSelection()) {
            return QPlainTextEdit::keyPressEvent(event);
//            cursor.clearSelection();
//            setTextCursor(cursor);
        }
        else {
            cursor.beginEditBlock();
            cursor.insertBlock();
            const QString &previousBlockText = cursor.block().previous().text();
            QString previousIndentationString = indentationString(previousBlockText);
            if (!previousIndentationString.isEmpty()) {
                cursor.insertText(previousIndentationString);
            }
            cursor.endEditBlock();
        }

        event->accept();
        return;
    }
    else switch (event->key()) {
    case Qt::Key_Tab: {
        QTextCursor cursor = textCursor();
        QTextCursor newCursor = indentOrUnindent(document(), cursor, true,
                                                 false, 0, nullptr);
        setTextCursor(newCursor);
        event->accept();
        return;
    }
    case Qt::Key_Backtab: {
        QTextCursor cursor = textCursor();
        QTextCursor newCursor = indentOrUnindent(document(), cursor, false,
                                                 false, 0, nullptr);
        setTextCursor(newCursor);
        event->accept();
        return;
    }
    }

    QPlainTextEdit::keyPressEvent(event);
}

void MSTextEditorWidget::onCursorPositionChanged()
{
    const QTextCursor cursor = textCursor();
    const QTextBlock block = cursor.block();
    const int pos = block.blockNumber() + 1;
    const int col = cursor.position() - block.position();

    QString str = cursor.selection().toPlainText();
    int selectedChars = 0;
    int selectedLines = 0;
    if(!str.isEmpty()) {
        selectedChars = str.count();
        selectedLines = str.count("\n") + 1;
    }

    emit cursorPositionChanged(pos, col, selectedChars, selectedLines);

    MSMainWindow::instance()->updateSelections(m_filePath, block.blockNumber());

    ensureCursorVisible();
}

bool MSTextEditorWidget::hasSelection() const
{
    const QTextCursor cursor = textCursor();
    QString str = cursor.selection().toPlainText();
    return !str.isEmpty();
}

int MSTextEditorWidget::line() const
{
    const QTextCursor cursor = textCursor();
    const QTextBlock block = cursor.block();
    return  block.blockNumber();
}

void MSTextEditorWidget::paintRightMarginLine(QPaintEvent *event, QPainter &painter) const
{
    const qreal rightMargin = QFontMetricsF(document()->defaultFont()).width(
                QLatin1Char('X')) * m_visibleWrapColumn;
    QRect vpRect = viewport()->rect();
    if (rightMargin <= 0 || vpRect.width() <= rightMargin) {
        return;
    }

    const QPen pen = painter.pen();
    painter.setPen(m_marginColor);
    painter.drawLine(QPointF(rightMargin, event->rect().top()),
                     QPointF(rightMargin, event->rect().bottom()));
    painter.setPen(pen);
}

int MSTextEditorWidget::lineNumberDigits() const
{
    int digits = 2;
    int max = qMax(1, blockCount());
    while (max >= 100) {
        max /= 10;
        ++digits;
    }
    return digits;
}

int MSTextEditorWidget::extraAreaWidth(int *markWidthPtr) const
{
    if (!m_marksVisible && !m_lineNumbersVisible) {
        return 0;
    }

    int space = 0;
    const QFontMetrics fm(m_extraArea->fontMetrics());

    if (m_lineNumbersVisible) {
        QFont fnt = m_extraArea->font();
        // this works under the assumption that bold or italic
        // can only make a font wider
        const QTextCharFormat &currentLineNumberFormat
                = m_currentLineNumberFormat;
        fnt.setBold(currentLineNumberFormat.font().bold());
        fnt.setItalic(currentLineNumberFormat.font().italic());
        const QFontMetrics linefm(fnt);

        space += linefm.width(QLatin1Char('9')) * lineNumberDigits();
    }
    int markWidth = 0;

    if (m_marksVisible) {
        markWidth += fm.lineSpacing() + 2;
        space += markWidth;
    } else {
        space += 2;
    }

    if (markWidthPtr)
        *markWidthPtr = markWidth;

    space += 4;

    return space;
}

void MSTextEditorWidget::paintLineNumbers(QPainter &painter,
                                          const ExtraAreaPaintEventData &data,
                                          const QRectF &blockBoundingRect) const
{
    if (!m_lineNumbersVisible) {
        return;
    }

    const QString &number = QString::number(data.block.blockNumber() + 1);
    const bool selected = (
                (data.selectionStart < data.block.position() + data.block.length() &&
                 data.selectionEnd > data.block.position()) ||
                (data.selectionStart == data.selectionEnd &&
                 data.selectionEnd == data.block.position()) );

    if (selected) {
        painter.save();
        QFont f = painter.font();
        f.setBold(m_currentLineNumberFormat.font().bold());
        f.setItalic(m_currentLineNumberFormat.font().italic());
        painter.setFont(f);
        painter.setPen(m_currentLineNumberFormat.foreground().color());
        if (m_currentLineNumberFormat.background() != Qt::NoBrush) {
            painter.fillRect(QRectF(0, blockBoundingRect.top(),
                                   data.extraAreaWidth, blockBoundingRect.height()),
                             m_currentLineNumberFormat.background().color());
        }
    }
    painter.drawText(QRectF(data.markWidth, blockBoundingRect.top(),
                            data.extraAreaWidth - data.markWidth - 4,
                            blockBoundingRect.height()),
                     Qt::AlignRight,
                     number);
    if (selected)
        painter.restore();
}

void MSTextEditorWidget::extraAreaPaintEvent(QPaintEvent *event)
{
    ExtraAreaPaintEventData data(this);
    QPainter painter(m_extraArea);

    painter.fillRect(event->rect(), m_marginColor);

    data.block = firstVisibleBlock();
    QPointF offset = contentOffset();
    QRectF boundingRect = blockBoundingRect(data.block).translated(offset);

    while (data.block.isValid() && boundingRect.top() <= event->rect().bottom()) {
        if (boundingRect.bottom() >= event->rect().top()) {

            painter.setPen(m_lineNumberColor);

            paintLineNumbers(painter, data, boundingRect);

            if (m_marksVisible) {
                painter.save();
                painter.setRenderHint(QPainter::Antialiasing, false);

                paintTextMarks(painter, data, boundingRect);

                painter.restore();
            }
        }

        offset.ry() += boundingRect.height();
        data.block = nextVisibleBlock(data.block, document());
        boundingRect = blockBoundingRect(data.block).translated(offset);
    }
}

void MSTextEditorWidget::onUpdateExtraAreaWidth()
{
    if (isLeftToRight())
        setViewportMargins(extraAreaWidth(), 0, 0, 0);
    else
        setViewportMargins(0, 0, extraAreaWidth(), 0);
}

void MSTextEditorWidget::extraAreaLeaveEvent(QEvent *)
{
    m_extraAreaPreviousMarkTooltipRequestedLine = -1;
    ToolTip::hide();

    // fake missing mouse move event from Qt
    QMouseEvent me(QEvent::MouseMove, QPoint(-1, -1), Qt::NoButton,
                   nullptr, nullptr);
    extraAreaMouseEvent(&me);
}


void MSTextEditorWidget::extraAreaMouseEvent(QMouseEvent *e)
{
    QTextCursor cursor = cursorForPosition(QPoint(0, e->pos().y()));

    int markWidth;
    extraAreaWidth(&markWidth);
    const bool inMarkArea = e->pos().x() <= markWidth && e->pos().x() >= 0;

    // Set whether the mouse cursor is a hand or normal arrow
    if (e->type() == QEvent::MouseMove) {
        if (inMarkArea) {
            int line = cursor.blockNumber() + 1;
            if (m_extraAreaPreviousMarkTooltipRequestedLine != line) {
                if (auto data = static_cast<RstBlockUserData *>(cursor.block().userData())) {
                    if (data->marks().isEmpty()) {
                        ToolTip::hide();
                    }
                    else {
                        auto layout = new QGridLayout;
                        layout->setContentsMargins(0, 0, 0, 0);
                        layout->setSpacing(2);
                        foreach (const RstBlockUserData::TextMark &mark, data->marks()) {
                            RstBlockUserData::addToToolTipLayout(mark, layout);
                        }
                        ToolTip::show(mapToGlobal(e->pos()), layout, this);
                    }
                }
            }
            m_extraAreaPreviousMarkTooltipRequestedLine = line;
        }
    }

    if (e->type() == QEvent::MouseButtonPress || e->type() == QEvent::MouseButtonDblClick) {
        if (e->button() == Qt::LeftButton) {
//            int boxWidth = foldBoxWidth(fontMetrics());
//            if (m_lineNumbersVisible && !inMarkArea) {
//                QTextCursor selection = cursor;
//                selection.setVisualNavigation(true);
//                d->extraAreaSelectionAnchorBlockNumber = selection.blockNumber();
//                selection.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
//                selection.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
//                setTextCursor(selection);
//            }
//            else {
//                d->extraAreaToggleMarkBlockNumber = cursor.blockNumber();
//                QTextBlock block = cursor.document()->findBlockByNumber(d->extraAreaToggleMarkBlockNumber);
//                if (TextBlockUserData *data = static_cast<TextBlockUserData *>(block.userData())) {
//                    TextMarks marks = data->marks();
//                    for (int i = marks.size(); --i >= 0; ) {
//                        TextMark *mark = marks.at(i);
//                        if (mark->isDraggable()) {
//                            d->m_markDragStart = e->pos();
//                            break;
//                        }
//                    }
//                }
//            }
        }
    }
//    else if (d->extraAreaSelectionAnchorBlockNumber >= 0) {
//        QTextCursor selection = cursor;
//        selection.setVisualNavigation(true);
//        if (e->type() == QEvent::MouseMove) {
//            QTextBlock anchorBlock = document()->findBlockByNumber(d->extraAreaSelectionAnchorBlockNumber);
//            selection.setPosition(anchorBlock.position());
//            if (cursor.blockNumber() < d->extraAreaSelectionAnchorBlockNumber) {
//                selection.movePosition(QTextCursor::EndOfBlock);
//                selection.movePosition(QTextCursor::Right);
//            }
//            selection.setPosition(cursor.block().position(), QTextCursor::KeepAnchor);
//            if (cursor.blockNumber() >= d->extraAreaSelectionAnchorBlockNumber) {
//                selection.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
//                selection.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
//            }

//            if (e->pos().y() >= 0 && e->pos().y() <= d->m_extraArea->height())
//                d->autoScrollTimer.stop();
//            else if (!d->autoScrollTimer.isActive())
//                d->autoScrollTimer.start(100, this);

//        } else {
//            d->autoScrollTimer.stop();
//            d->extraAreaSelectionAnchorBlockNumber = -1;
//            return;
//        }
//        setTextCursor(selection);
//    }
//    else if (d->extraAreaToggleMarkBlockNumber >= 0 && d->m_marksVisible && d->m_requestMarkEnabled) {
//        if (e->type() == QEvent::MouseButtonRelease && e->button() == Qt::LeftButton) {
//            int n = d->extraAreaToggleMarkBlockNumber;
//            d->extraAreaToggleMarkBlockNumber = -1;
//            const bool sameLine = cursor.blockNumber() == n;
//            const bool wasDragging = d->m_markDragging;
//            d->m_markDragging = false;
//            d->m_markDragStart = QPoint();
//            QTextBlock block = cursor.document()->findBlockByNumber(n);
//            if (TextBlockUserData *data = static_cast<TextBlockUserData *>(block.userData())) {
//                TextMarks marks = data->marks();
//                for (int i = marks.size(); --i >= 0; ) {
//                    TextMark *mark = marks.at(i);
//                    if (sameLine) {
//                        if (mark->isClickable()) {
//                            mark->clicked();
//                            return;
//                        }
//                    } else {
//                        if (wasDragging && mark->isDraggable()) {
//                            if (inMarkArea) {
//                                mark->dragToLine(cursor.blockNumber() + 1);
//                                d->m_extraArea->setCursor(Qt::PointingHandCursor);
//                            } else {
//                                d->m_extraArea->setCursor(Qt::ArrowCursor);
//                            }
//                            return;
//                        }
//                    }
//                }
//            }
//            int line = n + 1;
//            TextMarkRequestKind kind;
//            if (QApplication::keyboardModifiers() & Qt::ShiftModifier)
//                kind = BookmarkRequest;
//            else
//                kind = BreakpointRequest;

//            emit markRequested(this, line, kind);
//        }
//    }
}

void MSTextEditorWidget::onUpdateRequest(const QRect &r, int dy)
{
    if (dy) {
        m_extraArea->scroll(0, dy);
    } else if (r.width() > 4) { // wider than cursor width, not just cursor blinking
        m_extraArea->update(0, r.y(), m_extraArea->width(), r.height());
//        if (!m_searchExpr.isEmpty()) {
//            const int m = m_searchResultOverlay->dropShadowWidth();
//            q->viewport()->update(r.adjusted(-m, -m, m, m));
//        }
    }

    if (r.contains(viewport()->rect()))
        onUpdateExtraAreaWidth();
    scheduleUpdateHighlightScrollBar();
}

void MSTextEditorWidget::paintTextMarks(QPainter &painter,
                                        const ExtraAreaPaintEventData &data,
                                        const QRectF &blockBoundingRect) const
{
    RstBlockUserData *userData = static_cast<RstBlockUserData *>(data.block.userData());
    if (!userData || !m_marksVisible)
        return;
// TODO: Add priority for marks
//    int xoffset = 0;
//    TextMarks marks = userData->marks();
//    TextMarks::const_iterator it = marks.constBegin();
//    if (marks.size() > 3) {
//        // We want the 3 with the highest priority so iterate from the back
//        int count = 0;
//        it = marks.constEnd() - 1;
//        while (it != marks.constBegin()) {
//            if ((*it)->isVisible())
//                ++count;
//            if (count == 3)
//                break;
//            --it;
//        }
//    }
//    TextMarks::const_iterator end = marks.constEnd();
//    for ( ; it != end; ++it) {
//        TextMark *mark = *it;
//        if (!mark->isVisible())
//            continue;
//        const int height = data.lineSpacing - 1;
//        const int width = height;//int(.5 + height * mark->widthFactor());
//        const QRect r(xoffset, int(blockBoundingRect.top()), width, height);
//        QIcon(":/icons/warning-icon.svg").paint(&painter, r);
//        mark->paintIcon(&painter, r);
//        xoffset += 2;
//    }

    if(!userData->marks().isEmpty()) {
        const int height = data.lineSpacing - 1;
        const int width = height;
        const QRect r(0, int(blockBoundingRect.top()), width, height);
        RstBlockUserData::iconFor(userData->marks()[0].type).paint(&painter, r);
    }
}

QString MSTextEditorWidget::filePath() const
{
    return m_filePath;
}

void MSTextEditorWidget::updateCurrentLineInScrollbar()
{
//    if (m_highlightScrollBarController) {
//        m_highlightScrollBarController->removeHighlights(Highlight::CurrentLine);
//        if (m_highlightScrollBarController->scrollBar()->maximum() > 0) {
//            const QTextCursor &tc = textCursor();
//            if (QTextLayout *layout = tc.block().layout()) {
//                const int pos = textCursor().block().firstLineNumber() +
//                        layout->lineForTextPosition(tc.positionInBlock()).lineNumber();
//                m_highlightScrollBarController->addHighlight({Highlight::CurrentLine, pos,
//                                                    NGTheme::EditorCurrentLineScrollBarColor,
//                                                    Highlight::HighestPriority});
//            }
//        }
//    }
}

void MSTextEditorWidget::adjustScrollBarRanges()
{
    if (!m_highlightScrollBarController) {
        return;
    }
    const float lineSpacing = static_cast<float>(QFontMetricsF(font()).lineSpacing());
    if (lineSpacing > 0.00000001f && lineSpacing < 0.00000001f) {
        return;
    }

    const float offset = static_cast<float>(contentOffset().y());
    m_highlightScrollBarController->setVisibleRange(
                (viewport()->rect().height() - offset) / lineSpacing);
    m_highlightScrollBarController->setRangeOffset(offset / lineSpacing);
}

void MSTextEditorWidget::scheduleUpdateHighlightScrollBar()
{
    if (m_scrollBarUpdateScheduled)
        return;

    m_scrollBarUpdateScheduled = true;
    QTimer::singleShot(0, this, &MSTextEditorWidget::updateHighlightScrollBarNow);
}

void MSTextEditorWidget::updateHighlightScrollBarNow()
{
    m_scrollBarUpdateScheduled = false;
    if (!m_highlightScrollBarController)
        return;

    m_highlightScrollBarController->removeAllHighlights();

    updateCurrentLineInScrollbar();

    // update search results
//    addSearchResultsToScrollBar(m_searchResults);

    // update text marks
    QTextBlock block = document()->firstBlock();
    while(block.isValid()) {
        RstBlockUserData *userData = static_cast<RstBlockUserData *>(block.userData());
        if (userData && !userData->marks().isEmpty()) {
            if (block.isVisible()) {
                Highlight highlight;
                highlight.category = userData->marks()[0].type;
                highlight.position = block.firstLineNumber();
                switch(highlight.category) {
                case RstBlockUserData::Info:
                    highlight.color = NGTheme::TextColorLink;
                    break;
                case RstBlockUserData::Warning:
                    highlight.color = NGTheme::TextColorWarning;
                    break;
                case RstBlockUserData::Error:
                default:
                    highlight.color = NGTheme::TextColorError;
                }
                highlight.priority = Highlight::NormalPriority;
                m_highlightScrollBarController->addHighlight(highlight);
            }
        }
        block = block.next();
    }
}

//------------------------------------------------------------------------------
// MSLink
//------------------------------------------------------------------------------
MSLink::MSLink(int begin, int end) :
    m_begin(begin),
    m_end(end)
{

}

MSLink::MSLink(const QString &link, enum Type type, int begin, int end) :
    m_begin(begin),
    m_end(end),
    m_link(link),
    m_type(type)
{

}

int MSLink::begin() const
{
    return m_begin;
}

int MSLink::end() const
{
    return m_end;
}

bool MSLink::isValid() const
{
    return m_begin > -1 && m_end > -1;
}

bool MSLink::operator ==(const MSLink &other)
{
    return m_begin == other.m_begin && m_end == other.m_end;
}

enum MSLink::Type MSLink::type() const
{
    return m_type;
}
QString MSLink::link() const
{
    return m_link;
}

//------------------------------------------------------------------------------
// TextEditExtraArea
//------------------------------------------------------------------------------
TextEditExtraArea::TextEditExtraArea(MSTextEditorWidget *edit) : QWidget(edit)
{
    m_textEdit = edit;
    setAutoFillBackground(true);
}

QSize TextEditExtraArea::sizeHint() const
{
    return QSize(m_textEdit->extraAreaWidth(), 0);
}

void TextEditExtraArea::paintEvent(QPaintEvent *event)
{
    m_textEdit->extraAreaPaintEvent(event);
}

void TextEditExtraArea::wheelEvent(QWheelEvent *event)
{
    QCoreApplication::sendEvent(m_textEdit->viewport(), event);
}

void TextEditExtraArea::mousePressEvent(QMouseEvent *event)
{
    m_textEdit->extraAreaMouseEvent(event);
}

void TextEditExtraArea::mouseMoveEvent(QMouseEvent *event)
{
    m_textEdit->extraAreaMouseEvent(event);
}

void TextEditExtraArea::mouseReleaseEvent(QMouseEvent *event)
{
    m_textEdit->extraAreaMouseEvent(event);
}

void TextEditExtraArea::leaveEvent(QEvent *event) {
    m_textEdit->extraAreaLeaveEvent(event);
}
