/****************************************************************************
**
** Copyright (C) 2014 Klaralvdalens Datakonsult AB (KDAB).
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt3D module of the Qt Toolkit.
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

#include "qsortpolicy_p.h"
#include <Qt3DCore/qpropertyvalueaddedchange.h>
#include <Qt3DCore/qpropertyvalueremovedchange.h>
#include <Qt3DRender/qframegraphnodecreatedchange.h>

QT_BEGIN_NAMESPACE

using namespace Qt3DCore;

namespace Qt3DRender {

QSortPolicyPrivate::QSortPolicyPrivate()
    : QFrameGraphNodePrivate()
{
}

/*!
    \class Qt3DRender::QSortPolicy
    \inmodule Qt3DRender
    \brief Provides storage for the sort types to be used.
    \since 5.7

    \inherits Qt3DRender::QFrameGraphNode

    A Qt3DRender::QSortPolicy class stores the sorting type used by the FrameGraph.
    The sort types determine how drawable entities are sorted before drawing to
    determine the drawing order. When QSortPolicy is present in the FrameGraph,
    the sorting mechanism is determined by the sortTypes list. Multiple sort types
    can be used simultaneously. If QSortPolicy is not present in the FrameGraph,
    entities are drawn in the order they appear in the entity hierarchy.
 */

/*!
    \qmltype SortPolicy
    \inqmlmodule Qt3D.Render
    \since 5.7
    \instantiates Qt3DRender::QSortPolicy
    \inherits FrameGraphNode
    \brief Provides storage for the sort types to be used.

    A SortPolicy class stores the sorting type used by the FrameGraph.
    The sort types determine how drawable entities are sorted before drawing to
    determine the drawing order. When SortPolicy is present in the FrameGraph,
    the sorting mechanism is determined by the sortTypes list. Multiple sort
    types can be used simultaneously. If SortPolicy is not present in the FrameGraph,
    entities are drawn in the order they appear in the entity hierarchy.
 */

/*!
    \enum QSortPolicy::SortType

    This enum type describes the available sort types.

    \value StateChangeCost sort the objects so as to minimize the cost of
    changing from the currently rendered state

    \value BackToFront sort the objects from back to front based on inverted z
    order. More accurately, the sorting key is the z component of the
    projection of the camera-to-object-center vector onto the camera's view
    vector.

    \value Material sort the objects based on their material value

    \value FrontToBack sort the objects from front to back. The opposite of
    BackToFront.
*/

/*!
    \property QSortPolicy::sortTypes
    Specifies the sorting types to be used.
*/

/*!
    \qmlproperty list<int> SortPolicy::sortTypes
    Specifies the sorting types to be used.

    This list can include the following values:
    \list
    \li StateChangeCost - sort the objects so as to minimize the cost of
        changing from the currently rendered state
    \li BackToFront - sort the objects from back to front based on inverted z
        order. More accurately, the sorting key is the z component of the
        projection of the camera-to-object-center vector onto the camera's view
        vector.
    \li Material - sort the objects based on their material value
    \li FrontToBack - sort the objects from front to back. The opposite of
        BackToFront.
    \endlist
*/

/*!
    Constructs QSortPolicy with given \a parent.
 */
QSortPolicy::QSortPolicy(QNode *parent)
    : QFrameGraphNode(*new QSortPolicyPrivate, parent)
{
}

/*! \internal */
QSortPolicy::~QSortPolicy()
{
}

/*! \internal */
QSortPolicy::QSortPolicy(QSortPolicyPrivate &dd, QNode *parent)
    : QFrameGraphNode(dd, parent)
{
}

QNodeCreatedChangeBasePtr QSortPolicy::createNodeCreationChange() const
{
    auto creationChange = QFrameGraphNodeCreatedChangePtr<QSortPolicyData>::create(this);
    QSortPolicyData &data = creationChange->data;
    Q_D(const QSortPolicy);
    data.sortTypes = d->m_sortTypes;
    return creationChange;
}

/*!
    \return the current sort types in use
 */
QVector<QSortPolicy::SortType> QSortPolicy::sortTypes() const
{
    Q_D(const QSortPolicy);
    return d->m_sortTypes;
}

QVector<int> QSortPolicy::sortTypesInt() const
{
    Q_D(const QSortPolicy);
    QVector<int> sortTypesInt;
    transformVector(d->m_sortTypes, sortTypesInt);
    return sortTypesInt;
}

void QSortPolicy::setSortTypes(const QVector<SortType> &sortTypes)
{
    Q_D(QSortPolicy);
    if (sortTypes != d->m_sortTypes) {
        d->m_sortTypes = sortTypes;
        emit sortTypesChanged(sortTypes);
        emit sortTypesChanged(sortTypesInt());
    }
}

void QSortPolicy::setSortTypes(const QVector<int> &sortTypesInt)
{
    QVector<SortType> sortTypes;
    transformVector(sortTypesInt, sortTypes);
    setSortTypes(sortTypes);
}

} // namespace Qt3DRender

QT_END_NAMESPACE
