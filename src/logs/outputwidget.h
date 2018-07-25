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

#ifndef MANUSCRIPT_OUTPUTWIDGET_H
#define MANUSCRIPT_OUTPUTWIDGET_H

#include "ansiescapecodehandler.h"

#include <QPlainTextEdit>

class MSOutputWidget : public QPlainTextEdit
{
public:
    enum TextTypes {
        NORMAL = 1,
        MESSAGE
    };
public:
    MSOutputWidget(QWidget *parent = nullptr);
    ~MSOutputWidget();
    void appendText(const QString &textIn, const QTextCharFormat &format);
    void appendText(const QString &text, enum TextTypes format);

private:
    bool isScrollbarAtBottom() const;
    void scrollToBottom();
    QString doNewlineEnforcement(const QString &out);

private:
    int m_maxLineCount;
    QTextCursor m_cursor;
    bool m_enforceNewline;
    bool m_scrollToBottom;
    QTextCharFormat m_normalFormat, m_messageFormat;
    AnsiEscapeCodeHandler *m_escapeCodeHandler;
};

#endif // MANUSCRIPT_OUTPUTWIDGET_H
