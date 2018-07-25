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

#ifndef MANUSCRIPT_RSTHIGHLIGHTER_H
#define MANUSCRIPT_RSTHIGHLIGHTER_H

#include <QGridLayout>
#include <QSyntaxHighlighter>

class RstBlockUserData : public QTextBlockUserData
{
public:
    enum TextMarkType {
        Info,
        Warning,
        Error
    };

    enum TextMarkCheckType {
        Articles,
        Notes,
        Files,
        Ref,
        InTextRef
    };

    typedef struct _textMark {
        TextMarkType type;
        TextMarkCheckType checkType;
        QString message;
    } TextMark;

    void addMark(enum TextMarkType type, enum TextMarkCheckType checkType,
                 const QString &text);
    void removeMarks(enum TextMarkType type, enum TextMarkCheckType checkType);
    QList<TextMark> marks() const;

    // static
    static void addToToolTipLayout(TextMark mark, QGridLayout *target);
    static bool addToolTipContent(QLayout *target, const QString &text);
    static QIcon iconFor(enum TextMarkType type);

private:
    QList<TextMark> m_textMarks;
};

/**
 * @brief The RstHighlighter class
 */
class RstHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    RstHighlighter(QTextDocument *parent = nullptr);

    enum FormatItems : int {
        Unknown,
        SingleLineComment,
        MultiLineComment,
        Reference,
        Header,
        Note, NoteBold,
        Warning, WarningBold,
        Tip, TipBold,
        List,
        CodeSample,
        Bold,
        Italic,
        Image, ImageBold,
        TocTree, TocTreeBold,
        KeyWord, KeyWordBold
    };

    void setFilePath(const QString &filePath);

protected:
    void initFormats();
    int highlightLine(const QString &text, int stateParent, int stateChild, int start = 0);
    int highlightStartLine(const QString &text, int state, int stateBold,
                           int boldLength);
    void applyFormat(int &state, int previousState, int newState, int &anchor,
                     int pos, int extra = 0);
    void check(const QString &text, enum RstBlockUserData::TextMarkCheckType checkType);
    void addMark(QTextBlock &textBlock, enum RstBlockUserData::TextMarkType type,
                 enum RstBlockUserData::TextMarkCheckType checkType,
                 const QString &text);
    void removeMark(QTextBlock &textBlock, enum RstBlockUserData::TextMarkType type,
                    enum RstBlockUserData::TextMarkCheckType checkType);

    // QSyntaxHighlighter
protected:
    void highlightBlock(const QString &text) override;
private:
    QMap<int, QTextCharFormat> formats;
    QString m_filePath;
};

#endif // MANUSCRIPT_RSTHIGHLIGHTER_H
