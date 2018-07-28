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

#include "editorwidget.h"

#include <QFileInfo>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QTextStream>
#include <QToolButton>

#include <framework/styledbar.h>

#include "mainwindow.h"
#include "rsthighlighter.h"
#include "version.h"
#include "utils.h"

constexpr short TIMEOUT = 900;

MSEditorWidget::MSEditorWidget(QWidget *parent) :
    QWidget(parent)
{
    m_filesComboBox = new QComboBox(this);
    m_filesComboBox->setSizePolicy(QSizePolicy::Ignored,
                                   QSizePolicy::Ignored);
    m_filesComboBox->setFocusPolicy(Qt::TabFocus);
    m_filesComboBox->setMinimumContentsLength(0);
    m_filesComboBox->setProperty("panelwidget", true);
    MSProject &project = MSMainWindow::instance()->project();
    m_filesComboBox->setModel(project.filesModel());
#if QT_VERSION >= 0x050700
    connect(m_filesComboBox, QOverload<const QString &>::of(&QComboBox::activated),
            this, &MSEditorWidget::onFilesTextChanged);
#else
    connect(m_filesComboBox, SIGNAL(activated(const QString &)),
            this, SLOT(onFilesTextChanged(const QString &)));
#endif

    NGStyledBar *toolBar = new NGStyledBar(this);
    QHBoxLayout *toolBarLayout = new QHBoxLayout;
    toolBarLayout->setMargin(0);
    toolBarLayout->setSpacing(0);
    toolBar->setLayout(toolBarLayout);
    toolBarLayout->addWidget(m_filesComboBox);

    QToolButton *closeFileButton = new QToolButton();
    closeFileButton->setIcon(QIcon(":/icons/multiply.svg"));
    closeFileButton->setToolTip(tr("Show/hide navigation panel"));
    toolBarLayout->addWidget(closeFileButton);
    connect(closeFileButton, &QToolButton::clicked,
            this, &MSEditorWidget::onCloseCurrentFile);

    m_articlesComboBox = new QComboBox(this);
    m_articlesComboBox->setSizePolicy(QSizePolicy::Ignored,
                                      QSizePolicy::Ignored);
    m_articlesComboBox->setFocusPolicy(Qt::TabFocus);
    m_articlesComboBox->setMinimumContentsLength(0);
    m_articlesComboBox->setProperty("panelwidget", true);
    m_articlesComboBox->addItem(QIcon(":/icons/book.svg"), tr("<no articles>"));
    toolBarLayout->addWidget(m_articlesComboBox);
    m_articlesComboBox->hide(); // Hide by default
    m_articlesTree = new QTreeView(m_articlesComboBox);
    m_articlesComboBox->setView(m_articlesTree);
    m_articlesTree->setHeaderHidden(true);
    m_articlesTree->setItemsExpandable(true);
//    m_articlesTree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_articlesTree->setRootIsDecorated(false);
    m_articlesComboBox->setModelColumn(0);
    m_articlesTree->viewport()->installEventFilter(m_articlesComboBox);

//    connect(m_articlesTree, QOverload<const QModelIndex &>::of(&QTreeView::clicked),
//                         MSMainWindow::instance(), &MSMainWindow::onNavigationTreeClicked);
#if QT_VERSION >= 0x050700
    connect(m_articlesComboBox, QOverload<const QString &>::of(&QComboBox::activated),
            this, &MSEditorWidget::onArticlesTextChanged);
#else
    connect(m_articlesComboBox, SIGNAL(activated(const QString &)),
            this, SLOT(onArticlesTextChanged(const QString &)));
#endif

    m_navPanelButton = new QToolButton();
    m_navPanelButton->setIcon(QIcon(":/icons/left-pane.svg"));
    m_navPanelButton->setToolTip(tr("Show/hide navigation panel"));
    m_navPanelButton->setCheckable(true);
    m_navPanelButton->setChecked(true);

    m_bottomPanelButton = new QToolButton();
    m_bottomPanelButton->setIcon(QIcon(":/icons/bottom-pane.svg"));
    m_bottomPanelButton->setToolTip(tr("Show/hide bottom panel"));
    m_bottomPanelButton->setCheckable(true);
    m_bottomPanelButton->setChecked(true);

    toolBarLayout->addWidget(m_navPanelButton);
    toolBarLayout->addWidget(m_bottomPanelButton);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->setMargin(0);
    layout->setSpacing(0);
    setLayout(layout);
    layout->addWidget(toolBar);

    m_editorWidget = new MSTextEditorWidget(this);
    m_editorWidget->hide();
    m_welcomeWidget = new MSWelcomeWidget(this);

    layout->addWidget(m_welcomeWidget);

//    addAutoReleasedObject(new FindInFiles);
//        addAutoReleasedObject(new FindInCurrentFile);
//        addAutoReleasedObject(new FindInOpenFiles);

    select();

    connect(m_editorWidget, &QPlainTextEdit::textChanged,
            this, &MSEditorWidget::onTextChanged);

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &MSEditorWidget::onTimer);
}

void MSEditorWidget::select(const QString &filePath, int line)
{
    if(filePath.isEmpty()) {
        setWidget(true);
        m_filesComboBox->setCurrentIndex(0);
        return;
    }
    else {
        setWidget(false);
    }

    MSProject &project = MSMainWindow::instance()->project();
    rstFile *docFile = project.openedFile(filePath);
    QTextDocument *doc = nullptr;
    if(docFile) {
        doc = docFile->doc;
    }
    else {
        doc = new QTextDocument; //(m_editorWidget);
        setDocumentFont(doc);
        QFile file(filePath);
        file.open(QFile::ReadOnly | QFile::Text);
        QTextStream in(&file);
        in.setCodec("UTF-8");
        QString all = in.readAll();
        doc->setPlainText(all);
        QPlainTextDocumentLayout *layout = new QPlainTextDocumentLayout(doc);
        doc->setDocumentLayout(layout);
        RstHighlighter *highlighter = new RstHighlighter(doc);
        highlighter->setFilePath(filePath);

        project.addOpenedFile(filePath, doc);
        doc->setModified(false);
    }

    m_editorWidget->setFilePath(filePath);
    m_editorWidget->setDocument(doc);

    // Scroll to line
    m_editorWidget->scrollToLine(line);

    MSMainWindow::instance()->updateSelections(filePath, line);
}

void MSEditorWidget::saveCurrentDocument()
{
    if(hasCurrentDocument()) {
        QString fileName = m_editorWidget->filePath();

        MSProject &project = MSMainWindow::instance()->project();
        if(!project.saveDocument(fileName)) {
            QMessageBox::critical(this, tr("Error"), tr("Files save failed"));
        }

        MSMainWindow::instance()->updateSelections(fileName,
                                                   m_editorWidget->line());
    }
}

void MSEditorWidget::saveAllDocuments()
{
    MSProject &project = MSMainWindow::instance()->project();
    if(!project.saveAllDocuments()) {
        QMessageBox::critical(this, tr("Error"), tr("Files save failed"));
    }
    MSMainWindow::instance()->updateSelections(m_editorWidget->filePath(),
                                               m_editorWidget->line());
}

bool MSEditorWidget::canSaveCurrentDocument() const
{
    return hasCurrentDocument() &&
            m_editorWidget->document()->isModified();
}

bool MSEditorWidget::hasCurrentDocument() const
{
    return m_editorWidget && m_editorWidget->isVisible();
}

bool MSEditorWidget::canUndo() const
{
    return hasCurrentDocument() &&
            m_editorWidget->document()->availableUndoSteps();
}

bool MSEditorWidget::canRedo() const
{
    return hasCurrentDocument() &&
            m_editorWidget->document()->availableRedoSteps();
}

bool MSEditorWidget::canCopy() const
{
    return hasCurrentDocument() &&
            m_editorWidget->hasSelection();
}

bool MSEditorWidget::canCut() const
{
    return canCopy();
}

void MSEditorWidget::undo()
{
    if(hasCurrentDocument()) {
        m_editorWidget->undo();
    }
}

void MSEditorWidget::redo()
{
    if(hasCurrentDocument()) {
        m_editorWidget->redo();
    }
}

void MSEditorWidget::cut()
{
    if(hasCurrentDocument()) {
        m_editorWidget->cut();
    }
}

void MSEditorWidget::copy()
{
    if(hasCurrentDocument()) {
        m_editorWidget->copy();
    }
}

void MSEditorWidget::paste()
{
    if(hasCurrentDocument()) {
        m_editorWidget->paste();
    }
}

void MSEditorWidget::formatText(enum MSTextEditorWidget::TextFormat type)
{
    if(hasCurrentDocument()) {
        m_editorWidget->formatText(type);
    }
}

void MSEditorWidget::addNote(enum MSTextEditorWidget::NoteType type)
{
    if(hasCurrentDocument()) {
        m_editorWidget->addNote(type);
    }
}

void MSEditorWidget::updateSelections(const QModelIndex &fileIdx,
                                      const QModelIndex &articleIdx)
{
    MSProject &project = MSMainWindow::instance()->project();

    m_articlesComboBox->setModel(project.articlesModel());

    QModelIndex idx = articleIdx;
    while(idx.isValid()) {
        MSArticle *article = static_cast<MSArticle*>(idx.parent().internalPointer());
        if(article) {
            rstItem *item = article->internalItem();
            if(item->file.compare(m_editorWidget->filePath()) != 0) {
                break;
            }
        }
        idx = idx.parent();
    }

    m_articlesTree->expandAll();
    m_articlesTree->selectionModel()->select(articleIdx, QItemSelectionModel::ClearAndSelect);
    m_articlesTree->setCurrentIndex(articleIdx);

    MSArticle *article = static_cast<MSArticle*>(articleIdx.internalPointer());
    if(article) {
        rstItem *item = article->internalItem();

        m_articlesComboBox->setRootModelIndex(articleIdx.parent());
        m_articlesComboBox->setCurrentText(item->name);

        m_articlesComboBox->setRootModelIndex(idx);
    }
    rstFile *docFile = static_cast<rstFile*>(fileIdx.internalPointer());
    QString name;
    if(docFile) {
        m_filesComboBox->setCurrentText(docFile->name);
    }
}

QToolButton *MSEditorWidget::navPanelButton() const
{
    return m_navPanelButton;
}

QToolButton *MSEditorWidget::bottomPanelButton() const
{
    return m_bottomPanelButton;
}

void MSEditorWidget::setWidget(bool welcome)
{
    if(welcome) {
        QLayoutItem *item = layout()->replaceWidget(m_editorWidget, m_welcomeWidget);
        if(item) {
            item->widget()->hide();
            delete item;
        }
        m_welcomeWidget->show();
        m_articlesComboBox->hide();
    }
    else {
        QLayoutItem *item = layout()->replaceWidget(m_welcomeWidget, m_editorWidget);
        if(item) {
            item->widget()->hide();
            delete item;
        }
        m_editorWidget->show();
        m_articlesComboBox->show();
    }
}

void MSEditorWidget::onFilesTextChanged(const QString &text)
{
    MSMainWindow *wnd = MSMainWindow::instance();
    // Convert file name to path
    QString filePath = wnd->project().openedFilePath(text);
    rstItem item = {"", "", filePath, 0};
    wnd->navigateTo(&item);
}

void MSEditorWidget::onArticlesTextChanged(const QString &text)
{
    Q_UNUSED(text)
    QModelIndex idx = m_articlesTree->selectionModel()->currentIndex();
    MSMainWindow::instance()->onNavigationTreeClicked(idx);
}

void MSEditorWidget::onCloseCurrentFile(bool /*checked*/)
{
    MSProject &project = MSMainWindow::instance()->project();
    QTextDocument *doc = m_editorWidget->document();
    if(doc) {
        if(doc && doc->isModified()) {
            int ret = QMessageBox::warning(this, tr(APP_NAME),
                                           tr("The document %1 has been modified.\n"
                                              "Do you want to save your changes?").arg(m_editorWidget->filePath()),
                                           QMessageBox::Save |
                                           QMessageBox::Discard |
                                           QMessageBox::Cancel,
                                           QMessageBox::Save);




            switch (ret) {
              case QMessageBox::Save:
                if(!project.saveDocument(m_editorWidget->filePath())) {
                    QMessageBox::critical(this, tr("Error"), tr("Files save failed"));
                }
                break;
              case QMessageBox::Discard:
                // Don't Save was clicked
                break;
              case QMessageBox::Cancel:
                return;
              default:
                // should never be reached
                return;
            }
        }
    }

    QString previousFile = project.removeOpenedFile(m_editorWidget->filePath());
    m_editorWidget->setDocument(nullptr);
    select(previousFile, 0);
}

void MSEditorWidget::onTextChanged()
{
    if(hasCurrentDocument()) {
        m_timer->start(TIMEOUT);
    }
}

void MSEditorWidget::onTimer()
{
    m_timer->stop(); // one shoot for update articles, images and bookmarks

    bool hasChanges = m_editorWidget->document()->isModified();

    MSMainWindow::instance()->storeExpandState();

    MSProject &project = MSMainWindow::instance()->project();
    project.openedFileHasChanges(m_editorWidget->filePath(), hasChanges);

    MSMainWindow::instance()->updateActions();
    project.update(m_editorWidget->filePath());

    MSMainWindow::instance()->restoreExpandState();
    MSMainWindow::instance()->updateSelections(m_editorWidget->filePath(),
                                               m_editorWidget->line());

}
