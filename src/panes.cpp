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

#include "panes.h"

#include <QCoreApplication>
#include <QHeaderView>
#include <QTreeView>

////////////////////////////////////////////////////////////////////////////////
// ArticlesPane
////////////////////////////////////////////////////////////////////////////////

ArticlesPane::ArticlesPane(TreeModel *model, MSMainWindow *mainWindow) :
    m_model(model),
    m_mainWindow(mainWindow)
{

}

QString ArticlesPane::name() const
{
    return  QCoreApplication::translate("MSMainWindow", "Articles");
}

QWidget *ArticlesPane::widget() const
{
    QTreeView *w = new QTreeView();
    w->setModel(m_model);
    w->setHeaderHidden(true);
#if QT_VERSION >= 0x050700
    QObject::connect(w, QOverload<const QModelIndex &>::of(&QTreeView::clicked),
                     m_mainWindow, &MSMainWindow::onNavigationTreeClicked);
    QObject::connect(w, QOverload<const QPoint &>::of(&QTreeView::customContextMenuRequested),
                     m_mainWindow, &MSMainWindow::onArticlesContextMenu);
#else
    QObject::connect(w, SIGNAL(clicked(const QModelIndex &)),
                     m_mainWindow, SLOT(onNavigationTreeClicked(const QModelIndex &)));
    QObject::connect(w, SIGNAL(customContextMenuRequested(const QPoint &)),
                     m_mainWindow, SLOT(onArticlesContextMenu(const QPoint &)));
#endif
    w->setContextMenuPolicy(Qt::CustomContextMenu);

//    DEBUG: w->setStyleSheet("background-color: green;");
    return w;
}

QIcon ArticlesPane::icon() const
{
    return QIcon(":/icons/book.svg");
}

////////////////////////////////////////////////////////////////////////////////
// PicturesPane
////////////////////////////////////////////////////////////////////////////////

PicturesPane::PicturesPane(PlainModel *model, MSMainWindow *mainWindow) :
    m_model(model),
    m_mainWindow(mainWindow)
{

}

QString PicturesPane::name() const
{
    return QCoreApplication::translate("MSMainWindow", "Illustrations");
}

QWidget *PicturesPane::widget() const
{
    QTreeView *w = new QTreeView();
    w->setModel(m_model);
    w->setRootIsDecorated(false);
#if QT_VERSION >= 0x050700
    QObject::connect(w, QOverload<const QModelIndex &>::of(&QTreeView::clicked),
                     m_mainWindow, &MSMainWindow::onNavigationClicked);
#else
     QObject::connect(w, SIGNAL(clicked(const QModelIndex &)),
                      m_mainWindow, SLOT(onNavigationClicked(const QModelIndex &)));
#endif
//    DEBUG: w->setStyleSheet("background-color: blue;");
    return w;
}

QIcon PicturesPane::icon() const
{
    return QIcon(":/icons/drawing.svg");
}

////////////////////////////////////////////////////////////////////////////////
// BookmarksPane
////////////////////////////////////////////////////////////////////////////////
BookmarksPane::BookmarksPane(PlainModel *model, MSMainWindow *mainWindow) :
    m_model(model),
    m_mainWindow(mainWindow)
{

}

QString BookmarksPane::name() const
{
    return QCoreApplication::translate("MSMainWindow", "Bookmarks");
}

QWidget *BookmarksPane::widget() const
{
    QTreeView *w = new QTreeView();
    w->setModel(m_model);
    w->setRootIsDecorated(false);
#if QT_VERSION >= 0x050700
    QObject::connect(w, QOverload<const QModelIndex &>::of(&QTreeView::clicked),
                     m_mainWindow, &MSMainWindow::onNavigationClicked);
#else
    QObject::connect(w, SIGNAL(clicked(const QModelIndex &)),
                     m_mainWindow, SLOT(onNavigationClicked(const QModelIndex &)));
#endif
//    DEBUG: w->setStyleSheet("background-color: yellow;");
    return w;
}

QIcon BookmarksPane::icon() const
{
    return QIcon(":/icons/bookmark.svg");
}

////////////////////////////////////////////////////////////////////////////////
// FilesPane
////////////////////////////////////////////////////////////////////////////////
FilesPane::FilesPane(PlainModel *model, MSMainWindow *mainWindow) :
    m_model(model),
    m_mainWindow(mainWindow)
{

}

QString FilesPane::name() const
{
    return QCoreApplication::translate("MSMainWindow", "Opened files");
}

QWidget *FilesPane::widget() const
{
    QTreeView *w = new QTreeView();
    w->setModel(m_model);
    w->setRootIsDecorated(false);
    w->header()->hide();
#if QT_VERSION >= 0x050700
    QObject::connect(w, QOverload<const QModelIndex &>::of(&QTreeView::clicked),
                     m_mainWindow, &MSMainWindow::onNavigationClicked);
#else
    QObject::connect(w, SIGNAL(clicked(const QModelIndex &)),
                     m_mainWindow, SLOT(onNavigationClicked(const QModelIndex &)));
#endif
//    DEBUG: w->setStyleSheet("background-color: yellow;");
    return w;
}

QIcon FilesPane::icon() const
{
    return QIcon(":/icons/text.svg");
}
