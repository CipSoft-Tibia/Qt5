// Copyright (C) 2018 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qraycaster.h"
#include "qabstractraycaster_p.h"
#include <Qt3DCore/qentity.h>
#include <Qt3DCore/private/qcomponent_p.h>
#include <Qt3DCore/private/qscene_p.h>

QT_BEGIN_NAMESPACE

namespace Qt3DRender {

/*!
    \class Qt3DRender::QRayCaster
    \inmodule Qt3DRender

    \brief Qt3DRender::QRayCaster is used to perform ray casting tests in 3d world coordinates.
    \inmodule Qt3DRender
    \since 5.11
    \inherits QAbstractRayCaster

    The 3d ray is defined by its origin, direction and length. It will be affected by the
    transformations applied to the entity it belongs to.

    Ray casting tests will be performed every frame as long as the component is enabled.
    The hits property will be updated with the list of intersections.

    \sa QAbstractRayCaster, QScreenRayCaster, QNoPicking
*/
/*!
    \qmltype RayCaster
    \brief used to perform ray casting tests in 3d world coordinates.
    \inqmlmodule Qt3D.Render
    \since 5.11
    \instantiates Qt3DRender::QRayCaster

    The 3d ray is defined by its origin, direction and length. It will be affected by the
    transformations applied to the entity it belongs to.

    Ray casting tests will be performed every frame as long as the component is enabled.
    The hits property will be updated with the list of intersections.

    \sa AbstractRayCaster, ScreenRayCaster, NoPicking
*/

/*!
    \property Qt3DRender::QRayCaster::origin

    Holds the origin of the 3D ray in local coordinates.
*/
/*!
    \qmlproperty vector3d Qt3D.Render::RayCaster::origin

    Holds the origin of the 3D ray in local coordinates.
*/

/*!
    \property Qt3DRender::QRayCaster::direction

    Holds the direction of the 3D ray. This should be a unit vector.
*/

/*!
    \qmlproperty vector3D Qt3D.Render::RayCaster::direction

    Holds the direction of the 3D ray. This should be a unit vector.
*/

/*!
    \property Qt3DRender::QRayCaster::length

    Holds the length of the 3D ray.
*/

/*!
    \qmlproperty real Qt3D.Render::RayCaster::length

    Holds the length of the 3d ray.
*/


QRayCaster::QRayCaster(Qt3DCore::QNode *parent)
    : QAbstractRayCaster(parent)
{
    QAbstractRayCasterPrivate::get(this)->m_rayCasterType = QAbstractRayCasterPrivate::WorldSpaceRayCaster;
}

QRayCaster::QRayCaster(QAbstractRayCasterPrivate &dd, Qt3DCore::QNode *parent)
    : QAbstractRayCaster(dd, parent)
{
    QAbstractRayCasterPrivate::get(this)->m_rayCasterType = QAbstractRayCasterPrivate::WorldSpaceRayCaster;
}

/*! \internal */
QRayCaster::~QRayCaster()
{
}

QVector3D QRayCaster::origin() const
{
    auto d = QAbstractRayCasterPrivate::get(this);
    return d->m_origin;
}

void QRayCaster::setOrigin(const QVector3D &origin)
{
    auto d = QAbstractRayCasterPrivate::get(this);
    if (d->m_origin != origin) {
        d->m_origin = origin;
        emit originChanged(d->m_origin);
    }
}

QVector3D QRayCaster::direction() const
{
    auto d = QAbstractRayCasterPrivate::get(this);
    return d->m_direction;
}

void QRayCaster::setDirection(const QVector3D &direction)
{
    auto d = QAbstractRayCasterPrivate::get(this);
    if (d->m_direction != direction) {
        d->m_direction = direction;
        emit directionChanged(d->m_direction);
    }
}

float QRayCaster::length() const
{
    auto d = QAbstractRayCasterPrivate::get(this);
    return d->m_length;
}

/*!
 * \brief Sets the length of the ray to \a length.
 *
 * If the value is less than or equal to zero, the ray is concidered to be infinite.
 */
void QRayCaster::setLength(float length)
{
    auto d = QAbstractRayCasterPrivate::get(this);
    if (!qFuzzyCompare(d->m_length, length)) {
        d->m_length = length;
        emit lengthChanged(d->m_length);
    }
}

/*!
 * Convenience method to enable the component and trigger tests using the current ray.
 */
void QRayCaster::trigger()
{
    setEnabled(true);
}

/*!
 * Convenience method to set the ray details \a origin, \a direction, and \a length,
 * and enable the component to trigger tests.
 */
void QRayCaster::trigger(const QVector3D &origin, const QVector3D &direction, float length)
{
    setOrigin(origin);
    setDirection(direction);
    setLength(length);
    setEnabled(true);
}

QAbstractRayCaster::Hits QRayCaster::pick(const QVector3D &origin, const QVector3D &direction, float length)
{
    setOrigin(origin);
    setDirection(direction);
    setLength(length);

    auto d = QAbstractRayCasterPrivate::get(this);
    return d->pick();
}

} // Qt3DRender

QT_END_NAMESPACE

#include "moc_qraycaster.cpp"
