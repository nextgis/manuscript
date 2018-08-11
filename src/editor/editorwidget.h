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

#ifndef MANUSCRIPT_EDITORWIDGET_H
#define MANUSCRIPT_EDITORWIDGET_H

#include "texteditorwidget.h"
#include "welcomewidget.h"

#include <QComboBox>
#include <QModelIndex>
#include <QToolButton>
#include <QTreeView>
#include <QWidget>

/**
 * @brief The MSEditorWidget class
 */

class MSEditorWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MSEditorWidget(QWidget *parent = nullptr);
    void select(const QString &filePath = "", int line = -1);
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
    void addHeader(int level);

    QToolButton *navPanelButton() const;
    QToolButton *bottomPanelButton() const;
    void setWidget(bool welcome = true);

signals:

public slots:
    void onFilesTextChanged(const QString &text);
    void onArticlesTextChanged(const QString &text);
    void onCloseCurrentFile(bool checked = false);
    void onTextChanged();
    void onTimer();

protected:
    QComboBox *m_filesComboBox;
    QComboBox *m_articlesComboBox;
    MSTextEditorWidget *m_editorWidget;
    MSWelcomeWidget *m_welcomeWidget;
    QToolButton *m_navPanelButton, *m_bottomPanelButton;
    QTreeView *m_articlesTree;
    QTimer *m_timer;
};

#endif // MANUSCRIPT_EDITORWIDGET_H
