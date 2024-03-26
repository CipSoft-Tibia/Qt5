// Copyright (C) 2014 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qtransform.h"
#include "qtransform_p.h"

#include <Qt3DCore/private/qmath3d_p.h>

QT_BEGIN_NAMESPACE

namespace Qt3DCore {

QTransformPrivate::QTransformPrivate()
    : QComponentPrivate()
    , m_rotation()
    , m_scale(1.0f, 1.0f, 1.0f)
    , m_translation()
    , m_eulerRotationAngles()
    , m_matrixDirty(false)
{
    m_shareable = false;
}

QTransformPrivate::~QTransformPrivate()
{
}

/*!
    \qmltype Transform
    \inqmlmodule Qt3D.Core
    \inherits Component3D
    \instantiates Qt3DCore::QTransform
    \since 5.6
    \brief Used to perform transforms on meshes.

    The Transform component is not shareable between multiple Entity's.
    The transformation is held as vector3d scale, quaternion rotation and
    vector3d translation components. The transformations are applied to the
    mesh in that order. When Transform::matrix property is set, it is decomposed
    to these transform components and corresponding transform signals are emitted.

    Several helper functions are provided to set up the Transform;
    fromAxisAndAngle and fromAxesAndAngles can be used to set the rotation around
    specific axes, fromEulerAngles can be used to set the rotation based on euler
    angles and rotateAround can be used to rotate the object around specific point
    relative to local origin.
 */

/*!
    \qmlproperty matrix4x4 Transform::matrix

    Holds the matrix4x4 of the transform.
    \note When the matrix property is set, it is decomposed to translation, rotation and scale components.
 */

/*!
    \qmlproperty real Transform::rotationX

    Holds the x rotation of the transform as Euler angle.
 */

/*!
    \qmlproperty real Transform::rotationY

    Holds the y rotation of the transform as Euler angle.
 */

/*!
    \qmlproperty real Transform::rotationZ

    Holds the z rotation of the transform as Euler angle.
 */

/*!
    \qmlproperty vector3d Transform::scale3D

    Holds the scale of the transform as vector3d.
 */

/*!
    \qmlproperty real Transform::scale

    Holds the uniform scale of the transform. If the scale has been set with scale3D, holds
    the x value only.
 */

/*!
    \qmlproperty quaternion Transform::rotation

    Holds the rotation of the transform as quaternion.
 */

/*!
    \qmlproperty vector3d Transform::translation

    Holds the translation of the transform as vector3d.
 */

/*!
    \qmlproperty matrix4x4 QTransform::worldMatrix

    Holds the world transformation matrix for the transform. This assumes the
    Transform component is being referenced by an Entity. This makes it more
    convenient to identify when an Entity part of a subtree has been
    transformed in the world even though its local transformation might not
    have changed.

    \since 5.14
 */

/*!
    \qmlmethod quaternion Transform::fromAxisAndAngle(vector3d axis, real angle)
    Creates a quaternion from \a axis and \a angle.
    Returns the resulting quaternion.
 */

/*!
    \qmlmethod quaternion Transform::fromAxisAndAngle(real x, real y, real z, real angle)
    Creates a quaternion from \a x, \a y, \a z, and \a angle.
    Returns the resulting quaternion.
 */

/*!
    \qmlmethod quaternion Transform::fromAxesAndAngles(vector3d axis1, real angle1,
                                                       vector3d axis2, real angle2)
    Creates a quaternion from \a axis1, \a angle1, \a axis2, and \a angle2.
    Returns the resulting quaternion.
 */

/*!
    \qmlmethod quaternion Transform::fromAxesAndAngles(vector3d axis1, real angle1,
                                                       vector3d axis2, real angle2,
                                                       vector3d axis3, real angle3)
    Creates a quaternion from \a axis1, \a angle1, \a axis2, \a angle2, \a axis3, and \a angle3.
    Returns the resulting quaternion.
 */

/*!
    \qmlmethod quaternion Transform::fromEulerAngles(vector3d eulerAngles)
    Creates a quaternion from \a eulerAngles.
    Returns the resulting quaternion.
 */

/*!
    \qmlmethod quaternion Transform::fromEulerAngles(real pitch, real yaw, real roll)
    Creates a quaternion from \a pitch, \a yaw, and \a roll.
    Returns the resulting quaternion.
 */

/*!
    \qmlmethod matrix4x4 Transform::rotateAround(vector3d point, real angle, vector3d axis)
    Creates a rotation matrix from \a axis and \a angle around \a point relative to local origin.
    Returns the resulting matrix4x4.
 */

/*!
    \class Qt3DCore::QTransform
    \inmodule Qt3DCore
    \inherits Qt3DCore::QComponent
    \since 5.6
    \brief Used to perform transforms on meshes.

    The QTransform component is not shareable between multiple QEntity's.
    The transformation is held as QVector3D scale, QQuaternion rotation and
    QVector3D translation components. The transformations are applied to the
    mesh in that order. When QTransform::matrix property is set, it is decomposed
    to these transform components and corresponding signals are emitted.

    Several helper functions are provided to set up the QTransform;
    fromAxisAndAngle and fromAxesAndAngles can be used to set the rotation around
    specific axes, fromEulerAngles can be used to set the rotation based on euler
    angles and rotateAround can be used to rotate the object around specific point
    relative to local origin.
 */

/*!
    Constructs a new QTransform with \a parent.
 */
QTransform::QTransform(QNode *parent)
    : QComponent(*new QTransformPrivate, parent)
{
}

/*!
    \internal
 */
QTransform::~QTransform()
{
}

/*!
    \internal
 */
QTransform::QTransform(QTransformPrivate &dd, QNode *parent)
    : QComponent(dd, parent)
{
}

void QTransformPrivate::setWorldMatrix(const QMatrix4x4 &worldMatrix)
{
    Q_Q(QTransform);
    if (m_worldMatrix == worldMatrix)
        return;
    const bool blocked = q->blockNotifications(true);
    m_worldMatrix = worldMatrix;
    emit q->worldMatrixChanged(worldMatrix);
    q->blockNotifications(blocked);
}

void QTransformPrivate::update()
{
    if (!m_blockNotifications)
        m_dirty = true;
    markDirty(QScene::TransformDirty);
    QNodePrivate::update();
}

void QTransform::setMatrix(const QMatrix4x4 &m)
{
    Q_D(QTransform);
    if (m != matrix()) {
        d->m_matrix = m;
        d->m_matrixDirty = false;

        QVector3D s;
        QVector3D t;
        QQuaternion r;
        decomposeQMatrix4x4(m, t, r, s);
        d->m_scale = s;
        d->m_rotation = r;
        d->m_translation = t;
        d->m_eulerRotationAngles = d->m_rotation.toEulerAngles();
        emit scale3DChanged(s);
        emit rotationChanged(r);
        emit translationChanged(t);
        const bool wasBlocked = blockNotifications(true);
        emit matrixChanged();
        emit scaleChanged(d->m_scale.x());
        emit rotationXChanged(d->m_eulerRotationAngles.x());
        emit rotationYChanged(d->m_eulerRotationAngles.y());
        emit rotationZChanged(d->m_eulerRotationAngles.z());
        blockNotifications(wasBlocked);
    }
}

void QTransform::setRotationX(float rotationX)
{
    Q_D(QTransform);

    if (d->m_eulerRotationAngles.x() == rotationX)
        return;

    d->m_eulerRotationAngles.setX(rotationX);
    QQuaternion rotation = QQuaternion::fromEulerAngles(d->m_eulerRotationAngles);
    if (rotation != d->m_rotation) {
        d->m_rotation = rotation;
        d->m_matrixDirty = true;
        emit rotationChanged(rotation);
    }

    const bool wasBlocked = blockNotifications(true);
    emit rotationXChanged(rotationX);
    emit matrixChanged();
    blockNotifications(wasBlocked);
}

void QTransform::setRotationY(float rotationY)
{
    Q_D(QTransform);

    if (d->m_eulerRotationAngles.y() == rotationY)
        return;

    d->m_eulerRotationAngles.setY(rotationY);
    QQuaternion rotation = QQuaternion::fromEulerAngles(d->m_eulerRotationAngles);
    if (rotation != d->m_rotation) {
        d->m_rotation = rotation;
        d->m_matrixDirty = true;
        emit rotationChanged(rotation);
    }

    const bool wasBlocked = blockNotifications(true);
    emit rotationYChanged(rotationY);
    emit matrixChanged();
    blockNotifications(wasBlocked);
}

void QTransform::setRotationZ(float rotationZ)
{
    Q_D(QTransform);
    if (d->m_eulerRotationAngles.z() == rotationZ)
        return;

    d->m_eulerRotationAngles.setZ(rotationZ);
    QQuaternion rotation = QQuaternion::fromEulerAngles(d->m_eulerRotationAngles);
    if (rotation != d->m_rotation) {
        d->m_rotation = rotation;
        d->m_matrixDirty = true;
        emit rotationChanged(rotation);
    }

    const bool wasBlocked = blockNotifications(true);
    emit rotationZChanged(rotationZ);
    emit matrixChanged();
    blockNotifications(wasBlocked);
}

/*!
    \property Qt3DCore::QTransform::matrix

    Holds the QMatrix4x4 of the transform.
    \note When the matrix property is set, it is decomposed to translation, rotation and scale components.
 */
QMatrix4x4 QTransform::matrix() const
{
    Q_D(const QTransform);
    if (d->m_matrixDirty) {
        composeQMatrix4x4(d->m_translation, d->m_rotation, d->m_scale, d->m_matrix);
        d->m_matrixDirty = false;
    }
    return d->m_matrix;
}

/*!
    \property QTransform::worldMatrix

    Holds the world transformation matrix for the transform. This assumes the
    QTransform component is being referenced by a QEntity. This makes it more
    convenient to identify when a QEntity part of a subtree has been
    transformed in the world even though its local transformation might not
    have changed.

    \since 5.14
 */

/*!
    Returns the world transformation matrix associated to the QTransform when
    referenced by a QEntity which may be part of a QEntity hierarchy.

    \since 5.14
 */
QMatrix4x4 QTransform::worldMatrix() const
{
    Q_D(const QTransform);
    return d->m_worldMatrix;
}

/*!
    \property Qt3DCore::QTransform::rotationX

    Holds the x rotation of the transform as Euler angle.
 */
float QTransform::rotationX() const
{
    Q_D(const QTransform);
    return d->m_eulerRotationAngles.x();
}

/*!
    \property Qt3DCore::QTransform::rotationY

    Holds the y rotation of the transform as Euler angle.
 */
float QTransform::rotationY() const
{
    Q_D(const QTransform);
    return d->m_eulerRotationAngles.y();
}

/*!
    \property Qt3DCore::QTransform::rotationZ

    Holds the z rotation of the transform as Euler angle.
 */
float QTransform::rotationZ() const
{
    Q_D(const QTransform);
    return d->m_eulerRotationAngles.z();
}

void QTransform::setScale3D(const QVector3D &scale)
{
    Q_D(QTransform);
    if (scale != d->m_scale) {
        d->m_scale = scale;
        d->m_matrixDirty = true;
        emit scale3DChanged(scale);

        const bool wasBlocked = blockNotifications(true);
        emit matrixChanged();
        blockNotifications(wasBlocked);
    }
}

/*!
    \property Qt3DCore::QTransform::scale3D

    Holds the scale of the transform as QVector3D.
 */
QVector3D QTransform::scale3D() const
{
    Q_D(const QTransform);
    return d->m_scale;
}

void QTransform::setScale(float scale)
{
    Q_D(QTransform);
    if (scale != d->m_scale.x()) {
        setScale3D(QVector3D(scale, scale, scale));

        const bool wasBlocked = blockNotifications(true);
        emit scaleChanged(scale);
        blockNotifications(wasBlocked);
    }
}

/*!
    \property Qt3DCore::QTransform::scale

    Holds the uniform scale of the transform. If the scale has been set with setScale3D, holds
    the x value only.
 */
float QTransform::scale() const
{
    Q_D(const QTransform);
    return d->m_scale.x();
}

void QTransform::setRotation(const QQuaternion &rotation)
{
    Q_D(QTransform);
    if (rotation != d->m_rotation) {
        d->m_rotation = rotation;
        const QVector3D oldRotation = d->m_eulerRotationAngles;
        d->m_eulerRotationAngles = d->m_rotation.toEulerAngles();
        d->m_matrixDirty = true;
        emit rotationChanged(rotation);

        const bool wasBlocked = blockNotifications(true);
        emit matrixChanged();
        if (d->m_eulerRotationAngles.x() != oldRotation.x())
            emit rotationXChanged(d->m_eulerRotationAngles.x());
        if (d->m_eulerRotationAngles.y() != oldRotation.y())
            emit rotationYChanged(d->m_eulerRotationAngles.y());
        if (d->m_eulerRotationAngles.z() != oldRotation.z())
            emit rotationZChanged(d->m_eulerRotationAngles.z());
        blockNotifications(wasBlocked);
    }
}

/*!
    \property Qt3DCore::QTransform::rotation

    Holds the rotation of the transform as QQuaternion.
 */
QQuaternion QTransform::rotation() const
{
    Q_D(const QTransform);
    return d->m_rotation;
}

void QTransform::setTranslation(const QVector3D &translation)
{
    Q_D(QTransform);
    if (translation != d->m_translation) {
        d->m_translation = translation;
        d->m_matrixDirty = true;
        emit translationChanged(translation);

        const bool wasBlocked = blockNotifications(true);
        emit matrixChanged();
        blockNotifications(wasBlocked);
    }
}

/*!
    \property Qt3DCore::QTransform::translation

    Holds the translation of the transform as QVector3D.
 */
QVector3D QTransform::translation() const
{
    Q_D(const QTransform);
    return d->m_translation;
}

/*!
    Creates a QQuaternion from \a axis and \a angle.
    Returns the resulting QQuaternion.
 */
QQuaternion QTransform::fromAxisAndAngle(const QVector3D &axis, float angle)
{
    return QQuaternion::fromAxisAndAngle(axis, angle);
}

/*!
    Creates a QQuaternion from \a x, \a y, \a z, and \a angle.
    Returns the resulting QQuaternion.
 */
QQuaternion QTransform::fromAxisAndAngle(float x, float y, float z, float angle)
{
    return QQuaternion::fromAxisAndAngle(x, y, z, angle);
}

/*!
    Creates a QQuaternion from \a axis1, \a angle1, \a axis2, and \a angle2.
    Returns the resulting QQuaternion.
 */
QQuaternion QTransform::fromAxesAndAngles(const QVector3D &axis1, float angle1,
                                          const QVector3D &axis2, float angle2)
{
    const QQuaternion q1 = QQuaternion::fromAxisAndAngle(axis1, angle1);
    const QQuaternion q2 = QQuaternion::fromAxisAndAngle(axis2, angle2);
    return q2 * q1;
}

/*!
    Creates a QQuaternion from \a axis1, \a angle1, \a axis2, \a angle2, \a axis3, and \a angle3.
    Returns the resulting QQuaternion.
 */
QQuaternion QTransform::fromAxesAndAngles(const QVector3D &axis1, float angle1,
                                          const QVector3D &axis2, float angle2,
                                          const QVector3D &axis3, float angle3)
{
    const QQuaternion q1 = QQuaternion::fromAxisAndAngle(axis1, angle1);
    const QQuaternion q2 = QQuaternion::fromAxisAndAngle(axis2, angle2);
    const QQuaternion q3 = QQuaternion::fromAxisAndAngle(axis3, angle3);
    return q3 * q2 * q1;
}

/*!
    Creates a QQuaterniom definining a rotation from the axes \a xAxis, \a yAxis and \a zAxis.
    \since 5.11
 */
QQuaternion QTransform::fromAxes(const QVector3D &xAxis, const QVector3D &yAxis, const QVector3D &zAxis)
{
    return QQuaternion::fromAxes(xAxis, yAxis, zAxis);
}

/*!
    Creates a QQuaternion from \a eulerAngles.
    Returns the resulting QQuaternion.
 */
QQuaternion QTransform::fromEulerAngles(const QVector3D &eulerAngles)
{
    return QQuaternion::fromEulerAngles(eulerAngles);
}

/*!
    Creates a QQuaternion from \a pitch, \a yaw, and \a roll.
    Returns the resulting QQuaternion.
 */
QQuaternion QTransform::fromEulerAngles(float pitch, float yaw, float roll)
{
    return QQuaternion::fromEulerAngles(pitch, yaw, roll);
}

/*!
    Creates a rotation matrix from \a axis and \a angle around \a point.
    Returns the resulting QMatrix4x4.
 */
QMatrix4x4 QTransform::rotateAround(const QVector3D &point, float angle, const QVector3D &axis)
{
    QMatrix4x4 m;
    m.translate(point);
    m.rotate(angle, axis);
    m.translate(-point);
    return m;
}

/*!
    Returns a rotation matrix defined from the axes \a xAxis, \a yAxis, \a zAxis.
    \since 5.11
 */
QMatrix4x4 QTransform::rotateFromAxes(const QVector3D &xAxis, const QVector3D &yAxis, const QVector3D &zAxis)
{
    return QMatrix4x4(xAxis.x(), yAxis.x(), zAxis.x(), 0.0f,
                      xAxis.y(), yAxis.y(), zAxis.y(), 0.0f,
                      xAxis.z(), yAxis.z(), zAxis.z(), 0.0f,
                      0.0f, 0.0f, 0.0f, 1.0f);
}

} // namespace Qt3DCore

QT_END_NAMESPACE

#include "moc_qtransform.cpp"
