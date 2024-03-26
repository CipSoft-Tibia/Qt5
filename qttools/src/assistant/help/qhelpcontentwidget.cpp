// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qhelpcontentwidget.h"
#include "qhelpenginecore.h"
#include "qhelpengine_p.h"
#include "qhelpcollectionhandler_p.h"

#include <QDir>
#include <QtCore/QStack>
#include <QtCore/QThread>
#include <QtCore/QMutex>
#include <QtWidgets/QHeaderView>

QT_BEGIN_NAMESPACE

class QHelpContentItemPrivate
{
public:
    QHelpContentItemPrivate(const QString &t, const QUrl &l, QHelpContentItem *p)
        : parent(p),
          title(t),
          link(l)
    {
    }

    void appendChild(QHelpContentItem *item) { childItems.append(item); }

    QList<QHelpContentItem*> childItems;
    QHelpContentItem *parent;
    QString title;
    QUrl link;
};

class QHelpContentProvider : public QThread
{
    Q_OBJECT
public:
    QHelpContentProvider(QHelpEnginePrivate *helpEngine);
    ~QHelpContentProvider() override;
    void collectContents(const QString &customFilterName);
    void stopCollecting();
    QHelpContentItem *takeContentItem();

private:
    void run() override;

    QHelpEnginePrivate *m_helpEngine;
    QString m_currentFilter;
    QStringList m_filterAttributes;
    QString m_collectionFile;
    QHelpContentItem *m_rootItem = nullptr;
    QMutex m_mutex;
    bool m_usesFilterEngine = false;
    bool m_abort = false;
};

class QHelpContentModelPrivate
{
public:
    QHelpContentItem *rootItem = nullptr;
    QHelpContentProvider *qhelpContentProvider;
};



/*!
    \class QHelpContentItem
    \inmodule QtHelp
    \brief The QHelpContentItem class provides an item for use with QHelpContentModel.
    \since 4.4
*/

QHelpContentItem::QHelpContentItem(const QString &name, const QUrl &link, QHelpContentItem *parent)
{
    d = new QHelpContentItemPrivate(name, link, parent);
}

/*!
    Destroys the help content item.
*/
QHelpContentItem::~QHelpContentItem()
{
    qDeleteAll(d->childItems);
    delete d;
}

/*!
    Returns the child of the content item in the give \a row.

    \sa parent()
*/
QHelpContentItem *QHelpContentItem::child(int row) const
{
    return d->childItems.value(row);
}

/*!
    Returns the number of child items.
*/
int QHelpContentItem::childCount() const
{
    return d->childItems.size();
}

/*!
    Returns the row of this item from its parents view.
*/
int QHelpContentItem::row() const
{
    if (d->parent)
        return d->parent->d->childItems.indexOf(const_cast<QHelpContentItem*>(this));
    return 0;
}

/*!
    Returns the title of the content item.
*/
QString QHelpContentItem::title() const
{
    return d->title;
}

/*!
    Returns the URL of this content item.
*/
QUrl QHelpContentItem::url() const
{
    return d->link;
}

/*!
    Returns the parent content item.
*/
QHelpContentItem *QHelpContentItem::parent() const
{
    return d->parent;
}

/*!
    Returns the position of a given \a child.
*/
int QHelpContentItem::childPosition(QHelpContentItem *child) const
{
    return d->childItems.indexOf(child);
}



QHelpContentProvider::QHelpContentProvider(QHelpEnginePrivate *helpEngine)
    : QThread(helpEngine)
{
    m_helpEngine = helpEngine;
}

QHelpContentProvider::~QHelpContentProvider()
{
    stopCollecting();
}

void QHelpContentProvider::collectContents(const QString &customFilterName)
{
    m_mutex.lock();
    m_currentFilter = customFilterName;
    m_filterAttributes = m_helpEngine->q->filterAttributes(customFilterName);
    m_collectionFile = m_helpEngine->collectionHandler->collectionFile();
    m_usesFilterEngine = m_helpEngine->usesFilterEngine;
    m_mutex.unlock();

    if (isRunning())
        stopCollecting();
    start(LowPriority);
}

void QHelpContentProvider::stopCollecting()
{
    if (isRunning()) {
        m_mutex.lock();
        m_abort = true;
        m_mutex.unlock();
        wait();
        // we need to force-set m_abort to false, because the thread might either have
        // finished between the isRunning() check and the "m_abort = true" above, or the
        // isRunning() check might already happen after the "m_abort = false" in the run() method,
        // either way never resetting m_abort to false from within the run() method
        m_abort = false;
    }
    delete m_rootItem;
    m_rootItem = nullptr;
}

QHelpContentItem *QHelpContentProvider::takeContentItem()
{
    QMutexLocker locker(&m_mutex);
    QHelpContentItem *content = m_rootItem;
    m_rootItem = nullptr;
    return content;
}


static QUrl constructUrl(const QString &namespaceName,
                         const QString &folderName,
                         const QString &relativePath)
{
    const int idx = relativePath.indexOf(QLatin1Char('#'));
    const QString &rp = idx < 0 ? relativePath : relativePath.left(idx);
    const QString anchor = idx < 0 ? QString() : relativePath.mid(idx + 1);
    return QHelpCollectionHandler::buildQUrl(namespaceName, folderName, rp, anchor);
}

void QHelpContentProvider::run()
{
    m_mutex.lock();
    const QString currentFilter = m_currentFilter;
    const QStringList attributes = m_filterAttributes;
    const QString collectionFile = m_collectionFile;
    const bool usesFilterEngine = m_usesFilterEngine;
    delete m_rootItem;
    m_rootItem = nullptr;
    m_mutex.unlock();

    if (collectionFile.isEmpty())
        return;

    QHelpCollectionHandler collectionHandler(collectionFile);
    if (!collectionHandler.openCollectionFile())
        return;

    QString title;
    QString link;
    int depth = 0;
    QHelpContentItem *item = nullptr;
    QHelpContentItem * const rootItem = new QHelpContentItem(QString(), QString(), nullptr);

    const QList<QHelpCollectionHandler::ContentsData> result = usesFilterEngine
            ? collectionHandler.contentsForFilter(currentFilter)
            : collectionHandler.contentsForFilter(attributes);

    for (const auto &contentsData : result) {
        m_mutex.lock();
        if (m_abort) {
            delete rootItem;
            m_abort = false;
            m_mutex.unlock();
            return;
        }
        m_mutex.unlock();

        const QString namespaceName = contentsData.namespaceName;
        const QString folderName = contentsData.folderName;
        for (const QByteArray &contents : contentsData.contentsList)  {
            if (contents.size() < 1)
                continue;

            int _depth = 0;
            bool _root = false;
            QStack<QHelpContentItem*> stack;

            QDataStream s(contents);
            for (;;) {
                s >> depth;
                s >> link;
                s >> title;
                if (title.isEmpty())
                    break;
                const QUrl url = constructUrl(namespaceName, folderName, link);
CHECK_DEPTH:
                if (depth == 0) {
                    m_mutex.lock();
                    item = new QHelpContentItem(title, url, rootItem);
                    rootItem->d->appendChild(item);
                    m_mutex.unlock();
                    stack.push(item);
                    _depth = 1;
                    _root = true;
                } else {
                    if (depth > _depth && _root) {
                        _depth = depth;
                        stack.push(item);
                    }
                    if (depth == _depth) {
                        item = new QHelpContentItem(title, url, stack.top());
                        stack.top()->d->appendChild(item);
                    } else if (depth < _depth) {
                        stack.pop();
                        --_depth;
                        goto CHECK_DEPTH;
                    }
                }
            }
        }
    }

    m_mutex.lock();
    m_rootItem = rootItem;
    m_abort = false;
    m_mutex.unlock();
}

/*!
    \class QHelpContentModel
    \inmodule QtHelp
    \brief The QHelpContentModel class provides a model that supplies content to views.
    \since 4.4
*/

/*!
    \fn void QHelpContentModel::contentsCreationStarted()

    This signal is emitted when the creation of the contents has
    started. The current contents are invalid from this point on
    until the signal contentsCreated() is emitted.

    \sa isCreatingContents()
*/

/*!
    \fn void QHelpContentModel::contentsCreated()

    This signal is emitted when the contents have been created.
*/

QHelpContentModel::QHelpContentModel(QHelpEnginePrivate *helpEngine)
    : QAbstractItemModel(helpEngine)
{
    d = new QHelpContentModelPrivate();
    d->qhelpContentProvider = new QHelpContentProvider(helpEngine);

    connect(d->qhelpContentProvider, &QThread::finished,
            this, &QHelpContentModel::insertContents);
}

/*!
    Destroys the help content model.
*/
QHelpContentModel::~QHelpContentModel()
{
    delete d->rootItem;
    delete d;
}

/*!
    Creates new contents by querying the help system
    for contents specified for the \a customFilterName.
*/
void QHelpContentModel::createContents(const QString &customFilterName)
{
    const bool running = d->qhelpContentProvider->isRunning();
    d->qhelpContentProvider->collectContents(customFilterName);
    if (running)
        return;

    if (d->rootItem) {
        beginResetModel();
        delete d->rootItem;
        d->rootItem = nullptr;
        endResetModel();
    }
    emit contentsCreationStarted();
}

void QHelpContentModel::insertContents()
{
    if (d->qhelpContentProvider->isRunning())
        return;

    QHelpContentItem * const newRootItem = d->qhelpContentProvider->takeContentItem();
    if (!newRootItem)
        return;
    beginResetModel();
    delete d->rootItem;
    d->rootItem = newRootItem;
    endResetModel();
    emit contentsCreated();
}

/*!
    Returns true if the contents are currently rebuilt, otherwise
    false.
*/
bool QHelpContentModel::isCreatingContents() const
{
    return d->qhelpContentProvider->isRunning();
}

/*!
    Returns the help content item at the model index position
    \a index.
*/
QHelpContentItem *QHelpContentModel::contentItemAt(const QModelIndex &index) const
{
    if (index.isValid())
        return static_cast<QHelpContentItem*>(index.internalPointer());
    else
        return d->rootItem;
}

/*!
    Returns the index of the item in the model specified by
    the given \a row, \a column and \a parent index.
*/
QModelIndex QHelpContentModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!d->rootItem)
        return QModelIndex();

    QHelpContentItem *parentItem = contentItemAt(parent);
    QHelpContentItem *item = parentItem->child(row);
    if (!item)
        return QModelIndex();
    return createIndex(row, column, item);
}

/*!
    Returns the parent of the model item with the given
    \a index, or QModelIndex() if it has no parent.
*/
QModelIndex QHelpContentModel::parent(const QModelIndex &index) const
{
    QHelpContentItem *item = contentItemAt(index);
    if (!item)
        return QModelIndex();

    QHelpContentItem *parentItem = static_cast<QHelpContentItem*>(item->parent());
    if (!parentItem)
        return QModelIndex();

    QHelpContentItem *grandparentItem = static_cast<QHelpContentItem*>(parentItem->parent());
    if (!grandparentItem)
        return QModelIndex();

    int row = grandparentItem->childPosition(parentItem);
    return createIndex(row, index.column(), parentItem);
}

/*!
    Returns the number of rows under the given \a parent.
*/
int QHelpContentModel::rowCount(const QModelIndex &parent) const
{
    QHelpContentItem *parentItem = contentItemAt(parent);
    if (!parentItem)
        return 0;
    return parentItem->childCount();
}

/*!
    Returns the number of columns under the given \a parent. Currently returns always 1.
*/
int QHelpContentModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return 1;
}

/*!
    Returns the data stored under the given \a role for
    the item referred to by the \a index.
*/
QVariant QHelpContentModel::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    QHelpContentItem *item = contentItemAt(index);
    if (!item)
        return QVariant();
    return item->title();
}



/*!
    \class QHelpContentWidget
    \inmodule QtHelp
    \brief The QHelpContentWidget class provides a tree view for displaying help content model items.
    \since 4.4
*/

/*!
    \fn void QHelpContentWidget::linkActivated(const QUrl &link)

    This signal is emitted when a content item is activated and
    its associated \a link should be shown.
*/

QHelpContentWidget::QHelpContentWidget()
    : QTreeView(nullptr)
{
    header()->hide();
    setUniformRowHeights(true);
    connect(this, &QAbstractItemView::activated,
            this, &QHelpContentWidget::showLink);
}

/*!
    Returns the index of the content item with the \a link.
    An invalid index is returned if no such an item exists.
*/
QModelIndex QHelpContentWidget::indexOf(const QUrl &link)
{
    QHelpContentModel *contentModel = qobject_cast<QHelpContentModel*>(model());
    if (!contentModel || link.scheme() != QLatin1String("qthelp"))
        return QModelIndex();

    m_syncIndex = QModelIndex();
    for (int i = 0; i < contentModel->rowCount(); ++i) {
        QHelpContentItem *itm = contentModel->contentItemAt(contentModel->index(i, 0));
        if (itm && itm->url().host() == link.host()) {
            if (searchContentItem(contentModel, contentModel->index(i, 0), QDir::cleanPath(link.path())))
                return m_syncIndex;
        }
    }
    return QModelIndex();
}

bool QHelpContentWidget::searchContentItem(QHelpContentModel *model, const QModelIndex &parent,
    const QString &cleanPath)
{
    QHelpContentItem *parentItem = model->contentItemAt(parent);
    if (!parentItem)
        return false;

    if (QDir::cleanPath(parentItem->url().path()) == cleanPath) {
        m_syncIndex = parent;
        return true;
    }

    for (int i = 0; i < parentItem->childCount(); ++i) {
        if (searchContentItem(model, model->index(i, 0, parent), cleanPath))
            return true;
    }
    return false;
}

void QHelpContentWidget::showLink(const QModelIndex &index)
{
    QHelpContentModel *contentModel = qobject_cast<QHelpContentModel*>(model());
    if (!contentModel)
        return;

    QHelpContentItem *item = contentModel->contentItemAt(index);
    if (!item)
        return;
    QUrl url = item->url();
    if (url.isValid())
        emit linkActivated(url);
}

QT_END_NAMESPACE

#include "qhelpcontentwidget.moc"
