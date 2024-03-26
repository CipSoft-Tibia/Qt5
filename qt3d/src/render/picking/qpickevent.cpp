// Copyright (C) 2015 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpickevent.h"
#include "qpickevent_p.h"
#include <private/qobject_p.h>

QT_BEGIN_NAMESPACE

namespace Qt3DRender {

/*!
    \class Qt3DRender::QPickEvent
    \inmodule Qt3DRender

    \brief The QPickEvent class holds information when an object is picked.

    This is received as a parameter in most of the QObjectPicker component signals when picking
    succeeds.

    \sa QPickingSettings, QPickTriangleEvent, QObjectPicker

    \since 5.7
*/

/*!
 * \qmltype PickEvent
 * \instantiates Qt3DRender::QPickEvent
 * \inqmlmodule Qt3D.Render
 * \sa ObjectPicker PickingSettings
 * \brief PickEvent holds information when an object is picked.
 * This is received as a parameter in most of the QObjectPicker component signals when picking
 * succeeds.
 */

/*!
  \fn Qt3DRender::QPickEvent::QPickEvent()
  Constructs a new QPickEvent.
 */
QPickEvent::QPickEvent()
    : QObject(*new QPickEventPrivate())
{
}

QPickEventPrivate *QPickEventPrivate::get(QPickEvent *object)
{
    return object->d_func();
}

/*!
  \fn Qt3DRender::QPickEvent::QPickEvent(const QPointF &position, const QVector3D &intersection, const QVector3D &localIntersection, float distance)
  Constructs a new QPickEvent with the given parameters: \a position, \a intersection, \a localIntersection and \a distance
 */
// NOTE: remove in Qt6
QPickEvent::QPickEvent(const QPointF &position, const QVector3D &worldIntersection, const QVector3D &localIntersection,
                       float distance)
    : QObject(*new QPickEventPrivate())
{
    Q_D(QPickEvent);
    d->m_position = position;
    d->m_distance = distance;
    d->m_worldIntersection = worldIntersection;
    d->m_localIntersection = localIntersection;
}

/*!
  Constructs a new QPickEvent with the given parameters: \a position, \a worldIntersection, \a localIntersection, \a distance, \a button, \a buttons and \a modifiers
 */
QPickEvent::QPickEvent(const QPointF &position, const QVector3D &worldIntersection, const QVector3D &localIntersection,
                       float distance, QPickEvent::Buttons button, int buttons, int modifiers)
    : QObject(*new QPickEventPrivate())
{
    Q_D(QPickEvent);
    d->m_position = position;
    d->m_distance = distance;
    d->m_worldIntersection = worldIntersection;
    d->m_localIntersection = localIntersection;
    d->m_button = button;
    d->m_buttons = buttons;
    d->m_modifiers = modifiers;
}

/*! \internal */
QPickEvent::QPickEvent(QObjectPrivate &dd, QObject *parent)
    : QObject(dd, parent)
{

}

/*! \internal */
QPickEvent::~QPickEvent()
{
}

/*!
    \qmlproperty bool Qt3D.Render::PickEvent::accepted
    Specifies if event has been accepted
*/
/*!
  \property Qt3DRender::QPickEvent::accepted
    Specifies if event has been accepted
 */
/*!
 * \brief QPickEvent::isAccepted
 * \return true if the event has been accepted
 */
bool QPickEvent::isAccepted() const
{
    Q_D(const QPickEvent);
    return d->m_accepted;
}
/*!
 * \brief QPickEvent::setAccepted set if the event has been accepted to \a accepted
 */
void QPickEvent::setAccepted(bool accepted)
{
    Q_D(QPickEvent);
    if (accepted != d->m_accepted) {
        d->m_accepted = accepted;
        emit acceptedChanged(accepted);
    }
}

/*!
    \qmlproperty Point2D Qt3D.Render::PickEvent::position
    Specifies the mouse position with respect to the render area (window or quick item)
*/
/*!
  \property Qt3DRender::QPickEvent::position
    Specifies the mouse position with respect to the render area (window or quick item)
 */
/*!
 * \brief QPickEvent::position
 * \return mouse pointer coordinate of the pick query
 */
QPointF QPickEvent::position() const
{
    Q_D(const QPickEvent);
    return d->m_position;
}

/*!
    \qmlproperty real Qt3D.Render::PickEvent::distance
    Specifies the distance of the hit to the camera
*/
/*!
  \property Qt3DRender::QPickEvent::distance
    Specifies the distance of the hit to the camera
 */
/*!
 * \brief QPickEvent::distance
 * \return distance from camera to pick point
 */
float QPickEvent::distance() const
{
    Q_D(const QPickEvent);
    return d->m_distance;
}

/*!
    \qmlproperty Vector3D Qt3D.Render::PickEvent::worldIntersection
    Specifies the coordinates of the hit in world coordinate system
*/
/*!
  \property Qt3DRender::QPickEvent::worldIntersection
    Specifies the coordinates of the hit in world coordinate system
 */
/*!
 * \brief QPickEvent::worldIntersection
 * \return  coordinates of the hit in world coordinate system
 */
QVector3D QPickEvent::worldIntersection() const
{
    Q_D(const QPickEvent);
    return d->m_worldIntersection;
}

/*!
    \qmlproperty Vector3D Qt3D.Render::PickEvent::localIntersection
    Specifies the coordinates of the hit in the local coordinate system of the picked entity
*/
/*!
  \property Qt3DRender::QPickEvent::localIntersection
    Specifies the coordinates of the hit in the local coordinate system of the picked entity
 */
/*!
 * \brief QPickEvent::localIntersection
 * \return coordinates of the hit in the local coordinate system of the picked entity
 */
QVector3D QPickEvent::localIntersection() const
{
    Q_D(const QPickEvent);
    return d->m_localIntersection;
}

/*!
 * \enum Qt3DRender::QPickEvent::Buttons
 *
 * \value LeftButton
 * \value RightButton
 * \value MiddleButton
 * \value BackButton
 * \value NoButton
 */

/*!
    \qmlproperty int Qt3D.Render::PickEvent::button
    Specifies mouse button that caused the event
*/
/*!
  \property Qt3DRender::QPickEvent::button
    Specifies mouse button that caused the event
 */
/*!
 * \brief QPickEvent::button
 * \return mouse button that caused the event
 */
QPickEvent::Buttons QPickEvent::button() const
{
    Q_D(const QPickEvent);
    return d->m_button;
}

/*!
    \qmlproperty int Qt3D.Render::PickEvent::buttons
    Specifies state of the mouse buttons for the event
*/
/*!
  \property Qt3DRender::QPickEvent::buttons
    Specifies state of the mouse buttons for the event
 */
/*!
 * \brief QPickEvent::buttons
 * \return bitfield to be used to check for mouse buttons that may be accompanying the pick event.
 */
int QPickEvent::buttons() const
{
    Q_D(const QPickEvent);
    return d->m_buttons;
}

/*!
 * \enum Qt3DRender::QPickEvent::Modifiers
 *
 * \value NoModifier
 * \value ShiftModifier
 * \value ControlModifier
 * \value AltModifier
 * \value MetaModifier
 * \value KeypadModifier
 */

/*!
    \qmlproperty int Qt3D.Render::PickEvent::modifiers
    Specifies state of the mouse buttons for the event
*/
/*!
  \property Qt3DRender::QPickEvent::modifiers
    Specifies state of the mouse buttons for the event
 */
/*!
 * \brief QPickEvent::modifiers
 * \return bitfield to be used to check for keyboard modifiers that may be accompanying the pick event.
 */
int QPickEvent::modifiers() const
{
    Q_D(const QPickEvent);
    return d->m_modifiers;
}

/*!
 * \qmlproperty Viewport Qt3D.Render::PickEvent::viewport
 * The viewport in which this event originated. A null value means the event originated from a frame graph branch without a Viewport.
 * If a frame graph branch has a Viewport inside a Viewport the property will contain the leaf viewport.
 *
 * \since 5.14
 */
/*!
 * \property Qt3DRender::QPickEvent::viewport
 * The viewport in which this event originated. A null value means the event originated from a frame graph branch without a QViewport.
 * If a frame graph branch has a Viewport inside a Viewport the property will contain the leaf viewport.
 *
 * \since 5.14
 */
QViewport *QPickEvent::viewport() const
{
    Q_D(const QPickEvent);
    return d->m_viewport;
}


/*!
 * \qmlproperty Entity Qt3D.Render::PickEvent::entity
 * The entity that the picked geometry belongs to.
 *
 * If the object picker is not attached to a leaf node in the scene graph,
 * this is useful to find which child entity was actually picked.
 *
 * \since 5.14
 */
/*!
 * \property Qt3DRender::QPickEvent::entity
 * The entity that the picked geometry belongs to.
 *
 * If the object picker is not attached to a leaf node in the scene graph,
 * this is useful to find which child entity was actually picked.
 *
 * \since 5.14
 */
Qt3DCore::QEntity *QPickEvent::entity() const
{
    Q_D(const QPickEvent);
    return d->m_entityPtr;
}

} // Qt3DRender

QT_END_NAMESPACE

#include "moc_qpickevent.cpp"

