/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Layouts module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquicklayout_p.h"
#include <QEvent>
#include <QtCore/qcoreapplication.h>
#include <QtCore/private/qnumeric_p.h>
#include <QtCore/qstack.h>
#include <QtCore/qmath.h>
#include <QtQml/qqmlinfo.h>
#include <limits>

/*!
    \qmltype Layout
    \instantiates QQuickLayoutAttached
    \inqmlmodule QtQuick.Layouts
    \ingroup layouts
    \brief Provides attached properties for items pushed onto a \l GridLayout,
    \l RowLayout or \l ColumnLayout.

    An object of type Layout is attached to children of the layout to provide layout specific
    information about the item.
    The properties of the attached object influence how the layout will arrange the items.

    For instance, you can specify \l minimumWidth, \l preferredWidth, and
    \l maximumWidth if the default values are not satisfactory.

    When a layout is resized, items may grow or shrink. Due to this, items have a
    \l{Layout::minimumWidth}{minimum size}, \l{Layout::preferredWidth}{preferred size} and a
    \l{Layout::maximumWidth}{maximum size}.

    If minimum size has not been explicitly specified on an item, the size is set to \c 0.
    If maximum size has not been explicitly specified on an item, the size is set to
    \c Number.POSITIVE_INFINITY.

    For layouts, the implicit minimum and maximum sizes depend on the content of the layouts.

    The \l fillWidth and \l fillHeight properties can either be \c true or \c false. If they are \c
    false, the item's size will be fixed to its preferred size. Otherwise, it will grow or shrink
    between its minimum and maximum size as the layout is resized.

    \note Do not bind to the x, y, width, or height properties of items in a layout,
    as this would conflict with the goals of Layout, and can also cause binding loops.
    The width and height properties are used by the layout engine to store the current
    size of items as calculated from the minimum/preferred/maximum attached properties,
    and can be ovewritten each time the items are laid out. Use
    \l {Layout::preferredWidth}{Layout.preferredWidth} and
    \l {Layout::preferredHeight}{Layout.preferredHeight}, or \l {Item::}{implicitWidth}
    and \l {Item::}{implicitHeight} to specify the preferred size of items.

    \sa GridLayout
    \sa RowLayout
    \sa ColumnLayout
*/

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQuickLayouts, "qt.quick.layouts")

QQuickLayoutAttached::QQuickLayoutAttached(QObject *parent)
    : QObject(parent),
      m_minimumWidth(0),
      m_minimumHeight(0),
      m_preferredWidth(-1),
      m_preferredHeight(-1),
      m_maximumWidth(std::numeric_limits<qreal>::infinity()),
      m_maximumHeight(std::numeric_limits<qreal>::infinity()),
      m_defaultMargins(0),
      m_fallbackWidth(-1),
      m_fallbackHeight(-1),
      m_row(-1),
      m_column(-1),
      m_rowSpan(1),
      m_columnSpan(1),
      m_fillWidth(false),
      m_fillHeight(false),
      m_isFillWidthSet(false),
      m_isFillHeightSet(false),
      m_isMinimumWidthSet(false),
      m_isMinimumHeightSet(false),
      m_isMaximumWidthSet(false),
      m_isMaximumHeightSet(false),
      m_changesNotificationEnabled(true),
      m_isLeftMarginSet(false),
      m_isTopMarginSet(false),
      m_isRightMarginSet(false),
      m_isBottomMarginSet(false)
{

}

/*!
    \qmlattachedproperty real Layout::minimumWidth

    This property holds the minimum width of an item in a layout.
    The default value is the item's implicit minimum width.

    If the item is a layout, the implicit minimum width will be the minimum width the layout can
    have without any of its items shrinking below their minimum width.
    The implicit minimum width for any other item is \c 0.

    Setting this value to -1 will reset the width back to its implicit minimum width.


    \sa preferredWidth
    \sa maximumWidth
*/
void QQuickLayoutAttached::setMinimumWidth(qreal width)
{
    if (qt_is_nan(width))
        return;
    m_isMinimumWidthSet = width >= 0;
    if (m_minimumWidth == width)
        return;

    m_minimumWidth = width;
    invalidateItem();
    emit minimumWidthChanged();
}

/*!
    \qmlattachedproperty real Layout::minimumHeight

    This property holds the minimum height of an item in a layout.
    The default value is the item's implicit minimum height.

    If the item is a layout, the implicit minimum height will be the minimum height the layout can
    have without any of its items shrinking below their minimum height.
    The implicit minimum height for any other item is \c 0.

    Setting this value to -1 will reset the height back to its implicit minimum height.

    \sa preferredHeight
    \sa maximumHeight
*/
void QQuickLayoutAttached::setMinimumHeight(qreal height)
{
    if (qt_is_nan(height))
        return;
    m_isMinimumHeightSet = height >= 0;
    if (m_minimumHeight == height)
        return;

    m_minimumHeight = height;
    invalidateItem();
    emit minimumHeightChanged();
}

/*!
    \qmlattachedproperty real Layout::preferredWidth

    This property holds the preferred width of an item in a layout.
    If the preferred width is \c -1 it will be ignored, and the layout
    will use \l{Item::implicitWidth}{implicitWidth} instead.
    The default is \c -1.

    \sa minimumWidth
    \sa maximumWidth
*/
void QQuickLayoutAttached::setPreferredWidth(qreal width)
{
    if (qt_is_nan(width) || m_preferredWidth == width)
        return;

    m_preferredWidth = width;
    invalidateItem();
    emit preferredWidthChanged();
}

/*!
    \qmlattachedproperty real Layout::preferredHeight

    This property holds the preferred height of an item in a layout.
    If the preferred height is \c -1 it will be ignored, and the layout
    will use \l{Item::implicitHeight}{implicitHeight} instead.
    The default is \c -1.

    \sa minimumHeight
    \sa maximumHeight
*/
void QQuickLayoutAttached::setPreferredHeight(qreal height)
{
    if (qt_is_nan(height) || m_preferredHeight == height)
        return;

    m_preferredHeight = height;
    invalidateItem();
    emit preferredHeightChanged();
}

/*!
    \qmlattachedproperty real Layout::maximumWidth

    This property holds the maximum width of an item in a layout.
    The default value is the item's implicit maximum width.

    If the item is a layout, the implicit maximum width will be the maximum width the layout can
    have without any of its items growing beyond their maximum width.
    The implicit maximum width for any other item is \c Number.POSITIVE_INFINITY.

    Setting this value to \c -1 will reset the width back to its implicit maximum width.

    \sa minimumWidth
    \sa preferredWidth
*/
void QQuickLayoutAttached::setMaximumWidth(qreal width)
{
    if (qt_is_nan(width))
        return;
    m_isMaximumWidthSet = width >= 0;
    if (m_maximumWidth == width)
        return;

    m_maximumWidth = width;
    invalidateItem();
    emit maximumWidthChanged();
}

/*!
    \qmlattachedproperty real Layout::maximumHeight

    The default value is the item's implicit maximum height.

    If the item is a layout, the implicit maximum height will be the maximum height the layout can
    have without any of its items growing beyond their maximum height.
    The implicit maximum height for any other item is \c Number.POSITIVE_INFINITY.

    Setting this value to \c -1 will reset the height back to its implicit maximum height.

    \sa minimumHeight
    \sa preferredHeight
*/
void QQuickLayoutAttached::setMaximumHeight(qreal height)
{
    if (qt_is_nan(height))
        return;
    m_isMaximumHeightSet = height >= 0;
    if (m_maximumHeight == height)
        return;

    m_maximumHeight = height;
    invalidateItem();
    emit maximumHeightChanged();
}

void QQuickLayoutAttached::setMinimumImplicitSize(const QSizeF &sz)
{
    bool emitWidthChanged = false;
    bool emitHeightChanged = false;
    if (!m_isMinimumWidthSet && m_minimumWidth != sz.width()) {
        m_minimumWidth = sz.width();
        emitWidthChanged = true;
    }
    if (!m_isMinimumHeightSet && m_minimumHeight != sz.height()) {
        m_minimumHeight = sz.height();
        emitHeightChanged = true;
    }
    // Only invalidate the item once, and make sure we emit signal changed after the call to
    // invalidateItem()
    if (emitWidthChanged || emitHeightChanged) {
        invalidateItem();
        if (emitWidthChanged)
            emit minimumWidthChanged();
        if (emitHeightChanged)
            emit minimumHeightChanged();
    }
}

void QQuickLayoutAttached::setMaximumImplicitSize(const QSizeF &sz)
{
    bool emitWidthChanged = false;
    bool emitHeightChanged = false;
    if (!m_isMaximumWidthSet && m_maximumWidth != sz.width()) {
        m_maximumWidth = sz.width();
        emitWidthChanged = true;
    }
    if (!m_isMaximumHeightSet && m_maximumHeight != sz.height()) {
        m_maximumHeight = sz.height();
        emitHeightChanged = true;
    }
    // Only invalidate the item once, and make sure we emit changed signal after the call to
    // invalidateItem()
    if (emitWidthChanged || emitHeightChanged) {
        invalidateItem();
        if (emitWidthChanged)
            emit maximumWidthChanged();
        if (emitHeightChanged)
            emit maximumHeightChanged();
    }
}

/*!
    \qmlattachedproperty bool Layout::fillWidth

    If this property is \c true, the item will be as wide as possible while respecting
    the given constraints. If the property is \c false, the item will have a fixed width
    set to the preferred width.
    The default is \c false, except for layouts themselves, which default to \c true.

    \sa fillHeight
*/
void QQuickLayoutAttached::setFillWidth(bool fill)
{
    m_isFillWidthSet = true;
    if (m_fillWidth != fill) {
        m_fillWidth = fill;
        invalidateItem();
        emit fillWidthChanged();
    }
}

/*!
    \qmlattachedproperty bool Layout::fillHeight

    If this property is \c true, the item will be as tall as possible while respecting
    the given constraints. If the property is \c false, the item will have a fixed height
    set to the preferred height.
    The default is \c false, except for layouts themselves, which default to \c true.

    \sa fillWidth
*/
void QQuickLayoutAttached::setFillHeight(bool fill)
{
    m_isFillHeightSet = true;
    if (m_fillHeight != fill) {
        m_fillHeight = fill;
        invalidateItem();
        emit fillHeightChanged();
    }
}

/*!
    \qmlattachedproperty int Layout::row

    This property allows you to specify the row position of an item in a \l GridLayout.

    If both \l column and this property are not set, it is up to the layout to assign a cell to the item.

    The default value is \c 0.

    \sa column
    \sa rowSpan
*/
void QQuickLayoutAttached::setRow(int row)
{
    if (row >= 0 && row != m_row) {
        m_row = row;
        invalidateItem();
        emit rowChanged();
    }
}

/*!
    \qmlattachedproperty int Layout::column

    This property allows you to specify the column position of an item in a \l GridLayout.

    If both \l row and this property are not set, it is up to the layout to assign a cell to the item.

    The default value is \c 0.

    \sa row
    \sa columnSpan
*/
void QQuickLayoutAttached::setColumn(int column)
{
    if (column >= 0 && column != m_column) {
        m_column = column;
        invalidateItem();
        emit columnChanged();
    }
}


/*!
    \qmlattachedproperty Qt.Alignment Layout::alignment

    This property allows you to specify the alignment of an item within the cell(s) it occupies.

    The default value is \c 0, which means it will be \c{Qt.AlignVCenter | Qt.AlignLeft}.
    These defaults also apply if only a horizontal or vertical flag is specified:
    if only a horizontal flag is specified, the default vertical flag will be
    \c Qt.AlignVCenter, and if only a vertical flag is specified, the default
    horizontal flag will be \c Qt.AlignLeft.

    A valid alignment is a combination of the following flags:
    \list
    \li Qt::AlignLeft
    \li Qt::AlignHCenter
    \li Qt::AlignRight
    \li Qt::AlignTop
    \li Qt::AlignVCenter
    \li Qt::AlignBottom
    \li Qt::AlignBaseline
    \endlist

*/
void QQuickLayoutAttached::setAlignment(Qt::Alignment align)
{
    if (align != m_alignment) {
        m_alignment = align;
        if (QQuickLayout *layout = parentLayout()) {
            layout->setAlignment(item(), align);
            invalidateItem();
        }
        emit alignmentChanged();
    }
}

/*!
    \qmlattachedproperty real Layout::margins

    Sets the margins outside of an item to all have the same value. The item
    itself does not evaluate its own margins. It is the parent's responsibility
    to decide if it wants to evaluate the margins.

    Specifically, margins are only evaluated by ColumnLayout, RowLayout,
    GridLayout, and other layout-like containers, such as SplitView, where the
    effective cell size of an item will be increased as the margins are
    increased.

    Therefore, if an item with margins is a child of another \c Item, its
    position, size and implicit size will remain unchanged.

    Combining margins with alignment will align the item \e including its
    margins. For instance, a vertically-centered Item with a top margin of \c 1
    and a bottom margin of \c 9 will cause the Items effective alignment within
    the cell to be 4 pixels above the center.

    The default value is \c 0.

    \sa leftMargin
    \sa topMargin
    \sa rightMargin
    \sa bottomMargin

    \since QtQuick.Layouts 1.2
*/
void QQuickLayoutAttached::setMargins(qreal m)
{
    if (m == m_defaultMargins)
        return;

    m_defaultMargins = m;
    invalidateItem();
    if (!m_isLeftMarginSet && m_margins.left() != m)
        emit leftMarginChanged();
    if (!m_isTopMarginSet && m_margins.top() != m)
        emit topMarginChanged();
    if (!m_isRightMarginSet && m_margins.right() != m)
        emit rightMarginChanged();
    if (!m_isBottomMarginSet && m_margins.bottom() != m)
        emit bottomMarginChanged();
    emit marginsChanged();
}

/*!
    \qmlattachedproperty real Layout::leftMargin

    Specifies the left margin outside of an item.
    If the value is not set, it will use the value from \l margins.

    \sa margins

    \since QtQuick.Layouts 1.2
*/
void QQuickLayoutAttached::setLeftMargin(qreal m)
{
    const bool changed = leftMargin() != m;
    m_margins.setLeft(m);
    m_isLeftMarginSet = true;
    if (changed) {
        invalidateItem();
        emit leftMarginChanged();
    }
}

void QQuickLayoutAttached::resetLeftMargin()
{
    const bool changed = m_isLeftMarginSet && (m_defaultMargins != m_margins.left());
    m_isLeftMarginSet = false;
    if (changed) {
        invalidateItem();
        emit leftMarginChanged();
    }
}

/*!
    \qmlattachedproperty real Layout::topMargin

    Specifies the top margin outside of an item.
    If the value is not set, it will use the value from \l margins.

    \sa margins

    \since QtQuick.Layouts 1.2
*/
void QQuickLayoutAttached::setTopMargin(qreal m)
{
    const bool changed = topMargin() != m;
    m_margins.setTop(m);
    m_isTopMarginSet = true;
    if (changed) {
        invalidateItem();
        emit topMarginChanged();
    }
}

void QQuickLayoutAttached::resetTopMargin()
{
    const bool changed = m_isTopMarginSet && (m_defaultMargins != m_margins.top());
    m_isTopMarginSet = false;
    if (changed) {
        invalidateItem();
        emit topMarginChanged();
    }
}

/*!
    \qmlattachedproperty real Layout::rightMargin

    Specifies the right margin outside of an item.
    If the value is not set, it will use the value from \l margins.

    \sa margins

    \since QtQuick.Layouts 1.2
*/
void QQuickLayoutAttached::setRightMargin(qreal m)
{
    const bool changed = rightMargin() != m;
    m_margins.setRight(m);
    m_isRightMarginSet = true;
    if (changed) {
        invalidateItem();
        emit rightMarginChanged();
    }
}

void QQuickLayoutAttached::resetRightMargin()
{
    const bool changed = m_isRightMarginSet && (m_defaultMargins != m_margins.right());
    m_isRightMarginSet = false;
    if (changed) {
        invalidateItem();
        emit rightMarginChanged();
    }
}

/*!
    \qmlattachedproperty real Layout::bottomMargin

    Specifies the bottom margin outside of an item.
    If the value is not set, it will use the value from \l margins.

    \sa margins

    \since QtQuick.Layouts 1.2
*/
void QQuickLayoutAttached::setBottomMargin(qreal m)
{
    const bool changed = bottomMargin() != m;
    m_margins.setBottom(m);
    m_isBottomMarginSet = true;
    if (changed) {
        invalidateItem();
        emit bottomMarginChanged();
    }
}

void QQuickLayoutAttached::resetBottomMargin()
{
    const bool changed = m_isBottomMarginSet && (m_defaultMargins != m_margins.bottom());
    m_isBottomMarginSet = false;
    if (changed) {
        invalidateItem();
        emit bottomMarginChanged();
    }
}


/*!
    \qmlattachedproperty int Layout::rowSpan

    This property allows you to specify the row span of an item in a \l GridLayout.

    The default value is \c 1.

    \sa columnSpan
    \sa row
*/
void QQuickLayoutAttached::setRowSpan(int span)
{
    if (span != m_rowSpan) {
        m_rowSpan = span;
        invalidateItem();
        emit rowSpanChanged();
    }
}


/*!
    \qmlattachedproperty int Layout::columnSpan

    This property allows you to specify the column span of an item in a \l GridLayout.

    The default value is \c 1.

    \sa rowSpan
    \sa column
*/
void QQuickLayoutAttached::setColumnSpan(int span)
{
    if (span != m_columnSpan) {
        m_columnSpan = span;
        invalidateItem();
        emit columnSpanChanged();
    }
}


qreal QQuickLayoutAttached::sizeHint(Qt::SizeHint which, Qt::Orientation orientation) const
{
    qreal result = 0;
    if (QQuickLayout *layout = qobject_cast<QQuickLayout *>(item())) {
        const QSizeF sz = layout->sizeHint(which);
        result = (orientation == Qt::Horizontal ? sz.width() : sz.height());
    } else {
        if (which == Qt::MaximumSize)
            result = std::numeric_limits<qreal>::infinity();
    }
    return result;
}

void QQuickLayoutAttached::invalidateItem()
{
    qCDebug(lcQuickLayouts) << "QQuickLayoutAttached::invalidateItem";
    if (QQuickLayout *layout = parentLayout()) {
        layout->invalidate(item());
    }
}

QQuickLayout *QQuickLayoutAttached::parentLayout() const
{
    QQuickItem *parentItem = item();
    if (parentItem) {
        parentItem = parentItem->parentItem();
        return qobject_cast<QQuickLayout *>(parentItem);
    } else {
        qmlWarning(parent()) << "Layout must be attached to Item elements";
    }
    return nullptr;
}

QQuickItem *QQuickLayoutAttached::item() const
{
    return qobject_cast<QQuickItem *>(parent());
}

qreal QQuickLayoutPrivate::getImplicitWidth() const
{
    Q_Q(const QQuickLayout);
    if (q->invalidated()) {
        QQuickLayoutPrivate *that = const_cast<QQuickLayoutPrivate*>(this);
        that->implicitWidth = q->sizeHint(Qt::PreferredSize).width();
    }
    return implicitWidth;
}

qreal QQuickLayoutPrivate::getImplicitHeight() const
{
    Q_Q(const QQuickLayout);
    if (q->invalidated()) {
        QQuickLayoutPrivate *that = const_cast<QQuickLayoutPrivate*>(this);
        that->implicitHeight = q->sizeHint(Qt::PreferredSize).height();
    }
    return implicitHeight;
}

void QQuickLayoutPrivate::applySizeHints() const {
    Q_Q(const QQuickLayout);
    QQuickLayout *that = const_cast<QQuickLayout*>(q);
    QQuickLayoutAttached *info = attachedLayoutObject(that, true);

    const QSizeF min = q->sizeHint(Qt::MinimumSize);
    const QSizeF max = q->sizeHint(Qt::MaximumSize);
    const QSizeF pref = q->sizeHint(Qt::PreferredSize);
    info->setMinimumImplicitSize(min);
    info->setMaximumImplicitSize(max);
    that->setImplicitSize(pref.width(), pref.height());
}

QQuickLayout::QQuickLayout(QQuickLayoutPrivate &dd, QQuickItem *parent)
    : QQuickItem(dd, parent)
    , m_inUpdatePolish(false)
    , m_polishInsideUpdatePolish(0)
{
}

static QQuickItemPrivate::ChangeTypes changeTypes =
    QQuickItemPrivate::SiblingOrder
    | QQuickItemPrivate::ImplicitWidth
    | QQuickItemPrivate::ImplicitHeight
    | QQuickItemPrivate::Destroyed
    | QQuickItemPrivate::Visibility;

QQuickLayout::~QQuickLayout()
{
    d_func()->m_isReady = false;

    const auto childItems = d_func()->childItems;
    for (QQuickItem *child : childItems)
        QQuickItemPrivate::get(child)->removeItemChangeListener(this, changeTypes);
}

QQuickLayoutAttached *QQuickLayout::qmlAttachedProperties(QObject *object)
{
    return new QQuickLayoutAttached(object);
}

void QQuickLayout::updatePolish()
{
    qCDebug(lcQuickLayouts) << "updatePolish() ENTERING" << this;
    m_inUpdatePolish = true;

    // Might have become "undirty" before we reach this updatePolish()
    // (e.g. if somebody queried for implicitWidth it will immediately
    // calculate size hints)
    if (invalidated()) {
        // Ensure that all invalidated layouts are synced and valid again. Since
        // ensureLayoutItemsUpdated() will also call applySizeHints(), and sizeHint() will call its
        // childrens sizeHint(), and sizeHint() will call ensureLayoutItemsUpdated(), this will be done
        // recursive as we want.
        // Note that we need to call ensureLayoutItemsUpdated() *before* we query width() and height(),
        // because width()/height() might return their implicitWidth/implicitHeight (e.g. for a layout
        // with no explicitly specified size, (nor anchors.fill: parent))
        ensureLayoutItemsUpdated();
    }
    rearrange(QSizeF(width(), height()));
    m_inUpdatePolish = false;
    qCDebug(lcQuickLayouts) << "updatePolish() LEAVING" << this;
}

void QQuickLayout::componentComplete()
{
    Q_D(QQuickLayout);
    d->m_disableRearrange = true;
    QQuickItem::componentComplete();    // will call our geometryChanged(), (where isComponentComplete() == true)
    d->m_disableRearrange = false;
    d->m_isReady = true;
}

void QQuickLayout::invalidate(QQuickItem * /*childItem*/)
{
    Q_D(QQuickLayout);
    if (invalidated())
        return;

    qCDebug(lcQuickLayouts) << "QQuickLayout::invalidate()" << this;
    d->m_dirty = true;
    d->m_dirtyArrangement = true;

    if (!qobject_cast<QQuickLayout *>(parentItem())) {

        if (m_inUpdatePolish)
            ++m_polishInsideUpdatePolish;
        else
            m_polishInsideUpdatePolish = 0;

        if (m_polishInsideUpdatePolish <= 2) {
            // allow at most two consecutive loops in order to respond to height-for-width
            // (e.g QQuickText changes implicitHeight when its width gets changed)
            qCDebug(lcQuickLayouts) << "QQuickLayout::invalidate(), polish()";
            polish();
        } else {
            qWarning() << "Qt Quick Layouts: Polish loop detected. Aborting after two iterations.";
        }
    }
}

bool QQuickLayout::shouldIgnoreItem(QQuickItem *child, QQuickLayoutAttached *&info, QSizeF *sizeHints) const
{
    bool ignoreItem = true;
    QQuickItemPrivate *childPrivate = QQuickItemPrivate::get(child);
    if (childPrivate->explicitVisible) {
        effectiveSizeHints_helper(child, sizeHints, &info, true);
        QSizeF effectiveMaxSize = sizeHints[Qt::MaximumSize];
        if (!effectiveMaxSize.isNull()) {
            QSizeF &prefS = sizeHints[Qt::PreferredSize];
            if (effectiveSizePolicy_helper(child, Qt::Horizontal, info) == QLayoutPolicy::Fixed)
                effectiveMaxSize.setWidth(prefS.width());
            if (effectiveSizePolicy_helper(child, Qt::Vertical, info) == QLayoutPolicy::Fixed)
                effectiveMaxSize.setHeight(prefS.height());
        }
        ignoreItem = effectiveMaxSize.isNull();
    }

    if (!ignoreItem && childPrivate->isTransparentForPositioner())
        ignoreItem = true;

    return ignoreItem;
}

void QQuickLayout::checkAnchors(QQuickItem *item) const
{
    QQuickAnchors *anchors = QQuickItemPrivate::get(item)->_anchors;
    if (anchors && anchors->activeDirections())
        qmlWarning(item) << "Detected anchors on an item that is managed by a layout. This is undefined behavior; use Layout.alignment instead.";
}

void QQuickLayout::ensureLayoutItemsUpdated() const
{
    Q_D(const QQuickLayout);
    if (!invalidated())
        return;
    const_cast<QQuickLayout*>(this)->updateLayoutItems();
    d->m_dirty = false;
    d->applySizeHints();
}


void QQuickLayout::itemChange(ItemChange change, const ItemChangeData &value)
{
    if (change == ItemChildAddedChange) {
        Q_D(QQuickLayout);
        QQuickItem *item = value.item;
        qmlobject_connect(item, QQuickItem, SIGNAL(baselineOffsetChanged(qreal)), this, QQuickLayout, SLOT(invalidateSenderItem()));
        QQuickItemPrivate::get(item)->addItemChangeListener(this, changeTypes);
        d->m_hasItemChangeListeners = true;
        qCDebug(lcQuickLayouts) << "ChildAdded" << item;
        if (isReady())
            invalidate();
    } else if (change == ItemChildRemovedChange) {
        QQuickItem *item = value.item;
        qmlobject_disconnect(item, QQuickItem, SIGNAL(baselineOffsetChanged(qreal)), this, QQuickLayout, SLOT(invalidateSenderItem()));
        QQuickItemPrivate::get(item)->removeItemChangeListener(this, changeTypes);
        qCDebug(lcQuickLayouts) << "ChildRemoved" << item;
        if (isReady())
            invalidate();
    }
    QQuickItem::itemChange(change, value);
}

void QQuickLayout::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickLayout);
    QQuickItem::geometryChanged(newGeometry, oldGeometry);
    if (d->m_disableRearrange || !isReady() || !newGeometry.isValid())
        return;

    qCDebug(lcQuickLayouts) << "QQuickLayout::geometryChanged" << newGeometry << oldGeometry;
    rearrange(newGeometry.size());
}

void QQuickLayout::invalidateSenderItem()
{
    if (!isReady())
        return;
    QQuickItem *item = static_cast<QQuickItem *>(sender());
    Q_ASSERT(item);
    invalidate(item);
}

bool QQuickLayout::isReady() const
{
    return d_func()->m_isReady;
}

/*!
 * \brief QQuickLayout::deactivateRecur
 * \internal
 *
 * Call this from the dtor of the top-level layout.
 * Otherwise, it will trigger lots of unneeded item change listeners (itemVisibleChanged()) for all its descendants
 * that will have its impact thrown away.
 */
void QQuickLayout::deactivateRecur()
{
    if (d_func()->m_hasItemChangeListeners) {
        for (int i = 0; i < itemCount(); ++i) {
            QQuickItem *item = itemAt(i);
            // When deleting a layout with children, there is no reason for the children to inform the layout that their
            // e.g. visibility got changed. The layout already knows that all its children will eventually become invisible, so
            // we therefore remove its change listener.
            QQuickItemPrivate::get(item)->removeItemChangeListener(this, changeTypes);
            if (QQuickLayout *layout = qobject_cast<QQuickLayout*>(item))
                layout->deactivateRecur();
        }
        d_func()->m_hasItemChangeListeners = false;
    }
}

bool QQuickLayout::invalidated() const
{
    return d_func()->m_dirty;
}

bool QQuickLayout::invalidatedArrangement() const
{
    return d_func()->m_dirtyArrangement;
}

bool QQuickLayout::isMirrored() const
{
    return d_func()->isMirrored();
}

void QQuickLayout::itemSiblingOrderChanged(QQuickItem *item)
{
    Q_UNUSED(item);
    invalidate();
}

void QQuickLayout::itemImplicitWidthChanged(QQuickItem *item)
{
    if (!isReady() || item->signalsBlocked())
        return;
    invalidate(item);
}

void QQuickLayout::itemImplicitHeightChanged(QQuickItem *item)
{
    if (!isReady() || item->signalsBlocked())
        return;
    invalidate(item);
}

void QQuickLayout::itemDestroyed(QQuickItem *item)
{
    Q_UNUSED(item);
}

void QQuickLayout::itemVisibilityChanged(QQuickItem *item)
{
    Q_UNUSED(item);
}

void QQuickLayout::rearrange(const QSizeF &/*size*/)
{
    d_func()->m_dirtyArrangement = false;
}


/*
  The layout engine assumes:
    1. minimum <= preferred <= maximum
    2. descent is within minimum and maximum bounds     (### verify)

    This function helps to ensure that by the following rules (in the following order):
    1. If minimum > maximum, set minimum = maximum
    2. Clamp preferred to be between the [minimum,maximum] range.
    3. If descent > minimum, set descent = minimum      (### verify if this is correct, it might
                                                        need some refinements to multiline texts)

    If any values are "not set" (i.e. negative), they will be left untouched, so that we
    know which values needs to be fetched from the implicit hints (not user hints).
  */
static void normalizeHints(qreal &minimum, qreal &preferred, qreal &maximum, qreal &descent)
{
    if (minimum >= 0 && maximum >= 0 && minimum > maximum)
        minimum = maximum;

    if (preferred >= 0) {
        if (minimum >= 0 && preferred < minimum) {
            preferred = minimum;
        } else if (maximum >= 0 && preferred > maximum) {
            preferred = maximum;
        }
    }

    if (minimum >= 0 && descent > minimum)
        descent = minimum;
}

static void boundSize(QSizeF &result, const QSizeF &size)
{
    if (size.width() >= 0 && size.width() < result.width())
        result.setWidth(size.width());
    if (size.height() >= 0 && size.height() < result.height())
        result.setHeight(size.height());
}

static void expandSize(QSizeF &result, const QSizeF &size)
{
    if (size.width() >= 0 && size.width() > result.width())
        result.setWidth(size.width());
    if (size.height() >= 0 && size.height() > result.height())
        result.setHeight(size.height());
}

static inline void combineHints(qreal &current, qreal fallbackHint)
{
    if (current < 0)
        current = fallbackHint;
}

static inline void combineSize(QSizeF &result, const QSizeF &fallbackSize)
{
    combineHints(result.rwidth(), fallbackSize.width());
    combineHints(result.rheight(), fallbackSize.height());
}

static inline void combineImplicitHints(QQuickLayoutAttached *info, Qt::SizeHint which, QSizeF *size)
{
    if (!info) return;

    Q_ASSERT(which == Qt::MinimumSize || which == Qt::MaximumSize);

    const QSizeF constraint(which == Qt::MinimumSize
                            ? QSizeF(info->minimumWidth(), info->minimumHeight())
                            : QSizeF(info->maximumWidth(), info->maximumHeight()));

    if (!info->isExtentExplicitlySet(Qt::Horizontal, which))
        combineHints(size->rwidth(),  constraint.width());
    if (!info->isExtentExplicitlySet(Qt::Vertical, which))
        combineHints(size->rheight(), constraint.height());
}

typedef qreal (QQuickLayoutAttached::*SizeGetter)() const;

/*!
    \internal
    Note: Can potentially return the attached QQuickLayoutAttached object through \a attachedInfo.

    It is like this is because it enables it to be reused.

    The goal of this function is to return the effective minimum, preferred and maximum size hints
    that the layout will use for this item.
    This function takes care of gathering all explicitly set size hints, normalizes them so
    that min < pref < max.
    Further, the hints _not_explicitly_ set will then be initialized with the implicit size hints,
    which is usually derived from the content of the layouts (or items).

    The following table illustrates the preference of the properties used for measuring layout
    items. If present, the USER properties will be preferred. If USER properties are not present,
    the HINT properties will be preferred. Finally, the FALLBACK properties will be used as an
    ultimate fallback.

    Note that one can query if the value of Layout.minimumWidth or Layout.maximumWidth has been
    explicitly or implicitly set with QQuickLayoutAttached::isExtentExplicitlySet(). This
    determines if it should be used as a USER or as a HINT value.

    Fractional size hints will be ceiled to the closest integer. This is in order to give some
    slack when the items are snapped to the pixel grid.

                 | *Minimum*            | *Preferred*           | *Maximum*                |
+----------------+----------------------+-----------------------+--------------------------+
|USER (explicit) | Layout.minimumWidth  | Layout.preferredWidth | Layout.maximumWidth      |
|HINT (implicit) | Layout.minimumWidth  | implicitWidth         | Layout.maximumWidth      |
|FALLBACK        | 0                    | width                 | Number.POSITIVE_INFINITY |
+----------------+----------------------+-----------------------+--------------------------+
 */
void QQuickLayout::effectiveSizeHints_helper(QQuickItem *item, QSizeF *cachedSizeHints, QQuickLayoutAttached **attachedInfo, bool useFallbackToWidthOrHeight)
{
    for (int i = 0; i < Qt::NSizeHints; ++i)
        cachedSizeHints[i] = QSizeF();
    QQuickLayoutAttached *info = attachedLayoutObject(item, false);
    // First, retrieve the user-specified hints from the attached "Layout." properties
    if (info) {
        struct Getters {
            SizeGetter call[NSizes];
        };

        static Getters horGetters = {
            {&QQuickLayoutAttached::minimumWidth, &QQuickLayoutAttached::preferredWidth, &QQuickLayoutAttached::maximumWidth},
        };

        static Getters verGetters = {
            {&QQuickLayoutAttached::minimumHeight, &QQuickLayoutAttached::preferredHeight, &QQuickLayoutAttached::maximumHeight}
        };
        for (int i = 0; i < NSizes; ++i) {
            SizeGetter getter = horGetters.call[i];
            Q_ASSERT(getter);

            if (info->isExtentExplicitlySet(Qt::Horizontal, (Qt::SizeHint)i))
                cachedSizeHints[i].setWidth((info->*getter)());

            getter = verGetters.call[i];
            Q_ASSERT(getter);
            if (info->isExtentExplicitlySet(Qt::Vertical, (Qt::SizeHint)i))
                cachedSizeHints[i].setHeight((info->*getter)());
        }
    }

    QSizeF &minS = cachedSizeHints[Qt::MinimumSize];
    QSizeF &prefS = cachedSizeHints[Qt::PreferredSize];
    QSizeF &maxS = cachedSizeHints[Qt::MaximumSize];
    QSizeF &descentS = cachedSizeHints[Qt::MinimumDescent];

    // For instance, will normalize the following user-set hints
    // from: [10, 5, 60]
    // to:   [10, 10, 60]
    normalizeHints(minS.rwidth(), prefS.rwidth(), maxS.rwidth(), descentS.rwidth());
    normalizeHints(minS.rheight(), prefS.rheight(), maxS.rheight(), descentS.rheight());

    // All explicit values gathered, now continue to gather the implicit sizes

    //--- GATHER MAXIMUM SIZE HINTS ---
    combineImplicitHints(info, Qt::MaximumSize, &maxS);
    combineSize(maxS, QSizeF(std::numeric_limits<qreal>::infinity(), std::numeric_limits<qreal>::infinity()));
    // implicit max or min sizes should not limit an explicitly set preferred size
    expandSize(maxS, prefS);
    expandSize(maxS, minS);

    //--- GATHER MINIMUM SIZE HINTS ---
    combineImplicitHints(info, Qt::MinimumSize, &minS);
    expandSize(minS, QSizeF(0,0));
    boundSize(minS, prefS);
    boundSize(minS, maxS);

    //--- GATHER PREFERRED SIZE HINTS ---
    // First, from implicitWidth/Height
    qreal &prefWidth = prefS.rwidth();
    qreal &prefHeight = prefS.rheight();
    if (prefWidth < 0 && item->implicitWidth() > 0)
        prefWidth = qCeil(item->implicitWidth());
    if (prefHeight < 0 &&  item->implicitHeight() > 0)
        prefHeight = qCeil(item->implicitHeight());

    // If that fails, make an ultimate fallback to width/height
    if (useFallbackToWidthOrHeight && !prefS.isValid()) {
        /* If we want to support using width/height as preferred size hints in
           layouts, (which we think most people expect), we only want to use the
           initial width.
           This is because the width will change due to layout rearrangement,
           and the preferred width should return the same value, regardless of
           the current width.
           We therefore store this initial width in the attached layout object
           and reuse it if needed rather than querying the width another time.
           That means we need to ensure that an Layout attached object is available
           by creating one if necessary.
        */
        if (!info)
            info = attachedLayoutObject(item);

        auto updatePreferredSizes = [](qreal &cachedSize, qreal &attachedSize, qreal size) {
            if (cachedSize < 0) {
                if (attachedSize < 0)
                    attachedSize = size;

                cachedSize = attachedSize;
            }
        };
        updatePreferredSizes(prefWidth, info->m_fallbackWidth, item->width());
        updatePreferredSizes(prefHeight, info->m_fallbackHeight, item->height());
    }

    // Normalize again after the implicit hints have been gathered
    expandSize(prefS, minS);
    boundSize(prefS, maxS);

    //--- GATHER DESCENT
    // Minimum descent is only applicable for the effective minimum height,
    // so we gather the descent last.
    const qreal minimumDescent = minS.height() - item->baselineOffset();
    descentS.setHeight(minimumDescent);

    if (info) {
        QMarginsF margins = info->qMargins();
        QSizeF extraMargins(margins.left() + margins.right(), margins.top() + margins.bottom());
        minS += extraMargins;
        prefS += extraMargins;
        maxS += extraMargins;
        descentS += extraMargins;
    }
    if (attachedInfo)
        *attachedInfo = info;
}

/*!
    \internal

    Assumes \a info is set (if the object has an attached property)
 */
QLayoutPolicy::Policy QQuickLayout::effectiveSizePolicy_helper(QQuickItem *item, Qt::Orientation orientation, QQuickLayoutAttached *info)
{
    bool fillExtent = false;
    bool isSet = false;
    if (info) {
        if (orientation == Qt::Horizontal) {
            isSet = info->isFillWidthSet();
            if (isSet) fillExtent = info->fillWidth();
        } else {
            isSet = info->isFillHeightSet();
            if (isSet) fillExtent = info->fillHeight();
        }
    }
    if (!isSet && qobject_cast<QQuickLayout*>(item))
        fillExtent = true;
    return fillExtent ? QLayoutPolicy::Preferred : QLayoutPolicy::Fixed;

}

void QQuickLayout::_q_dumpLayoutTree() const
{
    QString buf;
    dumpLayoutTreeRecursive(0, buf);
    qDebug("\n%s", qPrintable(buf));
}

void QQuickLayout::dumpLayoutTreeRecursive(int level, QString &buf) const
{
    auto formatLine = [&level](const char *fmt) {
        QString ss(level *4, QLatin1Char(' '));
        return QString::fromLatin1("%1%2\n").arg(ss).arg(fmt);
    };

    auto f2s = [](qreal f) {
        return QString::number(f);
    };
    auto b2s = [](bool b) {
        static const char *strBool[] = {"false", "true"};
        return QLatin1String(strBool[int(b)]);
    };

    buf += formatLine("%1 {").arg(QQmlMetaType::prettyTypeName(this));
    ++level;
    buf += formatLine("// Effective calculated values:");
    buf += formatLine("sizeHintDirty: %2").arg(invalidated());
    QSizeF min = sizeHint(Qt::MinimumSize);
    buf += formatLine("sizeHint.min : [%1, %2]").arg(f2s(min.width()), 5).arg(min.height(), 5);
    QSizeF pref = sizeHint(Qt::PreferredSize);
    buf += formatLine("sizeHint.pref: [%1, %2]").arg(pref.width(), 5).arg(pref.height(), 5);
    QSizeF max = sizeHint(Qt::MaximumSize);
    buf += formatLine("sizeHint.max : [%1, %2]").arg(f2s(max.width()), 5).arg(f2s(max.height()), 5);

    for (QQuickItem *item : childItems()) {
        buf += QLatin1Char('\n');
        if (QQuickLayout *childLayout = qobject_cast<QQuickLayout*>(item)) {
            childLayout->dumpLayoutTreeRecursive(level, buf);
        } else {
            buf += formatLine("%1 {").arg(QQmlMetaType::prettyTypeName(item));
            ++level;
            if (item->implicitWidth() > 0)
                buf += formatLine("implicitWidth: %1").arg(f2s(item->implicitWidth()));
            if (item->implicitHeight() > 0)
                buf += formatLine("implicitHeight: %1").arg(f2s(item->implicitHeight()));
            QSizeF min;
            QSizeF pref;
            QSizeF max;
            QQuickLayoutAttached *info = attachedLayoutObject(item, false);
            if (info) {
                min = QSizeF(info->minimumWidth(), info->minimumHeight());
                pref = QSizeF(info->preferredWidth(), info->preferredHeight());
                max = QSizeF(info->maximumWidth(), info->maximumHeight());
                if (info->isExtentExplicitlySet(Qt::Horizontal, Qt::MinimumSize))
                    buf += formatLine("Layout.minimumWidth: %1").arg(f2s(min.width()));
                if (info->isExtentExplicitlySet(Qt::Vertical, Qt::MinimumSize))
                    buf += formatLine("Layout.minimumHeight: %1").arg(f2s(min.height()));
                if (pref.width() >= 0)
                    buf += formatLine("Layout.preferredWidth: %1").arg(f2s(pref.width()));
                if (pref.height() >= 0)
                    buf += formatLine("Layout.preferredHeight: %1").arg(f2s(pref.height()));
                if (info->isExtentExplicitlySet(Qt::Horizontal, Qt::MaximumSize))
                    buf += formatLine("Layout.maximumWidth: %1").arg(f2s(max.width()));
                if (info->isExtentExplicitlySet(Qt::Vertical, Qt::MaximumSize))
                    buf += formatLine("Layout.maximumHeight: %1").arg(f2s(max.height()));

                if (info->isFillWidthSet())
                    buf += formatLine("Layout.fillWidth: %1").arg(b2s(info->fillWidth()));
                if (info->isFillHeightSet())
                    buf += formatLine("Layout.fillHeight: %1").arg(b2s(info->fillHeight()));
            }
            --level;
            buf += formatLine("}");
        }
    }
    --level;
    buf += formatLine("}");
}

QT_END_NAMESPACE
