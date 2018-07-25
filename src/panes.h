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

#ifndef MANUSCRIPT_PANES_H
#define MANUSCRIPT_PANES_H

#include <framework/navigationwidget.h>
#include "mainwindow.h"
#include "project.h"

/**
 * @brief The ArticlesPane class
 */
class ArticlesPane : public INGNavigationPane
{
public:
    ArticlesPane(TreeModel *model, MSMainWindow *mainWindow);

    // INGNavigationPane interface
public:
    virtual QString name() const override;
    virtual QWidget *widget() const override;
    virtual QIcon icon() const override;

    void setModel(TreeModel *model);

private:
    TreeModel *m_model;
    MSMainWindow *m_mainWindow;
};

/**
 * @brief The PicturesPane class
 */
class PicturesPane : public INGNavigationPane
{
public:
    PicturesPane(PlainModel *model, MSMainWindow *mainWindow);

    // INGNavigationPane interface
public:
    virtual QString name() const override;
    virtual QWidget *widget() const override;
    virtual QIcon icon() const override;

    void setModel(PlainModel *model);

private:
    PlainModel *m_model;
    MSMainWindow *m_mainWindow;
};

/**
 * @brief The BookmarksPane class
 */
class BookmarksPane : public INGNavigationPane
{
public:
    BookmarksPane(PlainModel *model, MSMainWindow *mainWindow);

    // INGNavigationPane interface
public:
    virtual QString name() const override;
    virtual QWidget *widget() const override;
    virtual QIcon icon() const override;

    void setModel(PlainModel *model);

private:
    PlainModel *m_model;
    MSMainWindow *m_mainWindow;
};

/**
 * @brief The FilesPane class
 */
class FilesPane : public INGNavigationPane
{
public:
    FilesPane(PlainModel *model, MSMainWindow *mainWindow);

    // INGNavigationPane interface
public:
    virtual QString name() const override;
    virtual QWidget *widget() const override;
    virtual QIcon icon() const override;

    void setModel(PlainModel *model);

private:
    PlainModel *m_model;
    MSMainWindow *m_mainWindow;
};

#endif // MANUSCRIPT_PANES_H
