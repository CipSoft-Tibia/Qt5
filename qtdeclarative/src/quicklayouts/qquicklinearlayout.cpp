// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquicklinearlayout_p.h"
#include "qquickgridlayoutengine_p.h"
#include "qquicklayoutstyleinfo_p.h"
#include <QtCore/private/qnumeric_p.h>
#include <QtQml/qqmlinfo.h>
#include "qdebug.h"
#include <limits>

/*!
    \qmltype RowLayout
    //! \instantiates QQuickRowLayout
    \inherits Item
    \inqmlmodule QtQuick.Layouts
    \ingroup layouts
    \brief Identical to \l GridLayout, but having only one row.

    To be able to use this type more efficiently, it is recommended that you
    understand the general mechanism of the Qt Quick Layouts module. Refer to
    \l{Qt Quick Layouts Overview} for more information.

    It is available as a convenience for developers, as it offers a cleaner API.

    Items in a RowLayout support these attached properties:
    \list
    \input layout.qdocinc attached-properties
    \endlist

    \image rowlayout.png

    \code
    RowLayout {
        id: layout
        anchors.fill: parent
        spacing: 6
        Rectangle {
            color: 'teal'
            Layout.fillWidth: true
            Layout.minimumWidth: 50
            Layout.preferredWidth: 100
            Layout.maximumWidth: 300
            Layout.minimumHeight: 150
            Text {
                anchors.centerIn: parent
                text: parent.width + 'x' + parent.height
            }
        }
        Rectangle {
            color: 'plum'
            Layout.fillWidth: true
            Layout.minimumWidth: 100
            Layout.preferredWidth: 200
            Layout.preferredHeight: 100
            Text {
                anchors.centerIn: parent
                text: parent.width + 'x' + parent.height
            }
        }
    }
    \endcode

    Read more about attached properties \l{QML Object Attributes}{here}.
    \sa ColumnLayout
    \sa GridLayout
    \sa StackLayout
    \sa Row
    \sa {Qt Quick Layouts Overview}
*/

/*!
    \qmltype ColumnLayout
    //! \instantiates QQuickColumnLayout
    \inherits Item
    \inqmlmodule QtQuick.Layouts
    \ingroup layouts
    \brief Identical to \l GridLayout, but having only one column.

    To be able to use this type more efficiently, it is recommended that you
    understand the general mechanism of the Qt Quick Layouts module. Refer to
    \l{Qt Quick Layouts Overview} for more information.

    It is available as a convenience for developers, as it offers a cleaner API.

    Items in a ColumnLayout support these attached properties:
    \list
    \input layout.qdocinc attached-properties
    \endlist

    \image columnlayout.png

    \code
    ColumnLayout{
        spacing: 2

        Rectangle {
            Layout.alignment: Qt.AlignCenter
            color: "red"
            Layout.preferredWidth: 40
            Layout.preferredHeight: 40
        }

        Rectangle {
            Layout.alignment: Qt.AlignRight
            color: "green"
            Layout.preferredWidth: 40
            Layout.preferredHeight: 70
        }

        Rectangle {
            Layout.alignment: Qt.AlignBottom
            Layout.fillHeight: true
            color: "blue"
            Layout.preferredWidth: 70
            Layout.preferredHeight: 40
        }
    }
    \endcode

    Read more about attached properties \l{QML Object Attributes}{here}.

    \sa RowLayout
    \sa GridLayout
    \sa StackLayout
    \sa Column
    \sa {Qt Quick Layouts Overview}
*/


/*!
    \qmltype GridLayout
    //! \instantiates QQuickGridLayout
    \inherits Item
    \inqmlmodule QtQuick.Layouts
    \ingroup layouts
    \brief Provides a way of dynamically arranging items in a grid.

    To be able to use this type more efficiently, it is recommended that you
    understand the general mechanism of the Qt Quick Layouts module. Refer to
    \l{Qt Quick Layouts Overview} for more information.

    If the GridLayout is resized, all items in the layout will be rearranged. It is similar
    to the widget-based QGridLayout. All visible children of the GridLayout element will belong to
    the layout. If you want a layout with just one row or one column, you can use the
    \l RowLayout or \l ColumnLayout. These offer a bit more convenient API, and improve
    readability.

    By default items will be arranged according to the \l flow property. The default value of
    the \l flow property is \c GridLayout.LeftToRight.

    If the \l columns property is specified, it will be treated as a maximum limit of how many
    columns the layout can have, before the auto-positioning wraps back to the beginning of the
    next row. The \l columns property is only used when \l flow is  \c GridLayout.LeftToRight.

    \image gridlayout.png

    \code
    GridLayout {
        id: grid
        columns: 3

        Text { text: "Three"; font.bold: true; }
        Text { text: "words"; color: "red" }
        Text { text: "in"; font.underline: true }
        Text { text: "a"; font.pixelSize: 20 }
        Text { text: "row"; font.strikeout: true }
    }
    \endcode

    The \l rows property works in a similar way, but items are auto-positioned vertically. The \l
    rows property is only used when \l flow is \c GridLayout.TopToBottom.

    You can specify which cell you want an item to occupy by setting the
    \l{Layout::row}{Layout.row} and \l{Layout::column}{Layout.column} properties. You can also
    specify the row span or column span by setting the \l{Layout::rowSpan}{Layout.rowSpan} or
    \l{Layout::columnSpan}{Layout.columnSpan} properties.


    Items in a GridLayout support these attached properties:
    \list
        \li \l{Layout::row}{Layout.row}
        \li \l{Layout::column}{Layout.column}
        \li \l{Layout::rowSpan}{Layout.rowSpan}
        \li \l{Layout::columnSpan}{Layout.columnSpan}
        \input layout.qdocinc attached-properties
    \endlist

    Read more about attached properties \l{QML Object Attributes}{here}.

    \sa RowLayout
    \sa ColumnLayout
    \sa StackLayout
    \sa Grid
    \sa {Qt Quick Layouts Overview}
*/



QT_BEGIN_NAMESPACE

QQuickGridLayoutBase::QQuickGridLayoutBase()
    : QQuickLayout(*new QQuickGridLayoutBasePrivate)
{

}

QQuickGridLayoutBase::QQuickGridLayoutBase(QQuickGridLayoutBasePrivate &dd,
                                           Qt::Orientation orientation,
                                           QQuickItem *parent /*= nullptr */)
    : QQuickLayout(dd, parent)
{
    Q_D(QQuickGridLayoutBase);
    d->orientation = orientation;
    d->styleInfo = new QQuickLayoutStyleInfo;
}

Qt::Orientation QQuickGridLayoutBase::orientation() const
{
    Q_D(const QQuickGridLayoutBase);
    return d->orientation;
}

void QQuickGridLayoutBase::setOrientation(Qt::Orientation orientation)
{
    Q_D(QQuickGridLayoutBase);
    if (d->orientation == orientation)
        return;

    d->orientation = orientation;
    invalidate();
}

QSizeF QQuickGridLayoutBase::sizeHint(Qt::SizeHint whichSizeHint) const
{
    Q_D(const QQuickGridLayoutBase);
    return d->engine.sizeHint(whichSizeHint, QSizeF(), d->styleInfo);
}

/*!
    \qmlproperty enumeration GridLayout::layoutDirection
    \since QtQuick.Layouts 1.1

    This property holds the layout direction of the grid layout - it controls whether items are
    laid out from left to right or right to left. If \c Qt.RightToLeft is specified,
    left-aligned items will be right-aligned and right-aligned items will be left-aligned.

    Possible values:

    \value Qt.LeftToRight   (default) Items are laid out from left to right.
    \value Qt.RightToLeft   Items are laid out from right to left.

    \sa RowLayout::layoutDirection, ColumnLayout::layoutDirection
*/
Qt::LayoutDirection QQuickGridLayoutBase::layoutDirection() const
{
    Q_D(const QQuickGridLayoutBase);
    return d->m_layoutDirection;
}

void QQuickGridLayoutBase::setLayoutDirection(Qt::LayoutDirection dir)
{
    Q_D(QQuickGridLayoutBase);
    if (d->m_layoutDirection == dir)
        return;
    d->m_layoutDirection = dir;
    invalidate();
    emit layoutDirectionChanged();
}

Qt::LayoutDirection QQuickGridLayoutBase::effectiveLayoutDirection() const
{
    Q_D(const QQuickGridLayoutBase);
    return !d->effectiveLayoutMirror == (layoutDirection() == Qt::LeftToRight)
                                      ? Qt::LeftToRight : Qt::RightToLeft;
}

void QQuickGridLayoutBase::setAlignment(QQuickItem *item, Qt::Alignment alignment)
{
    Q_D(QQuickGridLayoutBase);
    d->engine.setAlignment(item, alignment);
    maybeSubscribeToBaseLineOffsetChanges(item);
}

void QQuickGridLayoutBase::setStretchFactor(QQuickItem *item, int stretchFactor, Qt::Orientation orient)
{
    Q_D(QQuickGridLayoutBase);
    d->engine.setStretchFactor(item, stretchFactor, orient);
}

QQuickGridLayoutBase::~QQuickGridLayoutBase()
{
    Q_D(QQuickGridLayoutBase);

    // Remove item listeners so we do not act on signalling unnecessarily
    // (there is no point, as the layout will be torn down anyway).
    deactivateRecur();
    delete d->styleInfo;
}

void QQuickGridLayoutBase::componentComplete()
{
    qCDebug(lcQuickLayouts) << "QQuickGridLayoutBase::componentComplete()" << this << parent();
    QQuickLayout::componentComplete();

    /* The layout is invalid when it is constructed, but during construction of the layout and
       its children (in the "static/from QML" case which this is trying to cover) things
       change and as a consequence invalidate() and ensureLayoutItemsUpdated() might be called.
       As soon as ensureLayoutItemsUpdated() is called it will set d->dirty = false.
       However, a subsequent invalidate() will return early if the component is not completed
       because it knows that componentComplete() will take care of doing the proper layouting
       (so it won't set d->dirty = true). When we then call ensureLayoutItemsUpdated() again here
       it sees that its not dirty and assumes everything up-to-date. For those cases we therefore
       need to call invalidate() in advance
    */
    invalidate();
    ensureLayoutItemsUpdated(QQuickLayout::ApplySizeHints);

    QQuickItem *par = parentItem();
    if (qobject_cast<QQuickLayout*>(par))
        return;
    rearrange(QSizeF(width(), height()));
    qCDebug(lcQuickLayouts) << "QQuickGridLayoutBase::componentComplete(). COMPLETED" << this << parent();
}

/*
  Invalidation happens like this as a reaction to that a size hint changes on an item "a":

  Suppose we have the following Qml document:
    RowLayout {
        id: l1
        RowLayout {
            id: l2
            Item {
                id: a
            }
            Item {
                id: b
            }
        }
    }

  1.    l2->invalidate(a) is called on l2, where item refers to "a".
        (this will dirty the cached size hints of item "a")
  2.    The layout engine will invalidate:
            i)  invalidate the layout engine
            ii) dirty the cached size hints of item "l2" (by calling parentLayout()->invalidate(l2)
        The recursion continues to the topmost layout
 */
/*!
   \internal

    Invalidates \a childItem and this layout.
    After a call to invalidate, the next call to retrieve e.g. sizeHint will be up-to date.
    This function will also call QQuickLayout::invalidate(0), to ensure that the parent layout
    is invalidated.
 */
void QQuickGridLayoutBase::invalidate(QQuickItem *childItem)
{
    Q_D(QQuickGridLayoutBase);
    if (!isReady())
        return;
    qCDebug(lcQuickLayouts) << "QQuickGridLayoutBase::invalidate()" << this << ", invalidated:" << invalidated();
    if (invalidated()) {
        return;
    }
    qCDebug(lcQuickLayouts) << "d->m_rearranging:" << d->m_rearranging;
    if (d->m_rearranging) {
        d->m_invalidateAfterRearrange << childItem;
        return;
    }

    if (childItem) {
        if (QQuickGridLayoutItem *layoutItem = d->engine.findLayoutItem(childItem))
            layoutItem->invalidate();
    }
    // invalidate engine
    d->engine.invalidate();

    qCDebug(lcQuickLayouts) << "calling QQuickLayout::invalidate();";
    QQuickLayout::invalidate();

    if (QQuickLayout *parentLayout = qobject_cast<QQuickLayout *>(parentItem()))
        parentLayout->invalidate(this);
    qCDebug(lcQuickLayouts) << "QQuickGridLayoutBase::invalidate() LEAVING" << this;
}

void QQuickGridLayoutBase::updateLayoutItems()
{
    Q_D(QQuickGridLayoutBase);
    if (!isReady())
        return;

    qCDebug(lcQuickLayouts) << "QQuickGridLayoutBase::updateLayoutItems ENTERING" << this;
    d->engine.deleteItems();
    insertLayoutItems();
    qCDebug(lcQuickLayouts) << "QQuickGridLayoutBase::updateLayoutItems() LEAVING" << this;
}

QQuickItem *QQuickGridLayoutBase::itemAt(int index) const
{
    Q_D(const QQuickGridLayoutBase);
    return static_cast<QQuickGridLayoutItem*>(d->engine.itemAt(index))->layoutItem();
}

int QQuickGridLayoutBase::itemCount() const
{
    Q_D(const QQuickGridLayoutBase);
    return d->engine.itemCount();
}

void QQuickGridLayoutBase::removeGridItem(QGridLayoutItem *gridItem)
{
    Q_D(QQuickGridLayoutBase);
    const int index = gridItem->firstRow(d->orientation);
    d->engine.removeItem(gridItem);
    d->engine.removeRows(index, 1, d->orientation);
}

void QQuickGridLayoutBase::itemDestroyed(QQuickItem *item)
{
    if (!isReady())
        return;
    Q_D(QQuickGridLayoutBase);
    qCDebug(lcQuickLayouts) << "QQuickGridLayoutBase::itemDestroyed";
    if (QQuickGridLayoutItem *gridItem = d->engine.findLayoutItem(item)) {
        removeGridItem(gridItem);
        delete gridItem;
        invalidate();
    }
}

void QQuickGridLayoutBase::itemVisibilityChanged(QQuickItem *item)
{
    Q_UNUSED(item);

    if (!isReady())
        return;
    qCDebug(lcQuickLayouts) << "QQuickGridLayoutBase::itemVisibilityChanged()";
    invalidate(item);
}

void QQuickGridLayoutBase::rearrange(const QSizeF &size)
{
    Q_D(QQuickGridLayoutBase);
    if (!isReady())
        return;

    qCDebug(lcQuickLayouts) << "QQuickGridLayoutBase::rearrange" << d->m_recurRearrangeCounter << this;
    const auto refCounter = qScopeGuard([&d] {
        --(d->m_recurRearrangeCounter);
    });
    if (d->m_recurRearrangeCounter++ == 2) {
        // allow a recursive depth of two in order to respond to height-for-width
        // (e.g QQuickText changes implicitHeight when its width gets changed)
        qWarning() << "Qt Quick Layouts: Detected recursive rearrange. Aborting after two iterations.";
        return;
    }

    // Should normally not be needed, but there might be an incoming window resize event that we
    // will process before we process updatePolish()
    ensureLayoutItemsUpdated(QQuickLayout::ApplySizeHints | QQuickLayout::Recursive);

    d->m_rearranging = true;
    qCDebug(lcQuickLayouts) << objectName() << "QQuickGridLayoutBase::rearrange()" << size;
    Qt::LayoutDirection visualDir = effectiveLayoutDirection();
    d->engine.setVisualDirection(visualDir);

    /*
    qreal left, top, right, bottom;
    left = top = right = bottom = 0;                    // ### support for margins?
    if (visualDir == Qt::RightToLeft)
        qSwap(left, right);
    */

    // Set m_dirty to false in case size hint changes during arrangement.
    // This could happen if there is a binding like implicitWidth: height
    QQuickLayout::rearrange(size);
    d->engine.setGeometries(QRectF(QPointF(0,0), size), d->styleInfo);
    d->m_rearranging = false;

    for (QQuickItem *invalid : std::as_const(d->m_invalidateAfterRearrange))
        invalidate(invalid);
    d->m_invalidateAfterRearrange.clear();
}

/**********************************
 **
 ** QQuickGridLayout
 **
 **/
QQuickGridLayout::QQuickGridLayout(QQuickItem *parent /* = nullptr*/)
    : QQuickGridLayoutBase(*new QQuickGridLayoutPrivate, Qt::Horizontal, parent)
{
}

/*!
    \qmlproperty real GridLayout::columnSpacing

    This property holds the spacing between each column.
    The default value is \c 5.
*/
qreal QQuickGridLayout::columnSpacing() const
{
    Q_D(const QQuickGridLayout);
    return d->engine.spacing(Qt::Horizontal, d->styleInfo);
}

void QQuickGridLayout::setColumnSpacing(qreal spacing)
{
    Q_D(QQuickGridLayout);
    if (qt_is_nan(spacing) || columnSpacing() == spacing)
        return;

    d->engine.setSpacing(spacing, Qt::Horizontal);
    invalidate();
    emit columnSpacingChanged();
}

/*!
    \qmlproperty real GridLayout::rowSpacing

    This property holds the spacing between each row.
    The default value is \c 5.
*/
qreal QQuickGridLayout::rowSpacing() const
{
    Q_D(const QQuickGridLayout);
    return d->engine.spacing(Qt::Vertical, d->styleInfo);
}

void QQuickGridLayout::setRowSpacing(qreal spacing)
{
    Q_D(QQuickGridLayout);
    if (qt_is_nan(spacing) || rowSpacing() == spacing)
        return;

    d->engine.setSpacing(spacing, Qt::Vertical);
    invalidate();
    emit rowSpacingChanged();
}

/*!
    \qmlproperty int GridLayout::columns

    This property holds the column limit for items positioned if \l flow is
    \c GridLayout.LeftToRight.
    The default value is that there is no limit.
*/
int QQuickGridLayout::columns() const
{
    Q_D(const QQuickGridLayout);
    return d->columns;
}

void QQuickGridLayout::setColumns(int columns)
{
    Q_D(QQuickGridLayout);
    if (d->columns == columns)
        return;
    d->columns = columns;
    invalidate();
    emit columnsChanged();
}


/*!
    \qmlproperty int GridLayout::rows

    This property holds the row limit for items positioned if \l flow is \c GridLayout.TopToBottom.
    The default value is that there is no limit.
*/
int QQuickGridLayout::rows() const
{
    Q_D(const QQuickGridLayout);
    return d->rows;
}

void QQuickGridLayout::setRows(int rows)
{
    Q_D(QQuickGridLayout);
    if (d->rows == rows)
        return;
    d->rows = rows;
    invalidate();
    emit rowsChanged();
}


/*!
    \qmlproperty enumeration GridLayout::flow

    This property holds the flow direction of items that does not have an explicit cell
    position set.
    It is used together with the \l columns or \l rows property, where
    they specify when flow is reset to the next row or column respectively.

    Possible values are:

    \value GridLayout.LeftToRight
        (default) Items are positioned next to each other, then wrapped to the next line.
    \value GridLayout.TopToBottom
        Items are positioned next to each other from top to bottom, then wrapped to the next column.

    \sa rows
    \sa columns
*/
QQuickGridLayout::Flow QQuickGridLayout::flow() const
{
    Q_D(const QQuickGridLayout);
    return d->flow;
}

void QQuickGridLayout::setFlow(QQuickGridLayout::Flow flow)
{
    Q_D(QQuickGridLayout);
    if (d->flow == flow)
        return;
    d->flow = flow;
    // If flow is changed, the layout needs to be repopulated
    invalidate();
    emit flowChanged();
}

/*!
    \qmlproperty bool GridLayout::uniformCellWidths
    \since QtQuick.Layouts 6.6

    If this property is set to \c true, the layout will force all cells to have
    a uniform width. The layout aims to respect
    \l{Layout::minimumWidth}{Layout.minimumWidth},
    \l{Layout::preferredWidth}{Layout.preferredWidth} and
    \l{Layout::maximumWidth}{Layout.maximumWidth} in this mode but might make
    compromisses to fullfill the requirements of all items.

    Default value is \c false.

    \note This API is considered tech preview and may change or be removed in future versions of
    Qt.

    \sa GridLayout::uniformCellHeights, RowLayout::uniformCellSizes, ColumnLayout::uniformCellSizes
*/
bool QQuickGridLayout::uniformCellWidths() const
{
    Q_D(const QQuickGridLayout);
    return d->engine.uniformCellWidths();
}

void QQuickGridLayout::setUniformCellWidths(bool uniformCellWidths)
{
    Q_D(QQuickGridLayout);
    if (d->engine.uniformCellWidths() == uniformCellWidths)
        return;
    d->engine.setUniformCellWidths(uniformCellWidths);
    invalidate();
    emit uniformCellWidthsChanged();
}

/*!
    \qmlproperty bool GridLayout::uniformCellHeights
    \since QtQuick.Layouts 6.6

    If this property is set to \c true, the layout will force all cells to have an
    uniform Height. The layout aims to respect
    \l{Layout::minimumHeight}{Layout.minimumHeight},
    \l{Layout::preferredHeight}{Layout.preferredHeight} and
    \l{Layout::maximumHeight}{Layout.maximumHeight} in this mode but might make
    compromisses to fullfill the requirements of all items.

    Default value is \c false.

    \note This API is considered tech preview and may change or be removed in future versions of
    Qt.

    \sa GridLayout::uniformCellWidths, RowLayout::uniformCellSizes, ColumnLayout::uniformCellSizes
*/
bool QQuickGridLayout::uniformCellHeights() const
{
    Q_D(const QQuickGridLayout);
    return d->engine.uniformCellHeights();
}

void QQuickGridLayout::setUniformCellHeights(bool uniformCellHeights)
{
    Q_D(QQuickGridLayout);
    if (d->engine.uniformCellHeights() == uniformCellHeights)
        return;
    d->engine.setUniformCellHeights(uniformCellHeights);
    invalidate();
    emit uniformCellHeightsChanged();
}


void QQuickGridLayout::insertLayoutItems()
{
    Q_D(QQuickGridLayout);

    int nextCellPos[2] = {0,0};
    int &nextColumn = nextCellPos[0];
    int &nextRow = nextCellPos[1];

    const QSize gridSize(columns(), rows());
    const int flowOrientation = flow();
    int &flowColumn = nextCellPos[flowOrientation];
    int &flowRow = nextCellPos[1 - flowOrientation];
    int flowBound = (flowOrientation == QQuickGridLayout::LeftToRight) ? columns() : rows();

    if (flowBound < 0)
        flowBound = std::numeric_limits<int>::max();

    const auto items = childItems();
    for (QQuickItem *child : items) {
        checkAnchors(child);
        // Will skip all items with effective maximum width/height == 0
        if (shouldIgnoreItem(child))
            continue;
        QQuickLayoutAttached *info = attachedLayoutObject(child, false);

        Qt::Alignment alignment;
        int hStretch = -1;
        int vStretch = -1;
        int row = -1;
        int column = -1;
        int span[2] = {1,1};
        int &columnSpan = span[0];
        int &rowSpan = span[1];

        if (info) {
            if (info->isRowSet() || info->isColumnSet()) {
                // If row is specified and column is not specified (or vice versa),
                // the unspecified component of the cell position should default to 0
                // The getters do this for us, as they will return 0 for an
                // unset (or negative) value.
                // In addition, if \a rows is less than Layout.row, treat Layout.row as if it was not set
                // This will basically make it find the next available cell (potentially wrapping to
                // the next line). Likewise for an invalid Layout.column

                if (gridSize.height() >= 0 && row >= gridSize.height()) {
                    qmlWarning(child) << QString::fromLatin1("Layout: row (%1) should be less than the number of rows (%2)").arg(info->row()).arg(rows());
                } else {
                    row = info->row();
                }

                if (gridSize.width() >= 0 && info->column() >= gridSize.width()) {
                    qmlWarning(child) << QString::fromLatin1("Layout: column (%1) should be less than the number of columns (%2)").arg(info->column()).arg(columns());
                } else {
                    column = info->column();
                }
            }
            rowSpan = info->rowSpan();
            columnSpan = info->columnSpan();
            if (columnSpan < 1) {
                qmlWarning(child) << "Layout: invalid column span: " << columnSpan;
                return;

            } else if (rowSpan < 1) {
                qmlWarning(child) << "Layout: invalid row span: " << rowSpan;
                return;
            }
            alignment = info->alignment();
            hStretch = info->horizontalStretchFactor();
            if (hStretch >= 0 && !info->fillWidth())
                qmlWarning(child) << "horizontalStretchFactor requires fillWidth to also be set to true";
            vStretch = info->verticalStretchFactor();
            if (vStretch >= 0 && !info->fillHeight())
                qmlWarning(child) << "verticalStretchFactor requires fillHeight to also be set to true";
        }

        Q_ASSERT(columnSpan >= 1);
        Q_ASSERT(rowSpan >= 1);
        const int sp = span[flowOrientation];
        if (sp > flowBound)
            return;

        if (row >= 0)
            nextRow = row;
        if (column >= 0)
            nextColumn = column;

        if (row < 0 || column < 0) {
            /* if row or column is not specified, find next position by
               advancing in the flow direction until there is a cell that
               can accept the item.

               The acceptance rules are pretty simple, but complexity arises
               when an item requires several cells (due to spans):
               1. Check if the cells that the item will require
                  does not extend beyond columns (for LeftToRight) or
                  rows (for TopToBottom).
               2. Check if the cells that the item will require is not already
                  taken by another item.
            */
            bool cellAcceptsItem;
            while (true) {
                // Check if the item does not span beyond the layout bound
                cellAcceptsItem = (flowColumn + sp) <= flowBound;

                // Check if all the required cells are not taken
                for (int rs = 0; cellAcceptsItem && rs < rowSpan; ++rs) {
                    for (int cs = 0; cellAcceptsItem && cs < columnSpan; ++cs) {
                        if (d->engine.itemAt(nextRow + rs, nextColumn + cs)) {
                            cellAcceptsItem = false;
                        }
                    }
                }
                if (cellAcceptsItem)
                    break;
                ++flowColumn;
                if (flowColumn == flowBound) {
                    flowColumn = 0;
                    ++flowRow;
                }
            }
        }
        column = nextColumn;
        row = nextRow;
        QQuickGridLayoutItem *layoutItem = new QQuickGridLayoutItem(child, row, column, rowSpan, columnSpan, alignment);
        if (hStretch >= 0)
            layoutItem->setStretchFactor(hStretch, Qt::Horizontal);
        if (vStretch >= 0)
            layoutItem->setStretchFactor(vStretch, Qt::Vertical);
        d->engine.insertItem(layoutItem, -1);
    }
}

/**********************************
 **
 ** QQuickLinearLayout
 **
 **/
QQuickLinearLayout::QQuickLinearLayout(Qt::Orientation orientation,
                                        QQuickItem *parent /*= nullptr*/)
    : QQuickGridLayoutBase(*new QQuickLinearLayoutPrivate, orientation, parent)
{
}

/*!
    \qmlproperty enumeration RowLayout::layoutDirection
    \since QtQuick.Layouts 1.1

    This property holds the layout direction of the row layout - it controls whether items are laid
    out from left to right or right to left. If \c Qt.RightToLeft is specified,
    left-aligned items will be right-aligned and right-aligned items will be left-aligned.

    Possible values:

    \value Qt.LeftToRight   (default) Items are laid out from left to right.
    \value Qt.RightToLeft   Items are laid out from right to left

    \sa GridLayout::layoutDirection, ColumnLayout::layoutDirection
*/
/*!
    \qmlproperty enumeration ColumnLayout::layoutDirection
    \since QtQuick.Layouts 1.1

    This property holds the layout direction of the column layout - it controls whether items are laid
    out from left to right or right to left. If \c Qt.RightToLeft is specified,
    left-aligned items will be right-aligned and right-aligned items will be left-aligned.

    Possible values:

    \value Qt.LeftToRight   (default) Items are laid out from left to right.
    \value Qt.RightToLeft   Items are laid out from right to left

    \sa GridLayout::layoutDirection, RowLayout::layoutDirection
*/

/*!
    \qmlproperty bool RowLayout::uniformCellSizes
    \since QtQuick.Layouts 6.6

    If this property is set to \c true, the layout will force all cells to have
    a uniform size.

    \note This API is considered tech preview and may change or be removed in future versions of
    Qt.

    \sa GridLayout::uniformCellWidths, GridLayout::uniformCellHeights, ColumnLayout::uniformCellSizes
*/
/*!
    \qmlproperty bool ColumnLayout::uniformCellSizes
    \since QtQuick.Layouts 6.6

    If this property is set to \c true, the layout will force all cells to have
    a uniform size.

    \note This API is considered tech preview and may change or be removed in future versions of
    Qt.

    \sa GridLayout::uniformCellWidths, GridLayout::uniformCellHeights, RowLayout::uniformCellSizes
*/
bool QQuickLinearLayout::uniformCellSizes() const
{
    Q_D(const QQuickLinearLayout);
    Q_ASSERT(d->engine.uniformCellWidths() == d->engine.uniformCellHeights());
    return d->engine.uniformCellWidths();
}

void QQuickLinearLayout::setUniformCellSizes(bool uniformCellSizes)
{
    Q_D(QQuickLinearLayout);
    Q_ASSERT(d->engine.uniformCellWidths() == d->engine.uniformCellHeights());
    if (d->engine.uniformCellHeights() == uniformCellSizes)
        return;
    d->engine.setUniformCellWidths(uniformCellSizes);
    d->engine.setUniformCellHeights(uniformCellSizes);
    invalidate();
    emit uniformCellSizesChanged();
}


/*!
    \qmlproperty real RowLayout::spacing

    This property holds the spacing between each cell.
    The default value is \c 5.
*/
/*!
    \qmlproperty real ColumnLayout::spacing

    This property holds the spacing between each cell.
    The default value is \c 5.
*/

qreal QQuickLinearLayout::spacing() const
{
    Q_D(const QQuickLinearLayout);
    return d->engine.spacing(d->orientation, d->styleInfo);
}

void QQuickLinearLayout::setSpacing(qreal space)
{
    Q_D(QQuickLinearLayout);
    if (qt_is_nan(space) || spacing() == space)
        return;

    d->engine.setSpacing(space, Qt::Horizontal | Qt::Vertical);
    invalidate();
    emit spacingChanged();
}

void QQuickLinearLayout::insertLayoutItems()
{
    Q_D(QQuickLinearLayout);
    const auto items = childItems();
    for (QQuickItem *child : items) {
        Q_ASSERT(child);
        checkAnchors(child);

        // Will skip all items with effective maximum width/height == 0
        if (shouldIgnoreItem(child))
            continue;
        QQuickLayoutAttached *info = attachedLayoutObject(child, false);

        Qt::Alignment alignment;
        int hStretch = -1;
        int vStretch = -1;
        bool fillWidth = false;
        bool fillHeight = false;
        if (info) {
            alignment = info->alignment();
            hStretch = info->horizontalStretchFactor();
            vStretch = info->verticalStretchFactor();
            fillWidth = info->fillWidth();
            fillHeight = info->fillHeight();
        }

        const int index = d->engine.rowCount(d->orientation);
        d->engine.insertRow(index, d->orientation);

        int gridRow = 0;
        int gridColumn = index;
        if (d->orientation == Qt::Vertical)
            qSwap(gridRow, gridColumn);
        QQuickGridLayoutItem *layoutItem = new QQuickGridLayoutItem(child, gridRow, gridColumn, 1, 1, alignment);

        if (hStretch >= 0) {
            if (!fillWidth)
                qmlWarning(child) << "horizontalStretchFactor requires fillWidth to also be set to true";
            layoutItem->setStretchFactor(hStretch, Qt::Horizontal);
        }
        if (vStretch >= 0) {
            if (!fillHeight)
                qmlWarning(child) << "verticalStretchFactor requires fillHeight to also be set to true";
            layoutItem->setStretchFactor(vStretch, Qt::Vertical);
        }
        d->engine.insertItem(layoutItem, index);
    }
}

QT_END_NAMESPACE

#include "moc_qquicklinearlayout_p.cpp"
