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

#ifndef MANUSCRIPT_MAINWINDOW_H
#define MANUSCRIPT_MAINWINDOW_H

#include "framework/mainwindow.h"

#include "centralwidget.h"
#include "navigationwidget.h"
#include "project.h"

#include <QLabel>
#include <QStringList>

inline QColor readColor(const QString &color)
{
    bool ok = true;
    const QRgb rgba = static_cast<QRgb>(color.toLongLong(&ok, 16));
    return QColor::fromRgba(rgba);
}


class MSMainWindow : public NGMainWindow
{
    friend class MSApplication;
    Q_OBJECT
public:
    MSMainWindow();
    virtual ~MSMainWindow() override;
    MSProject &project();
    void navigateTo(rstItem *item);
    void updateActions();
    void updateSelections(const QString &filePath, int lineNo);
    void storeExpandState();
    void restoreExpandState();
    QString executeConsole(const QString &program, const QStringList &arguments);

public:
    static MSMainWindow *instance();

private:
    void createDockWindows();
    void loadFile(const QString &path);
    void updateRecentFileActions();
    void addRecentFile(const QString &fileName);
    void openProject(const QString &path);

    // NGMainWindow interface
protected:
    virtual void writeSettings() override;
    virtual void readSettings() override;
    virtual void loadInterface() override;
    virtual void createCommands() override;
    virtual bool maybeSave() override;

    // NGMainWindow interface
protected slots:
    virtual void open() override;
    virtual void about() override;
    virtual void preferences() override;
    void save();
    void saveAll();
    void undo();
    void redo();
    void cut();
    void copy();
    void paste();
    void makeBold();
    void makeItalic();
    void makeProject();
    void runProject();
    void addNote();
    void addTip();
    void addWarning();
    void import();
    void gitPush();
    void gitPull();
    void addArticle();

public slots:
    void onCursorPositionChanged(int pos, int col, int chars, int lines);
    void onOpenRecentFile();
    void onNavigationClicked(const QModelIndex &item);
    void onNavigationTreeClicked(const QModelIndex &item);
    void onSupportInfoUpdated();
    void onArticlesContextMenu(const QPoint &point);

private:
    NGMiniSplitter *m_splitter;
    MSNavigationWidget *m_navigationWidget;
    MSProject m_project;
    MSCentralWidget *m_centralWidget;
    QList <QAction *> recentFileActs;
    QAction *m_recentSeparator;
    QLabel *m_selectionLabel, *m_cursorPositionLabel;
    INGNavigationPane *m_articlesPane, *m_filesPane;
    QString m_indexHtmlFile;
    QMap<QTreeView*,QStringList> m_expandItemsMap;
};

#endif // MANUSCRIPT_MAINWINDOW_H
