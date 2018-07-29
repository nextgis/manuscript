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
#include "panes.h"
#include "version.h"

#include "dialogs/addarticledlg.h"
#include "dialogs/firstlaunchdlg.h"
#include "dialogs/preferences.h"

#include <QDesktopServices>
#include <QFileDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QProcess>
#include <QSettings>
#include <QStatusBar>
#include <QToolBar>
#include <QTreeView>

#include <framework/access/signbutton.h>
#include <framework/access/access.h>

#include <git/importfromgitwizard.h>

constexpr unsigned char maxRecentFiles = 5;
static MSMainWindow *g_wnd = nullptr;

MSMainWindow::MSMainWindow() : NGMainWindow(),
    m_centralWidget(nullptr),
    m_recentSeparator(nullptr)
{
    g_wnd = this;
    setWindowIcon(QIcon(":/images/main_logo.svg"));
    setWindowTitle(tr(APP_NAME));

    QFontDatabase::addApplicationFont ( ":/fonts/DejaVuSansMono.ttf" );
    QFontDatabase::addApplicationFont ( ":/fonts/DejaVuSansMono-Bold.ttf" );
    QFontDatabase::addApplicationFont ( ":/fonts/DejaVuSansMono-BoldOblique.ttf" );
    QFontDatabase::addApplicationFont ( ":/fonts/DejaVuSansMono-Oblique.ttf" );

    tr("&Edit");
    tr("&Format");
    tr("&Notes");
    tr("&Build");
    tr("&Git");
    tr("&Options");
    tr("Edit");
    tr("Format");
    tr("Build");
    tr("Git");
}

MSMainWindow::~MSMainWindow()
{
    NGAccess::instance().save();
}

MSProject &MSMainWindow::project()
{
    return m_project;
}

MSMainWindow *MSMainWindow::instance()
{
    return g_wnd;
}

void MSMainWindow::createDockWindows()
{
    m_splitter = new NGMiniSplitter(Qt::Horizontal);

    // Add navigation widget
    m_navigationWidget = new MSNavigationWidget(m_splitter);

    // Add articles, illustrations, bookmarks panes
    m_articlesPane = new ArticlesPane(m_project.articlesModel(), this);
    m_navigationWidget->addPane(m_articlesPane);
    m_filesPane = new FilesPane(m_project.filesModel(), this);
    m_navigationWidget->addPane(m_filesPane);
    m_navigationWidget->addPane(new PicturesPane(m_project.picturesModel(), this));
    m_navigationWidget->addPane(new BookmarksPane(m_project.bookmarksModel(), this));

    m_splitter->addWidget(m_navigationWidget);

    // Add editor widget
    m_centralWidget = new MSCentralWidget(m_splitter, m_navigationWidget);
    m_splitter->addWidget(m_centralWidget);

    m_splitter->setStretchFactor(1, 3);

    setCentralWidget(m_splitter);
}


void MSMainWindow::writeSettings()
{
    NGMainWindow::writeSettings();
    QSettings settings;
    settings.beginGroup(QLatin1String("MainWindow"));
    settings.setValue("splitter.sizes", m_splitter->saveState());
    settings.endGroup();

    m_navigationWidget->writeSettings();
    m_centralWidget->writeSettings();
}

void MSMainWindow::readSettings()
{
    NGMainWindow::readSettings();
    QSettings settings;
    settings.beginGroup(QLatin1String("MainWindow"));
    m_splitter->restoreState(settings.value("splitter.sizes").toByteArray());
    settings.endGroup();

    m_navigationWidget->readSettings();
    m_centralWidget->readSettings();

    if(settings.value("first_launch", true).toBool()) {
        settings.setValue("first_launch", false);
        QString name("Anonymous");
        QString email("anonymous@nomail.com");
        MSFirstLaunchDlg dlg(this);
        if(dlg.exec() == QDialog::Accepted) {
            name = dlg.name();
            email = dlg.email();
        }

        settings.beginGroup(QLatin1String("General"));
        settings.setValue("user", name);
        settings.setValue("email", email);
        settings.endGroup();
    }
}

void MSMainWindow::loadInterface()
{
    NGMainWindow::loadInterface();

    QToolBar *toolBar = addToolBar("nextgis.account");
    toolBar->setAllowedAreas(Qt::TopToolBarArea);
    toolBar->setFloatable(false);
    toolBar->setMovable(false);

    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    toolBar->addWidget(spacer);

    toolBar->addWidget(new NGSignInButton("GMGugI1U3Eh5evHYQ7wkEMWPScIomEIr54GHsJZ3"));

    // Add recent menus
    foreach (QAction *action, menuBar()->actions()) {
        if(action->menu() && action->text() == tr("&File")) {
            QMenu *fileMenu = action->menu();
            QList<QAction*> actions = fileMenu->actions();
            QAction *lastItem = actions.last();
            fileMenu->insertActions(lastItem, recentFileActs);
            m_recentSeparator = fileMenu->insertSeparator(lastItem);
            m_recentSeparator->setVisible(false);
            break;
        }
    }

    m_selectionLabel = new QLabel(this);
    m_selectionLabel->setText(tr("Sel chars: %1, lines: %2").arg(0).arg(0));
//        selectionLabel->setProperty("panelwidget_singlerow", true);

    m_cursorPositionLabel = new QLabel(this);
    m_cursorPositionLabel->setText(tr("Pos line: %1, col: %2").arg(0).arg(0));
//        cursorPositionLabel->setProperty("panelwidget_singlerow", true);
//        cursorPositionLabel->setProperty("lightColored", true);

    statusBar()->addPermanentWidget(m_selectionLabel);
    statusBar()->addPermanentWidget(m_cursorPositionLabel);

    updateRecentFileActions();

    createDockWindows();

    connect(&NGAccess::instance(), &NGAccess::supportInfoUpdated,
            this, &MSMainWindow::onSupportInfoUpdated);
}

void MSMainWindow::openProject(const QString &path)
{
    if(m_project.hasChanges()) {
        int ret = QMessageBox::warning(this, tr(APP_NAME),
                                       tr("The document has been modified.\n"
                                          "Do you want to save your changes?"),
                                       QMessageBox::Save |
                                       QMessageBox::Discard |
                                       QMessageBox::Cancel,
                                       QMessageBox::Save);

        switch (ret) {
          case QMessageBox::Save:
              m_project.save();
              break;
          case QMessageBox::Discard:
              // Don't Save was clicked
              break;
          case QMessageBox::Cancel:
              return;
          default:
              // should never be reached
              break;
        }
    }

    loadFile(path);
    QDir sourcesDir = QFileInfo(project().filePath()).dir();
    sourcesDir.cdUp();
    m_indexHtmlFile = sourcesDir.absolutePath() + QDir::separator() +
            QLatin1String("build") + QDir::separator() + QLatin1String("html") +
            QDir::separator() + QLatin1String("index.html");
    updateActions();
}

void MSMainWindow::open()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open project"), "", tr("Project Files (conf.py)"));

    if(!fileName.isEmpty()) {
        openProject(fileName);
    }
}

void MSMainWindow::onOpenRecentFile()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action) {
        loadFile(action->data().toString());
    }
}

void MSMainWindow::navigateTo(rstItem* item)
{
    m_centralWidget->select(item->file, item->line);
}

void MSMainWindow::onNavigationClicked(const QModelIndex &item)
{
    rstItem *plainItem = static_cast<rstItem *>(item.internalPointer());
    navigateTo(plainItem);
}

void MSMainWindow::onNavigationTreeClicked(const QModelIndex &item)
{
    MSArticle *article = static_cast<MSArticle *>(item.internalPointer());
    rstItem *plainItem = article->internalItem();
    navigateTo(plainItem);
}

void MSMainWindow::onSupportInfoUpdated()
{
    QIcon saveIcon(":/icons/save.svg");
    if(!NGAccess::instance().isFunctionAvailable(APP_NAME, "file.save")) {
        saveIcon = NGAccess::lockIcon(saveIcon, QSize(64,64));
    }
    m_commands["file.save"]->setIcon(saveIcon);

    QIcon saveAllIcon(":/icons/save-all.svg");
    if(!NGAccess::instance().isFunctionAvailable(APP_NAME, "file.saveall")) {
        saveAllIcon = NGAccess::lockIcon(saveAllIcon, QSize(64,64));
    }
    m_commands["file.saveall"]->setIcon(saveAllIcon);
}

void MSMainWindow::onCursorPositionChanged(int pos, int col, int chars, int lines)
{
    m_cursorPositionLabel->setText(tr("Pos line: %1, col: %2").arg(pos).arg(col));
    m_selectionLabel->setText(tr("Sel chars: %1, lines: %2").arg(chars).arg(lines));
}

void MSMainWindow::loadFile(const QString &path)
{
    if(!m_project.open(path)) {
        QMessageBox::critical(this, tr("Error"), tr("Failed to open") + " " +
                              path);
    }
    else {
        setWindowTitle(tr(APP_NAME) + " - " + m_project.name());
        statusBar()->showMessage(tr("Project loaded"), 2000);

        addRecentFile(path);

        rstItem *plainItem = new rstItem;
        navigateTo(plainItem);
        delete plainItem;
    }
}

void MSMainWindow::about()
{
    QMessageBox::about(this, tr("About") + " " + APP_NAME,
                       tr("The <b>%1</b> application for edit and compile sphinx "
                          "documentation<br><br>%2 version: %3")
                       .arg(tr(APP_NAME))
                       .arg(tr(APP_NAME))
                       .arg(MANUSCRIPT_VERSION_STRING));

}

void MSMainWindow::preferences()
{
    MSPreferences dlg(this);
    dlg.exec();
}

void MSMainWindow::save()
{
#ifdef NDEBUG
    if(NGAccess::instance().isFunctionAvailable(APP_NAME, "file.save")) {
        m_centralWidget->saveCurrentDocument();
        m_commands["file.save"]->setEnabled(false);
        m_commands["file.saveall"]->setEnabled( project().filesModel()->hasChanges() );
    }
    else {
        NGAccess::showUnsupportedMessage();
    }
#else
    m_centralWidget->saveCurrentDocument();
    m_commands["file.save"]->setEnabled(false);
    m_commands["file.saveall"]->setEnabled( project().filesModel()->hasChanges() );
#endif // NDEBUG
}

void MSMainWindow::saveAll()
{
#ifdef NDEBUG
    if(NGAccess::instance().isFunctionAvailable(APP_NAME, "file.saveall")) {
        m_centralWidget->saveAllDocuments();
        m_commands["file.saveall"]->setEnabled(false);
        m_commands["file.save"]->setEnabled(false);
    }
    else {
        NGAccess::showUnsupportedMessage();
    }
#else
    m_centralWidget->saveAllDocuments();
    m_commands["file.saveall"]->setEnabled(false);
    m_commands["file.save"]->setEnabled(false);
#endif // NDEBUG
}

void MSMainWindow::undo()
{
    m_centralWidget->undo();
}

void MSMainWindow::redo()
{
    m_centralWidget->redo();
}

void MSMainWindow::cut()
{
    m_centralWidget->cut();
}

void MSMainWindow::copy()
{
    m_centralWidget->copy();
}

void MSMainWindow::paste()
{
    m_centralWidget->paste();
}

void MSMainWindow::makeBold()
{
    m_centralWidget->formatText(MSTextEditorWidget::Bold);
}

void MSMainWindow::makeItalic()
{
    m_centralWidget->formatText(MSTextEditorWidget::Italic);
}

void MSMainWindow::makeProject()
{

}

void MSMainWindow::runProject()
{
    QDesktopServices::openUrl(m_indexHtmlFile);
}

void MSMainWindow::addNote()
{
    m_centralWidget->addNote(MSTextEditorWidget::Note);
}

void MSMainWindow::addTip()
{
    m_centralWidget->addNote(MSTextEditorWidget::Tip);
}

void MSMainWindow::addWarning()
{
    m_centralWidget->addNote(MSTextEditorWidget::Warning);
}

void MSMainWindow::import()
{
    MSImportFromGitWizard importWizard(this);
    if(importWizard.exec() == QDialog::Accepted) {
        QString projectPath = importWizard.projectPath();
        openProject(projectPath);
    }
}

void MSMainWindow::gitPush()
{

}

void MSMainWindow::gitPull()
{

}

void MSMainWindow::addArticle()
{
    QMutexLocker locker(m_project.mutex());

    QAction* action = qobject_cast<QAction*>(sender());
    if(action) {
        QString articleStr = action->data().toString();
        QStringList articleStrParts = articleStr.split('#');
        QString filePath = articleStrParts[0];
        int fileLine = articleStrParts[1].toInt();
        QModelIndex index = m_project.articlesModel()->indexByPath(filePath,
                                                                   fileLine);

        MSArticle *article = static_cast<MSArticle*>(index.internalPointer());
        if(article) {

            QString afterRstFile;
            MSArticle *parentArticle = nullptr;
            QString currentDir;
            if(article->type() == MSArticle::H1) {
                rstItem *data = article->internalItem();
                if(data) {
                    QFileInfo fileInfo(data->file);
                    afterRstFile = fileInfo.baseName();
                    currentDir = fileInfo.dir().path() + QDir::separator();
                }
                MSArticle *currentParentArticle = article->parent();
                while (currentParentArticle) {
                    if(currentParentArticle->type() == MSArticle::TOCTREE) {
                        parentArticle = currentParentArticle;
                        break;
                    }
                    currentParentArticle = currentParentArticle->parent();
                }
            }
            else {
                parentArticle = article;
                rstItem *data = article->internalItem();
                if(data) {
                    QFileInfo fileInfo(data->file);
                    currentDir = fileInfo.dir().path() + QDir::separator();
                }
            }

            // Get from user Artice name, file, bookmark
            MSAddArticleDlg dlg(this);
            if(dlg.exec() == QDialog::Accepted) {
                QString fileName = dlg.fileName();
                if(!fileName.endsWith(".rst")) {
                    fileName.append(".rst");
                }
                fileName.prepend(currentDir);

                locker.unlock();
                m_project.addArticle(dlg.name(), fileName, dlg.bookmark(),
                                     parentArticle, afterRstFile);
            }
        }
    }
}

void MSMainWindow::onArticlesContextMenu(const QPoint &point)
{
    if(m_project.filePath().isEmpty()) {
        return;
    }
    QTreeView *treeView = qobject_cast<QTreeView*>(sender());
    if(treeView) {
        QModelIndex index = treeView->indexAt(point);
        MSArticle* article = static_cast<MSArticle*>(index.internalPointer());
        if(article && (article->type() == MSArticle::TOCTREE ||
                       article->type() == MSArticle::H1)) {
            QMenu menu;
            QAction *action = m_commands["edit.addarticle"];
            QString articleString = article->internalItem()->file +
                    QString("#%1").arg(article->internalItem()->line);
            action->setData(articleString);
            menu.addAction(action);
            menu.exec(treeView->mapToGlobal(point));
        }
    }
}

void MSMainWindow::createCommands()
{
    NGMainWindow::createCommands();

    QString gitOutput = executeConsole("git", QStringList() << "--version");
    bool hasGit = !gitOutput.isEmpty();

    QIcon saveIcon(":/icons/save.svg");
    if(!NGAccess::instance().isFunctionAvailable(APP_NAME, "file.save")) {
        saveIcon = NGAccess::lockIcon(saveIcon, QSize(64,64));
    }
    QAction *saveAct = new QAction(saveIcon, tr("&Save"), this);
    saveAct->setShortcuts(QKeySequence::Save);
    saveAct->setStatusTip(tr("Save file"));
    saveAct->setDisabled(true);
    connect(saveAct, &QAction::triggered, this, &MSMainWindow::save);

    m_commands["file.save"] = saveAct;

    QIcon saveAllIcon(":/icons/save-all.svg");
    if(!NGAccess::instance().isFunctionAvailable(APP_NAME, "file.saveall")) {
        saveAllIcon = NGAccess::lockIcon(saveAllIcon, QSize(64,64));
    }
    QAction *saveAllAct = new QAction(saveAllIcon, tr("Save &All"), this);
    saveAllAct->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_A));
    saveAllAct->setStatusTip(tr("Save all changed files"));
    saveAllAct->setDisabled(true);
    connect(saveAllAct, &QAction::triggered, this, &MSMainWindow::saveAll);
    m_commands["file.saveall"] = saveAllAct;

    QAction *importAct = new QAction(QIcon(":/icons/git-import.svg"), tr("Import from git"), this);
    importAct->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_I));
    importAct->setStatusTip(tr("Import project from git repository"));
    importAct->setEnabled(hasGit);
    connect(importAct, &QAction::triggered, this, &MSMainWindow::import);
    m_commands["file.import"] = importAct;

    QAction *undoAct = new QAction(QIcon(":/icons/undo.svg"), tr("&Undo"), this);
    undoAct->setShortcuts(QKeySequence::Undo);
    undoAct->setStatusTip(tr("Undo edits"));
    undoAct->setDisabled(true);
    connect(undoAct, &QAction::triggered, this, &MSMainWindow::undo);
    m_commands["edit.undo"] = undoAct;

    QAction *redoAct = new QAction(QIcon(":/icons/redo.svg"), tr("&Redo"), this);
    redoAct->setShortcuts(QKeySequence::Redo);
    redoAct->setStatusTip(tr("Redo edits"));
    redoAct->setDisabled(true);
    connect(redoAct, &QAction::triggered, this, &MSMainWindow::redo);
    m_commands["edit.redo"] = redoAct;

    QAction *cutAct = new QAction(QIcon(":/icons/cut.svg"), tr("&Cut"), this);
    cutAct->setShortcuts(QKeySequence::Cut);
    cutAct->setStatusTip(tr("Cut"));
    cutAct->setDisabled(true);
    connect(cutAct, &QAction::triggered, this, &MSMainWindow::cut);
    m_commands["edit.cut"] = cutAct;

    QAction *copyAct = new QAction(QIcon(":/icons/copy.svg"), tr("&Copy"), this);
    copyAct->setShortcuts(QKeySequence::Copy);
    copyAct->setStatusTip(tr("Copy"));
    copyAct->setDisabled(true);
    connect(copyAct, &QAction::triggered, this, &MSMainWindow::copy);
    m_commands["edit.copy"] = copyAct;

    QAction *pasteAct = new QAction(QIcon(":/icons/paste.svg"), tr("&Paste"), this);
    pasteAct->setShortcuts(QKeySequence::Paste);
    pasteAct->setStatusTip(tr("Paste"));
    pasteAct->setDisabled(true);
    connect(pasteAct, &QAction::triggered, this, &MSMainWindow::paste);
    m_commands["edit.paste"] = pasteAct;

    // Format
    QAction *boldAct = new QAction(QIcon(":/icons/text-bold.svg"), tr("&Bold"), this);
    boldAct->setShortcuts(QKeySequence::Bold);
    boldAct->setStatusTip(tr("Make text bold"));
    boldAct->setDisabled(true);
    connect(boldAct, &QAction::triggered, this, &MSMainWindow::makeBold);
    m_commands["format.bold"] = boldAct;

    QAction *italicAct = new QAction(QIcon(":/icons/text-italic.svg"), tr("&Italic"), this);
    italicAct->setShortcuts(QKeySequence::Italic);
    italicAct->setStatusTip(tr("Make text italic"));
    italicAct->setDisabled(true);
    connect(italicAct, &QAction::triggered, this, &MSMainWindow::makeItalic);
    m_commands["format.italic"] = italicAct;

    // Build
    QAction *makeAct = new QAction(QIcon(":/icons/build.svg"), tr("&Make"), this);
    makeAct->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_B));
    makeAct->setStatusTip(tr("Make document"));
    makeAct->setDisabled(true);
    connect(makeAct, &QAction::triggered, this, &MSMainWindow::makeProject);
    m_commands["build.make"] = makeAct;

    QAction *runAct = new QAction(QIcon(":/icons/run.svg"), tr("&Run"), this);
    runAct->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_R));
    runAct->setStatusTip(tr("View builded document"));
    runAct->setDisabled(true);
    connect(runAct, &QAction::triggered, this, &MSMainWindow::runProject);
    m_commands["build.run"] = runAct;

    QAction *notesAct = new QAction(QIcon(":/icons/text-notes.svg"), tr("&Notes"), this);
    notesAct->setStatusTip(tr("Show insert notes, tips, warnings menu"));
    connect(notesAct, &QAction::triggered, this, &MSMainWindow::addNote);

    m_commands["format.notes"] = notesAct;

    QAction *noteAct = new QAction(tr("Note"), this);
    noteAct->setStatusTip(tr("Add note"));
    noteAct->setDisabled(true);
    connect(noteAct, &QAction::triggered, this, &MSMainWindow::addNote);
    m_commands["format.notes.note"] = noteAct;

    QAction *tipAct = new QAction(tr("Tip"), this);
    tipAct->setStatusTip(tr("Add tip"));
    tipAct->setDisabled(true);
    connect(tipAct, &QAction::triggered, this, &MSMainWindow::addTip);
    m_commands["format.notes.tip"] = tipAct;

    QAction *warningAct = new QAction(tr("Warning"), this);
    warningAct->setStatusTip(tr("Add warning"));
    warningAct->setDisabled(true);
    connect(warningAct, &QAction::triggered, this, &MSMainWindow::addWarning);
    m_commands["format.notes.warning"] = warningAct;

    QAction *gitPushAct = new QAction(QIcon(":/icons/git-push.svg"), tr("Push to git"), this);
    gitPushAct->setStatusTip(tr("Push changes to git"));
    gitPushAct->setEnabled(hasGit);
    connect(gitPushAct, &QAction::triggered, this, &MSMainWindow::gitPush);
    m_commands["git.push"] = gitPushAct;

    QAction *gitPullAct = new QAction(QIcon(":/icons/git-pull.svg"), tr("Pull from git"), this);
    gitPullAct->setStatusTip(tr("Pull changes from git"));
    gitPullAct->setEnabled(hasGit);
    connect(gitPullAct, &QAction::triggered, this, &MSMainWindow::gitPull);
    m_commands["git.pull"] = gitPullAct;

    QAction *addArticle = new QAction(QIcon(":/icons/book.svg"), tr("Add article"), this);
    addArticle->setStatusTip(tr("Add new article"));
    connect(addArticle, &QAction::triggered, this, &MSMainWindow::addArticle);
    m_commands["edit.addarticle"] = addArticle;


    for (int i = 0; i < maxRecentFiles; ++i) {
        QAction *recentAction = new QAction(this);
        recentAction->setVisible(false);
        connect(recentAction, &QAction::triggered,
                this, &MSMainWindow::onOpenRecentFile);
        recentFileActs << recentAction;
    }
}

bool MSMainWindow::maybeSave()
{
    if(project().hasChanges()) {
        int ret = QMessageBox::warning(this, tr(APP_NAME),
                                       tr("There are unsaved changes.\n"
                                          "Do you want to save them?"),
                                       QMessageBox::Save |
                                       QMessageBox::Discard |
                                       QMessageBox::Cancel,
                                       QMessageBox::Save);
        switch (ret) {
          case QMessageBox::Save:
            if(!project().saveAllDocuments()) {
                QMessageBox::critical(this, tr("Error"), tr("Files save failed"));
            }
            return true;
          case QMessageBox::Discard:
            // Don't Save was clicked
            return true;
          case QMessageBox::Cancel:
            return false;
          default:
            // should never be reached
            return true;
        }
    }
    else {
        return true;
    }
}

void MSMainWindow::addRecentFile(const QString &fileName)
{
    QSettings settings;
    settings.beginGroup(QLatin1String("MainWindow"));
    QStringList files = settings.value("recentFileList").toStringList();
    files.removeAll(fileName);
    files.prepend(fileName);
    while (files.size() > maxRecentFiles)
        files.removeLast();

    settings.setValue("recentFileList", files);
    settings.endGroup();

    updateRecentFileActions();
}

void MSMainWindow::updateActions()
{
// TODO: check python + sphinx installed
    if(project().filePath().isEmpty()) {
        m_commands["build.make"]->setEnabled(false);
        m_commands["build.run"]->setEnabled(false);
    }
    else {
//        m_commands["build.make"]->setEnabled(true);
        QFileInfo indexFileInfo(m_indexHtmlFile);
        if(indexFileInfo.exists()) {
            m_commands["build.run"]->setEnabled(true);
        }
        else {
            m_commands["build.run"]->setEnabled(false);
        }
    }

    m_commands["file.save"]->setEnabled(m_centralWidget->canSaveCurrentDocument());
    m_commands["file.saveall"]->setEnabled( project().filesModel()->hasChanges() );
    m_commands["edit.undo"]->setEnabled(m_centralWidget->canUndo());
    m_commands["edit.redo"]->setEnabled(m_centralWidget->canRedo());
    m_commands["edit.cut"]->setEnabled(m_centralWidget->canCut());
    m_commands["edit.copy"]->setEnabled(m_centralWidget->canCopy());
    m_commands["edit.paste"]->setEnabled(m_centralWidget->hasCurrentDocument());

    m_commands["format.bold"]->setEnabled(m_centralWidget->hasCurrentDocument());
    m_commands["format.italic"]->setEnabled(m_centralWidget->hasCurrentDocument());
    m_commands["format.notes"]->setEnabled(m_centralWidget->hasCurrentDocument());
    m_commands["format.notes.note"]->setEnabled(m_centralWidget->hasCurrentDocument());
    m_commands["format.notes.tip"]->setEnabled(m_centralWidget->hasCurrentDocument());
    m_commands["format.notes.warning"]->setEnabled(m_centralWidget->hasCurrentDocument());
}

void MSMainWindow::updateSelections(const QString &filePath, int lineNo)
{
    // Sync articles and files views
    QList<QWidget *> openFiles = m_navigationWidget->paneHolders(m_filesPane->name());
    FilesModel *filesModel = static_cast<FilesModel *>(project().filesModel());
    QModelIndex fileIdx = filesModel->indexByPath(filePath);
    foreach(QWidget* widget, openFiles) {
        QTreeView *tv = static_cast<QTreeView *>(widget);
        tv->setCurrentIndex(fileIdx);
    }

    QList<QWidget *> articles = m_navigationWidget->paneHolders(m_articlesPane->name());
    TreeModel *articlesModel = static_cast<TreeModel *>(project().articlesModel());
    QModelIndex articleIdx = articlesModel->indexByPath(filePath, lineNo);
    foreach(QWidget* widget, articles) {
        QTreeView *tv = static_cast<QTreeView *>(widget);
        tv->setCurrentIndex(articleIdx);
    }

    m_centralWidget->updateSelections(fileIdx, articleIdx);
}

static QString strippedName(const QString &fullFileName)
{
    QFileInfo info(fullFileName);
    QDir dir = info.dir();
    unsigned char testCount = 0;
    while(dir.dirName().compare("source", Qt::CaseInsensitive) == 0 && testCount < 5) {
        testCount++;
        dir.cdUp();
    }

    if(testCount == 5) {
        return QFileInfo(fullFileName).fileName();
    }
    return dir.dirName();
}

void MSMainWindow::updateRecentFileActions()
{
    QSettings settings;
    settings.beginGroup(QLatin1String("MainWindow"));
    QStringList files = settings.value("recentFileList").toStringList();

    int numRecentFiles = qMin(files.size(), static_cast<int>(maxRecentFiles));

    for (int i = 0; i < numRecentFiles; ++i) {
        QString text = tr("&%1 %2").arg(i + 1).arg(strippedName(files[i]));
        recentFileActs[i]->setText(text);
        recentFileActs[i]->setData(files[i]);
        recentFileActs[i]->setVisible(true);
    }

    for (int j = numRecentFiles; j < maxRecentFiles; ++j) {
        recentFileActs[j]->setVisible(false);
    }

    if(m_recentSeparator)
        m_recentSeparator->setVisible(numRecentFiles > 0);
}

void MSMainWindow::storeExpandState()
{
    m_expandItemsMap.clear();
    QList<QWidget *> articles = m_navigationWidget->paneHolders(m_articlesPane->name());
    foreach(QWidget* widget, articles) {
        QTreeView *tv = static_cast<QTreeView *>(widget);
        QStringList list = project().articlesModel()->expandedItems(tv);
        m_expandItemsMap[tv] = list;
    }
}

void MSMainWindow::restoreExpandState()
{
    QList<QWidget *> articles = m_navigationWidget->paneHolders(m_articlesPane->name());
    foreach(QWidget* widget, articles) {
        QTreeView *tv = static_cast<QTreeView *>(widget);
        QStringList list = m_expandItemsMap[tv];
        foreach (QString item, list) {
            QModelIndex itemIndex =
                    project().articlesModel()->indexByName(QModelIndex(), item);
            tv->setExpanded(itemIndex, true);
        }
    }
}

QString MSMainWindow::executeConsole(const QString &program,
                                     const QStringList &arguments)
{
    QString cmdStr(QTime::currentTime().toString() + ": " + program + " " +
                   arguments.join(" ") + "\n");
    if(m_centralWidget)
        m_centralWidget->appendText(cmdStr, MSOutputWidget::MESSAGE);

    QProcess cmd;
    cmd.start(program, arguments, QIODevice::ReadOnly);
    cmd.closeWriteChannel();
    if (!cmd.waitForStarted())
        return QString();

//    cmd.write("Qt rocks!");
//    cmd.closeWriteChannel();

    if (!cmd.waitForFinished())
        return QString();

    QByteArray result = cmd.readAll();
    if(m_centralWidget)
        m_centralWidget->appendText(result, MSOutputWidget::NORMAL);
    return QString(result);
}
