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

#include "project.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPlainTextEdit>
#include <QSettings>

#include "mainwindow.h"
#include "utils.h"

#include "editor/rsthighlighter.h"

////////////////////////////////////////////////////////////////////////////////
// MSArticle
////////////////////////////////////////////////////////////////////////////////
MSArticle::MSArticle(rstItem *data, MSArticle *parent, enum Type type) :
    m_data(data),
    m_parent(parent),
    m_type(type)
{

}

MSArticle::~MSArticle()
{
    delete m_data;
    m_data = nullptr;
    qDeleteAll(m_childItems);
}

void MSArticle::appendChild(MSArticle *item)
{
    item->m_parent = this;
    m_childItems.append(item);
}

MSArticle *MSArticle::child(int row)
{
    return m_childItems.value(row);
}

int MSArticle::childCount() const
{
    return m_childItems.count();
}

int MSArticle::row() const
{
    if (m_parent) {
        return m_parent->m_childItems.indexOf(const_cast<MSArticle *>(this));
    }
    return 0;
}

int MSArticle::columnCount() const
{
    return 1;
}

QVariant MSArticle::data(int column) const
{
    if(m_data == nullptr) {
        return QVariant();
    }

    switch (column) {
    case 0:
        return m_data->name;
    case 1:
        return m_data->text;
    case 2:
        return m_data->file;
    case 3:
        return m_data->line;
    }
    return QVariant();
}

rstItem *MSArticle::internalItem() const
{
    return m_data;
}

MSArticle *MSArticle::parent() const
{
    return m_parent;
}

unsigned char MSArticle::level() const
{
    unsigned char level = 0;
    MSArticle *parent = m_parent;
    while(parent != nullptr) {
        level++;
        parent = parent->parent();
    }
    return level;
}

MSArticle *MSArticle::findByPath(const QString &filePath, int lineNo)
{
    if(m_data && m_data->file.compare(filePath) == 0) {
        if(m_childItems.isEmpty() || lineNo == 0) {
            return this;
        }
        else {
            MSArticle *findArticle = nullptr;
            foreach(MSArticle *article, m_childItems) {
                if(article->m_data->file.compare(filePath) == 0) {
                    if(article->m_data->line > lineNo) {
                        // Check if article has children
                        if(findArticle && !findArticle->m_childItems.isEmpty()) {
                            return findArticle->findByPath(filePath, lineNo);
                        }
                        return findArticle == nullptr ? this : findArticle;
                    }
                }
                findArticle = article;
            }


            if(findArticle->m_data->file.compare(filePath) == 0) {
                return findArticle == nullptr ? this : findArticle;
            }
            else {
                return this;
            }
        }
    }

    foreach(MSArticle *article, m_childItems) {
        MSArticle *findArticle = article->findByPath(filePath, lineNo);
        if(findArticle) {
            return findArticle;
        }
    }
    return nullptr;
}

MSArticle *MSArticle::findByName(const QString &name)
{
    if(m_data && m_data->name.compare(name) == 0) {
        return this;
    }

    foreach(MSArticle *article, m_childItems) {
        MSArticle *findArticle = article->findByName(name);
        if(findArticle) {
            return findArticle;
        }
    }
    return nullptr;
}

bool MSArticle::hasChild(const QString &filePath) const
{
    foreach(MSArticle *article, m_childItems) {
        if(article->internalItem()->file.compare(filePath) == 0) {
            return true;
        }
    }
    return false;
}

void MSArticle::removeChildren()
{
    qDeleteAll(m_childItems);
    m_childItems.clear();
}

void MSArticle::removeChild(MSArticle *child)
{
    m_childItems.removeAll(child);
    delete child;
}

void MSArticle::insertChild(int position, MSArticle *child)
{
    child->m_parent = this;
    m_childItems.insert(position, child);
}

enum MSArticle::Type MSArticle::type() const
{
    return m_type;
}

void MSArticle::setType(const enum MSArticle::Type &type)
{
    m_type = type;
}

void MSArticle::fillFilePathSet(QSet<QString> &filePathSet) const
{
    if(m_data) {
        filePathSet << m_data->file;
    }

    foreach(MSArticle *article, m_childItems) {
        article->fillFilePathSet(filePathSet);
    }
}

////////////////////////////////////////////////////////////////////////////////
// PlainModel
////////////////////////////////////////////////////////////////////////////////
PlainModel::PlainModel(QObject *parent) : QAbstractItemModel(parent)
{

}

PlainModel::~PlainModel()
{
    qDeleteAll(m_items);
}

void PlainModel::reset()
{
    beginResetModel();
    qDeleteAll(m_items);
    m_items.clear();
    endResetModel();
}

void PlainModel::appendItem(rstItem *item)
{
    beginResetModel();
    m_items.append(item);
    endResetModel();
}

void PlainModel::removeItems(const QString &filePath)
{
    beginResetModel();
    QMutableListIterator<rstItem *> it(m_items);
    while (it.hasNext()) {
        rstItem* item = it.next();
        if(item && item->file.compare(filePath) == 0) {
            it.remove();
        }
    }
    endResetModel();
}

QModelIndex PlainModel::index(int row, int column, const QModelIndex &parent) const
{
    if(!hasIndex(row, column, parent))
        return QModelIndex();

    rstItem *childItem = m_items[row];
    if(nullptr != childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex PlainModel::parent(const QModelIndex &/*child*/) const
{
    return QModelIndex(); // Plain list
}

bool PlainModel::hasChildren(const QModelIndex &parent) const
{
    if(parent.internalPointer())
        return false;
    return true;
}

int PlainModel::rowCount(const QModelIndex &/*parent*/) const
{
    return m_items.count();
}

int PlainModel::columnCount(const QModelIndex &/*parent*/) const
{
    return 2; // Name and text
}

QVariant PlainModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if(role == Qt::DecorationRole && index.column() == 0) {
        return m_icon;
    }

    if(role != Qt::DisplayRole && role != Qt::EditRole) {
        return QVariant();
    }

    rstItem *item = static_cast<rstItem *>(index.internalPointer());
    return valueForItem(item, index.column());
}

QVariant PlainModel::valueForItem(rstItem *item, int column) const
{
    return column == 0 ? item->name : item->text;
}

void PlainModel::setIcon(const QIcon &icon)
{
    m_icon = icon;
}

rstItem *PlainModel::itemByName(const QString &searchName) const
{
    foreach(rstItem *item, m_items) {
        if(item && item->name.compare(searchName) == 0) {
            return item;
        }
    }
    return nullptr;
}


QVariant PlainModel::headerData(int section, Qt::Orientation orientation,
                                int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        if(section == 0) {
            return tr("Name");
        }
        else {
            return tr("Description");
        }
    }
    return QVariant();
}

////////////////////////////////////////////////////////////////////////////////
// FilesModel
////////////////////////////////////////////////////////////////////////////////
FilesModel::FilesModel(QObject *parent) : PlainModel(parent)
{
    addNoDocumentItem();
}

FilesModel::~FilesModel()
{
    deleteDocumentPointers();
}

void FilesModel::reset()
{
    deleteDocumentPointers();
    PlainModel::reset();
    addNoDocumentItem();
}

rstFile *FilesModel::fileDoc(const QString &filePath) const
{
    foreach(rstItem *item, m_items) {
        if(item && item->file.compare(filePath) == 0) {
            return static_cast<rstFile*>(item);
        }
    }
    return nullptr;
}

QString FilesModel::filePath(const QString &fileName) const
{
    foreach(rstItem *item, m_items) {
        if(item && item->name.compare(fileName) == 0) {
            rstFile* fileItem = static_cast<rstFile*>(item);
            return fileItem->file;
        }
    }
    return "";
}

QString FilesModel::removeItem(const QString &filePath)
{
    if(filePath.isEmpty()) {
        return "";
    }
    foreach(rstItem *item, m_items) {
        if(item && item->file.compare(filePath) == 0) {
            rstFile* fileItem = static_cast<rstFile*>(item);
            fileItem->doc->deleteLater();
            int index = m_items.indexOf(item);
            beginResetModel();
            m_items.removeAt(index--);
            endResetModel();
            return m_items[index]->file;
        }
    }
    return "";
}

void FilesModel::setHasChanges(const QString &filePath, bool hasChanges)
{
    foreach(rstItem *item, m_items) {
        if(item && item->file.compare(filePath) == 0) {
            if(hasChanges) {
                if(!item->name.endsWith('*')) {
                    beginResetModel();
                    item->name.append('*');
                    endResetModel();
                    break;
                }
            }
            else {
                if(item->name.endsWith('*')) {
                    beginResetModel();
                    item->name.truncate(item->name.length() - 1);
                    endResetModel();
                    break;
                }
            }
        }
    }
}

QModelIndex FilesModel::indexByPath(const QString &filePath) const
{
    for(int i = 0; i < m_items.size(); ++i) {
        rstItem *item = m_items[i];
        if(item && item->file.compare(filePath) == 0) {
            return createIndex(i, 0, item);
        }
    }
    return QModelIndex();
}

bool FilesModel::hasChanges() const
{
    foreach(rstItem *item, m_items) {
        rstFile* fileItem = static_cast<rstFile*>(item);
        if(fileItem->doc && fileItem->doc->isModified())
            return true;
    }
    return false;
}

bool FilesModel::saveAllDocs()
{
    return saveDoc(QString());
}

bool FilesModel::saveDoc(const QString &filePath)
{
    bool result = true;
    beginResetModel();
    foreach(rstItem *item, m_items) {
        rstFile* fileItem = static_cast<rstFile*>(item);
        if(fileItem && fileItem->doc && fileItem->doc->isModified() &&
                (filePath.isEmpty() || fileItem->file.compare(filePath) == 0)) {

            QFile outfile;
            outfile.setFileName(fileItem->file);
            if(!outfile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                result = false;
                break;
            }

            QTextStream out(&outfile);
            out.setCodec("UTF-8");
            out << fileItem->doc->toPlainText();

            fileItem->doc->setModified(false);

            if(item->name.endsWith('*')) {
                item->name.truncate(item->name.length() - 1);
            }
        }
    }
    endResetModel();
    return result;
}

void FilesModel::deleteDocumentPointers()
{
    foreach(rstItem *item, m_items) {
        rstFile* fileItem = static_cast<rstFile*>(item);
        delete fileItem->doc;
    }
}

void FilesModel::addNoDocumentItem()
{
    rstFile *fileItem = new rstFile;
    fileItem->doc = nullptr;
    fileItem->line = -1;
    fileItem->name = QCoreApplication::translate("project", "<no document>");
    fileItem->text = QLatin1String("no_document");
    appendItem(fileItem);
}

int FilesModel::columnCount(const QModelIndex &/*parent*/) const
{
    return 1; // File Name
}

void FilesModel::appendItem(rstItem *item)
{
    beginResetModel();

    // Add subdirectory to file name if item with same name alredy exists
    foreach(rstItem *existItem, m_items) {
        if(existItem && existItem->name.compare(item->name) == 0) {
            QFileInfo oldFile(existItem->file);
            QFileInfo newFile(item->file);
            QDir oldDir = oldFile.dir();
            QDir newDir = newFile.dir();

            QString oldFileName = oldDir.dirName() + QDir::separator() + existItem->name;
            QString newFileName = newDir.dirName() + QDir::separator() + item->name;
            int counter = 0;
            while(oldFileName.compare(newFileName) == 0) {
                oldDir.cdUp();
                newDir.cdUp();

                oldFileName.prepend(oldDir.dirName() + QDir::separator());
                newFileName.prepend(newDir.dirName() + QDir::separator());

                // Case of forever loop
                if(counter > 3) {
                    break;
                }
                counter++;
            }

            item->name = newFileName;
            existItem->name = oldFileName;
            break;
        }
    }
    m_items.append(item);
    endResetModel();
}

////////////////////////////////////////////////////////////////////////////////
// TreeModel
////////////////////////////////////////////////////////////////////////////////
TreeModel::TreeModel(MSProject *project, QObject *parent) :
    QAbstractItemModel(parent),
    m_project(project)
{
    m_rootItem = new MSArticle();

    rstItem *root = new rstItem;
    root->name = m_project->name();
    root->file = m_project->filePath();
    root->line = 0;
    m_rootItem->appendChild(new MSArticle(root));
}

TreeModel::~TreeModel()
{
    delete m_rootItem;
    m_project = nullptr;
}

void TreeModel::reset(enum ResetType type)
{
    switch (type) {
    case FULL:
        beginResetModel();

        delete m_rootItem;
        m_rootItem = new MSArticle();
        {
            rstItem *root = new rstItem;
            root->name = m_project->name();
            root->file = m_project->filePath();
            root->line = 0;
            m_rootItem->appendChild(new MSArticle(root));
        }
        endResetModel();
        break;
    case BEGIN:
        beginResetModel();
        break;
    case END:
        endResetModel();
        break;
    }

}

MSArticle *TreeModel::rootItem()
{
    return m_rootItem->child(0);
}

QModelIndex TreeModel::indexByPath(const QString &filePath, int lineNo) const
{
    MSArticle *article = m_rootItem->findByPath(filePath, lineNo);
    if(article) {
        return createIndex(article->row(), 0, article);
    }
    return QModelIndex();
}

QModelIndex TreeModel::indexByName(const QModelIndex &parent,
                                   const QString &name) const
{
    MSArticle *parentArticle = static_cast<MSArticle *>(parent.internalPointer());
    if(parentArticle) {
        MSArticle *childArticle = parentArticle->findByName(name);
        if(childArticle) {
            return createIndex(childArticle->row(), 0, childArticle);
        }
    }
    else {
        MSArticle *childArticle = m_rootItem->findByName(name);
        if(childArticle) {
            return createIndex(childArticle->row(), 0, childArticle);
        }
    }
    return QModelIndex();
}

MSArticle *TreeModel::itemByPath(const QString &filePath) const
{
    return m_rootItem->findByPath(filePath);
}

QModelIndex TreeModel::index(MSArticle *item) const
{
    if(item)
        return createIndex(item->row(), 0, item);
    return QModelIndex();
}

void TreeModel::addChild(MSArticle *child)
{

    MSArticle *parent = child->parent();
    if(parent == nullptr)
        parent = m_rootItem;
    beginInsertRows(index(parent), -1, -1);
    parent->appendChild(child);
    endInsertRows();
}

void TreeModel::removeChild(MSArticle *child)
{
    MSArticle *parent = child->parent();
    if(parent == nullptr)
        parent = m_rootItem;
    beginRemoveRows(index(parent), child->row(), child->row());
    parent->removeChild(child);
    endRemoveRows();
}

void TreeModel::insertChild(int position, MSArticle *child)
{
    MSArticle *parent = child->parent();
    if(parent == nullptr)
        parent = m_rootItem;
    beginInsertRows(index(parent), position, position);
    parent->insertChild(position, child);
    endInsertRows();
}

QStringList TreeModel::expandedItems(const QTreeView *tv) const
{
    QStringList out;
    foreach (QModelIndex index, persistentIndexList()) {
        if (tv->isExpanded(index)) {
            out << index.data(Qt::DisplayRole).toString();
        }
    }
    return out;
}

QModelIndex TreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if(!hasIndex(row, column, parent)) {
        return QModelIndex();
    }

    MSArticle *parentItem;

    if (!parent.isValid()) {
        parentItem = m_rootItem;
    }
    else {
        parentItem = static_cast<MSArticle *>(parent.internalPointer());
    }

    MSArticle *childItem = parentItem->child(row);
    if (childItem) {
        return createIndex(row, column, childItem);
    }
    else {
        return QModelIndex();
    }
}

QModelIndex TreeModel::parent(const QModelIndex &child) const
{
    if (!child.isValid())
        return QModelIndex();

    MSArticle *childItem = static_cast<MSArticle *>(child.internalPointer());
    MSArticle *parentItem = childItem->parent();

    if (parentItem == m_rootItem || parentItem == nullptr)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int TreeModel::rowCount(const QModelIndex &parent) const
{
    MSArticle *parentItem;
    if (parent.column() > 0) {
        return 0;
    }

    if (!parent.isValid()) {
        parentItem = m_rootItem;
    }
    else {
        parentItem = static_cast<MSArticle *>(parent.internalPointer());
    }
    return parentItem->childCount();
}

int TreeModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return static_cast<MSArticle *>(parent.internalPointer())->columnCount();
    else
        return m_rootItem->columnCount();
}

QVariant TreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    MSArticle *item = static_cast<MSArticle *>(index.internalPointer());

    if(role == Qt::DecorationRole) {
        switch(item->level()) {
        case 0:
        case 1:
        case 2:
            return QIcon(":/icons/book.svg");
        case 3:
            return QIcon(":/icons/document.svg");
        default:
            return QIcon(":/icons/text.svg");
        }
    }

    if (role != Qt::DisplayRole && role != Qt::EditRole) {
        return QVariant();
    }

    return item->data(index.column());
}

////////////////////////////////////////////////////////////////////////////////
// MSProject
////////////////////////////////////////////////////////////////////////////////
static void cleanValue(QString &value)
{
    if(value[0] == 'u') {
        value.remove(0, 1);
    }

    if(value[0] == '\'' || value[0] == '"') {
        value.remove(0, 1);
    }

    int lastIndex = value.length() - 1;
    if(value[lastIndex] == '\'' || value[lastIndex] == '"') {
        value.remove(lastIndex, 1);
    }
}

static QString parseValue(const QString &value)
{
    QStringList values = value.split("=");
    if(values.count() < 2) {
        return "";
    }

    QString rawValue = values[1].trimmed();
    cleanValue(rawValue);
    return rawValue;
}

static QStringList parseValueOrArray(const QString &value, QTextStream &in)
{
    QStringList out;
    int index = value.indexOf('[');
    if(index == -1) {
        out << parseValue(value);
    }
    else {
        QString listStr = value.mid(index + 1);
        index = listStr.indexOf(']');
        while(index == -1) {
            QString line = in.readLine().trimmed();
            index = line.indexOf(']');
            if(index != -1) {
                listStr += line.mid(0, index);
            }
            else {
                listStr += line;
            }
        }

        QStringList rawList = listStr.split(',');
        foreach(const QString &rawItem, rawList) {
            QString value = rawItem.trimmed();
            cleanValue(value);
            out << value;
        }
    }
    return out;
}


typedef struct _rstTag {
    QString name;
    QMap<QString, QString> properties;
    QStringList values1, values2;
}  rstTag;

static rstTag readTag(QString &line, QTextStream &in, int &lineNo)
{
    int end = line.indexOf("::");
    rstTag tag;
    tag.name = line.mid(3, end - 3);

    char valuesId = 0;
    while (!in.atEnd()) {
        QString parseLine = in.readLine();
        lineNo++;

        if(!parseLine.isEmpty()) {
            if(parseLine.trimmed().isEmpty()) {
                valuesId++;
                continue;
            }
            if(!parseLine.startsWith(" ")) {
                line = parseLine;
                return tag;
            }
        }

        parseLine = parseLine.trimmed();
        if(parseLine.startsWith(":")) {
            parseLine = parseLine.mid(1);
            int index = parseLine.indexOf(":");
            QString key = parseLine.mid(0, index).trimmed();
            QString value = parseLine.mid(index + 1).trimmed();
            tag.properties[key] = value;
            continue;
        }

        if(parseLine.isEmpty()) {
            valuesId++;
            continue;
        }

        if(valuesId == 1) {
            tag.values1 << parseLine;
        }
        else {
            tag.values2 << parseLine;
        }
    }

    return tag;
}

MSProject::MSProject() :
    m_name(QCoreApplication::translate("main", "New project")),
    m_masterDoc("index"),
    m_copyright(""),
    m_author(""),
    m_version("1.0.0"),
    m_release("1.0"),
    m_language("en")
{
    m_extensions << ".rst";
    m_articles = new TreeModel(this);
    m_bookmarks = new PlainModel;
    m_bookmarks->setIcon(QIcon(":/icons/bookmark.svg"));
    m_pictures = new PlainModel;
    m_pictures->setIcon(QIcon(":/icons/drawing.svg"));
    m_files = new FilesModel;
    m_files->setIcon(QIcon(":/icons/text.svg"));
}

MSProject::~MSProject()
{
    delete m_articles;
    delete m_bookmarks;
    delete m_pictures;
    delete m_files;
}

bool MSProject::open(const QString &path)
{
    m_filePath = path;
    QFile inputFile(path);
    if (!inputFile.open(QIODevice::ReadOnly)) {
        return false;
    }

    QTextStream in(&inputFile);
    in.setCodec("UTF-8");
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if(line.startsWith("source_suffix", Qt::CaseInsensitive)) {
            m_extensions = parseValueOrArray(line, in);
        }
        else if(line.startsWith("master_doc", Qt::CaseInsensitive)) {
            m_masterDoc = parseValue(line);
        }
        else if(line.startsWith("project", Qt::CaseInsensitive)) {
            m_name = parseValue(line);
        }
        else if(line.startsWith("copyright", Qt::CaseInsensitive)) {
            m_copyright = parseValue(line);
        }
        else if(line.startsWith("author", Qt::CaseInsensitive)) {
            m_author = parseValue(line);
        }
        else if(line.startsWith("version", Qt::CaseInsensitive)) {
            m_version = parseValue(line);
        }
        else if(line.startsWith("release", Qt::CaseInsensitive)) {
            m_release = parseValue(line);
        }
        else if(line.startsWith("language", Qt::CaseInsensitive)) {
            m_language = parseValue(line);
        }
    }
    inputFile.close();

    // Fill models
    m_articles->reset();
    m_bookmarks->reset();
    m_pictures->reset();
    m_files->reset();

    QDir sourcesDir = QFileInfo(m_filePath).dir();

    QString indexFile;
    foreach(const QString &extension, m_extensions) {
        QFileInfo masterDoc(sourcesDir, m_masterDoc + extension);
        if(masterDoc.isFile()) {
            indexFile = masterDoc.absoluteFilePath();
            break;
        }
    }

    if(indexFile.isEmpty()) {
        return false;
    }

    // Read and analyse index.rst

    parseRstFile(indexFile, m_articles->rootItem());

    return true;
}

void MSProject::parseRstFile(const QString &rstFileStr, MSArticle *parent)
{
    QFile file(rstFileStr);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    QTextStream in(&file);
    in.setCodec("UTF-8");
    parseRstFile(in, rstFileStr, parent);

    if(rstFileStr.indexOf(m_masterDoc) != -1) {
        QDir sourcesDir = QFileInfo(rstFileStr).dir();
        foreach(const QString &extension, m_extensions) {
            QFileInfo rstDoc(sourcesDir, QLatin1String("glossary") + extension);
            QModelIndex glossaryIndex = m_articles->indexByPath(rstDoc.absoluteFilePath());
            if(!glossaryIndex.isValid() && rstDoc.isFile()) {
                parseRstFile(rstDoc.absoluteFilePath(), parent);
            }
        }
    }
}

void MSProject::parseRstFile(QTextStream &in, const QString &rstFileStr,
                             MSArticle *parent, int position)
{
    int lineNo = -1;
    rstItem *bookMarkItem = nullptr;
    QString prevLine;
    MSArticle *h1 = parent;
    MSArticle *h2 = nullptr;
    MSArticle *h3 = nullptr;
    MSArticle *h4 = nullptr;
    while (!in.atEnd()) {
        QString line = in.readLine();
        lineNo++;

        if(bookMarkItem) {
            QString trimmedLine = line.trimmed();
            if(!trimmedLine.isEmpty()) {
                if(line.startsWith("..")) {
                    bookMarkItem->text =  QCoreApplication::translate("MSProject",
                                                                      "!!! Unnamed");
                }
                else {
                    bookMarkItem->text = line;
                }
                bookMarkItem = nullptr;
            }
        }

        QString trimmedLine = line.trimmed();
        if(trimmedLine.startsWith(".. _")) {
            int end = trimmedLine.indexOf(':', 4);
            QString name = trimmedLine.mid(4, end - 4);
            // Add bookmark
            bookMarkItem = new rstItem({ name, "", rstFileStr, lineNo});
            m_bookmarks->appendItem(bookMarkItem);
        }
        else if(trimmedLine.startsWith(".. figure::")) {
            QString fileName = trimmedLine.mid(12).trimmed();
            int figLineNo = lineNo;
            rstTag tag = readTag(line, in, lineNo);

            // Add drawing
            rstPicture *drawingItem = new rstPicture;
            drawingItem->imagePath = fileName;
            drawingItem->file = rstFileStr;
            drawingItem->line = figLineNo;
            drawingItem->name = tag.properties["name"];
            foreach(const QString &value, tag.values1) {
                drawingItem->text += value + " ";
            }

            foreach(const QString &value, tag.values2) {
                drawingItem->fullText += value + " ";
            }

            m_pictures->appendItem(drawingItem);
        }
        else if(trimmedLine.startsWith(".. include::")) {
            QString fileName = trimmedLine.mid(12).trimmed();
            QDir sourcesDir = QFileInfo(rstFileStr).dir();
            QFileInfo rstDoc(sourcesDir, fileName);
            if(rstDoc.isFile()) {
                parseRstFile(rstDoc.absoluteFilePath(), parent);
            }
        }
        else if(line.startsWith("===") || line.startsWith("###") ||
                line.startsWith("***")) {
            if(!prevLine.isEmpty()) {
                rstItem *article = new rstItem;
                article->name = prevLine;
                article->file = rstFileStr;
                article->line = lineNo - 1;
                h1 = new MSArticle(article, parent, MSArticle::H1);
                if(position == -1) {
                    m_articles->addChild(h1);
                }
                else {
                    m_articles->insertChild(position, h1);
                }
            }
        }
        else if(line.startsWith("---")) {
            if(!prevLine.isEmpty()) {
                rstItem *article = new rstItem;
                article->name = prevLine;
                article->file = rstFileStr;
                article->line = lineNo - 1;
                h2 = new MSArticle(article, nullptr, MSArticle::H2);
                h1->appendChild(h2);
            }
        }
        else if(line.startsWith("^^^")) {
            if(!prevLine.isEmpty() && h2) {
                rstItem *article = new rstItem;
                article->name = prevLine;
                article->file = rstFileStr;
                article->line = lineNo - 1;
                h3 = new MSArticle(article, nullptr, MSArticle::H3);
                h2->appendChild(h3);
            }
        }
        else if(line.startsWith("\"\"\"")) {
            if(!prevLine.isEmpty() && h3) {
                rstItem *article = new rstItem;
                article->name = prevLine;
                article->file = rstFileStr;
                article->line = lineNo - 1;
                h4 = new MSArticle(article, nullptr, MSArticle::H4);
                h3->appendChild(h4);
            }
        }
        else if(line.startsWith(".. toctree::")) {
            rstTag tag = readTag(line, in, lineNo);
            h1->setType(MSArticle::TOCTREE);
            QDir sourcesDir = QFileInfo(rstFileStr).dir();
            foreach(const QString &extension, m_extensions) {
                foreach(const QString &value, tag.values1) {
                    QString v = value;
                    if(v.startsWith('/')) {
                        v = v.mid(1);
                    }

                    int i,j;
                    v = filterLinkText(v, i, j);
                    QFileInfo rstDoc(sourcesDir, v + extension);
                    rstFile *tocFile = m_files->fileDoc(rstDoc.absoluteFilePath());
                    if(tocFile && tocFile->doc) {
                        QString content = tocFile->doc->toPlainText();
                        QTextStream in(&content);
                        in.setCodec("UTF-8");
                        parseRstFile(in, tocFile->file, h1);
                    } else if(rstDoc.isFile()) {
                        parseRstFile(rstDoc.absoluteFilePath(), h1);
                    }
                }

//                QFileInfo rstDoc(sourcesDir, QLatin1String("glossary") + extension);
//                QString glossaryPath = rstDoc.absoluteFilePath();
//                QModelIndex glossaryIndex = m_articles->indexByPath(rstDoc.absoluteFilePath());
//                if( !glossaryIndex.isValid() && rstDoc.isFile() ) {
//                    parseRstFile(rstDoc.absoluteFilePath(), h1);
//                }
            }
        }

        prevLine = line;
    }
}

void MSProject::addOpenedFile(const QString &filePath, QTextDocument *doc)
{
    rstFile *fileItem = new rstFile;
    fileItem->doc = doc;
    fileItem->line = -1;
    if(doc) {
        fileItem->file = filePath;
        QFileInfo info(filePath);
        // Check if such name already present and add directories for both
        fileItem->text = info.fileName();
        fileItem->name = fileItem->text.toLower().replace(' ', '_');
    }

    m_files->appendItem(fileItem);
}

QString MSProject::removeOpenedFile(const QString &filePath)
{
    return m_files->removeItem(filePath);
}

rstFile *MSProject::openedFile(const QString &filePath) const
{
    return m_files->fileDoc(filePath);
}

QString MSProject::openedFilePath(const QString &fileName) const
{
    return m_files->filePath(fileName);
}

void MSProject::openedFileHasChanges(const QString &filePath, bool hasChanges)
{
    m_files->setHasChanges(filePath, hasChanges);
}

bool MSProject::saveAllDocuments()
{
    return m_files->saveAllDocs();
}

bool MSProject::saveDocument(const QString &filePath)
{
    return m_files->saveDoc(filePath);
}

/**
 * @brief MSProject::glossaryItem Search glossary item.
 * @param term Term to search in glossary
 * @return Return rstItem ponter. User mast delete pointer manually.
 */
rstItem *MSProject::glossaryItem(const QString &term) const
{
    QString path = getFileByName("glossary.rst");
    rstFile *fileItem = m_files->fileDoc(path);
    QTextStream in;
    in.setCodec("UTF-8");
    QString content;
    QFile file(path);
    if(fileItem) {
        content = fileItem->doc->toPlainText();
        in.setString(&content);
    }
    else if(file.open(QFile::ReadOnly | QFile::Text)) {
        in.setDevice(&file);
    }

    QString line;
    int pos = 0;
    while (!in.atEnd()) {
        line = in.readLine();
        if(line.startsWith("   " + term)) {
            return new rstItem({term, "", path, pos});
        }
        pos++;
    }
    return nullptr;
}

void MSProject::update(const QString &filePath)
{
    QFileInfo fileInfo(filePath);
    QString ext = fileInfo.suffix();
    if(m_extensions.indexOf(QLatin1String(".") + ext) == -1) {
        return;
    }

    QMutexLocker locker(&m_mutex);

    // Find openned document in files
    rstFile *doc = m_files->fileDoc(filePath);
    if(!doc || !doc->doc) {
        return;
    }
    QString content = doc->doc->toPlainText();
    QTextStream in(&content);
    in.setCodec("UTF-8");

    // Remove all bookmarks, images and articles with filePath
    MSArticle *article = m_articles->itemByPath(filePath);
    if(article) {
        MSArticle *parent = article->parent();
        if(parent) {
            // Find all child paths
            QSet<QString> pathSet;
            article->fillFilePathSet(pathSet);

            int position = article->row();
            m_articles->removeChild(article);

            foreach(const QString &childFilePath, pathSet) {
                m_bookmarks->removeItems(childFilePath);
                m_pictures->removeItems(childFilePath);
            }

            // Update at specific position
            parseRstFile(in, filePath, parent, position);
        }
    }
}

void MSProject::addArticle(const QString &name, const QString &filePath,
                           const QString bookmark, MSArticle *toc,
                           const QString &afterFile)
{
    QSettings settings;
    settings.beginGroup(QLatin1String("General"));
    QString user = settings.value("user").toString();
    QString email = settings.value("email").toString();
    settings.endGroup();

    rstFile *file = new rstFile;
    file->file = filePath;
    file->name = name;
    file->line = bookmark.isEmpty() ? 3 : 5;

    QString all;
    all.append(QString(".. sectionauthor:: %1 <%2>\n\n").arg(user).arg(email));
    if(!bookmark.isEmpty()) {
        all.append(QString(".. _%1:\n\n").arg(bookmark));
    }
    all.append(name + "\n");
    for(int i = 0; i < name.size(); ++i) {
        all.append("=");
    }
    all.append("\n\n");
    all.append(".. " + QObject::tr("Add your article text here."));

    QTextDocument *doc = new QTextDocument;
    file->doc = doc;
    setDocumentFont(doc);
    addOpenedFile(filePath, doc);
    doc->setPlainText(all);
    QPlainTextDocumentLayout *layout = new QPlainTextDocumentLayout(doc);
    doc->setDocumentLayout(layout);
    RstHighlighter *highlighter = new RstHighlighter(doc);
    highlighter->setFilePath(filePath);

    doc->setModified(true);
    openedFileHasChanges(filePath, true);

    // Add file name to toc and articles

    QString tocFilePath = toc->internalItem()->file;
    rstFile *docFile = openedFile(tocFilePath);
    QFile tocFile(tocFilePath);
    QTextDocument *textDoc = nullptr;
    QTextStream in;
    in.setCodec("UTF-8");
    QString content;
    bool isOpened = false;
    if(docFile) {
        isOpened = true;
        textDoc = docFile->doc;
        content = textDoc->toPlainText();
        in.setString(&content);
    }
    else {
        textDoc = new QTextDocument;
        setDocumentFont(textDoc);
        QPlainTextDocumentLayout *layout = new QPlainTextDocumentLayout(textDoc);
        textDoc->setDocumentLayout(layout);
        RstHighlighter *highlighter = new RstHighlighter(textDoc);
        highlighter->setFilePath(tocFilePath);

        if(tocFile.open(QFile::ReadOnly | QFile::Text)) {
            in.setDevice(&tocFile);
        }
    }

    int beginToc = 0;
    QString currentLine;
    QString allStr;
    while (!in.atEnd()) {
        currentLine = in.readLine(); 
        qDebug() << currentLine;
        if(currentLine.startsWith(".. toctree::")) {
            qDebug() << "Find toctree";
            beginToc = 1;
            allStr += currentLine + "\n";
            continue;
        }

        if(beginToc == 0) {
            allStr += currentLine + "\n";
            continue;
        }

        if(beginToc == 1) {
            qDebug() << "Find toctree items";

            if( currentLine.trimmed().isEmpty() ) {
                beginToc++;
            }
            allStr += currentLine + "\n";
            continue;
        }

        if(beginToc == 2) {
            qDebug() << "Toctree item " << currentLine;

            QString rstFileName = currentLine.trimmed();
            if(rstFileName.compare(afterFile) == 0) {
                QFileInfo fileInfo(filePath);
                if(afterFile.isEmpty()) {
                    allStr += "   " + fileInfo.baseName() + "\n";
                    allStr += currentLine + "\n";
                }
                else {
                    allStr += currentLine + "\n";
                    allStr += "   " + fileInfo.baseName() + "\n";
                }
                beginToc++;
                continue;
            }
            else {
                allStr += currentLine + "\n";
            }
        }

        if(beginToc > 2) {
            allStr += currentLine + "\n";
        }
    }

    textDoc->setPlainText(allStr);

    if(!isOpened) {
        addOpenedFile(tocFilePath, textDoc);
    }

    textDoc->setModified(true);
    openedFileHasChanges(tocFilePath, true);

    qDebug() << allStr;

    update(tocFilePath);

    MSMainWindow::instance()->navigateTo(file);
}

QMutex *MSProject::mutex()
{
    return &m_mutex;
}

bool MSProject::save()
{
    return false;
}

bool MSProject::saveAs(const QString &/*path*/)
{
    return false;
}

bool MSProject::hasChanges() const
{
    return m_files->hasChanges();
}

PlainModel *MSProject::picturesModel() const
{
    return m_pictures;
}

FilesModel *MSProject::filesModel() const
{
    return m_files;
}

bool MSProject::hasReference(const QString &name) const
{
    return m_bookmarks->itemByName(name) != nullptr;
}

bool MSProject::hasFile(const QString &fileName, bool relative) const
{
    QString check_str;
    if(relative) {
        QFileInfo info(m_filePath);
        QString dir = info.path();
        QString fixedFileName(fileName);
        fixedFileName = fixedFileName.replace('/', QDir::separator());
        check_str = dir + QDir::separator() + fixedFileName;
    }
    else {
        check_str = fileName;
    }

    QStringList testExt = m_extensions;
    testExt << "";
    foreach(const QString &ext, testExt) {
        QFileInfo check_file(check_str + ext);
        if(check_file.exists() && check_file.isFile()) {
            return true;
        }
    }
    return false;
}

bool MSProject::hasImage(const QString &name) const
{
    return m_pictures->itemByName(name) != nullptr;
}

bool MSProject::hasTerm(const QString &name) const
{
    rstItem *item = glossaryItem(name);
    bool result = item != nullptr;
    if(result) {
        delete item;
    }
    return result;
}

QString MSProject::getFileByName(const QString &name) const
{
    QFileInfo info(m_filePath);
    QString dirPath = info.path();
    return getFileByName(dirPath, name);
}

QString MSProject::getFileByName(const QString &dir, const QString &name) const
{
    QString fixedFileName(name);
    fixedFileName = fixedFileName.replace('/', QDir::separator());

    QString check_str = dir + QDir::separator() + fixedFileName;
    QStringList testExt = m_extensions;
    testExt << "";
    foreach(const QString &ext, testExt) {
        QFileInfo check_file(check_str + ext);
        if(check_file.exists() && check_file.isFile()) {
            return check_file.absoluteFilePath();
        }
    }
    return "";
}

PlainModel *MSProject::bookmarksModel() const
{
    return m_bookmarks;
}

TreeModel *MSProject::articlesModel() const
{
    return m_articles;
}

QString MSProject::filePath() const
{
    return m_filePath;
}

QString MSProject::name() const
{
    return m_name;
}

QString MSProject::filterLinkText(QString &text, int &pos, int &end)
{
    int linkPos = -1;
    if((linkPos = text.indexOf('<')) != -1) {
        int linkEnd = text.indexOf('>');
        if(linkEnd == -1) {
            linkEnd = text.length();
        }
        linkPos++;
        int len = linkEnd - linkPos;
        pos += linkPos;
        end = pos + len;
        text = text.mid(linkPos, len);
    }
    return text;
}
