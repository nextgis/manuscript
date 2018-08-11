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

#include "framework/application.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QRegularExpression>
#include <QSettings>

#include "utils.h"

static QRegularExpression listPattern("(^\\s*\\*\\s+)|(^\\s*#\\.\\s+)|(^\\s*\\d*\\.\\s+)");

////////////////////////////////////////////////////////////////////////////////
/// RstBlockUserData
////////////////////////////////////////////////////////////////////////////////

void RstBlockUserData::addMark(enum RstBlockUserData::TextMarkType type,
                               enum RstBlockUserData::TextMarkCheckType checkType,
                               const QString &text)
{
    m_textMarks.append({type, checkType, text});
}

void RstBlockUserData::removeMarks(enum TextMarkType type,
                                   enum TextMarkCheckType checkType)
{
    for(int i = 0; i < m_textMarks.size(); ++i) {
        if(m_textMarks[i].type == type && m_textMarks[i].checkType == checkType) {
            m_textMarks.removeAt(i);
            i--;
        }
    }
}

QList<RstBlockUserData::TextMark> RstBlockUserData::marks() const
{
    return m_textMarks;
}

void RstBlockUserData::addToToolTipLayout(RstBlockUserData::TextMark mark,
                                          QGridLayout *target)
{
    auto *contentLayout = new QVBoxLayout;
    addToolTipContent(contentLayout, mark.message);
    if (contentLayout->count() > 0) {
        const int row = target->rowCount();
        QIcon icon = iconFor(mark.type);
        if (!icon.isNull()) {
            auto iconLabel = new QLabel;
            iconLabel->setPixmap(icon.pixmap(16, 16));
            target->addWidget(iconLabel, row, 0, Qt::AlignTop | Qt::AlignHCenter);
        }
        target->addLayout(contentLayout, row, 1);
    }
}

bool RstBlockUserData::addToolTipContent(QLayout *target, const QString &text)
{
    if (text.isEmpty()) {
        return false;
    }

    auto textLabel = new QLabel;
    textLabel->setText(text);
    // Differentiate between tool tips that where explicitly set and default tool tips.
//    textLabel->setEnabled(!text.isEmpty());
    target->addWidget(textLabel);

    return true;
}

QIcon RstBlockUserData::iconFor(RstBlockUserData::TextMarkType type)
{
    static QIcon gWarningIcon(":/icons/warning-icon.svg");
    static QIcon gErrorIcon(":/icons/error-icon.svg");
    static QIcon gInfoIcon(":/icons/info-icon.svg");
    switch (type) {
    case Error:
        return gErrorIcon;
    case Warning:
        return gWarningIcon;
    case Info:
        return gInfoIcon;
    }
    return QIcon();
}
////////////////////////////////////////////////////////////////////////////////
/// RstHighlighter
////////////////////////////////////////////////////////////////////////////////

RstHighlighter::RstHighlighter(QTextDocument *parent) : QSyntaxHighlighter(parent)
{
    initFormats();
}

void RstHighlighter::initFormats()
{
    QString themeName = NGGUIApplication::style();
    QString themeURI = QStringLiteral(":/highlighter/%1.theme").arg(themeName);
    QSettings themeSettings(themeURI, QSettings::IniFormat);

    // SingleLineComment
    {
        QColor c = readColor(themeSettings.value("Colors/SingleLineComment").toString());
        QTextCharFormat format;
        format.setForeground(c);
        format.setFontItalic(true);
        formats[RstHighlighter::SingleLineComment] = format;
    }

    // MultiLineComment
    {
        QColor c = readColor(themeSettings.value("Colors/MultiLineComment").toString());
        QTextCharFormat format;
        format.setForeground(c);
        format.setFontItalic(true);
        formats[RstHighlighter::MultiLineComment] = format;
    }

    // Header
    {
        QColor c = readColor(themeSettings.value("Colors/Header").toString());
        QTextCharFormat format;
        format.setForeground(c);
        format.setFontWeight(QFont::Bold);
        formats[RstHighlighter::Header] = format;
    }

    // Reference
    {
        QColor c = readColor(themeSettings.value("Colors/Reference").toString());
        QTextCharFormat format;
        format.setForeground(c);
        formats[RstHighlighter::Reference] = format;
    }

    // Note
    {
        QColor c = readColor(themeSettings.value("Colors/Note").toString());
        QTextCharFormat format;
        format.setForeground(c);
        formats[RstHighlighter::Note] = format;

        format.setFontWeight(QFont::Bold);
        formats[RstHighlighter::NoteBold] = format;
    }

    // Warning
    {
        QColor c = readColor(themeSettings.value("Colors/Warning").toString());
        QTextCharFormat format;
        format.setForeground(c);
        formats[RstHighlighter::Warning] = format;

        format.setFontWeight(QFont::Bold);
        formats[RstHighlighter::WarningBold] = format;
    }

    // Tip
    {
        QColor c = readColor(themeSettings.value("Colors/Tip").toString());
        QTextCharFormat format;
        format.setForeground(c);
        formats[RstHighlighter::Tip] = format;

        format.setFontWeight(QFont::Bold);
        formats[RstHighlighter::TipBold] = format;
    }

    // List
    {
        QColor c = readColor(themeSettings.value("Colors/List").toString());
        QTextCharFormat format;
        format.setForeground(c);
        format.setFontWeight(QFont::DemiBold);
        formats[RstHighlighter::List] = format;
    }

    // CodeSample
    {
        QColor c = readColor(themeSettings.value("Colors/CodeSample").toString());
        QTextCharFormat format;
        format.setForeground(c);
#if QT_VERSION >= 0x050500
        format.setFontWeight(QFont::Medium);
#endif
        formats[RstHighlighter::CodeSample] = format;
    }

    // Bold
    {
        QColor c = readColor(themeSettings.value("Colors/Bold").toString());
        QTextCharFormat format;
        format.setForeground(c);
        format.setFontWeight(QFont::Bold);
        formats[RstHighlighter::Bold] = format;
    }

    // Italic
    {
        QColor c = readColor(themeSettings.value("Colors/Italic").toString());
        QTextCharFormat format;
        format.setForeground(c);
        format.setFontItalic(true);
        formats[RstHighlighter::Italic] = format;
    }

    // Image
    {
        QColor c = readColor(themeSettings.value("Colors/Image").toString());
        QTextCharFormat format;
        format.setForeground(c);
        formats[RstHighlighter::Image] = format;

        format.setFontWeight(QFont::Bold);
        formats[RstHighlighter::ImageBold] = format;
    }

    // TocTree
    {
        QColor c = readColor(themeSettings.value("Colors/TocTree").toString());
        QTextCharFormat format;
        format.setForeground(c);
        formats[RstHighlighter::TocTree] = format;

        format.setFontWeight(QFont::Bold);
        formats[RstHighlighter::TocTreeBold] = format;
    }

    // KeyWord
    {
        QColor c = readColor(themeSettings.value("Colors/KeyWord").toString());
        QTextCharFormat format;
        format.setForeground(c);
        formats[RstHighlighter::KeyWord] = format;

        format.setFontWeight(QFont::Bold);
        formats[RstHighlighter::KeyWordBold] = format;
    }
}

void RstHighlighter::applyFormat(int &state, int previousState, int newState,
                                 int &anchor, int pos, int extra)
{
    if(state == newState) {
        if(state > -1) {
            setFormat(anchor, pos + extra - anchor, formats[state]);
        }
        anchor = pos + extra;
        state = previousState;
    }
    else {
        if(state > -1) {
            setFormat(anchor, pos - anchor, formats[state]);
        }
        anchor = pos;
        state = newState;
    }
}

int RstHighlighter::highlightLine(const QString &text, int stateParent,
                                  int stateChild, int start)
{
    int anchor = start;
    int state = stateChild;
    int previousState = stateParent;
    int colonPos = -1;

    for(int i = start; i < text.length(); ++i) {
        if(text[i] == '`') {
            if(i + 1 < text.length() && text[i + 1] == '`' ) { // Code Sample
                applyFormat(state, previousState, RstHighlighter::CodeSample,
                            anchor, i, 2);
                colonPos = -1;
                i++;
            }
            else if( i > 0 && text[i - 1] == ':' ) {
                applyFormat(state, previousState, RstHighlighter::KeyWord,
                            anchor, i);
                colonPos = -1;
            }
            else if( state == RstHighlighter::KeyWord ) {
                applyFormat(state, previousState, RstHighlighter::KeyWord,
                            anchor, i, 1);
                colonPos = -1;
            }
            else {
                applyFormat(state, previousState, RstHighlighter::Reference,
                            anchor, i, 2);
                colonPos = -1;
            }
        }
        else if(text[i] == '*') {
            if(i + 1 < text.length() && text[i + 1] == '*') { // Bold
                applyFormat(state, previousState, RstHighlighter::Bold,
                            anchor, i, 2);
                colonPos = -1;
                i++;
            }
            else { // italic
                if(state != RstHighlighter::Italic &&
                        (i + 1 < text.length() && text[i + 1] != ' ')) {
                    applyFormat(state, previousState, RstHighlighter::Italic,
                                anchor, i, 1);
                    colonPos = -1;
                }
                else if(state == RstHighlighter::Italic){
                    applyFormat(state, previousState, RstHighlighter::Italic,
                                anchor, i, 1);
                    colonPos = -1;
                }
            }
        }
        else if(text[i] == ':') { // Key words
            if(colonPos > -1) {
                int length = i - colonPos;
                if(length > 1) {
                    // In keywords no spaces
                    if(text.mid(colonPos, length).indexOf(' ') == -1) {
                        if(state > -1) {
                            setFormat(anchor, i - anchor, formats[state]);
                        }
                        setFormat(colonPos, length + 1,
                                  formats[RstHighlighter::KeyWordBold]);
                        anchor = i + 1;

                        check(text, RstBlockUserData::InTextRef);
                    }
                }
                colonPos = -1;
            }
            else if(i + 1 < text.length() &&
                    text[i + 1] >= 'a' &&
                    text[i + 1] <= 'z') {
                colonPos = i;
            }
        }
    }

    if(state > -1) {
        setFormat(anchor, text.length() - anchor, formats[state]);
    }
    return state;
}

int RstHighlighter::highlightStartLine(const QString &text, int state,
                                       int stateBold, int boldLength)
{
    int stateParent = state;
    setFormat(0, boldLength, formats[stateBold]);
    int stateChild = highlightLine(text, -1, stateParent, boldLength);
    if(stateChild == stateParent) {
        return stateParent;
    }
    else {
        return stateParent * 10000 + stateChild;
    }
}

void RstHighlighter::highlightBlock(const QString &text)
{
    int initialState = previousBlockState();
    int stateParent = -1;
    int stateChild = -1;
    if(initialState > 10000) {
        stateParent = initialState / 10000;
        stateChild = initialState - stateParent * 10000;
    }
    else {
        stateParent = stateChild = initialState;
    }

    if(initialState == RstHighlighter::MultiLineComment ||
       initialState == RstHighlighter::Image ||
       initialState == RstHighlighter::CodeSample ||
       initialState == RstHighlighter::TocTree ) {
        if(text.startsWith("\t") || text.startsWith(" ") || text.isEmpty()) {
            setFormat(0, text.length(), formats[initialState]);
        }
        else {
            initialState = -1;
        }
    }

    if(stateParent == RstHighlighter::Note ||
       stateParent == RstHighlighter::Warning ||
       stateParent == RstHighlighter::Tip) {
        if(text.startsWith("\t") || text.startsWith(" ") || text.isEmpty()) {
            stateChild = highlightLine(text, stateParent, stateChild);
            if(stateChild == stateParent) {
                initialState = stateParent;
            }
            else {
                initialState = stateParent * 10000 + stateChild;
            }
        }
        else {
            initialState = -1;
        }
    }

    if(initialState == RstHighlighter::Bold ||
       initialState == RstHighlighter::Italic ||
       initialState == RstHighlighter::Reference ||
       initialState == RstHighlighter::KeyWord) {
       initialState = highlightLine(text, -1, initialState);
    }
    else if (initialState == -1) {
        if(text.startsWith(".. ")) {
            QString part = text.mid(3);
            if(part.startsWith("_")) { // reference
                setFormat(0, text.length(), formats[RstHighlighter::Reference]);
                initialState = -1;
                check(text, RstBlockUserData::Ref);
            }
            else if(part.startsWith("sectionauthor::")) { // comment
                setFormat(0, text.length(), formats[RstHighlighter::SingleLineComment]);
                initialState = -1;
            }
            else if(part.startsWith("only::") || part.startsWith("glossary::")) {
                initialState = highlightStartLine(text, RstHighlighter::TocTree,
                                                  RstHighlighter::TocTreeBold, 9);
                initialState = -1;
            }
            else if(part.startsWith("include::")) {
                initialState = highlightStartLine(text, RstHighlighter::TocTree,
                                                  RstHighlighter::TocTreeBold, 9);
                initialState = -1;
                check(part.mid(9).trimmed(), RstBlockUserData::Files);
            }
            else if(part.startsWith("code-block::") || part.startsWith("code::")) {
                setFormat(0, text.length(), formats[RstHighlighter::CodeSample]);
                initialState = RstHighlighter::CodeSample;
            }
            else if(part.startsWith("note::")) {
                initialState = highlightStartLine(text, RstHighlighter::Note,
                                                  RstHighlighter::NoteBold, 9);
                check(part.mid(6), RstBlockUserData::Notes);
            }
            else if(part.startsWith("tip::")) {
                initialState = highlightStartLine(text, RstHighlighter::Tip,
                                                  RstHighlighter::TipBold, 8);
                check(part.mid(5), RstBlockUserData::Notes);
            }
            else if(part.startsWith("warning::")) {
                initialState = highlightStartLine(text, RstHighlighter::Warning,
                                                  RstHighlighter::WarningBold, 12);
                check(part.mid(9), RstBlockUserData::Notes);
            }
            else if(part.startsWith("figure::") || part.startsWith("image::")) {
                initialState = highlightStartLine(text, RstHighlighter::Image,
                                                  RstHighlighter::ImageBold, 11);
                QString imagePath = part.mid(7);
                if(imagePath.startsWith(':')) {
                    imagePath = imagePath.mid(1);
                }
                check(imagePath.trimmed(), RstBlockUserData::Files);
            }
            else if(part.startsWith("toctree::")) {
                initialState = highlightStartLine(text, RstHighlighter::TocTree,
                                                  RstHighlighter::TocTreeBold, 12);
            }
            else { // multiline comment
                setFormat(0, text.length(), formats[RstHighlighter::MultiLineComment]);
                initialState = RstHighlighter::MultiLineComment;
            }
        }
        else if(text.startsWith("===") || text.startsWith("---") ||
                text.startsWith("^^^") || text.startsWith("'''") ||
                text.startsWith("~~~") || text.startsWith("***")) {
            setFormat(0, text.length(), formats[RstHighlighter::Header]);
            check(text, RstBlockUserData::Articles);
            initialState = -1;
        }
        else if(text.trimmed().endsWith("::")) {
            initialState = RstHighlighter::CodeSample;
        }
        else {
            int start = 0;
            QRegularExpressionMatchIterator matchIterator = listPattern.globalMatch(text);
            while (matchIterator.hasNext()) {
                QRegularExpressionMatch match = matchIterator.next();
                setFormat(match.capturedStart(), match.capturedLength(),
                          formats[RstHighlighter::List]);
                start = match.capturedLength();
            }

            initialState = highlightLine(text, -1, initialState, start);
        }
    }

    setCurrentBlockState(initialState);
}

void RstHighlighter::setFilePath(const QString &filePath)
{
    m_filePath = filePath;
}

void RstHighlighter::check(const QString &text, enum RstBlockUserData::TextMarkCheckType checkType)
{
    switch (checkType) {
    case RstBlockUserData::Articles:
        if(!text.trimmed().isEmpty()) {
            QChar articleType = text[0];
            int i = 0;
            while(text.length() > i && text[i++] == articleType) {

            }
            QTextBlock pcb = currentBlock().previous();
            QString articleText = pcb.text().trimmed();
            if(!articleText.isEmpty()) {
                if(articleText.length() > i) {
                    addMark(pcb, RstBlockUserData::Error, checkType,
                            tr("Underline character count should be equal article name or greater"));
                }
                else {
                    removeMark(pcb, RstBlockUserData::Error, checkType);
                }

                QTextBlock ncb = currentBlock().next();
                if(!ncb.text().trimmed().isEmpty()) {
                    addMark(pcb, RstBlockUserData::Warning, checkType,
                            tr("Need an empty line after the article"));
                }
                else {
                    removeMark(pcb, RstBlockUserData::Warning, checkType);
                }
            }
        }
        break;
    case RstBlockUserData::Notes:
        if(text.isEmpty()) {
            QTextBlock ncb = currentBlock().next();
            if(ncb.text().trimmed().isEmpty()) {
                addMark(ncb, RstBlockUserData::Warning, checkType,
                        tr("No need empty line here"));
            }
            else {
                removeMark(ncb, RstBlockUserData::Warning, checkType);
            }
        }
        break;
    case RstBlockUserData::Files:
        {
            QTextBlock cb = currentBlock();
            if(!isAscii(text)) {
                addMark(cb, RstBlockUserData::Warning, checkType,
                        tr("File path should consist only ASCII characters"));
            }
            else {
                removeMark(cb, RstBlockUserData::Warning, checkType);
            }
            QFileInfo info(m_filePath);
            QString path = info.path() + QDir::separator() + text;
            QFileInfo file(path);
            if(file.exists()) {
                removeMark(cb, RstBlockUserData::Error, checkType);
            }
            else {
                if(MSMainWindow::instance()->project().getFileByName(info.path(),
                                                                     text).isEmpty()) {
                    addMark(cb, RstBlockUserData::Error, checkType,
                            tr("File not exists"));
                }
                else {
                    removeMark(cb, RstBlockUserData::Error, checkType);
                }
            }
        }
        break;
    case RstBlockUserData::Ref:
        {
            QTextBlock ncb = currentBlock().next();
            if(!ncb.text().trimmed().isEmpty()) {
                addMark(ncb, RstBlockUserData::Warning, checkType,
                        tr("Need an empty line after the reference"));
            }
            else {
                removeMark(ncb, RstBlockUserData::Warning, checkType);
            }
        }
        break;
    case RstBlockUserData::InTextRef: //Add check exist of ref, numref, term in qlossary
    {
        MSProject &project = MSMainWindow::instance()->project();
        int pos = 0;
        // Check not filled keywords
        while((pos = text.indexOf(":term", pos)) != -1) {
            QTextBlock cb = currentBlock();
            pos += 5;
            if(text[pos] != ':') {
                addMark(cb, RstBlockUserData::Error, checkType,
                        tr("Invalid keyword '%1'. Mast end with colon").arg(":term"));
                return;
            }
            else {
                removeMark(cb, RstBlockUserData::Error, checkType);
            }
        }

        pos = 0;
        while((pos = text.indexOf(":ref", pos)) != -1) {
            QTextBlock cb = currentBlock();
            pos += 4;
            if(text[pos] != ':') {
                addMark(cb, RstBlockUserData::Error, checkType,
                        tr("Invalid keyword '%1'. Mast end with colon").arg(":term"));
                return;
            }
            else {
                removeMark(cb, RstBlockUserData::Error, checkType);
            }
        }

        pos = 0;
        while((pos = text.indexOf(":numref", pos)) != -1) {
            QTextBlock cb = currentBlock();
            pos += 7;
            if(text[pos] != ':') {
                addMark(cb, RstBlockUserData::Error, checkType,
                        tr("Invalid keyword '%1'. Mast end with colon").arg(":term"));
                return;
            }
            else {
                removeMark(cb, RstBlockUserData::Error, checkType);
            }
        }

        pos = 0;
        while((pos = text.indexOf(":term:", pos)) != -1) {
            int end = text.indexOf('`', pos + 7);
            if(end == -1) {
                break;
            }
            pos += 7;
            QString linkText = text.mid(pos, end - pos);
            linkText = MSProject::filterLinkText(linkText, pos, end);
            QTextBlock cb = currentBlock();
            if(project.hasTerm(linkText)) {
                removeMark(cb, RstBlockUserData::Warning, checkType);
            }
            else {
                addMark(cb, RstBlockUserData::Warning, checkType,
                        tr("Term '%1' is not exists in glossary").arg(linkText));
                return;
            }
        }

        pos = 0;
        while((pos = text.indexOf(":ref:", pos)) != -1) {
            int end = text.indexOf('`', pos + 6);
            if(end == -1) {
                break;
            }
            pos += 6;
            QString linkText = text.mid(pos, end - pos);
            // Skip autogenerated items
            if(linkText == QLatin1String("search") ||
                    linkText == QLatin1String("genindex")) {
                break;
            }
            linkText = MSProject::filterLinkText(linkText, pos, end);
            QTextBlock cb = currentBlock();
            if(project.hasReference(linkText)) {
                removeMark(cb, RstBlockUserData::Warning, checkType);
            }
            else {
                addMark(cb, RstBlockUserData::Warning, checkType,
                        tr("Reference with name '%1' is not exists").arg(linkText));
                return;
            }
        }

        pos = 0;
        while((pos = text.indexOf(":numref:", pos)) != -1) {
            int end = text.indexOf('`', pos + 9);
            if(end == -1) {
                break;
            }
            pos += 9;
            QString linkText = text.mid(pos, end - pos);
            linkText = MSProject::filterLinkText(linkText, pos, end);
            QTextBlock cb = currentBlock();
            if(project.hasImage(linkText)) {
                removeMark(cb, RstBlockUserData::Warning, checkType);
            }
            else {
                addMark(cb, RstBlockUserData::Warning, checkType,
                        tr("Image with name '%1' is not exists").arg(linkText));
                return;
            }
        }
    }
        break;
    }
}



void RstHighlighter::addMark(QTextBlock &textBlock,
                             enum RstBlockUserData::TextMarkType type,
                             enum RstBlockUserData::TextMarkCheckType checkType,
                             const QString &text)
{
    RstBlockUserData *userData;
    if(textBlock.userData() == nullptr) {
        userData = new RstBlockUserData;
        textBlock.setUserData(userData);
    }
    else {
        userData = static_cast<RstBlockUserData *>(textBlock.userData());
    }
    userData->addMark(type, checkType, text);
}

void RstHighlighter::removeMark(QTextBlock &textBlock,
                             enum RstBlockUserData::TextMarkType type,
                             enum RstBlockUserData::TextMarkCheckType checkType)
{
    if(textBlock.userData() == nullptr) {
        return;
    }

    RstBlockUserData *userData = static_cast<RstBlockUserData *>(textBlock.userData());
    userData->removeMarks(type, checkType);
}
