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

#ifndef MANUSCRIPT_PROJECT_H
#define MANUSCRIPT_PROJECT_H

#include <QAbstractItemModel>
#include <QIcon>
#include <QString>
#include <QStringList>
#include <QTextDocument>
#include <QTextStream>
#include <QTreeView>

/**
 * @brief rst document item
 */
typedef struct _rstItem {
    QString name;   // Internal name
    QString text;   // Some text
    QString file;   // Item file
    int line;       // Line in file
} rstItem;

/**
 * @brief bookmark
 */
typedef rstItem bookmark;

/**
 * @brief picture
 */
typedef struct _picture : _rstItem {
    QString imagePath;  // Picture image path
    QString fullText;   // Picture additional text
} rstPicture;

typedef struct _file : _rstItem {
    QTextDocument *doc;
} rstFile;

/**
 * @brief The MSArticle class. Article in rst document
 */
class MSArticle {
public:
    enum Type {
        TOCTREE,
        H1,
        H2,
        H3,
        H4
    };
public:
    explicit MSArticle(rstItem *data = nullptr, MSArticle* parent = nullptr, enum Type type = TOCTREE);
    ~MSArticle();
    void appendChild(MSArticle *item);
    MSArticle *child(int row);
    int childCount() const;
    int row() const;
    int columnCount() const;
    QVariant data(int column) const;
    rstItem *internalItem() const;
    MSArticle *parent() const;
    unsigned char level() const;
    MSArticle *findByPath(const QString &filePath, int lineNo = 0);
    MSArticle *findByName(const QString &name);
    bool hasChild(const QString &filePath) const;
    void removeChildren();
    void removeChild(MSArticle *child);
    void insertChild(int position, MSArticle *child);
    enum Type type() const;
    void setType(const enum Type &type);
    void fillFilePathSet(QSet<QString> &filePathSet) const;

private:
    rstItem *m_data;
    MSArticle *m_parent;
    QList<MSArticle *> m_childItems;
    enum Type m_type;
};

/**
 * @brief The PlainModel class
 */
class PlainModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit PlainModel(QObject *parent = nullptr);
    virtual ~PlainModel() override;

    void reset();
    virtual void appendItem(rstItem *item);
    virtual void removeItems(const QString &filePath);

    // QAbstractItemModel interface
public:
    virtual QModelIndex index(int row, int column,
                              const QModelIndex &parent) const override;
    virtual QModelIndex parent(const QModelIndex &child) const override;
    virtual int rowCount(const QModelIndex &parent) const override;
    virtual int columnCount(const QModelIndex &parent) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;
    virtual QVariant headerData(int section, Qt::Orientation orientation,
                                int role = Qt::DisplayRole) const override;
    virtual bool hasChildren(const QModelIndex &parent) const override;

    void setIcon(const QIcon &icon);
    virtual rstItem *itemByName(const QString &searchName) const;

protected:
    virtual QVariant valueForItem(rstItem *item, int column) const;

protected:
    QList<rstItem *> m_items;
    QIcon m_icon;
};

/**
 * @brief The FilesModel class
 */
class FilesModel : public PlainModel
{
    Q_OBJECT
public:
    explicit FilesModel(QObject *parent = nullptr);
    virtual ~FilesModel() override;

    void reset();
    rstFile *fileDoc(const QString& filePath) const;
    QString filePath(const QString& fileName) const;
    QString removeItem(const QString &filePath);
    void setHasChanges(const QString &filePath, bool hasChanges);
    QModelIndex indexByPath(const QString &filePath) const;
    bool hasChanges() const;
    bool saveAllDocs();
    bool saveDoc(const QString &filePath);

    // PlainModel
    virtual int columnCount(const QModelIndex &parent) const override;
    virtual void appendItem(rstItem *item) override;
private:
    void deleteDocumentPointers();
    void addNoDocumentItem();
};

/**
 * @brief The TreeModel class
 */
class MSProject;
class TreeModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit TreeModel(MSProject* project, QObject *parent = nullptr);
    virtual ~TreeModel() override;

    enum ResetType {
        FULL,
        BEGIN,
        END
    };

    void reset(enum ResetType type = FULL);
    MSArticle *rootItem();
    QModelIndex indexByPath(const QString &filePath, int lineNo = 0) const;
    QModelIndex indexByName(const QModelIndex &parent, const QString &name) const;
    MSArticle *itemByPath(const QString &filePath) const;
    QModelIndex index(MSArticle *item) const;

    void addChild(MSArticle *child);
    void removeChild(MSArticle *child);
    void insertChild(int position, MSArticle *child);

    QStringList expandedItems(const QTreeView *tv) const;

    // QAbstractItemModel interface
public:
    virtual QModelIndex index(int row, int column,
                              const QModelIndex &parent) const override;
    virtual QModelIndex parent(const QModelIndex &child) const override;
    virtual int rowCount(const QModelIndex &parent) const override;
    virtual int columnCount(const QModelIndex &parent) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;

private:
    MSArticle *m_rootItem;
    MSProject *m_project;
};


class MSProject
{
public:
    MSProject();
    ~MSProject();
    bool open(const QString &path);
    bool save();
    bool saveAs(const QString &path);
    bool hasChanges() const;
    QString name() const;
    QString filePath() const;
    TreeModel *articlesModel() const;
    PlainModel *bookmarksModel() const;
    PlainModel *picturesModel() const;
    FilesModel *filesModel() const;
    bool hasReference(const QString &name) const;
    bool hasFile(const QString &fileName) const;
    bool hasImage(const QString &name) const;
    bool hasTerm(const QString &name) const;
    QString getFileByName(const QString &name) const;
    QString getFileByName(const QString &dir, const QString &name) const;
    void addOpenedFile(const QString &filePath, QTextDocument *doc);
    QString removeOpenedFile(const QString &filePath);
    rstFile *openedFile(const QString &filePath) const;
    QString openedFilePath(const QString &fileName) const;
    void openedFileHasChanges(const QString &filePath, bool hasChanges);
    bool saveAllDocuments();
    bool saveDocument(const QString &filePath);
    rstItem *glossaryItem(const QString &term) const;
    void update(const QString &filePath);
    void addArticle(const QString &name, const QString &filePath,
                    const QString bookmark, MSArticle *toc,
                    const QString &afterFile);
    QMutex *mutex();

    //static
public:
    static QString filterLinkText(QString &text, int &pos, int &end);

private:
    void parseRstFile(const QString &rstFileStr, MSArticle *parent);
    void parseRstFile(QTextStream &in, const QString &rstFileStr,
                      MSArticle *parent, int position = -1);

protected:
    QString m_filePath;
    QString m_name;
    QString m_masterDoc;
    QStringList m_extensions;
    QString m_copyright, m_author, m_version, m_release, m_language;
    PlainModel *m_bookmarks, *m_pictures;
    FilesModel *m_files;
    TreeModel *m_articles;
    QMutex m_mutex;
};

#endif // MANUSCRIPT_PROJECT_H
