// Copyright (C) 2015 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qobjectpicker.h"
#include "qobjectpicker_p.h"
#include <Qt3DCore/qentity.h>
#include <Qt3DCore/private/qcomponent_p.h>
#include <Qt3DCore/private/qscene_p.h>
#include <Qt3DRender/qpickevent.h>
#include <Qt3DRender/QViewport>
#include <Qt3DRender/private/qpickevent_p.h>

QT_BEGIN_NAMESPACE

namespace Qt3DRender {

/*!
    \class Qt3DRender::QObjectPicker
    \inmodule Qt3DRender

    \brief The QObjectPicker class instantiates a component that can
    be used to interact with a QEntity by a process known as picking.

    For every combination of viewport and camera, picking casts a ray through the scene to
    find entities who's bounding volume intersects the ray. The bounding volume is computed using
    the values in the attribute buffer specified by the boundingVolumePositionAttribute of the
    geometry.

    The signals pressed(), released(), clicked(), moved(), entered(), and exited() are
    emitted when the bounding volume defined by the pickAttribute property intersects
    with a ray.

    Most signals carry a QPickEvent instance. If QPickingSettings::pickMode() is set to
    QPickingSettings::TrianglePicking, the actual type of the pick parameter will be
    QPickTriangleEvent.

    Pick queries are performed on mouse press and mouse release.
    If drag is enabled, queries also happen on each mouse move while any button is pressed.
    If hover is enabled, queries happen on every mouse move even if no button is pressed.

    For generalised ray casting queries, see Qt3DRender::QRayCaster and Qt3DRender::QScreenRayCaster.

    \sa Qt3DRender::QPickingSettings, Qt3DCore::QGeometry, Qt3DCore::QAttribute,
        Qt3DRender::QPickEvent, Qt3DRender::QPickTriangleEvent, Qt3DRender::QNoPicking

    \note Instances of this component shouldn't be shared, not respecting that
    condition will most likely result in undefined behavior.

    \note The camera far plane value affects picking and produces incorrect results due to
    floating-point precision if it is greater than ~100 000.

    \since 5.6
*/

/*!
    \qmltype ObjectPicker
    \instantiates Qt3DRender::QObjectPicker
    \inqmlmodule Qt3D.Render
    \brief The ObjectPicker class instantiates a component that can
    be used to interact with an Entity by a process known as picking.

    For every combination of viewport and camera, picking casts a ray through the scene to
    find entities who's bounding volume intersects the ray. The bounding volume is computed using
    the values in the attribute buffer specified by the boundingVolumePositionAttribute of the
    geometry.

    The signals pressed(), released(), clicked(), moved(), entered(), and exited() are
    emitted when the bounding volume defined by the pickAttribute property intersects
    with a ray.

    Most signals carry a PickEvent instance. If PickingSettings.pickMode is set to
    PickingSettings.TrianglePicking, the actual type of the pick parameter will be
    PickTriangleEvent.

    Pick queries are performed on mouse press and mouse release.
    If drag is enabled, queries also happen on each mouse move while any button is pressed.
    If hover is enabled, queries happen on every mouse move even if no button is pressed.

    \sa PickingSettings, Geometry, Attribute, PickEvent, PickTriangleEvent, NoPicking

    \note To receive hover events in QtQuick, the hoverEnabled property of Scene3D must also be set.

    \note Instances of this component shouldn't be shared, not respecting that
    condition will most likely result in undefined behavior.

    \note The camera far plane value affects picking and produces incorrect results due to
    floating-point precision if it is greater than ~100 000.
 */

/*!
    \qmlsignal Qt3D.Render::ObjectPicker::pressed(PickEvent pick)

    This signal is emitted when the bounding volume defined by the
    pickAttribute property intersects with a ray on a mouse press. Intersection
    information are accessible through the \a pick parameter.
*/

/*!
    \qmlsignal Qt3D.Render::ObjectPicker::released(PickEvent pick)

    This signal is emitted when the bounding volume defined by the
    pickAttribute property intersects with a ray on a mouse release.
    Intersection information are accessible through the \a pick parameter.
*/

/*!
    \qmlsignal Qt3D.Render::ObjectPicker::clicked(PickEvent pick)

    This signal is emitted when the bounding volume defined by the
    pickAttribute property intersects with a ray on a mouse click. Intersection
    information are accessible through the \a pick parameter.
*/

/*!
    \qmlsignal Qt3D.Render::ObjectPicker::moved(PickEvent pick)

    This signal is emitted when the bounding volume defined by the
    pickAttribute property intersects with a ray on a mouse move with a button
    pressed. Intersection information are accessible through the \a pick
    parameter.
*/

/*!
    \qmlsignal Qt3D.Render::ObjectPicker::entered()

    This signal is emitted when the bounding volume defined by the pickAttribute
    property intersects with a ray on the mouse entering the volume.
*/

/*!
    \qmlsignal Qt3D.Render::ObjectPicker::exited()

    This signal is emitted when the bounding volume defined by the pickAttribute
    property intersects with a ray on the ray exiting the volume.
*/

/*!
    \fn Qt3DRender::QObjectPicker::clicked(Qt3DRender::QPickEvent *pick)

    This signal is emitted when the bounding volume defined by the pickAttribute
    property intersects with a ray on a mouse click the QPickEvent \a pick contains
    details of the event.
*/

/*!
    \fn Qt3DRender::QObjectPicker::entered()

    This signal is emitted when the bounding volume defined by the pickAttribute
    property intersects with a ray on the mouse entering the volume.
*/

/*!
    \fn Qt3DRender::QObjectPicker::exited()

    This signal is emitted when the bounding volume defined by the pickAttribute
    property intersects with a ray on the ray exiting the volume.
*/

/*!
    \fn Qt3DRender::QObjectPicker::moved(Qt3DRender::QPickEvent *pick)

    This signal is emitted when the bounding volume defined by the
    pickAttribute property intersects with a ray on a mouse move with a button
    pressed the QPickEvent \a pick contains details of the event.
*/

/*!
    \fn Qt3DRender::QObjectPicker::pressed(Qt3DRender::QPickEvent *pick)

    This signal is emitted when the bounding volume defined by the
    pickAttribute property intersects with a ray on a mouse press the
    QPickEvent \a pick contains details of the event.
*/

/*!
    \fn Qt3DRender::QObjectPicker::released(Qt3DRender::QPickEvent *pick)

    This signal is emitted when the bounding volume defined by the
    pickAttribute property intersects with a ray on a mouse release the
    QPickEvent \a pick contains details of the event.
*/

QObjectPicker::QObjectPicker(Qt3DCore::QNode *parent)
    : Qt3DCore::QComponent(*new QObjectPickerPrivate(), parent)
{
}

/*! \internal */
QObjectPicker::~QObjectPicker()
{
}

/*!
 * Sets the hoverEnabled Property to \a hoverEnabled
 */
void QObjectPicker::setHoverEnabled(bool hoverEnabled)
{
    Q_D(QObjectPicker);
    if (hoverEnabled != d->m_hoverEnabled) {
        d->m_hoverEnabled = hoverEnabled;
        emit hoverEnabledChanged(hoverEnabled);
    }
}

/*!
    \qmlproperty bool Qt3D.Render::ObjectPicker::hoverEnabled
    Specifies if hover is enabled
*/
/*!
  \property Qt3DRender::QObjectPicker::hoverEnabled
    Specifies if hover is enabled
 */
/*!
 * \return true if hover enabled
 */
bool QObjectPicker::isHoverEnabled() const
{
    Q_D(const QObjectPicker);
    return d->m_hoverEnabled;
}

/*!
 * Sets the dragEnabled Property to \a dragEnabled
 */
void QObjectPicker::setDragEnabled(bool dragEnabled)
{
    Q_D(QObjectPicker);
    if (dragEnabled != d->m_dragEnabled) {
        d->m_dragEnabled = dragEnabled;
        emit dragEnabledChanged(dragEnabled);
    }
}

/*!
 * Sets the picker's priority to \a priority. This is used when the pick result
 * mode on QPickingSettings is set to QPickingSettings::NearestPriorityPick.
 * Picking results are sorted by highest priority and shortest picking
 * distance.
 *
 * \since 5.13
 */
void QObjectPicker::setPriority(int priority)
{
    Q_D(QObjectPicker);
    if (priority != d->m_priority) {
        d->m_priority = priority;
        emit priorityChanged(priority);
    }
}

/*!
    \qmlproperty bool Qt3D.Render::ObjectPicker::dragEnabled
*/
/*!
  \property Qt3DRender::QObjectPicker::dragEnabled
    Specifies if drag is enabled
 */
/*!
 * \return true if dragging is enabled
 */
bool QObjectPicker::isDragEnabled() const
{
    Q_D(const QObjectPicker);
    return d->m_dragEnabled;
}

/*!
    \qmlproperty bool Qt3D.Render::ObjectPicker::containsMouse
    Specifies if the object picker currently contains the mouse
*/
/*!
  \property Qt3DRender::QObjectPicker::containsMouse
    Specifies if the object picker currently contains the mouse
 */
/*!
 * \return true if the object picker currently contains the mouse
 */
bool QObjectPicker::containsMouse() const
{
    Q_D(const QObjectPicker);
    return d->m_containsMouse;
}

/*!
    \qmlproperty bool Qt3D.Render::ObjectPicker::pressed
    Specifies if the object picker is currently pressed
*/
/*!
  \property Qt3DRender::QObjectPicker::pressed
    Specifies if the object picker is currently pressed
 */
bool QObjectPicker::isPressed() const
{
    Q_D(const QObjectPicker);
    return d->m_pressed;
}

/*!
    \qmlproperty int Qt3D.Render::ObjectPicker::priority

    The priority to be used when filtering pick results by priority when
    PickingSettings.pickResultMode is set to PickingSettings.PriorityPick.
*/
/*!
  \property Qt3DRender::QObjectPicker::priority

    The priority to be used when filtering pick results by priority when
    QPickingSettings::pickResultMode is set to
    QPickingSettings::NearestPriorityPick.
*/
int QObjectPicker::priority() const
{
    Q_D(const QObjectPicker);
    return d->m_priority;
}

/*!
    \internal
 */
void QObjectPickerPrivate::setPressed(bool pressed)
{
    Q_Q(QObjectPicker);
    if (m_pressed != pressed) {
        const bool blocked = q->blockNotifications(true);
        m_pressed = pressed;
        emit q->pressedChanged(pressed);
        q->blockNotifications(blocked);
    }
}

/*!
    \internal
*/
void QObjectPickerPrivate::setContainsMouse(bool containsMouse)
{
    Q_Q(QObjectPicker);
    if (m_containsMouse != containsMouse) {
        const bool blocked = q->blockNotifications(true);
        m_containsMouse = containsMouse;
        emit q->containsMouseChanged(containsMouse);
        q->blockNotifications(blocked);
    }
}

void QObjectPickerPrivate::propagateEvent(QPickEvent *event, EventType type)
{
    if (!m_entities.isEmpty()) {
        Qt3DCore::QEntity *entity = m_entities.first();
        Qt3DCore::QEntity *parentEntity = nullptr;
        while (entity != nullptr && entity->parentEntity() != nullptr && !event->isAccepted()) {
            parentEntity = entity->parentEntity();
            const auto components = parentEntity->components();
            for (Qt3DCore::QComponent *c : components) {
                if (auto objectPicker = qobject_cast<Qt3DRender::QObjectPicker *>(c)) {
                    QObjectPickerPrivate *objectPickerPrivate = static_cast<QObjectPickerPrivate *>(QObjectPickerPrivate::get(objectPicker));
                    switch (type) {
                    case Pressed:
                        objectPickerPrivate->pressedEvent(event);
                        break;
                    case Released:
                        objectPickerPrivate->releasedEvent(event);
                        break;
                    case Clicked:
                        objectPickerPrivate->clickedEvent(event);
                        break;
                    case EventType::Moved:
                        objectPickerPrivate->movedEvent(event);
                        break;
                    }
                    break;
                }
            }
            entity = parentEntity;
        }
    }
}

/*!
    \internal
 */
void QObjectPickerPrivate::pressedEvent(QPickEvent *event)
{
    Q_Q(QObjectPicker);
    emit q->pressed(event);

    m_acceptedLastPressedEvent = event->isAccepted();
    if (!m_acceptedLastPressedEvent) {
        // Travel parents to transmit the event
        propagateEvent(event, Pressed);
    } else {
        setPressed(true);
    }
}

/*!
    \internal
 */
void QObjectPickerPrivate::clickedEvent(QPickEvent *event)
{
    Q_Q(QObjectPicker);
    emit q->clicked(event);
    if (!event->isAccepted())
        propagateEvent(event, Clicked);
}

/*!
    \internal
 */
void QObjectPickerPrivate::movedEvent(QPickEvent *event)
{
    Q_Q(QObjectPicker);
    emit q->moved(event);
    if (!event->isAccepted())
        propagateEvent(event, EventType::Moved);
}

/*!
    \internal
 */
void QObjectPickerPrivate::releasedEvent(QPickEvent *event)
{
    Q_Q(QObjectPicker);
    if (m_acceptedLastPressedEvent) {
        emit q->released(event);
        setPressed(false);
    } else {
        event->setAccepted(false);
        propagateEvent(event, Released);
    }
}

} // Qt3DRender

QT_END_NAMESPACE

#include "moc_qobjectpicker.cpp"
