// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

/*!
    view.cpp

    Provides a view to represent a one-dimensional sequence of integers
    obtained from a list model as a series of rows.
*/

#include <QAbstractItemModel>
#include <QBrush>
#include <QItemSelection>
#include <QPainter>
#include <QPaintEvent>
#include <QPen>
#include <QPoint>
#include <QResizeEvent>
#include <QScrollBar>
#include <QSizePolicy>

#include "view.h"

LinearView::LinearView(QWidget *parent)
    : QAbstractItemView(parent)
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}

/*!
    Returns the position of the item in viewport coordinates.
*/

QRect LinearView::itemViewportRect(const QModelIndex &index) const
{
    QRect rect = itemRect(index);
    QRect result(rect.left() - horizontalScrollBar()->value(),
                 rect.top() - verticalScrollBar()->value(),
                 rect.width(), viewport()->height());

    return result;
}

/*!
    Returns the rectangle of the item at position \a index in the
    model. The rectangle is in contents coordinates.
*/

QRect LinearView::itemRect(const QModelIndex &index) const
{
    if (!index.isValid())
        return QRect();
    else
        return QRect(index.row(), 0, 1, 1);
}


void LinearView::ensureVisible(const QModelIndex &index)
{
    QRect area = viewport()->rect();
    QRect rect = itemViewportRect(index);

    if (rect.left() < area.left())
        horizontalScrollBar()->setValue(
            horizontalScrollBar()->value() - rect.left());
    else if (rect.right() > area.right())
        horizontalScrollBar()->setValue(
            horizontalScrollBar()->value() + rect.left() - area.width());
}

/*!
    Returns the item that covers the coordinate given in the view.
*/

QModelIndex LinearView::itemAt(int x, int /* y */) const
{
    int row = x + horizontalScrollBar()->value();

    return model()->index(row, 0, QModelIndex());
}

//void LinearView::dataChanged(const QModelIndex &/* topLeft */,
//    const QModelIndex &/* bottomRight */)
//{
//    updateGeometries();
//    if (isVisible())
//        repaint();
//}

void LinearView::rowsInserted(const QModelIndex &/* parent */, int /* start */,
    int /* end */)
{
    updateGeometries();
    if (isVisible())
        repaint();
}

void LinearView::rowsRemoved(const QModelIndex &/* parent */, int /* start */,
    int /* end */)
{
    updateGeometries();
    if (isVisible())
        repaint();
}
/*
void LinearView::verticalScrollbarAction(int action)
{
}

void LinearView::horizontalScrollbarAction(int action)
{
}
*/

/*!
    Select the items in the model that lie within the rectangle specified by
    \a rect, using the selection \a command.
*/

void LinearView::setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags command)
{
    QModelIndex leftIndex = itemAt(rect.left(), 0);
    QModelIndex rightIndex = itemAt(rect.right(), 0);

    QItemSelection selection(leftIndex, rightIndex);

    selectionModel()->select(selection, command);
}

QModelIndex LinearView::moveCursor(QAbstractItemView::CursorAction cursorAction,
                                   Qt::KeyboardModifiers)
{
    QModelIndex current = currentIndex();

    switch (cursorAction) {
    case MoveLeft:{
        if (current.row() > 0)
            return model()->index(current.row() - 1, 0, QModelIndex());
        else
            return model()->index(0, 0, QModelIndex());
        break;}
    case MoveRight:{
        if (current.row() < rows(current) - 1)
            return model()->index(current.row() + 1, 0, QModelIndex());
        else
            return model()->index(rows(current) - 1, 0,QModelIndex());
        break;}
    case MoveUp:
        return current;
    case MoveDown:
        return current;
    case MovePageUp:
        return current;
    case MovePageDown:
        return current;
    case MoveHome:
        return model()->index(0, 0, QModelIndex());
    case MoveEnd:
        return model()->index(rows(current) - 1, 0, QModelIndex());
    default:
        return current;
    }
}

int LinearView::horizontalOffset() const
{
    return horizontalScrollBar()->value();
}

int LinearView::verticalOffset() const
{
    return verticalScrollBar()->value();
}

/*!
    Returns a rectangle corresponding to the selection in viewport cooridinates.
*/

QRect LinearView::selectionViewportRect(const QItemSelection &selection) const
{
    int ranges = selection.count();

    if (ranges == 0)
        return QRect();

    // Note that we use the top and bottom functions of the selection range
    // since the data is stored in rows.

    int firstRow = selection.at(0).top();
    int lastRow = selection.at(0).top();

    for (int i = 0; i < ranges; ++i) {
        firstRow = qMin(firstRow, selection.at(i).top());
        lastRow = qMax(lastRow, selection.at(i).bottom());
    }

    QModelIndex firstItem = model()->index(qMin(firstRow, lastRow), 0,
        QModelIndex());
    QModelIndex lastItem = model()->index(qMax(firstRow, lastRow), 0,
        QModelIndex());

    QRect firstRect = itemViewportRect(firstItem);
    QRect lastRect = itemViewportRect(lastItem);

    return QRect(firstRect.left(), firstRect.top(),
        lastRect.right() - firstRect.left(), firstRect.height());
}

void LinearView::paintEvent(QPaintEvent *event)
{
    QPainter painter(viewport());

    QRect updateRect = event->rect();
    QBrush background(Qt::black);
    QPen foreground(Qt::white);

    painter.fillRect(updateRect, background);
    painter.setPen(foreground);

    QModelIndex firstItem = itemAt(updateRect.left(), updateRect.top());
    if (!firstItem.isValid())
        firstItem = model()->index(0, 0, QModelIndex());

    QModelIndex lastItem = itemAt(updateRect.right(), updateRect.bottom());
    if (!lastItem.isValid())
        lastItem = model()->index(rows() - 1, 0, QModelIndex());

    int x = updateRect.left();
    //int top = updateRect.top();
    //int bottom = updateRect.bottom();

    int row = firstItem.row();
    QModelIndex index = model()->index(row, 0, QModelIndex());
    int value = model()->data(index, Qt::DisplayRole).toInt();
    int midPoint = viewport()->height()/2;
    int y2 = midPoint - int(value * midPoint/255.0);

    while (row <= lastItem.row()) {

        QModelIndex index = model()->index(row, 0, QModelIndex());
        int value = model()->data(index, Qt::DisplayRole).toInt();

        int y1 = y2;
        y2 = midPoint - int(value * midPoint/255.0);

        painter.drawLine(x-1, y1, x, y2);
        ++row; ++x;
    }
}

void LinearView::resizeEvent(QResizeEvent * /* event */)
{
    updateGeometries();
}

void LinearView::updateGeometries()
{
    if (viewport()->width() < rows()) {
        horizontalScrollBar()->setPageStep(viewport()->width());
        horizontalScrollBar()->setRange(0, rows() - viewport()->width() - 1);
    }
}

QSize LinearView::sizeHint() const
{
    return QSize(rows(), 200);
}

int LinearView::rows(const QModelIndex &index) const
{
    return model()->rowCount(model()->parent(index));
}

bool LinearView::isIndexHidden(const QModelIndex &index) const
{
    return false;
}
