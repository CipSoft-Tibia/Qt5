// Copyright (C) 2018 Luca Beldi <v.ronin@yahoo.it>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qtransposeproxymodel.h"
#include <private/qtransposeproxymodel_p.h>
#include <QtCore/qlist.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qsize.h>
#include <QtCore/qmap.h>

QT_BEGIN_NAMESPACE

QModelIndex QTransposeProxyModelPrivate::uncheckedMapToSource(const QModelIndex &proxyIndex) const
{
    if (!model || !proxyIndex.isValid())
        return QModelIndex();
    Q_Q(const QTransposeProxyModel);
    return q->createSourceIndex(proxyIndex.column(), proxyIndex.row(), proxyIndex.internalPointer());
}

QModelIndex QTransposeProxyModelPrivate::uncheckedMapFromSource(const QModelIndex &sourceIndex) const
{
    if (!model || !sourceIndex.isValid())
        return QModelIndex();
    Q_Q(const QTransposeProxyModel);
    return q->createIndex(sourceIndex.column(), sourceIndex.row(), sourceIndex.internalPointer());
}

void QTransposeProxyModelPrivate::onLayoutChanged(const QList<QPersistentModelIndex> &parents, QAbstractItemModel::LayoutChangeHint hint)
{
    Q_Q(QTransposeProxyModel);
    Q_ASSERT(layoutChangeProxyIndexes.size() == layoutChangePersistentIndexes.size());
    QModelIndexList toList;
    toList.reserve(layoutChangePersistentIndexes.size());
    for (const QPersistentModelIndex &persistIdx : std::as_const(layoutChangePersistentIndexes))
        toList << q->mapFromSource(persistIdx);
    q->changePersistentIndexList(layoutChangeProxyIndexes, toList);
    layoutChangeProxyIndexes.clear();
    layoutChangePersistentIndexes.clear();
    QList<QPersistentModelIndex> proxyParents;
    proxyParents.reserve(parents.size());
    for (const QPersistentModelIndex &srcParent : parents)
        proxyParents << q->mapFromSource(srcParent);
    QAbstractItemModel::LayoutChangeHint proxyHint = QAbstractItemModel::NoLayoutChangeHint;
    if (hint == QAbstractItemModel::VerticalSortHint)
        proxyHint = QAbstractItemModel::HorizontalSortHint;
    else if (hint == QAbstractItemModel::HorizontalSortHint)
        proxyHint = QAbstractItemModel::VerticalSortHint;
    emit q->layoutChanged(proxyParents, proxyHint);
}

void QTransposeProxyModelPrivate::onLayoutAboutToBeChanged(const QList<QPersistentModelIndex> &sourceParents, QAbstractItemModel::LayoutChangeHint hint)
{
    Q_Q(QTransposeProxyModel);
    QList<QPersistentModelIndex> proxyParents;
    proxyParents.reserve(sourceParents.size());
    for (const QPersistentModelIndex &parent : sourceParents) {
        if (!parent.isValid()) {
            proxyParents << QPersistentModelIndex();
            continue;
        }
        const QModelIndex mappedParent = q->mapFromSource(parent);
        Q_ASSERT(mappedParent.isValid());
        proxyParents << mappedParent;
    }
    QAbstractItemModel::LayoutChangeHint proxyHint = QAbstractItemModel::NoLayoutChangeHint;
    if (hint == QAbstractItemModel::VerticalSortHint)
        proxyHint = QAbstractItemModel::HorizontalSortHint;
    else if (hint == QAbstractItemModel::HorizontalSortHint)
        proxyHint = QAbstractItemModel::VerticalSortHint;
    emit q->layoutAboutToBeChanged(proxyParents, proxyHint);
    const QModelIndexList proxyPersistentIndexes = q->persistentIndexList();
    layoutChangeProxyIndexes.clear();
    layoutChangePersistentIndexes.clear();
    layoutChangeProxyIndexes.reserve(proxyPersistentIndexes.size());
    layoutChangePersistentIndexes.reserve(proxyPersistentIndexes.size());
    for (const QModelIndex &proxyPersistentIndex : proxyPersistentIndexes) {
        layoutChangeProxyIndexes << proxyPersistentIndex;
        Q_ASSERT(proxyPersistentIndex.isValid());
        const QPersistentModelIndex srcPersistentIndex = q->mapToSource(proxyPersistentIndex);
        Q_ASSERT(srcPersistentIndex.isValid());
        layoutChangePersistentIndexes << srcPersistentIndex;
    }
}

void QTransposeProxyModelPrivate::onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight,
                                                const QList<int> &roles)
{
    Q_Q(QTransposeProxyModel);
    emit q->dataChanged(q->mapFromSource(topLeft), q->mapFromSource(bottomRight), roles);
}

void QTransposeProxyModelPrivate::onHeaderDataChanged(Qt::Orientation orientation, int first, int last)
{
    Q_Q(QTransposeProxyModel);
    emit q->headerDataChanged(orientation == Qt::Horizontal ? Qt::Vertical : Qt::Horizontal, first, last);
}

void QTransposeProxyModelPrivate::onColumnsAboutToBeInserted(const QModelIndex &parent, int first, int last)
{
    Q_Q(QTransposeProxyModel);
    q->beginInsertRows(q->mapFromSource(parent), first, last);
}

void QTransposeProxyModelPrivate::onColumnsAboutToBeRemoved(const QModelIndex &parent, int first, int last)
{
    Q_Q(QTransposeProxyModel);
    q->beginRemoveRows(q->mapFromSource(parent), first, last);
}

void QTransposeProxyModelPrivate::onColumnsAboutToBeMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd, const QModelIndex &destinationParent, int destinationColumn)
{
    Q_Q(QTransposeProxyModel);
    q->beginMoveRows(q->mapFromSource(sourceParent), sourceStart, sourceEnd, q->mapFromSource(destinationParent), destinationColumn);
}

void QTransposeProxyModelPrivate::onRowsAboutToBeInserted(const QModelIndex &parent, int first, int last)
{
    Q_Q(QTransposeProxyModel);
    q->beginInsertColumns(q->mapFromSource(parent), first, last);
}

void QTransposeProxyModelPrivate::onRowsAboutToBeRemoved(const QModelIndex &parent, int first, int last)
{
    Q_Q(QTransposeProxyModel);
    q->beginRemoveColumns(q->mapFromSource(parent), first, last);
}

void QTransposeProxyModelPrivate::onRowsAboutToBeMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd, const QModelIndex &destinationParent, int destinationRow)
{
    Q_Q(QTransposeProxyModel);
    q->beginMoveColumns(q->mapFromSource(sourceParent), sourceStart, sourceEnd, q->mapFromSource(destinationParent), destinationRow);
}

/*!
    \since 5.13
    \class QTransposeProxyModel
    \inmodule QtCore
    \brief This proxy transposes the source model.

    This model will make the rows of the source model become columns of the proxy model and vice-versa.

    If the model is a tree, the parents will be transposed as well. For example, if an index in the source model had parent `index(2,0)`, it will have parent `index(0,2)` in the proxy.
*/

/*!
    Constructs a new proxy model with the given \a parent.
*/
QTransposeProxyModel::QTransposeProxyModel(QObject* parent)
    : QAbstractProxyModel(*new QTransposeProxyModelPrivate, parent)
{}

/*!
    Destructs the proxy model.
*/
QTransposeProxyModel::~QTransposeProxyModel() = default;

/*!
    \internal
*/
QTransposeProxyModel::QTransposeProxyModel(QTransposeProxyModelPrivate &dd, QObject *parent)
    : QAbstractProxyModel(dd, parent)
{}

/*!
    \reimp
*/
void QTransposeProxyModel::setSourceModel(QAbstractItemModel* newSourceModel)
{
    Q_D(QTransposeProxyModel);
    if (newSourceModel == d->model)
        return;
    beginResetModel();
    if (d->model) {
        for (const QMetaObject::Connection& discIter : std::as_const(d->sourceConnections))
            disconnect(discIter);
    }
    d->sourceConnections.clear();
    QAbstractProxyModel::setSourceModel(newSourceModel);
    if (d->model) {
        using namespace std::placeholders;
        d->sourceConnections = QList<QMetaObject::Connection>{
            connect(d->model, &QAbstractItemModel::modelAboutToBeReset, this, &QTransposeProxyModel::beginResetModel),
            connect(d->model, &QAbstractItemModel::modelReset, this, &QTransposeProxyModel::endResetModel),
            connect(d->model, &QAbstractItemModel::dataChanged, this, std::bind(&QTransposeProxyModelPrivate::onDataChanged, d, _1, _2, _3)),
            connect(d->model, &QAbstractItemModel::headerDataChanged, this, std::bind(&QTransposeProxyModelPrivate::onHeaderDataChanged, d, _1, _2, _3)),
            connect(d->model, &QAbstractItemModel::columnsAboutToBeInserted, this, std::bind(&QTransposeProxyModelPrivate::onColumnsAboutToBeInserted, d, _1, _2, _3)),
            connect(d->model, &QAbstractItemModel::columnsAboutToBeMoved, this, std::bind(&QTransposeProxyModelPrivate::onColumnsAboutToBeMoved, d, _1, _2, _3, _4, _5)),
            connect(d->model, &QAbstractItemModel::columnsAboutToBeRemoved, this, std::bind(&QTransposeProxyModelPrivate::onColumnsAboutToBeRemoved, d, _1, _2, _3)),
            connect(d->model, &QAbstractItemModel::columnsInserted, this, &QTransposeProxyModel::endInsertRows),
            connect(d->model, &QAbstractItemModel::columnsRemoved, this, &QTransposeProxyModel::endRemoveRows),
            connect(d->model, &QAbstractItemModel::columnsMoved, this, &QTransposeProxyModel::endMoveRows),
            connect(d->model, &QAbstractItemModel::rowsAboutToBeInserted, this, std::bind(&QTransposeProxyModelPrivate::onRowsAboutToBeInserted, d, _1, _2, _3)),
            connect(d->model, &QAbstractItemModel::rowsAboutToBeMoved, this, std::bind(&QTransposeProxyModelPrivate::onRowsAboutToBeMoved, d, _1, _2, _3, _4, _5)),
            connect(d->model, &QAbstractItemModel::rowsAboutToBeRemoved, this, std::bind(&QTransposeProxyModelPrivate::onRowsAboutToBeRemoved, d, _1, _2, _3)),
            connect(d->model, &QAbstractItemModel::rowsInserted, this, &QTransposeProxyModel::endInsertColumns),
            connect(d->model, &QAbstractItemModel::rowsRemoved, this, &QTransposeProxyModel::endRemoveColumns),
            connect(d->model, &QAbstractItemModel::rowsMoved, this, &QTransposeProxyModel::endMoveColumns),
            connect(d->model, &QAbstractItemModel::layoutAboutToBeChanged, this, std::bind(&QTransposeProxyModelPrivate::onLayoutAboutToBeChanged, d, _1, _2)),
            connect(d->model, &QAbstractItemModel::layoutChanged, this, std::bind(&QTransposeProxyModelPrivate::onLayoutChanged, d, _1, _2))
        };
    }
    endResetModel();
}

/*!
    \reimp
*/
int QTransposeProxyModel::rowCount(const QModelIndex &parent) const
{
    Q_D(const QTransposeProxyModel);
    if (!d->model)
        return 0;
    Q_ASSERT(checkIndex(parent));
    return d->model->columnCount(mapToSource(parent));
}

/*!
    \reimp
*/
int QTransposeProxyModel::columnCount(const QModelIndex &parent) const
{
    Q_D(const QTransposeProxyModel);
    if (!d->model)
        return 0;
    Q_ASSERT(checkIndex(parent));
    return d->model->rowCount(mapToSource(parent));
}

/*!
    \reimp
*/
QVariant QTransposeProxyModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_D(const QTransposeProxyModel);
    if (!d->model)
        return QVariant();
    return d->model->headerData(section, orientation == Qt::Horizontal ? Qt::Vertical : Qt::Horizontal, role);
}

/*!
    \reimp
*/
bool QTransposeProxyModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role)
{
    Q_D(QTransposeProxyModel);
    if (!d->model)
        return false;
    return d->model->setHeaderData(section, orientation == Qt::Horizontal ? Qt::Vertical : Qt::Horizontal, value, role);
}

/*!
    \reimp
*/
bool QTransposeProxyModel::setItemData(const QModelIndex &index, const QMap<int, QVariant> &roles)
{
    Q_D(QTransposeProxyModel);
    Q_ASSERT(checkIndex(index));
    if (!d->model || !index.isValid())
        return false;
    return d->model->setItemData(mapToSource(index), roles);
}

/*!
    \reimp
*/
QSize QTransposeProxyModel::span(const QModelIndex &index) const
{
    Q_D(const QTransposeProxyModel);
    Q_ASSERT(checkIndex(index));
    if (!d->model || !index.isValid())
        return QSize();
    return d->model->span(mapToSource(index)).transposed();
}

/*!
    \reimp
*/
QMap<int, QVariant> QTransposeProxyModel::itemData(const QModelIndex &index) const
{
    Q_D(const QTransposeProxyModel);
    if (!d->model)
        return QMap<int, QVariant>();
    Q_ASSERT(checkIndex(index));
    return d->model->itemData(mapToSource(index));
}

/*!
    \reimp
*/
QModelIndex QTransposeProxyModel::mapFromSource(const QModelIndex &sourceIndex) const
{
    Q_D(const QTransposeProxyModel);
    if (!d->model || !sourceIndex.isValid())
        return QModelIndex();
    Q_ASSERT(d->model->checkIndex(sourceIndex));
    return d->uncheckedMapFromSource(sourceIndex);
}

/*!
    \reimp
*/
QModelIndex QTransposeProxyModel::mapToSource(const QModelIndex &proxyIndex) const
{
    Q_D(const QTransposeProxyModel);
    Q_ASSERT(checkIndex(proxyIndex));
    if (!d->model || !proxyIndex.isValid())
        return QModelIndex();
    return d->uncheckedMapToSource(proxyIndex);
}

/*!
    \reimp
*/
QModelIndex QTransposeProxyModel::parent(const QModelIndex &index) const
{
    Q_D(const QTransposeProxyModel);
    Q_ASSERT(checkIndex(index, CheckIndexOption::DoNotUseParent));
    if (!d->model || !index.isValid())
        return QModelIndex();
    return d->uncheckedMapFromSource(d->uncheckedMapToSource(index).parent());
}

/*!
    \reimp
*/
QModelIndex QTransposeProxyModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_D(const QTransposeProxyModel);
    Q_ASSERT(checkIndex(parent));
    if (!d->model)
        return QModelIndex();
    return mapFromSource(d->model->index(column, row, mapToSource(parent)));
}

/*!
    \reimp
*/
bool QTransposeProxyModel::insertRows(int row, int count, const QModelIndex &parent)
{
    Q_D(QTransposeProxyModel);
    Q_ASSERT(checkIndex(parent));
    if (!d->model)
        return false;
    return d->model->insertColumns(row, count, mapToSource(parent));
}

/*!
    \reimp
*/
bool QTransposeProxyModel::removeRows(int row, int count, const QModelIndex &parent)
{
    Q_D(QTransposeProxyModel);
    Q_ASSERT(checkIndex(parent));
    if (!d->model)
        return false;
    return d->model->removeColumns(row, count, mapToSource(parent));
}

/*!
    \reimp
*/
bool QTransposeProxyModel::moveRows(const QModelIndex &sourceParent, int sourceRow, int count, const QModelIndex &destinationParent, int destinationChild)
{
    Q_D(QTransposeProxyModel);
    Q_ASSERT(checkIndex(sourceParent));
    Q_ASSERT(checkIndex(destinationParent));
    if (!d->model)
        return false;
    return d->model->moveColumns(mapToSource(sourceParent), sourceRow, count, mapToSource(destinationParent), destinationChild);
}

/*!
    \reimp
*/
bool QTransposeProxyModel::insertColumns(int column, int count, const QModelIndex &parent)
{
    Q_D(QTransposeProxyModel);
    Q_ASSERT(checkIndex(parent));
    if (!d->model)
        return false;
    return d->model->insertRows(column, count, mapToSource(parent));
}

/*!
    \reimp
*/
bool QTransposeProxyModel::removeColumns(int column, int count, const QModelIndex &parent)
{
    Q_D(QTransposeProxyModel);
    Q_ASSERT(checkIndex(parent));
    if (!d->model)
        return false;
    return d->model->removeRows(column, count, mapToSource(parent));
}

/*!
    \reimp
*/
bool QTransposeProxyModel::moveColumns(const QModelIndex &sourceParent, int sourceRow, int count, const QModelIndex &destinationParent, int destinationChild)
{
    Q_D(QTransposeProxyModel);
    Q_ASSERT(checkIndex(sourceParent));
    Q_ASSERT(checkIndex(destinationParent));
    if (!d->model)
        return false;
    return d->model->moveRows(mapToSource(sourceParent), sourceRow, count, mapToSource(destinationParent), destinationChild);
}

/*!
    \reimp
    This method will perform no action. Use a QSortFilterProxyModel on top of this one if you require sorting.
*/
void QTransposeProxyModel::sort(int column, Qt::SortOrder order)
{
    Q_UNUSED(column);
    Q_UNUSED(order);
    return;
}

QT_END_NAMESPACE

#include "moc_qtransposeproxymodel.cpp"
