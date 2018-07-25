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

#include "centralwidget.h"

#include <QSettings>

MSCentralWidget::MSCentralWidget(QWidget *parent,
                                 MSNavigationWidget *navigation) :
    NGMiniSplitter(parent)
{
    setOrientation(Qt::Vertical);

    // Add editor widget
    m_textEditor = new MSEditorWidget(this);
    addWidget(m_textEditor);

    m_bottomWidget = new MSBottomWidget(this);
    addWidget(m_bottomWidget);

    // Add log window widget
    setStretchFactor(0, 4);
    setStretchFactor(1, 2);

    navigation->setShowButton(m_textEditor->navPanelButton());
    m_bottomWidget->setShowButton(m_textEditor->bottomPanelButton());

}

void MSCentralWidget::writeSettings()
{
    QSettings settings;
    settings.beginGroup(QLatin1String("CentralWidget"));
    settings.setValue("splitter.sizes", saveState());
    settings.endGroup();
}

void MSCentralWidget::readSettings()
{
    QSettings settings;
    settings.beginGroup(QLatin1String("CentralWidget"));
    restoreState(settings.value("splitter.sizes").toByteArray());
    settings.endGroup();
}

void MSCentralWidget::select(const QString &filePath, int line)
{
    m_textEditor->select(filePath, line);
}

void MSCentralWidget::saveCurrentDocument()
{
    m_textEditor->saveCurrentDocument();
}

void MSCentralWidget::saveAllDocuments()
{
    m_textEditor->saveAllDocuments();
}

bool MSCentralWidget::canSaveCurrentDocument() const
{
    return m_textEditor->canSaveCurrentDocument();
}

bool MSCentralWidget::hasCurrentDocument() const
{
    return m_textEditor->hasCurrentDocument();
}

bool MSCentralWidget::canUndo() const
{
    return m_textEditor->canUndo();
}

bool MSCentralWidget::canRedo() const
{
    return m_textEditor->canRedo();
}

bool MSCentralWidget::canCopy() const
{
    return m_textEditor->canCopy();
}

bool MSCentralWidget::canCut() const
{
    return m_textEditor->canCut();
}

void MSCentralWidget::undo()
{
    m_textEditor->undo();
}

void MSCentralWidget::redo()
{
    m_textEditor->redo();
}

void MSCentralWidget::cut()
{
    m_textEditor->cut();
}

void MSCentralWidget::copy()
{
    m_textEditor->copy();
}

void MSCentralWidget::paste()
{
    m_textEditor->paste();
}

void MSCentralWidget::addNote(MSTextEditorWidget::NoteType type)
{
    m_textEditor->addNote(type);
}

void MSCentralWidget::updateSelections(const QModelIndex &fileIdx,
                                       const QModelIndex &articleIdx)
{
    m_textEditor->updateSelections(fileIdx, articleIdx);
}

void MSCentralWidget::appendText(const QString &text,
                                 MSOutputWidget::TextTypes format)
{
    m_bottomWidget->appendText(text, format);
}

void MSCentralWidget::formatText(enum MSTextEditorWidget::TextFormat type)
{
    m_textEditor->formatText(type);
}

void MSCentralWidget::hideNavigation()
{
    m_textEditor->navPanelButton()->setChecked(false);
}
