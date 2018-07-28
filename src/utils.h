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

#ifndef MANUSCRIPT_UTILS_H
#define MANUSCRIPT_UTILS_H

#include <QFont>
#include <QString>
#include <QTextDocument>


inline bool isAscii(const QString &text) {
    for(int i = 0; i < text.size(); ++i) {
        if(text[i].unicode() > 127) {
            return false;
        }
    }
    return true;
}

inline void setDocumentFont(QTextDocument *doc) {
    QFont font = doc->defaultFont();

#ifdef Q_OS_WIN    
    font.setStyleHint(QFont::TypeWriter);
    font.setFamily("DejaVu Sans Mono");
    font.setPointSize(11);
#elif defined(Q_OS_MAC)   
    font.setStyleHint(QFont::Monospace);
    font.setFamily("Monaco");
#else
    font.setStyleHint(QFont::Monospace);
    font.setFamily("Ubuntu Mono");
#endif    
        
    font.setWeight(13);
    doc->setDefaultFont(font);
}

#endif // MANUSCRIPT_UTILS_H
