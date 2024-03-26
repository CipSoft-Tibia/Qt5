// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "bookmarkfiltermodel.h"

#include "bookmarkitem.h"
#include "bookmarkmodel.h"

BookmarkFilterModel::BookmarkFilterModel(QObject *parent)
    : QAbstractProxyModel(parent)
{
}

void BookmarkFilterModel::setSourceModel(QAbstractItemModel *_sourceModel)
{
    beginResetModel();

    if (sourceModel) {
        disconnect(sourceModel, &QAbstractItemModel::dataChanged,
                this, &BookmarkFilterModel::changed);
        disconnect(sourceModel, &QAbstractItemModel::rowsInserted,
                this, &BookmarkFilterModel::rowsInserted);
        disconnect(sourceModel, &QAbstractItemModel::rowsAboutToBeRemoved,
                this, &BookmarkFilterModel::rowsAboutToBeRemoved);
        disconnect(sourceModel, &QAbstractItemModel::rowsRemoved,
                this, &BookmarkFilterModel::rowsRemoved);
        disconnect(sourceModel, &QAbstractItemModel::layoutAboutToBeChanged,
                this, &BookmarkFilterModel::layoutAboutToBeChanged);
        disconnect(sourceModel, &QAbstractItemModel::layoutChanged,
                this, &BookmarkFilterModel::layoutChanged);
        disconnect(sourceModel, &QAbstractItemModel::modelAboutToBeReset,
                this, &BookmarkFilterModel::modelAboutToBeReset);
        disconnect(sourceModel, &QAbstractItemModel::modelReset,
                this, &BookmarkFilterModel::modelReset);
    }

    sourceModel = qobject_cast<BookmarkModel*> (_sourceModel);
    QAbstractProxyModel::setSourceModel(sourceModel);

    if (sourceModel) {
        connect(sourceModel, &QAbstractItemModel::dataChanged,
                this, &BookmarkFilterModel::changed);
        connect(sourceModel, &QAbstractItemModel::rowsInserted,
                this, &BookmarkFilterModel::rowsInserted);
        connect(sourceModel, &QAbstractItemModel::rowsAboutToBeRemoved,
                this, &BookmarkFilterModel::rowsAboutToBeRemoved);
        connect(sourceModel, &QAbstractItemModel::rowsRemoved,
                this, &BookmarkFilterModel::rowsRemoved);
        connect(sourceModel, &QAbstractItemModel::layoutAboutToBeChanged,
                this, &BookmarkFilterModel::layoutAboutToBeChanged);
        connect(sourceModel, &QAbstractItemModel::layoutChanged,
                this, &BookmarkFilterModel::layoutChanged);
        connect(sourceModel, &QAbstractItemModel::modelAboutToBeReset,
                this, &BookmarkFilterModel::modelAboutToBeReset);
        connect(sourceModel, &QAbstractItemModel::modelReset,
                this, &BookmarkFilterModel::modelReset);

        setupCache(sourceModel->index(0, 0, QModelIndex()).parent());
    }
    endResetModel();
}

int BookmarkFilterModel::rowCount(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return cache.size();
}

int BookmarkFilterModel::columnCount(const QModelIndex &index) const
{
    Q_UNUSED(index);
    if (sourceModel)
        return sourceModel->columnCount();
    return 0;
}

QModelIndex BookmarkFilterModel::mapToSource(const QModelIndex &proxyIndex) const
{
    const int row = proxyIndex.row();
    if (proxyIndex.isValid() && row >= 0 && row < cache.size())
        return cache[row];
    return QModelIndex();
}

QModelIndex BookmarkFilterModel::mapFromSource(const QModelIndex &sourceIndex) const
{
    return index(cache.indexOf(sourceIndex), 0, QModelIndex());
}

QModelIndex BookmarkFilterModel::parent(const QModelIndex &child) const
{
    Q_UNUSED(child);
    return QModelIndex();
}

QModelIndex BookmarkFilterModel::index(int row, int column,
    const QModelIndex &index) const
{
    Q_UNUSED(index);
    if (row < 0 || column < 0 || cache.size() <= row
        || !sourceModel || sourceModel->columnCount() <= column) {
        return QModelIndex();
    }
    return createIndex(row, 0);
}

Qt::DropActions BookmarkFilterModel::supportedDropActions () const
{
    if (sourceModel)
        return sourceModel->supportedDropActions();
    return Qt::IgnoreAction;
}

Qt::ItemFlags BookmarkFilterModel::flags(const QModelIndex &index) const
{
    if (sourceModel)
        return sourceModel->flags(index);
    return Qt::NoItemFlags;
}

QVariant BookmarkFilterModel::data(const QModelIndex &index, int role) const
{
    if (sourceModel)
        return sourceModel->data(mapToSource(index), role);
    return QVariant();
}

bool BookmarkFilterModel::setData(const QModelIndex &index, const QVariant &value,
    int role)
{
    if (sourceModel)
        return sourceModel->setData(mapToSource(index), value, role);
    return false;
}

void BookmarkFilterModel::filterBookmarks()
{
    if (sourceModel) {
        beginResetModel();
        hideBookmarks = true;
        setupCache(sourceModel->index(0, 0, QModelIndex()).parent());
        endResetModel();
    }
}

void BookmarkFilterModel::filterBookmarkFolders()
{
    if (sourceModel) {
        beginResetModel();
        hideBookmarks = false;
        setupCache(sourceModel->index(0, 0, QModelIndex()).parent());
        endResetModel();
    }
}

void BookmarkFilterModel::changed(const QModelIndex &topLeft,
    const QModelIndex &bottomRight)
{
    emit dataChanged(mapFromSource(topLeft), mapFromSource(bottomRight));
}

void BookmarkFilterModel::rowsInserted(const QModelIndex &parent, int start,
    int end)
{
    if (!sourceModel)
        return;

    QModelIndex cachePrevious = parent;
    if (BookmarkItem *parentItem = sourceModel->itemFromIndex(parent)) {
        BookmarkItem *newItem = parentItem->child(start);

        // iterate over tree hirarchie to find the previous folder
        for (int i = 0; i < parentItem->childCount(); ++i) {
            if (BookmarkItem *child = parentItem->child(i)) {
                const QModelIndex &tmp = sourceModel->indexFromItem(child);
                if (tmp.data(UserRoleFolder).toBool() && child != newItem)
                    cachePrevious = tmp;
            }
        }

        const QModelIndex &newIndex = sourceModel->indexFromItem(newItem);
        const bool isFolder = newIndex.data(UserRoleFolder).toBool();
        if ((isFolder && hideBookmarks) || (!isFolder && !hideBookmarks)) {
            beginInsertRows(mapFromSource(parent), start, end);
            const int index = cache.indexOf(cachePrevious) + 1;
            if (cache.value(index, QPersistentModelIndex()) != newIndex)
                cache.insert(index, newIndex);
            endInsertRows();
        }
    }
}

void BookmarkFilterModel::rowsAboutToBeRemoved(const QModelIndex &parent,
    int start, int end)
{
    if (!sourceModel)
        return;

    if (BookmarkItem *parentItem = sourceModel->itemFromIndex(parent)) {
        if (BookmarkItem *child = parentItem->child(start)) {
            indexToRemove = sourceModel->indexFromItem(child);
            if (cache.contains(indexToRemove))
                beginRemoveRows(mapFromSource(parent), start, end);
        }
    }
}

void BookmarkFilterModel::rowsRemoved(const QModelIndex &/*parent*/, int, int)
{
    if (cache.contains(indexToRemove)) {
        cache.removeAll(indexToRemove);
        endRemoveRows();
    }
}

void BookmarkFilterModel::layoutAboutToBeChanged()
{
    // TODO: ???
}

void BookmarkFilterModel::layoutChanged()
{
    // TODO: ???
}

void BookmarkFilterModel::modelAboutToBeReset()
{
    beginResetModel();
}

void BookmarkFilterModel::modelReset()
{
    if (sourceModel)
        setupCache(sourceModel->index(0, 0, QModelIndex()).parent());
    endResetModel();
}

void BookmarkFilterModel::setupCache(const QModelIndex &parent)
{
    cache.clear();
    for (int i = 0; i < sourceModel->rowCount(parent); ++i)
        collectItems(sourceModel->index(i, 0, parent));
}

void BookmarkFilterModel::collectItems(const QModelIndex &parent)
{
    if (parent.isValid()) {
        bool isFolder = sourceModel->data(parent, UserRoleFolder).toBool();
        if ((isFolder && hideBookmarks) || (!isFolder && !hideBookmarks))
            cache.append(parent);

        if (sourceModel->hasChildren(parent)) {
            for (int i = 0; i < sourceModel->rowCount(parent); ++i)
                collectItems(sourceModel->index(i, 0, parent));
        }
    }
}

// -- BookmarkTreeModel

BookmarkTreeModel::BookmarkTreeModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

int BookmarkTreeModel::columnCount(const QModelIndex &parent) const
{
    return qMin(1, QSortFilterProxyModel::columnCount(parent));
}

bool BookmarkTreeModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    Q_UNUSED(row);
    BookmarkModel *model = qobject_cast<BookmarkModel*> (sourceModel());
    if (model->rowCount(parent) > 0
        && model->data(model->index(row, 0, parent), UserRoleFolder).toBool())
        return true;
    return false;
}
