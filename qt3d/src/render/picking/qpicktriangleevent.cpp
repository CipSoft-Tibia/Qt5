// Copyright (C) 2015 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpicktriangleevent.h"
#include "qpicktriangleevent_p.h"
#include "qpickevent_p.h"
#include <private/qobject_p.h>

QT_BEGIN_NAMESPACE

namespace Qt3DRender {


Qt3DRender::QPickTriangleEventPrivate::QPickTriangleEventPrivate()
    : QPickEventPrivate()
    , m_triangleIndex(0)
    , m_vertex1Index(0)
    , m_vertex2Index(0)
    , m_vertex3Index(0)
{
}

const QPickTriangleEventPrivate *QPickTriangleEventPrivate::get(const QPickTriangleEvent *ev)
{
    return ev->d_func();
}

QPickTriangleEvent *QPickTriangleEventPrivate::clone() const
{
    auto res = new QPickTriangleEvent();
    res->d_func()->m_accepted = m_accepted;
    res->d_func()->m_position = m_position;
    res->d_func()->m_worldIntersection = m_worldIntersection;
    res->d_func()->m_localIntersection = m_localIntersection;
    res->d_func()->m_distance = m_distance;
    res->d_func()->m_button = m_button;
    res->d_func()->m_buttons = m_buttons;
    res->d_func()->m_modifiers = m_modifiers;
    res->d_func()->m_entity = m_entity;
    res->d_func()->m_entityPtr = m_entityPtr;
    res->d_func()->m_viewport = m_viewport;
    res->d_func()->m_triangleIndex = m_triangleIndex;
    res->d_func()->m_vertex1Index = m_vertex1Index;
    res->d_func()->m_vertex2Index = m_vertex2Index;
    res->d_func()->m_vertex3Index = m_vertex3Index;
    return res;
}


/*!
    \class Qt3DRender::QPickTriangleEvent
    \inmodule Qt3DRender

    \brief The QPickTriangleEvent class holds information when a triangle is picked.

    When QPickingSettings::pickMode() is set to QPickingSettings::TrianglePicking, the signals
    on QObjectPicker will carry an instance of QPickTriangleEvent.

    This contains the details of the triangle that was picked.

    \note In the case of indexed rendering, the point indices are relative to the
    array of coordinates, not the array of indices.

    \sa QPickingSettings, QPickEvent, QObjectPicker
    \since 5.7
*/

/*!
 * \qmltype PickTriangleEvent
 * \instantiates Qt3DRender::QPickTriangleEvent
 * \inqmlmodule Qt3D.Render
 * \brief PickTriangleEvent holds information when a triangle is picked.
 *
 * When QPickingSettings::pickMode() is set to QPickingSettings::TrianglePicking, the signals
 * on QObjectPicker will carry an instance of QPickTriangleEvent.
 *
 * This contains the details of the triangle that was picked.
 *
 * \note In case of indexed rendering, the point indices are relative to the
 * array of indices, not the array of coordinates.
 *
 * \sa PickingSettings, PickEvent, ObjectPicker, Attribute
 */


/*!
  \fn Qt3DRender::QPickTriangleEvent::QPickTriangleEvent()
  Constructs a new QPickEvent.
 */
QPickTriangleEvent::QPickTriangleEvent()
    : QPickEvent(*new QPickTriangleEventPrivate())
{
}

/*!
 * \brief QPickTriangleEvent::QPickTriangleEvent Constructs a new QPickEvent with the given parameters
 * \a position,
 * \a worldIntersection,
 * \a localIntersection,
 * \a distance,
 * \a triangleIndex,
 * \a vertex1Index,
 * \a vertex2Index and
 * \a vertex3Index

//! NOTE: remove in Qt6
 */
QPickTriangleEvent::QPickTriangleEvent(const QPointF &position, const QVector3D &worldIntersection, const QVector3D &localIntersection, float distance,
                                       uint triangleIndex, uint vertex1Index, uint vertex2Index,
                                       uint vertex3Index)
    : QPickEvent(*new QPickTriangleEventPrivate())
{
    Q_D(QPickTriangleEvent);
    d->m_position = position;
    d->m_distance = distance;
    d->m_worldIntersection = worldIntersection;
    d->m_localIntersection = localIntersection;
    d->m_triangleIndex = triangleIndex;
    d->m_vertex1Index = vertex1Index;
    d->m_vertex2Index = vertex2Index;
    d->m_vertex3Index = vertex3Index;
}

QPickTriangleEvent::QPickTriangleEvent(const QPointF &position, const QVector3D &worldIntersection,
                                       const QVector3D &localIntersection, float distance,
                                       uint triangleIndex, uint vertex1Index, uint vertex2Index,
                                       uint vertex3Index, QPickEvent::Buttons button, int buttons,
                                       int modifiers, const QVector3D &uvw)
    : QPickEvent(*new QPickTriangleEventPrivate())
{
    Q_D(QPickTriangleEvent);
    d->m_position = position;
    d->m_distance = distance;
    d->m_worldIntersection = worldIntersection;
    d->m_localIntersection = localIntersection;
    d->m_triangleIndex = triangleIndex;
    d->m_vertex1Index = vertex1Index;
    d->m_vertex2Index = vertex2Index;
    d->m_vertex3Index = vertex3Index;
    d->m_button = button;
    d->m_buttons = buttons;
    d->m_modifiers = modifiers;
    d->m_uvw = uvw;
}

/*! \internal */
QPickTriangleEvent::~QPickTriangleEvent()
{
}

/*!
    \qmlproperty uint Qt3D.Render::PickTriangleEvent::triangleIndex
    Specifies the triangle index of the event
*/
/*!
  \property Qt3DRender::QPickTriangleEvent::triangleIndex
    Specifies the triangle index of the event
 */
/*!
 * \brief QPickTriangleEvent::triangleIndex
 * Returns the index of the picked triangle
 */
uint QPickTriangleEvent::triangleIndex() const
{
    Q_D(const QPickTriangleEvent);
    return d->m_triangleIndex;
}

/*!
    \qmlproperty uint Qt3D.Render::PickTriangleEvent::vertex1Index
    Specifies the index of the first vertex in the triangle
*/
/*!
  \property Qt3DRender::QPickTriangleEvent::vertex1Index
    Specifies the index of the first vertex in the triangle
 */
/*!
 * \brief QPickTriangleEvent::vertex1Index
 * Returns the index of the first point of the picked triangle
 */
uint QPickTriangleEvent::vertex1Index() const
{
    Q_D(const QPickTriangleEvent);
    return d->m_vertex1Index;
}

/*!
    \qmlproperty uint Qt3D.Render::PickTriangleEvent::vertex2Index
    Specifies the index of the second vertex in the triangle
*/
/*!
  \property Qt3DRender::QPickTriangleEvent::vertex2Index
    Specifies the index of the second vertex in the triangle
 */
/*!
 * \brief QPickTriangleEvent::vertex2Index
 * Returns the index of the second point of the picked triangle
 */
uint QPickTriangleEvent::vertex2Index() const
{
    Q_D(const QPickTriangleEvent);
    return d->m_vertex2Index;
}

/*!
    \qmlproperty uint Qt3D.Render::PickTriangleEvent::vertex3Index
    Specifies the index of the third vertex in the triangle
*/
/*!
  \property Qt3DRender::QPickTriangleEvent::vertex3Index
    Specifies the index of the third vertex in the triangle
 */
/*!
 * \brief QPickTriangleEvent::vertex3Index
 * Returns index of third point of picked triangle
 */
uint QPickTriangleEvent::vertex3Index() const
{
    Q_D(const QPickTriangleEvent);
    return d->m_vertex3Index;
}

/*!
    \property Qt3DRender::QPickTriangleEvent::uvw

*/
/*!
    Returns the 3D coordinates u,v, and w.
*/
QVector3D QPickTriangleEvent::uvw() const
{
    Q_D(const QPickTriangleEvent);
    return d->m_uvw;
}

} // Qt3DRender

QT_END_NAMESPACE

#include "moc_qpicktriangleevent.cpp"

