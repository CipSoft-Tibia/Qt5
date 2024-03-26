// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

/*
  model.cpp

  A simple model that uses a QList as its data source.
*/

#include "model.h"

/*!
    Returns the number of items in the string list as the number of rows
    in the model.
*/

int LinearModel::rowCount(const QModelIndex &parent) const
{
    Q_USING(parent);

    return values.count();
}

/*
    Returns an appropriate value for the requested data.
    If the view requests an invalid index, an invalid variant is returned.
    If a header is requested then we just return the column or row number,
    depending on the orientation of the header.
    Any valid index that corresponds to a string in the list causes that
    string to be returned.
*/

/*!
    Returns a model index for other component to use when referencing the
    item specified by the given row, column, and type. The parent index
    is ignored.
*/

QModelIndex LinearModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent == QModelIndex() && row >= 0 && row < rowCount()
        && column == 0)
        return createIndex(row, column);
    else
        return QModelIndex();
}

QVariant LinearModel::data(const QModelIndex &index, int role) const
{
    Q_UNUSED(role);

    if (!index.isValid())
        return QVariant();

    return values.at(index.row());
}

/*!
    Returns Qt::ItemIsEditable so that all items in the list can be edited.
*/

Qt::ItemFlags LinearModel::flags(const QModelIndex &index) const
{
    // all items in the model are editable
    return QAbstractListModel::flags(index) | Qt::ItemIsEditable;
}

/*!
    Changes an item in the string list, but only if the following conditions
    are met:

    * The index supplied is valid.
    * The index corresponds to an item to be shown in a view.
    * The role associated with editing text is specified.

    The dataChanged() signal is emitted if the item is changed.
*/

bool LinearModel::setData(const QModelIndex &index,
                          const QVariant &value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;
    values.replace(index.row(), value.toInt());
    emit dataChanged(index, index);
    return true;
}

/*!
    Inserts a number of rows into the model at the specified position.
*/

bool LinearModel::insertRows(int position, int rows, const QModelIndex &parent)
{
    beginInsertRows(parent, position, position + rows - 1);

    values.insert(position, rows, 0);

    endInsertRows();
    return true;
}

/*!
    Removes a number of rows from the model at the specified position.
*/

bool LinearModel::removeRows(int position, int rows, const QModelIndex &parent)
{
    beginRemoveRows(QModelIndex(), position, position+rows-1);

    values.remove(position, rows);

    endRemoveRows();
    return true;
}
