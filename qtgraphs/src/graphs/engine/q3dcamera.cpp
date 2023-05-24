// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "q3dcamera_p.h"
#include "utils_p.h"

#include <QtCore/qmath.h>

QT_BEGIN_NAMESPACE

/*!
 * \class Q3DCamera
 * \inmodule QtGraphs
 * \brief Representation of a camera in 3D space.
 *
 * Q3DCamera represents a basic orbit around centerpoint 3D camera that is used when rendering the
 * graphs. The class offers simple methods for rotating the camera around the origin
 * and setting the zoom level.
 */

/*!
 * \enum Q3DCamera::CameraPreset
 *
 * Predefined positions for camera.
 *
 * \value CameraPresetNone
 *        Used to indicate a preset has not been set, or the scene has been rotated freely.
 * \value CameraPresetFrontLow
 * \value CameraPresetFront
 * \value CameraPresetFrontHigh
 * \value CameraPresetLeftLow
 * \value CameraPresetLeft
 * \value CameraPresetLeftHigh
 * \value CameraPresetRightLow
 * \value CameraPresetRight
 * \value CameraPresetRightHigh
 * \value CameraPresetBehindLow
 * \value CameraPresetBehind
 * \value CameraPresetBehindHigh
 * \value CameraPresetIsometricLeft
 * \value CameraPresetIsometricLeftHigh
 * \value CameraPresetIsometricRight
 * \value CameraPresetIsometricRightHigh
 * \value CameraPresetDirectlyAbove
 * \value CameraPresetDirectlyAboveCW45
 * \value CameraPresetDirectlyAboveCCW45
 * \value CameraPresetFrontBelow
 *        In Q3DBars from CameraPresetFrontBelow onward these only work for graphs including negative
 *        values. They act as Preset...Low for positive-only values.
 * \value CameraPresetLeftBelow
 * \value CameraPresetRightBelow
 * \value CameraPresetBehindBelow
 * \value CameraPresetDirectlyBelow
 *        Acts as CameraPresetFrontLow for positive-only bars.
 */

/*!
 * \qmltype Camera3D
 * \inqmlmodule QtGraphs
 * \ingroup graphs_qml
 * \instantiates Q3DCamera
 * \brief Representation of a camera in 3D space.
 *
 * Camera3D represents a basic orbit around centerpoint 3D camera that is used when rendering the
 * graphs. The type offers simple methods for rotating the camera around the origin
 * and setting the zoom level.
 *
 * For Camera3D enums, see \l{Q3DCamera::CameraPreset}.
 */

/*!
 * \qmlproperty float Camera3D::xRotation
 *
 * The X-rotation angle of the camera around the target point in degrees
 * starting from the current base position.
 */

/*!
 * \qmlproperty float Camera3D::yRotation
 *
 * The Y-rotation angle of the camera around the target point in degrees
 * starting from the current base position.
 */

/*!
 * \qmlproperty Camera3D.CameraPreset Camera3D::cameraPreset
 *
 * The currently active camera preset, which is one of
 * \l{Q3DCamera::CameraPreset}{Camera3D.CameraPreset}. If no preset is active, the value
 * is \l{Q3DCamera::CameraPresetNone}{Camera3D.CameraPresetNone}.
 */

/*!
 * \qmlproperty float Camera3D::zoomLevel
 *
 * The camera zoom level in percentage. The default value of \c{100.0}
 * means there is no zoom in or out set in the camera.
 * The value is limited by the minZoomLevel and maxZoomLevel properties.
 *
 * \sa minZoomLevel, maxZoomLevel
 */

/*!
 * \qmlproperty float Camera3D::minZoomLevel
 *
 * Sets the minimum allowed camera zoom level.
 * If the new minimum level is higher than the existing maximum level, the maximum level is
 * adjusted to the new minimum as well.
 * If the current zoomLevel is outside the new bounds, it is adjusted as well.
 * The minZoomLevel cannot be set below \c{1.0}.
 * Defaults to \c{10.0}.
 *
 * \sa zoomLevel, maxZoomLevel
 */

/*!
 * \qmlproperty float Camera3D::maxZoomLevel
 *
 * Sets the maximum allowed camera zoom level.
 * If the new maximum level is lower than the existing minimum level, the minimum level is
 * adjusted to the new maximum as well.
 * If the current zoomLevel is outside the new bounds, it is adjusted as well.
 * Defaults to \c{500.0f}.
 *
 * \sa zoomLevel, minZoomLevel
 */

/*!
 * \qmlproperty bool Camera3D::wrapXRotation
 *
 * The behavior of the minimum and maximum limits in the X-rotation.
 * By default, the X-rotation wraps from minimum value to maximum and from
 * maximum to minimum.
 *
 * If set to \c true, the X-rotation of the camera is wrapped from minimum to
 * maximum and from maximum to minimum. If set to \c false, the X-rotation of
 * the camera is limited to the sector determined by the minimum and maximum
 * values.
 */

/*!
 * \qmlproperty bool Camera3D::wrapYRotation
 *
 * The behavior of the minimum and maximum limits in the Y-rotation.
 * By default, the Y-rotation is limited between the minimum and maximum values
 * without any wrapping.
 *
 * If \c true, the Y-rotation of the camera is wrapped from minimum to maximum
 * and from maximum to minimum. If \c false, the Y-rotation of the camera is
 * limited to the sector determined by the minimum and maximum values.
 */

/*!
 * \qmlproperty vector3d Camera3D::target
 *
 * The camera target as a vector3d. Defaults to \c {vector3d(0.0, 0.0, 0.0)}.
 *
 * Valid coordinate values are between \c{-1.0...1.0}, where the edge values indicate
 * the edges of the corresponding axis range. Any values outside this range are clamped to the edge.
 *
 * \note For bar graphs, the Y-coordinate is ignored and camera always targets a point on
 * the horizontal background.
 */

/*!
 * Constructs a new 3D camera with position set to origin, up direction facing towards the Y-axis
 * and looking at origin by default. An optional \a parent parameter can be given and is then passed
 * to QObject constructor.
 */
Q3DCamera::Q3DCamera(QObject *parent) :
    Q3DObject(new Q3DCameraPrivate(this), parent)
{
}

/*!
 *  Destroys the camera object.
 */
Q3DCamera::~Q3DCamera()
{
}

/*!
 * Copies the 3D camera's properties from the given source camera.
 * Values are copied from the \a source to this object.
 */
void Q3DCamera::copyValuesFrom(const Q3DObject &source)
{
    Q_D(Q3DCamera);
    // Note: Do not copy values from parent, as we are handling the position internally

    const Q3DCamera &sourceCamera = static_cast<const Q3DCamera &>(source);

    d->m_requestedTarget = sourceCamera.d_func()->m_requestedTarget;

    d->m_xRotation = sourceCamera.d_func()->m_xRotation;
    d->m_yRotation = sourceCamera.d_func()->m_yRotation;

    d->m_minXRotation = sourceCamera.d_func()->m_minXRotation;
    d->m_minYRotation = sourceCamera.d_func()->m_minYRotation;
    d->m_maxXRotation = sourceCamera.d_func()->m_maxXRotation;
    d->m_maxYRotation = sourceCamera.d_func()->m_maxYRotation;

    d->m_wrapXRotation = sourceCamera.d_func()->m_wrapXRotation;
    d->m_wrapYRotation = sourceCamera.d_func()->m_wrapYRotation;

    d->m_zoomLevel = sourceCamera.d_func()->m_zoomLevel;
    d->m_minZoomLevel = sourceCamera.d_func()->m_minZoomLevel;
    d->m_maxZoomLevel = sourceCamera.d_func()->m_maxZoomLevel;
    d->m_activePreset = sourceCamera.d_func()->m_activePreset;
}

/*!
 * \property Q3DCamera::xRotation
 *
 * \brief The X-rotation angle of the camera around the target point in degrees.
 */
float Q3DCamera::xRotation() const
{
    const Q_D(Q3DCamera);
    return d->m_xRotation;
}

void Q3DCamera::setXRotation(float rotation)
{
    Q_D(Q3DCamera);
    if (d->m_wrapXRotation) {
        rotation = Utils::wrapValue(rotation, d->m_minXRotation, d->m_maxXRotation);
    } else {
        rotation = qBound(float(d->m_minXRotation), float(rotation),
                          float(d->m_maxXRotation));
    }

    if (d->m_xRotation != rotation) {
        d->setXRotation(rotation);
        if (d->m_activePreset != CameraPresetNone) {
            d->m_activePreset = CameraPresetNone;
            setDirty(true);
        }

        emit xRotationChanged(d->m_xRotation);
    }
}

/*!
 * \property Q3DCamera::yRotation
 *
 * \brief The Y-rotation angle of the camera around the target point in degrees.
 */
float Q3DCamera::yRotation() const
{
    const Q_D(Q3DCamera);
    return d->m_yRotation;
}

void Q3DCamera::setYRotation(float rotation)
{
    Q_D(Q3DCamera);
    if (d->m_wrapYRotation) {
        rotation = Utils::wrapValue(rotation, d->m_minYRotation, d->m_maxYRotation);
    } else {
        rotation = qBound(float(d->m_minYRotation), float(rotation),
                          float(d->m_maxYRotation));
    }

    if (d->m_yRotation != rotation) {
        d->setYRotation(rotation);
        if (d->m_activePreset != CameraPresetNone) {
            d->m_activePreset = CameraPresetNone;
            setDirty(true);
        }

        emit yRotationChanged(d->m_yRotation);
    }
}

/*!
 * \property Q3DCamera::cameraPreset
 *
 * \brief The currently active camera preset.
 *
 * If no CameraPreset value is set, CameraPresetNone is used by default.
 */
Q3DCamera::CameraPreset Q3DCamera::cameraPreset() const
{
    const Q_D(Q3DCamera);
    return d->m_activePreset;
}

void Q3DCamera::setCameraPreset(Q3DCamera::CameraPreset preset)
{
    Q_D(Q3DCamera);
    switch (preset) {
    case CameraPresetFrontLow: {
        setXRotation(0.0f);
        setYRotation(0.0f);
        break;
    }
    case CameraPresetFront: {
        setXRotation(0.0f);
        setYRotation(22.5f);
        break;
    }
    case CameraPresetFrontHigh: {
        setXRotation(0.0f);
        setYRotation(45.0f);
        break;
    }
    case CameraPresetLeftLow: {
        setXRotation(90.0f);
        setYRotation(0.0f);
        break;
    }
    case CameraPresetLeft: {
        setXRotation(90.0f);
        setYRotation(22.5f);
        break;
    }
    case CameraPresetLeftHigh: {
        setXRotation(90.0f);
        setYRotation(45.0f);
        break;
    }
    case CameraPresetRightLow: {
        setXRotation(-90.0f);
        setYRotation(0.0f);
        break;
    }
    case CameraPresetRight: {
        setXRotation(-90.0f);
        setYRotation(22.5f);
        break;
    }
    case CameraPresetRightHigh: {
        setXRotation(-90.0f);
        setYRotation(45.0f);
        break;
    }
    case CameraPresetBehindLow: {
        setXRotation(180.0f);
        setYRotation(0.0f);
        break;
    }
    case CameraPresetBehind: {
        setXRotation(180.0f);
        setYRotation(22.5f);
        break;
    }
    case CameraPresetBehindHigh: {
        setXRotation(180.0f);
        setYRotation(45.0f);
        break;
    }
    case CameraPresetIsometricLeft: {
        setXRotation(45.0f);
        setYRotation(22.5f);
        break;
    }
    case CameraPresetIsometricLeftHigh: {
        setXRotation(45.0f);
        setYRotation(45.0f);
        break;
    }
    case CameraPresetIsometricRight: {
        setXRotation(-45.0f);
        setYRotation(22.5f);
        break;
    }
    case CameraPresetIsometricRightHigh: {
        setXRotation(-45.0f);
        setYRotation(45.0f);
        break;
    }
    case CameraPresetDirectlyAbove: {
        setXRotation(0.0f);
        setYRotation(90.0f);
        break;
    }
    case CameraPresetDirectlyAboveCW45: {
        setXRotation(-45.0f);
        setYRotation(90.0f);
        break;
    }
    case CameraPresetDirectlyAboveCCW45: {
        setXRotation(45.0f);
        setYRotation(90.0f);
        break;
    }
    case CameraPresetFrontBelow: {
        setXRotation(0.0f);
        setYRotation(-45.0f);
        break;
    }
    case CameraPresetLeftBelow: {
        setXRotation(90.0f);
        setYRotation(-45.0f);
        break;
    }
    case CameraPresetRightBelow: {
        setXRotation(-90.0f);
        setYRotation(-45.0f);
        break;
    }
    case CameraPresetBehindBelow: {
        setXRotation(180.0f);
        setYRotation(-45.0f);
        break;
    }
    case CameraPresetDirectlyBelow: {
        setXRotation(0.0f);
        setYRotation(-90.0f);
        break;
    }
    default:
        preset = CameraPresetNone;
        break;
    }

    // All presets target the center of the graph
    setTarget(zeroVector);

    if (d->m_activePreset != preset) {
        d->m_activePreset = preset;
        setDirty(true);
        emit cameraPresetChanged(preset);
    }
}

/*!
 * \property Q3DCamera::zoomLevel
 *
 * \brief The camera zoom level in percentage.
 *
 * The default value of \c{100.0f} means there is no zoom in or out set in the
 * camera. The value is limited by the minZoomLevel and maxZoomLevel properties.
 *
 * \sa minZoomLevel, maxZoomLevel
 */
float Q3DCamera::zoomLevel() const
{
    const Q_D(Q3DCamera);
    return d->m_zoomLevel;
}

void Q3DCamera::setZoomLevel(float zoomLevel)
{
    Q_D(Q3DCamera);
    float newZoomLevel = qBound(d->m_minZoomLevel, zoomLevel, d->m_maxZoomLevel);

    if (d->m_zoomLevel != newZoomLevel) {
        d->m_zoomLevel = newZoomLevel;
        setDirty(true);
        emit zoomLevelChanged(newZoomLevel);
    }
}

/*!
 * \property Q3DCamera::minZoomLevel
 *
 * \brief The minimum allowed camera zoom level.
 *
 * If the minimum level is set to a new value that is higher than the existing
 * maximum level, the maximum level is adjusted to the new minimum as well.
 * If the current zoomLevel is outside the new bounds, it is adjusted as well.
 * The minZoomLevel cannot be set below \c{1.0f}.
 * Defaults to \c{10.0f}.
 *
 * \sa zoomLevel, maxZoomLevel
 */
float Q3DCamera::minZoomLevel() const
{
    const Q_D(Q3DCamera);
    return d->m_minZoomLevel;
}

void Q3DCamera::setMinZoomLevel(float zoomLevel)
{
    Q_D(Q3DCamera);
    // Don't allow minimum to be below one, as that can cause zoom to break.
    float newMinLevel = qMax(zoomLevel, 1.0f);
    if (d->m_minZoomLevel != newMinLevel) {
        d->m_minZoomLevel = newMinLevel;
        if (d->m_maxZoomLevel < newMinLevel)
            setMaxZoomLevel(newMinLevel);
        setZoomLevel(d->m_zoomLevel);
        setDirty(true);
        emit minZoomLevelChanged(newMinLevel);
    }
}

/*!
 * \property Q3DCamera::maxZoomLevel
 *
 * \brief The maximum allowed camera zoom level.
 *
 * If the maximum level is set to a new value that is lower than the existing
 * minimum level, the minimum level is adjusted to the new maximum as well.
 * If the current zoomLevel is outside the new bounds, it is adjusted as well.
 * Defaults to \c{500.0f}.
 *
 * \sa zoomLevel, minZoomLevel
 */
float Q3DCamera::maxZoomLevel() const
{
    const Q_D(Q3DCamera);
    return d->m_maxZoomLevel;
}

void Q3DCamera::setMaxZoomLevel(float zoomLevel)
{
    Q_D(Q3DCamera);
    // Don't allow maximum to be below one, as that can cause zoom to break.
    float newMaxLevel = qMax(zoomLevel, 1.0f);
    if (d->m_maxZoomLevel != newMaxLevel) {
        d->m_maxZoomLevel = newMaxLevel;
        if (d->m_minZoomLevel > newMaxLevel)
            setMinZoomLevel(newMaxLevel);
        setZoomLevel(d->m_zoomLevel);
        setDirty(true);
        emit maxZoomLevelChanged(newMaxLevel);
    }
}

/*!
 * \property Q3DCamera::wrapXRotation
 *
 * \brief The behavior of the minimum and maximum limits in the X-rotation.
 *
 * If set to \c true, the X-rotation of the camera is wrapped from minimum to
 * maximum and from maximum to minimum. If set to \c false, the X-rotation of
 * the camera is limited to the sector determined by the minimum and maximum
 * values. Set to \c true by default.
 */
bool Q3DCamera::wrapXRotation() const
{
    const Q_D(Q3DCamera);
    return d->m_wrapXRotation;
}

void Q3DCamera::setWrapXRotation(bool isEnabled)
{
    Q_D(Q3DCamera);
    d->m_wrapXRotation = isEnabled;
}

/*!
 * \property Q3DCamera::wrapYRotation
 *
 * \brief The behavior of the minimum and maximum limits in the Y-rotation.
 *
 * If \c true, the Y-rotation of the camera is wrapped from minimum to maximum
 * and from maximum to minimum. If \c false, the Y-rotation of the camera is
 * limited to the sector determined by the minimum and maximum values.
 * Set to \c true by default.
 */
bool Q3DCamera::wrapYRotation() const
{
    const Q_D(Q3DCamera);
    return d->m_wrapYRotation;
}

void Q3DCamera::setWrapYRotation(bool isEnabled)
{
    Q_D(Q3DCamera);
    d->m_wrapYRotation = isEnabled;
}

/*!
 * Utility function that sets the camera rotations and distance.\a horizontal and \a vertical
 * define the camera rotations to be used.
 * Optional \a zoom parameter can be given to set the zoom percentage of the camera within
 * the bounds defined by minZoomLevel and maxZoomLevel properties.
 */
void Q3DCamera::setCameraPosition(float horizontal, float vertical, float zoom)
{
    setZoomLevel(zoom);
    setXRotation(horizontal);
    setYRotation(vertical);
}

/*!
 * \property Q3DCamera::target
 *
 * \brief The camera target as a vector or vertex in the 3D space.
 *
 * Defaults to \c {QVector3D(0.0, 0.0, 0.0)}.
 *
 * Valid coordinate values are between \c{-1.0...1.0}, where the edge values indicate
 * the edges of the corresponding axis range. Any values outside this range are clamped to the edge.
 *
 * \note For bar graphs, the Y-coordinate is ignored and camera always targets a point on
 * the horizontal background.
 */
QVector3D Q3DCamera::target() const
{
    const Q_D(Q3DCamera);
    return d->m_requestedTarget;
}

void Q3DCamera::setTarget(const QVector3D &target)
{
    Q_D(Q3DCamera);
    QVector3D newTarget = target;

    if (newTarget.x() < -1.0f)
        newTarget.setX(-1.0f);
    else if (newTarget.x() > 1.0f)
        newTarget.setX(1.0f);

    if (newTarget.y() < -1.0f)
        newTarget.setY(-1.0f);
    else if (newTarget.y() > 1.0f)
        newTarget.setY(1.0f);

    if (newTarget.z() < -1.0f)
        newTarget.setZ(-1.0f);
    else if (newTarget.z() > 1.0f)
        newTarget.setZ(1.0f);

    if (d->m_requestedTarget != newTarget) {
        if (d->m_activePreset != CameraPresetNone)
            d->m_activePreset = CameraPresetNone;
        d->m_requestedTarget = newTarget;
        setDirty(true);
        emit targetChanged(newTarget);
    }
}

Q3DCameraPrivate::Q3DCameraPrivate(Q3DCamera *q) :
    Q3DObjectPrivate(q),
    m_isViewMatrixUpdateActive(true),
    m_xRotation(0.0f),
    m_yRotation(0.0f),
    m_minXRotation(-180.0f),
    m_minYRotation(0.0f),
    m_maxXRotation(180.0f),
    m_maxYRotation(90.0f),
    m_zoomLevel(100.0f),
    m_minZoomLevel(10.0f),
    m_maxZoomLevel(500.0f),
    m_wrapXRotation(true),
    m_wrapYRotation(false),
    m_activePreset(Q3DCamera::CameraPresetNone)
{
}

Q3DCameraPrivate::~Q3DCameraPrivate()
{
}

// Copies changed values from this camera to the other camera. If the other camera had same changes,
// those changes are discarded.
void Q3DCameraPrivate::sync(Q3DCamera &other)
{
    Q_Q(Q3DCamera);
    if (q->isDirty()) {
        other.copyValuesFrom(*q);
        q->setDirty(false);
        other.setDirty(false);
    }
}

void Q3DCameraPrivate::setXRotation(const float rotation)
{
    Q_Q(Q3DCamera);
    if (m_xRotation != rotation) {
        m_xRotation = rotation;
        q->setDirty(true);
    }
}

void Q3DCameraPrivate::setYRotation(const float rotation)
{
    Q_Q(Q3DCamera);
    if (m_yRotation != rotation) {
        m_yRotation = rotation;
        q->setDirty(true);
    }
}

/*!
 * \internal
 * The current minimum X-rotation for the camera.
 * The full circle range is \c{[-180, 180]} and the minimum value is limited to \c -180.
 * Also the value can't be higher than the maximum, and is adjusted if necessary.
 *
 * \sa wrapXRotation, maxXRotation
 */
float Q3DCameraPrivate::minXRotation() const
{
    return m_minXRotation;
}

void Q3DCameraPrivate::setMinXRotation(float minRotation)
{
    Q_Q(Q3DCamera);
    minRotation = qBound(-180.0f, minRotation, 180.0f);
    if (minRotation > m_maxXRotation)
        minRotation = m_maxXRotation;

    if (m_minXRotation != minRotation) {
        m_minXRotation = minRotation;
        emit q->minXRotationChanged(minRotation);

        if (m_xRotation < m_minXRotation)
            setXRotation(m_xRotation);
        q->setDirty(true);
    }
}

/*!
 * \internal
 * The current minimum Y-rotation for the camera.
 * The full Y angle range is \c{[-90, 90]} and the minimum value is limited to \c -90.
 * Also the value can't be higher than the maximum, and is adjusted if necessary.
 *
 * \sa wrapYRotation, maxYRotation
 */
float Q3DCameraPrivate::minYRotation() const
{
    return m_minYRotation;
}

void Q3DCameraPrivate::setMinYRotation(float minRotation)
{
    Q_Q(Q3DCamera);
    minRotation = qBound(-90.0f, minRotation, 90.0f);
    if (minRotation > m_maxYRotation)
        minRotation = m_maxYRotation;

    if (m_minYRotation != minRotation) {
        m_minYRotation = minRotation;
        emit q->minYRotationChanged(minRotation);

        if (m_yRotation < m_minYRotation)
            setYRotation(m_yRotation);
        q->setDirty(true);
    }
}

/*!
 * \internal
 * The current maximum X-rotation for the camera.
 * The full circle range is \c{[-180, 180]} and the maximum value is limited to \c 180.
 * Also the value can't be lower than the minimum, and is adjusted if necessary.
 *
 * \sa wrapXRotation, minXRotation
 */
float Q3DCameraPrivate::maxXRotation() const
{
    return m_maxXRotation;
}

void Q3DCameraPrivate::setMaxXRotation(float maxRotation)
{
    Q_Q(Q3DCamera);
    maxRotation = qBound(-180.0f, maxRotation, 180.0f);

    if (maxRotation < m_minXRotation)
        maxRotation = m_minXRotation;

    if (m_maxXRotation != maxRotation) {
        m_maxXRotation = maxRotation;
        emit q->maxXRotationChanged(maxRotation);

        if (m_xRotation > m_maxXRotation)
            setXRotation(m_xRotation);
        q->setDirty(true);
    }
}

/*!
 * \internal
 * The current maximum Y-rotation for the camera.
 * The full Y angle range is \c{[-90, 90]} and the maximum value is limited to \c 90.
 * Also the value can't be lower than the minimum, and is adjusted if necessary.
 *
 * \sa wrapYRotation, minYRotation
 */
float Q3DCameraPrivate::maxYRotation() const
{
    return m_maxYRotation;
}

void Q3DCameraPrivate::setMaxYRotation(float maxRotation)
{
    Q_Q(Q3DCamera);
    maxRotation = qBound(-90.0f, maxRotation, 90.0f);

    if (maxRotation < m_minYRotation)
        maxRotation = m_minYRotation;

    if (m_maxYRotation != maxRotation) {
        m_maxYRotation = maxRotation;
        emit q->maxYRotationChanged(maxRotation);

        if (m_yRotation > m_maxYRotation)
            setYRotation(m_yRotation);
        q->setDirty(true);
    }
}

// Recalculates the view matrix based on the currently set base orientation, rotation and zoom level values.
// zoomAdjustment is adjustment to ensure that the 3D visualization stays inside the view area in the 100% zoom.
void Q3DCameraPrivate::updateViewMatrix(float zoomAdjustment)
{
    Q_Q(Q3DCamera);
    if (!m_isViewMatrixUpdateActive)
        return;

    float zoom = m_zoomLevel * zoomAdjustment;
    QMatrix4x4 viewMatrix;

    // Apply to view matrix
    viewMatrix.lookAt(q->position(), m_actualTarget, m_up);
    // Compensate for translation (if d->m_target is off origin)
    viewMatrix.translate(m_actualTarget.x(), m_actualTarget.y(), m_actualTarget.z());
    // Apply rotations
    // Handle x and z rotation when y -angle is other than 0
    viewMatrix.rotate(m_xRotation, 0, qCos(qDegreesToRadians(m_yRotation)),
                      qSin(qDegreesToRadians(m_yRotation)));
    // y rotation is always "clean"
    viewMatrix.rotate(m_yRotation, 1.0f, 0.0f, 0.0f);
    // handle zoom by scaling
    viewMatrix.scale(zoom / 100.0f);
    // Compensate for translation (if d->m_target is off origin)
    viewMatrix.translate(-m_actualTarget.x(), -m_actualTarget.y(), -m_actualTarget.z());

    setViewMatrix(viewMatrix);
}

/*!
 * \internal
 * The view matrix used in the 3D calculations. When the default orbiting
 * camera behavior is sufficient, there is no need to touch this property. If the default
 * behavior is insufficient, the view matrix can be set directly.
 * \note When setting the view matrix directly remember to set viewMatrixAutoUpdateEnabled to
 * \c false.
 */
QMatrix4x4 Q3DCameraPrivate::viewMatrix() const
{
    return m_viewMatrix;
}

void Q3DCameraPrivate::setViewMatrix(const QMatrix4x4 &viewMatrix)
{
    Q_Q(Q3DCamera);
    if (m_viewMatrix != viewMatrix) {
        m_viewMatrix = viewMatrix;
        q->setDirty(true);
        emit q->viewMatrixChanged(m_viewMatrix);
    }
}

/*!
 * \internal
 * This property determines if view matrix is automatically updated each render cycle using the
 * current base orientation and rotations. If set to \c false, no automatic recalculation is done
 * and the view matrix can be set using the viewMatrix property.
 */
bool Q3DCameraPrivate::isViewMatrixAutoUpdateEnabled() const
{
    return m_isViewMatrixUpdateActive;
}

void Q3DCameraPrivate::setViewMatrixAutoUpdateEnabled(bool isEnabled)
{
    Q_Q(Q3DCamera);
    m_isViewMatrixUpdateActive = isEnabled;
    emit q->viewMatrixAutoUpdateChanged(isEnabled);
}

/*!
 * \internal
 * Sets the base values for the camera that are used when calculating the camera position using the
 * rotation values. The base position of the camera is defined by \a basePosition, expectation is
 * that the x and y values are 0. Look at target point is defined by \a target and the camera
 * rotates around it. Up direction for the camera is defined by \a baseUp, normally this is a
 * vector with only y value set to 1.
 */
void Q3DCameraPrivate::setBaseOrientation(const QVector3D &basePosition,
                                          const QVector3D &target,
                                          const QVector3D &baseUp)
{
    Q_Q(Q3DCamera);
    if (q->position() != basePosition || m_actualTarget != target || m_up != baseUp) {
        q->setPosition(basePosition);
        m_actualTarget = target;
        m_up = baseUp;
        q->setDirty(true);
    }
}

/*!
 * \internal
 * Calculates and returns a position relative to the camera using the given parameters
 * and the current camera viewMatrix property.
 * The relative 3D offset to the current camera position is defined in \a relativePosition.
 * An optional fixed rotation of the calculated point around the graph area can be
 * given in \a fixedRotation. The rotation is given in degrees.
 * An optional \a distanceModifier modifies the distance of the calculated point from the graph.
 * \return calculated position relative to this camera's position.
 */
QVector3D Q3DCameraPrivate::calculatePositionRelativeToCamera(const QVector3D &relativePosition,
                                                              float fixedRotation,
                                                              float distanceModifier) const
{
    // Move the position with camera
    const float radiusFactor = cameraDistance * (1.5f + distanceModifier);
    float xAngle;
    float yAngle;

    if (!fixedRotation) {
        xAngle = qDegreesToRadians(m_xRotation);
        float yRotation = m_yRotation;
        // Light must not be paraller to eye vector, so fudge the y rotation a bit.
        // Note: This needs redoing if we ever allow arbitrary light positioning.
        const float yMargin = 0.1f; // Smaller margins cause weird shadow artifacts on tops of bars
        const float absYRotation = qAbs(yRotation);
        if (absYRotation < 90.0f + yMargin && absYRotation > 90.0f - yMargin) {
            if (yRotation < 0.0f)
                yRotation = -90.0f + yMargin;
            else
                yRotation = 90.0f - yMargin;
        }
        yAngle = qDegreesToRadians(yRotation);
    } else {
        xAngle = qDegreesToRadians(fixedRotation);
        yAngle = 0;
    }
    // Set radius to match the highest height of the position
    const float radius = (radiusFactor + relativePosition.y());
    const float zPos = radius * qCos(xAngle) * qCos(yAngle);
    const float xPos = radius * qSin(xAngle) * qCos(yAngle);
    const float yPos = radius * qSin(yAngle);

    // Keep in the set position in relation to camera
    return QVector3D(-xPos + relativePosition.x(),
                     yPos + relativePosition.y(),
                     zPos + relativePosition.z());
}

QT_END_NAMESPACE
