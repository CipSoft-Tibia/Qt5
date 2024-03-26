// Copyright (C) 2011 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Stephen Kelly <stephen.kelly@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qidentityproxymodel.h"
#include "qidentityproxymodel_p.h"
#include "qitemselectionmodel.h"
#include <private/qabstractproxymodel_p.h>

QT_BEGIN_NAMESPACE

/*!
    \since 4.8
    \class QIdentityProxyModel
    \inmodule QtCore
    \brief The QIdentityProxyModel class proxies its source model unmodified.

    \ingroup model-view

    QIdentityProxyModel can be used to forward the structure of a source model exactly, with no sorting, filtering or other transformation.
    This is similar in concept to an identity matrix where A.I = A.

    Because it does no sorting or filtering, this class is most suitable to proxy models which transform the data() of the source model.
    For example, a proxy model could be created to define the font used, or the background colour, or the tooltip etc. This removes the
    need to implement all data handling in the same class that creates the structure of the model, and can also be used to create
    re-usable components.

    This also provides a way to change the data in the case where a source model is supplied by a third party which cannot be modified.

    \snippet code/src_gui_itemviews_qidentityproxymodel.cpp 0

    \sa QAbstractProxyModel, {Model/View Programming}, QAbstractItemModel

*/

/*!
    Constructs an identity model with the given \a parent.
*/
QIdentityProxyModel::QIdentityProxyModel(QObject* parent)
  : QAbstractProxyModel(*new QIdentityProxyModelPrivate, parent)
{

}

/*!
    \internal
 */
QIdentityProxyModel::QIdentityProxyModel(QIdentityProxyModelPrivate &dd, QObject* parent)
  : QAbstractProxyModel(dd, parent)
{

}

/*!
    Destroys this identity model.
*/
QIdentityProxyModel::~QIdentityProxyModel()
{
}

/*!
    \reimp
 */
int QIdentityProxyModel::columnCount(const QModelIndex& parent) const
{
    Q_ASSERT(parent.isValid() ? parent.model() == this : true);
    Q_D(const QIdentityProxyModel);
    return d->model->columnCount(mapToSource(parent));
}

/*!
    \reimp
 */
bool QIdentityProxyModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
    Q_ASSERT(parent.isValid() ? parent.model() == this : true);
    Q_D(QIdentityProxyModel);
    return d->model->dropMimeData(data, action, row, column, mapToSource(parent));
}

/*!
    \reimp
 */
QModelIndex QIdentityProxyModel::index(int row, int column, const QModelIndex& parent) const
{
    Q_ASSERT(parent.isValid() ? parent.model() == this : true);
    Q_D(const QIdentityProxyModel);
    const QModelIndex sourceParent = mapToSource(parent);
    const QModelIndex sourceIndex = d->model->index(row, column, sourceParent);
    return mapFromSource(sourceIndex);
}

/*!
    \reimp
 */
QModelIndex QIdentityProxyModel::sibling(int row, int column, const QModelIndex &idx) const
{
    Q_D(const QIdentityProxyModel);
    return mapFromSource(d->model->sibling(row, column, mapToSource(idx)));
}

/*!
    \reimp
 */
bool QIdentityProxyModel::insertColumns(int column, int count, const QModelIndex& parent)
{
    Q_ASSERT(parent.isValid() ? parent.model() == this : true);
    Q_D(QIdentityProxyModel);
    return d->model->insertColumns(column, count, mapToSource(parent));
}

/*!
    \reimp
 */
bool QIdentityProxyModel::insertRows(int row, int count, const QModelIndex& parent)
{
    Q_ASSERT(parent.isValid() ? parent.model() == this : true);
    Q_D(QIdentityProxyModel);
    return d->model->insertRows(row, count, mapToSource(parent));
}

/*!
    \reimp
 */
QModelIndex QIdentityProxyModel::mapFromSource(const QModelIndex& sourceIndex) const
{
    Q_D(const QIdentityProxyModel);
    if (!d->model || !sourceIndex.isValid())
        return QModelIndex();

    Q_ASSERT(sourceIndex.model() == d->model);
    return createIndex(sourceIndex.row(), sourceIndex.column(), sourceIndex.internalPointer());
}

/*!
    \reimp
 */
QItemSelection QIdentityProxyModel::mapSelectionFromSource(const QItemSelection& selection) const
{
    Q_D(const QIdentityProxyModel);
    QItemSelection proxySelection;

    if (!d->model)
        return proxySelection;

    QItemSelection::const_iterator it = selection.constBegin();
    const QItemSelection::const_iterator end = selection.constEnd();
    proxySelection.reserve(selection.size());
    for ( ; it != end; ++it) {
        Q_ASSERT(it->model() == d->model);
        const QItemSelectionRange range(mapFromSource(it->topLeft()), mapFromSource(it->bottomRight()));
        proxySelection.append(range);
    }

    return proxySelection;
}

/*!
    \reimp
 */
QItemSelection QIdentityProxyModel::mapSelectionToSource(const QItemSelection& selection) const
{
    Q_D(const QIdentityProxyModel);
    QItemSelection sourceSelection;

    if (!d->model)
        return sourceSelection;

    QItemSelection::const_iterator it = selection.constBegin();
    const QItemSelection::const_iterator end = selection.constEnd();
    sourceSelection.reserve(selection.size());
    for ( ; it != end; ++it) {
        Q_ASSERT(it->model() == this);
        const QItemSelectionRange range(mapToSource(it->topLeft()), mapToSource(it->bottomRight()));
        sourceSelection.append(range);
    }

    return sourceSelection;
}

/*!
    \reimp
 */
QModelIndex QIdentityProxyModel::mapToSource(const QModelIndex& proxyIndex) const
{
    Q_D(const QIdentityProxyModel);
    if (!d->model || !proxyIndex.isValid())
        return QModelIndex();
    Q_ASSERT(proxyIndex.model() == this);
    return createSourceIndex(proxyIndex.row(), proxyIndex.column(), proxyIndex.internalPointer());
}

/*!
    \reimp
 */
QModelIndexList QIdentityProxyModel::match(const QModelIndex& start, int role, const QVariant& value, int hits, Qt::MatchFlags flags) const
{
    Q_D(const QIdentityProxyModel);
    Q_ASSERT(start.isValid() ? start.model() == this : true);
    if (!d->model)
        return QModelIndexList();

    const QModelIndexList sourceList = d->model->match(mapToSource(start), role, value, hits, flags);
    QModelIndexList::const_iterator it = sourceList.constBegin();
    const QModelIndexList::const_iterator end = sourceList.constEnd();
    QModelIndexList proxyList;
    proxyList.reserve(sourceList.size());
    for ( ; it != end; ++it)
        proxyList.append(mapFromSource(*it));
    return proxyList;
}

/*!
    \reimp
 */
QModelIndex QIdentityProxyModel::parent(const QModelIndex& child) const
{
    Q_ASSERT(child.isValid() ? child.model() == this : true);
    const QModelIndex sourceIndex = mapToSource(child);
    const QModelIndex sourceParent = sourceIndex.parent();
    return mapFromSource(sourceParent);
}

/*!
    \reimp
 */
bool QIdentityProxyModel::removeColumns(int column, int count, const QModelIndex& parent)
{
    Q_ASSERT(parent.isValid() ? parent.model() == this : true);
    Q_D(QIdentityProxyModel);
    return d->model->removeColumns(column, count, mapToSource(parent));
}

/*!
    \reimp
 */
bool QIdentityProxyModel::removeRows(int row, int count, const QModelIndex& parent)
{
    Q_ASSERT(parent.isValid() ? parent.model() == this : true);
    Q_D(QIdentityProxyModel);
    return d->model->removeRows(row, count, mapToSource(parent));
}

/*!
    \reimp
    \since 5.15
 */
bool QIdentityProxyModel::moveRows(const QModelIndex &sourceParent, int sourceRow, int count, const QModelIndex &destinationParent, int destinationChild)
{
    Q_ASSERT(sourceParent.isValid() ? sourceParent.model() == this : true);
    Q_ASSERT(destinationParent.isValid() ? destinationParent.model() == this : true);
    Q_D(QIdentityProxyModel);
    return d->model->moveRows(mapToSource(sourceParent), sourceRow, count, mapToSource(destinationParent), destinationChild);
}

/*!
    \reimp
    \since 5.15
 */
bool QIdentityProxyModel::moveColumns(const QModelIndex &sourceParent, int sourceColumn, int count, const QModelIndex &destinationParent, int destinationChild)
{
    Q_ASSERT(sourceParent.isValid() ? sourceParent.model() == this : true);
    Q_ASSERT(destinationParent.isValid() ? destinationParent.model() == this : true);
    Q_D(QIdentityProxyModel);
    return d->model->moveColumns(mapToSource(sourceParent), sourceColumn, count, mapToSource(destinationParent), destinationChild);
}

/*!
    \reimp
 */
int QIdentityProxyModel::rowCount(const QModelIndex& parent) const
{
    Q_ASSERT(parent.isValid() ? parent.model() == this : true);
    Q_D(const QIdentityProxyModel);
    return d->model->rowCount(mapToSource(parent));
}

/*!
    \reimp
 */
QVariant QIdentityProxyModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_D(const QIdentityProxyModel);
    return d->model->headerData(section, orientation, role);
}

/*!
    \reimp
 */
void QIdentityProxyModel::setSourceModel(QAbstractItemModel* newSourceModel)
{
    beginResetModel();

    if (sourceModel()) {
        disconnect(sourceModel(), SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)),
                   this, SLOT(_q_sourceRowsAboutToBeInserted(QModelIndex,int,int)));
        disconnect(sourceModel(), SIGNAL(rowsInserted(QModelIndex,int,int)),
                   this, SLOT(_q_sourceRowsInserted(QModelIndex,int,int)));
        disconnect(sourceModel(), SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
                   this, SLOT(_q_sourceRowsAboutToBeRemoved(QModelIndex,int,int)));
        disconnect(sourceModel(), SIGNAL(rowsRemoved(QModelIndex,int,int)),
                   this, SLOT(_q_sourceRowsRemoved(QModelIndex,int,int)));
        disconnect(sourceModel(), SIGNAL(rowsAboutToBeMoved(QModelIndex,int,int,QModelIndex,int)),
                   this, SLOT(_q_sourceRowsAboutToBeMoved(QModelIndex,int,int,QModelIndex,int)));
        disconnect(sourceModel(), SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)),
                   this, SLOT(_q_sourceRowsMoved(QModelIndex,int,int,QModelIndex,int)));
        disconnect(sourceModel(), SIGNAL(columnsAboutToBeInserted(QModelIndex,int,int)),
                   this, SLOT(_q_sourceColumnsAboutToBeInserted(QModelIndex,int,int)));
        disconnect(sourceModel(), SIGNAL(columnsInserted(QModelIndex,int,int)),
                   this, SLOT(_q_sourceColumnsInserted(QModelIndex,int,int)));
        disconnect(sourceModel(), SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)),
                   this, SLOT(_q_sourceColumnsAboutToBeRemoved(QModelIndex,int,int)));
        disconnect(sourceModel(), SIGNAL(columnsRemoved(QModelIndex,int,int)),
                   this, SLOT(_q_sourceColumnsRemoved(QModelIndex,int,int)));
        disconnect(sourceModel(), SIGNAL(columnsAboutToBeMoved(QModelIndex,int,int,QModelIndex,int)),
                   this, SLOT(_q_sourceColumnsAboutToBeMoved(QModelIndex,int,int,QModelIndex,int)));
        disconnect(sourceModel(), SIGNAL(columnsMoved(QModelIndex,int,int,QModelIndex,int)),
                   this, SLOT(_q_sourceColumnsMoved(QModelIndex,int,int,QModelIndex,int)));
        disconnect(sourceModel(), SIGNAL(modelAboutToBeReset()),
                   this, SLOT(_q_sourceModelAboutToBeReset()));
        disconnect(sourceModel(), SIGNAL(modelReset()),
                   this, SLOT(_q_sourceModelReset()));
        disconnect(sourceModel(), SIGNAL(dataChanged(QModelIndex,QModelIndex,QList<int>)),
                   this, SLOT(_q_sourceDataChanged(QModelIndex,QModelIndex,QList<int>)));
        disconnect(sourceModel(), SIGNAL(headerDataChanged(Qt::Orientation,int,int)),
                   this, SLOT(_q_sourceHeaderDataChanged(Qt::Orientation,int,int)));
        disconnect(sourceModel(), SIGNAL(layoutAboutToBeChanged(QList<QPersistentModelIndex>,QAbstractItemModel::LayoutChangeHint)),
                   this, SLOT(_q_sourceLayoutAboutToBeChanged(QList<QPersistentModelIndex>,QAbstractItemModel::LayoutChangeHint)));
        disconnect(sourceModel(), SIGNAL(layoutChanged(QList<QPersistentModelIndex>,QAbstractItemModel::LayoutChangeHint)),
                   this, SLOT(_q_sourceLayoutChanged(QList<QPersistentModelIndex>,QAbstractItemModel::LayoutChangeHint)));
    }

    QAbstractProxyModel::setSourceModel(newSourceModel);

    if (sourceModel()) {
        connect(sourceModel(), SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)),
                SLOT(_q_sourceRowsAboutToBeInserted(QModelIndex,int,int)));
        connect(sourceModel(), SIGNAL(rowsInserted(QModelIndex,int,int)),
                SLOT(_q_sourceRowsInserted(QModelIndex,int,int)));
        connect(sourceModel(), SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
                SLOT(_q_sourceRowsAboutToBeRemoved(QModelIndex,int,int)));
        connect(sourceModel(), SIGNAL(rowsRemoved(QModelIndex,int,int)),
                SLOT(_q_sourceRowsRemoved(QModelIndex,int,int)));
        connect(sourceModel(), SIGNAL(rowsAboutToBeMoved(QModelIndex,int,int,QModelIndex,int)),
                SLOT(_q_sourceRowsAboutToBeMoved(QModelIndex,int,int,QModelIndex,int)));
        connect(sourceModel(), SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)),
                SLOT(_q_sourceRowsMoved(QModelIndex,int,int,QModelIndex,int)));
        connect(sourceModel(), SIGNAL(columnsAboutToBeInserted(QModelIndex,int,int)),
                SLOT(_q_sourceColumnsAboutToBeInserted(QModelIndex,int,int)));
        connect(sourceModel(), SIGNAL(columnsInserted(QModelIndex,int,int)),
                SLOT(_q_sourceColumnsInserted(QModelIndex,int,int)));
        connect(sourceModel(), SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)),
                SLOT(_q_sourceColumnsAboutToBeRemoved(QModelIndex,int,int)));
        connect(sourceModel(), SIGNAL(columnsRemoved(QModelIndex,int,int)),
                SLOT(_q_sourceColumnsRemoved(QModelIndex,int,int)));
        connect(sourceModel(), SIGNAL(columnsAboutToBeMoved(QModelIndex,int,int,QModelIndex,int)),
                SLOT(_q_sourceColumnsAboutToBeMoved(QModelIndex,int,int,QModelIndex,int)));
        connect(sourceModel(), SIGNAL(columnsMoved(QModelIndex,int,int,QModelIndex,int)),
                SLOT(_q_sourceColumnsMoved(QModelIndex,int,int,QModelIndex,int)));
        connect(sourceModel(), SIGNAL(modelAboutToBeReset()),
                SLOT(_q_sourceModelAboutToBeReset()));
        connect(sourceModel(), SIGNAL(modelReset()),
                SLOT(_q_sourceModelReset()));
        connect(sourceModel(), SIGNAL(dataChanged(QModelIndex,QModelIndex,QList<int>)),
                SLOT(_q_sourceDataChanged(QModelIndex,QModelIndex,QList<int>)));
        connect(sourceModel(), SIGNAL(headerDataChanged(Qt::Orientation,int,int)),
                SLOT(_q_sourceHeaderDataChanged(Qt::Orientation,int,int)));
        connect(sourceModel(), SIGNAL(layoutAboutToBeChanged(QList<QPersistentModelIndex>,QAbstractItemModel::LayoutChangeHint)),
                SLOT(_q_sourceLayoutAboutToBeChanged(QList<QPersistentModelIndex>,QAbstractItemModel::LayoutChangeHint)));
        connect(sourceModel(), SIGNAL(layoutChanged(QList<QPersistentModelIndex>,QAbstractItemModel::LayoutChangeHint)),
                SLOT(_q_sourceLayoutChanged(QList<QPersistentModelIndex>,QAbstractItemModel::LayoutChangeHint)));
    }

    endResetModel();
}

void QIdentityProxyModelPrivate::_q_sourceColumnsAboutToBeInserted(const QModelIndex &parent, int start, int end)
{
    Q_ASSERT(parent.isValid() ? parent.model() == model : true);
    Q_Q(QIdentityProxyModel);
    q->beginInsertColumns(q->mapFromSource(parent), start, end);
}

void QIdentityProxyModelPrivate::_q_sourceColumnsAboutToBeMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd, const QModelIndex &destParent, int dest)
{
    Q_ASSERT(sourceParent.isValid() ? sourceParent.model() == model : true);
    Q_ASSERT(destParent.isValid() ? destParent.model() == model : true);
    Q_Q(QIdentityProxyModel);
    q->beginMoveColumns(q->mapFromSource(sourceParent), sourceStart, sourceEnd, q->mapFromSource(destParent), dest);
}

void QIdentityProxyModelPrivate::_q_sourceColumnsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
    Q_ASSERT(parent.isValid() ? parent.model() == model : true);
    Q_Q(QIdentityProxyModel);
    q->beginRemoveColumns(q->mapFromSource(parent), start, end);
}

void QIdentityProxyModelPrivate::_q_sourceColumnsInserted(const QModelIndex &parent, int start, int end)
{
    Q_ASSERT(parent.isValid() ? parent.model() == model : true);
    Q_Q(QIdentityProxyModel);
    Q_UNUSED(parent);
    Q_UNUSED(start);
    Q_UNUSED(end);
    q->endInsertColumns();
}

void QIdentityProxyModelPrivate::_q_sourceColumnsMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd, const QModelIndex &destParent, int dest)
{
    Q_ASSERT(sourceParent.isValid() ? sourceParent.model() == model : true);
    Q_ASSERT(destParent.isValid() ? destParent.model() == model : true);
    Q_Q(QIdentityProxyModel);
    Q_UNUSED(sourceParent);
    Q_UNUSED(sourceStart);
    Q_UNUSED(sourceEnd);
    Q_UNUSED(destParent);
    Q_UNUSED(dest);
    q->endMoveColumns();
}

void QIdentityProxyModelPrivate::_q_sourceColumnsRemoved(const QModelIndex &parent, int start, int end)
{
    Q_ASSERT(parent.isValid() ? parent.model() == model : true);
    Q_Q(QIdentityProxyModel);
    Q_UNUSED(parent);
    Q_UNUSED(start);
    Q_UNUSED(end);
    q->endRemoveColumns();
}

void QIdentityProxyModelPrivate::_q_sourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles)
{
    Q_ASSERT(topLeft.isValid() ? topLeft.model() == model : true);
    Q_ASSERT(bottomRight.isValid() ? bottomRight.model() == model : true);
    Q_Q(QIdentityProxyModel);
    emit q->dataChanged(q->mapFromSource(topLeft), q->mapFromSource(bottomRight), roles);
}

void QIdentityProxyModelPrivate::_q_sourceHeaderDataChanged(Qt::Orientation orientation, int first, int last)
{
    Q_Q(QIdentityProxyModel);
    emit q->headerDataChanged(orientation, first, last);
}

void QIdentityProxyModelPrivate::_q_sourceLayoutAboutToBeChanged(const QList<QPersistentModelIndex> &sourceParents, QAbstractItemModel::LayoutChangeHint hint)
{
    Q_Q(QIdentityProxyModel);

    QList<QPersistentModelIndex> parents;
    parents.reserve(sourceParents.size());
    for (const QPersistentModelIndex &parent : sourceParents) {
        if (!parent.isValid()) {
            parents << QPersistentModelIndex();
            continue;
        }
        const QModelIndex mappedParent = q->mapFromSource(parent);
        Q_ASSERT(mappedParent.isValid());
        parents << mappedParent;
    }

    emit q->layoutAboutToBeChanged(parents, hint);

    const auto proxyPersistentIndexes = q->persistentIndexList();
    for (const QModelIndex &proxyPersistentIndex : proxyPersistentIndexes) {
        proxyIndexes << proxyPersistentIndex;
        Q_ASSERT(proxyPersistentIndex.isValid());
        const QPersistentModelIndex srcPersistentIndex = q->mapToSource(proxyPersistentIndex);
        Q_ASSERT(srcPersistentIndex.isValid());
        layoutChangePersistentIndexes << srcPersistentIndex;
    }
}

void QIdentityProxyModelPrivate::_q_sourceLayoutChanged(const QList<QPersistentModelIndex> &sourceParents, QAbstractItemModel::LayoutChangeHint hint)
{
    Q_Q(QIdentityProxyModel);

    for (int i = 0; i < proxyIndexes.size(); ++i) {
        q->changePersistentIndex(proxyIndexes.at(i), q->mapFromSource(layoutChangePersistentIndexes.at(i)));
    }

    layoutChangePersistentIndexes.clear();
    proxyIndexes.clear();

    QList<QPersistentModelIndex> parents;
    parents.reserve(sourceParents.size());
    for (const QPersistentModelIndex &parent : sourceParents) {
        if (!parent.isValid()) {
            parents << QPersistentModelIndex();
            continue;
        }
        const QModelIndex mappedParent = q->mapFromSource(parent);
        Q_ASSERT(mappedParent.isValid());
        parents << mappedParent;
    }

    emit q->layoutChanged(parents, hint);
}

void QIdentityProxyModelPrivate::_q_sourceModelAboutToBeReset()
{
    Q_Q(QIdentityProxyModel);
    q->beginResetModel();
}

void QIdentityProxyModelPrivate::_q_sourceModelReset()
{
    Q_Q(QIdentityProxyModel);
    q->endResetModel();
}

void QIdentityProxyModelPrivate::_q_sourceRowsAboutToBeInserted(const QModelIndex &parent, int start, int end)
{
    Q_ASSERT(parent.isValid() ? parent.model() == model : true);
    Q_Q(QIdentityProxyModel);
    q->beginInsertRows(q->mapFromSource(parent), start, end);
}

void QIdentityProxyModelPrivate::_q_sourceRowsAboutToBeMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd, const QModelIndex &destParent, int dest)
{
    Q_ASSERT(sourceParent.isValid() ? sourceParent.model() == model : true);
    Q_ASSERT(destParent.isValid() ? destParent.model() == model : true);
    Q_Q(QIdentityProxyModel);
    q->beginMoveRows(q->mapFromSource(sourceParent), sourceStart, sourceEnd, q->mapFromSource(destParent), dest);
}

void QIdentityProxyModelPrivate::_q_sourceRowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
    Q_ASSERT(parent.isValid() ? parent.model() == model : true);
    Q_Q(QIdentityProxyModel);
    q->beginRemoveRows(q->mapFromSource(parent), start, end);
}

void QIdentityProxyModelPrivate::_q_sourceRowsInserted(const QModelIndex &parent, int start, int end)
{
    Q_ASSERT(parent.isValid() ? parent.model() == model : true);
    Q_Q(QIdentityProxyModel);
    Q_UNUSED(parent);
    Q_UNUSED(start);
    Q_UNUSED(end);
    q->endInsertRows();
}

void QIdentityProxyModelPrivate::_q_sourceRowsMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd, const QModelIndex &destParent, int dest)
{
    Q_ASSERT(sourceParent.isValid() ? sourceParent.model() == model : true);
    Q_ASSERT(destParent.isValid() ? destParent.model() == model : true);
    Q_Q(QIdentityProxyModel);
    Q_UNUSED(sourceParent);
    Q_UNUSED(sourceStart);
    Q_UNUSED(sourceEnd);
    Q_UNUSED(destParent);
    Q_UNUSED(dest);
    q->endMoveRows();
}

void QIdentityProxyModelPrivate::_q_sourceRowsRemoved(const QModelIndex &parent, int start, int end)
{
    Q_ASSERT(parent.isValid() ? parent.model() == model : true);
    Q_Q(QIdentityProxyModel);
    Q_UNUSED(parent);
    Q_UNUSED(start);
    Q_UNUSED(end);
    q->endRemoveRows();
}

QT_END_NAMESPACE

#include "moc_qidentityproxymodel.cpp"
