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

#ifndef MANUSCRIPT_CENTRALWIDGET_H
#define MANUSCRIPT_CENTRALWIDGET_H

#include "framework/minisplitter.h"

#include "bottomwidget.h"
#include "navigationwidget.h"

#include "editor/editorwidget.h"

class MSCentralWidget : public NGMiniSplitter
{
public:
    MSCentralWidget(QWidget *parent = nullptr,
                    MSNavigationWidget *navigation = nullptr);
    void writeSettings();
    void readSettings();
    void select(const QString &filePath, int line);
    void saveCurrentDocument();
    void saveAllDocuments();
    bool canSaveCurrentDocument() const;
    bool hasCurrentDocument() const;
    bool canUndo() const;
    bool canRedo() const;
    bool canCopy() const;
    bool canCut() const;
    void undo();
    void redo();
    void cut();
    void copy();
    void paste();
    void formatText(enum MSTextEditorWidget::TextFormat type);
    void addNote(enum MSTextEditorWidget::NoteType type);
    void updateSelections(const QModelIndex &fileIdx,
                          const QModelIndex &articleIdx);
    void appendText(const QString &text, enum MSOutputWidget::TextTypes format);

private:
    void hideNavigation();

private:
    MSEditorWidget *m_textEditor;
    MSBottomWidget *m_bottomWidget;
};

#endif // MANUSCRIPT_CENTRALWIDGET_H
