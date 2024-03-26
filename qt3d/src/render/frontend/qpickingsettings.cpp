// Copyright (C) 2016 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpickingsettings.h"
#include "qpickingsettings_p.h"

QT_BEGIN_NAMESPACE

namespace Qt3DRender {

/*!
    \class Qt3DRender::QPickingSettings
    \brief The QPickingSettings class specifies how entity picking is handled.
    \since 5.7
    \inmodule Qt3DRender
    \inherits Qt3DCore::QNode

    The picking settings determine how the entity picking is handled. For more details about
    entity picking, see QObjectPicker and QRayCaster component documentation.

    When using QObjectPicker components, picking is triggered by mouse events.

    When using QRayCaster or QScreenRayCaster components, picking can be explicitly triggered by
    the application.

    In both cases, a ray will be cast through the scene to find geometry intersecting the ray.

   \sa QObjectPicker, QPickEvent, QPickTriangleEvent, QRayCaster, QScreenRayCaster
 */

/*!
    \qmltype PickingSettings
    \brief The PickingSettings class specifies how entity picking is handled.
    \since 5.7
    \inqmlmodule Qt3D.Render
    \instantiates Qt3DRender::QPickingSettings

    The picking settings determine how the entity picking is handled. For more details about
    entity picking, see Qt3D.Render::ObjectPicker or Qt3D.Render::RayCaster component documentation.

    When using ObjectPicker components, picking is triggered by mouse events.

    When using RayCaster or ScreenRayCaster components, picking can be explicitly triggered by
    the application.

    In both cases, a ray will be cast through the scene to find geometry intersecting the ray.

    \sa ObjectPicker, RayCaster, ScreenRayCaster
 */

QPickingSettingsPrivate::QPickingSettingsPrivate()
    : Qt3DCore::QNodePrivate()
    , m_pickMethod(QPickingSettings::BoundingVolumePicking)
    , m_pickResultMode(QPickingSettings::NearestPick)
    , m_faceOrientationPickingMode(QPickingSettings::FrontFace)
    , m_worldSpaceTolerance(.1f)
{
}

QPickingSettings::QPickingSettings(Qt3DCore::QNode *parent)
    : Qt3DCore::QNode(*new QPickingSettingsPrivate, parent)
{
    // Block all notifications for this class as it should have been a QObject
    blockNotifications(true);
}

/*! \internal */
QPickingSettings::~QPickingSettings()
{
}

/*! \internal */
QPickingSettings::QPickingSettings(QPickingSettingsPrivate &dd, Qt3DCore::QNode *parent)
    : Qt3DCore::QNode(dd, parent)
{
}

QPickingSettings::PickMethod QPickingSettings::pickMethod() const
{
    Q_D(const QPickingSettings);
    return d->m_pickMethod;
}

QPickingSettings::PickResultMode QPickingSettings::pickResultMode() const
{
    Q_D(const QPickingSettings);
    return d->m_pickResultMode;
}

QPickingSettings::FaceOrientationPickingMode QPickingSettings::faceOrientationPickingMode() const
{
    Q_D(const QPickingSettings);
    return d->m_faceOrientationPickingMode;
}

/*!
 * \return the line and point precision worldSpaceTolerance
 */
float QPickingSettings::worldSpaceTolerance() const
{
    Q_D(const QPickingSettings);
    return d->m_worldSpaceTolerance;
}

/*!
 * \enum Qt3DRender::QPickingSettings::PickMethod
 *
 * Specifies the picking method.
 *
 * \value BoundingVolumePicking An entity is considered picked if the picking ray intersects
 * the bounding volume of the entity (default).
 * \value TrianglePicking An entity is considered picked if the picking ray intersects with
 * any triangle of the entity's mesh component.
 * \value LinePicking An entity is considered picked if the picking ray intersects with
 * any edge of the entity's mesh component.
 * \value PointPicking An entity is considered picked if the picking ray intersects with
 * any point of the entity's mesh component.
 * \value PrimitivePicking An entity is considered picked if the picking ray intersects with
 * any point, edge or triangle of the entity's mesh component.
 */

/*!
    \qmlproperty enumeration PickingSettings::pickMethod

    Holds the current pick method.

    \list
        \li PickingSettings.BoundingVolumePicking
        \li PickingSettings.TrianglePicking
        \li PickingSettings.LinePicking
        \li PickingSettings.PointPicking
        \li PickingSettings.PrimitivePicking: picks either points, lines or triangles
    \endlist

    \sa Qt3DRender::QPickingSettings::PickMethod
*/
/*!
    \property QPickingSettings::pickMethod

    Holds the current pick method.

    By default, for performance reasons, ray casting will use bounding volume picking.
    This may however lead to unexpected results if a small object is englobed
    in the bounding sphere of a large object behind it.

    Triangle picking will produce exact results but is computationally more expensive.
*/
void QPickingSettings::setPickMethod(QPickingSettings::PickMethod pickMethod)
{
    Q_D(QPickingSettings);
    if (d->m_pickMethod == pickMethod)
        return;

    d->m_pickMethod = pickMethod;
    emit pickMethodChanged(pickMethod);
}

/*!
 * \enum Qt3DRender::QPickingSettings::PickResultMode
 *
 * Specifies what is included into the picking results.
 *
 * \value NearestPick Only the nearest entity to picking ray origin intersected by the picking ray
 * is picked (default).
 * \value AllPicks All entities that intersect the picking ray are picked.
 * \value NearestPriorityPick Selects the entity whose object picker has the highest
 * value. If several object pickers have the same priority, the closest one on
 * the ray is selected.
 *
 * \sa Qt3DRender::QPickEvent
 */

/*!
    \qmlproperty enumeration PickingSettings::pickResultMode

    Holds the current pick results mode.

    \list
        \li PickingSettings.NearestPick
        \li PickingSettings.AllPicks
        \li PickingSettings.NearestPriorityPick
    \endlist

    \sa Qt3DRender::QPickingSettings::PickResultMode
*/
/*!
    \property QPickingSettings::pickResultMode

    Holds the current pick results mode.

    By default, pick results will only be produced for the entity closest to the camera.

    When setting the pick method to AllPicks, events will be triggered for all the
    entities with a QObjectPicker along the ray.

    When setting the pick method to NearestPriorityPick, events will be
    triggered for the nearest highest priority picker. This can be used when a
    given element should always be selected even if others are in front of it.

    If a QObjectPicker is assigned to an entity with multiple children, an event will
    be triggered for each child entity that intersects the ray.
*/
void QPickingSettings::setPickResultMode(QPickingSettings::PickResultMode pickResultMode)
{
    Q_D(QPickingSettings);
    if (d->m_pickResultMode == pickResultMode)
        return;

    d->m_pickResultMode = pickResultMode;
    emit pickResultModeChanged(pickResultMode);
}

/*!
    \enum Qt3DRender::QPickingSettings::FaceOrientationPickingMode

    Specifies how face orientation affects triangle picking

    \value FrontFace Only front-facing triangles will be picked (default).
    \value BackFace Only back-facing triangles will be picked.
    \value FrontAndBackFace Both front- and back-facing triangles will be picked.
*/

/*!
    \qmlproperty enumeration PickingSettings::faceOrientationPickingMode

    Specifies how face orientation affects triangle picking

    \list
        \li PickingSettings.FrontFace Only front-facing triangles will be picked (default).
        \li PickingSettings.BackFace Only back-facing triangles will be picked.
        \li PickingSettings.FrontAndBackFace Both front- and back-facing triangles will be picked.
    \endlist
*/
/*!
    \property QPickingSettings::faceOrientationPickingMode

    Specifies how face orientation affects triangle picking
*/
void QPickingSettings::setFaceOrientationPickingMode(QPickingSettings::FaceOrientationPickingMode faceOrientationPickingMode)
{
    Q_D(QPickingSettings);
    if (d->m_faceOrientationPickingMode == faceOrientationPickingMode)
        return;

    d->m_faceOrientationPickingMode = faceOrientationPickingMode;
    emit faceOrientationPickingModeChanged(faceOrientationPickingMode);
}

/*!
    \qmlproperty qreal PickingSettings::worldSpaceTolerance

    Holds the threshold, in model space coordinates, used to evaluate line and point picking.
*/
/*!
    \property QPickingSettings::worldSpaceTolerance

    Holds the threshold, in model space coordinates, used to evaluate line and point picking.
*/
/*!
 * Sets the threshold used for line and point picking to \a worldSpaceTolerance.
 */
void QPickingSettings::setWorldSpaceTolerance(float worldSpaceTolerance)
{
    Q_D(QPickingSettings);
    if (qFuzzyCompare(worldSpaceTolerance, d->m_worldSpaceTolerance))
        return;

    d->m_worldSpaceTolerance = worldSpaceTolerance;
    emit worldSpaceToleranceChanged(worldSpaceTolerance);
}

} // namespace Qt3Drender

QT_END_NAMESPACE

#include "moc_qpickingsettings.cpp"
