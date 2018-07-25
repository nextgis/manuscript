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
#include "outputwidget.h"

#include "framework/application.h"

#include <QScrollBar>
#include <QSettings>

static QString normalizeNewlines(const QString &text)
{
    QString res = text;
    res.replace(QLatin1String("\r\n"), QLatin1String("\n"));
    return res;
}

MSOutputWidget::MSOutputWidget(QWidget *parent) : QPlainTextEdit(parent),
    m_maxLineCount(5000),
    m_cursor(document()),
    m_enforceNewline(false),
    m_scrollToBottom(true),
    m_escapeCodeHandler(new AnsiEscapeCodeHandler)
{
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setFrameShape(QFrame::NoFrame);
    setMouseTracking(true);
    setUndoRedoEnabled(false);
    setReadOnly(true);

    QString themeName = NGGUIApplication::style();
    QString themeURI = QStringLiteral(":/highlighter/%1.theme").arg(themeName);
    QSettings themeSettings(themeURI, QSettings::IniFormat);

    {
        QColor c = readColor(themeSettings.value("Colors/LineNumberSelected").toString());
        m_normalFormat.setForeground(c);
        m_normalFormat.setFontWeight(QFont::Normal);
    }

    {
        QColor c = readColor(themeSettings.value("Colors/ExecCommandLine").toString());
        m_messageFormat.setForeground(c);
        m_messageFormat.setFontWeight(QFont::Normal);
    }

}

MSOutputWidget::~MSOutputWidget()
{
    delete m_escapeCodeHandler;
}

void MSOutputWidget::appendText(const QString &textIn, const QTextCharFormat &format)
{
    const QString text = normalizeNewlines(textIn);
    if (m_maxLineCount > 0 && document()->blockCount() >= m_maxLineCount)
        return;
    const bool atBottom = isScrollbarAtBottom();
    if (!m_cursor.atEnd())
        m_cursor.movePosition(QTextCursor::End);
    m_cursor.beginEditBlock();
    m_cursor.insertText(doNewlineEnforcement(text), format);

    if (m_maxLineCount > 0 && document()->blockCount() >= m_maxLineCount) {
        QTextCharFormat tmp;
        tmp.setFontWeight(QFont::Bold);
        m_cursor.insertText(doNewlineEnforcement(tr("Additional output omitted") +
                                                 QLatin1Char('\n')), tmp);
    }

    m_cursor.endEditBlock();
    if (atBottom)
        scrollToBottom();
}

bool MSOutputWidget::isScrollbarAtBottom() const
{
    return verticalScrollBar()->value() == verticalScrollBar()->maximum();
}

void MSOutputWidget::scrollToBottom()
{
    verticalScrollBar()->setValue(verticalScrollBar()->maximum());
    // QPlainTextEdit destroys the first calls value in case of multiline
    // text, so make sure that the scroll bar actually gets the value set.
    // Is a noop if the first call succeeded.
    verticalScrollBar()->setValue(verticalScrollBar()->maximum());
}

QString MSOutputWidget::doNewlineEnforcement(const QString &out)
{
    m_scrollToBottom = true;
    QString s = out;
    if (m_enforceNewline) {
        s.prepend(QLatin1Char('\n'));
        m_enforceNewline = false;
    }

    if (s.endsWith(QLatin1Char('\n'))) {
        m_enforceNewline = true; // make appendOutputInline put in a newline next time
        s.chop(1);
    }

    return s;
}

void MSOutputWidget::appendText(const QString &text, enum TextTypes format)
{
    QTextCharFormat textFormat;
    switch (format) {
    case NORMAL:
        textFormat = m_normalFormat;
        break;
    case MESSAGE:
        textFormat = m_messageFormat;
        break;
    }

    foreach (const FormattedText &output,
             m_escapeCodeHandler->parseText(FormattedText(text, textFormat)))
        appendText(output.text, output.format);

}
