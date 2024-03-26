// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef Q3DCAMERA_H
#define Q3DCAMERA_H

#include <QtGraphs/q3dobject.h>

QT_BEGIN_NAMESPACE

class Q3DCameraPrivate;

class Q_GRAPHS_EXPORT Q3DCamera : public Q3DObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(Q3DCamera)
    Q_PROPERTY(float xRotation READ xRotation WRITE setXRotation NOTIFY xRotationChanged)
    Q_PROPERTY(float yRotation READ yRotation WRITE setYRotation NOTIFY yRotationChanged)
    Q_PROPERTY(float zoomLevel READ zoomLevel WRITE setZoomLevel NOTIFY zoomLevelChanged)
    Q_PROPERTY(Q3DCamera::CameraPreset cameraPreset READ cameraPreset WRITE setCameraPreset NOTIFY cameraPresetChanged)
    Q_PROPERTY(bool wrapXRotation READ wrapXRotation WRITE setWrapXRotation NOTIFY wrapXRotationChanged)
    Q_PROPERTY(bool wrapYRotation READ wrapYRotation WRITE setWrapYRotation NOTIFY wrapYRotationChanged)
    Q_PROPERTY(QVector3D target READ target WRITE setTarget NOTIFY targetChanged)
    Q_PROPERTY(float minZoomLevel READ minZoomLevel WRITE setMinZoomLevel NOTIFY minZoomLevelChanged)
    Q_PROPERTY(float maxZoomLevel READ maxZoomLevel WRITE setMaxZoomLevel NOTIFY maxZoomLevelChanged)

public:
    enum CameraPreset {
        CameraPresetNone = -1,
        CameraPresetFrontLow = 0,
        CameraPresetFront,
        CameraPresetFrontHigh,
        CameraPresetLeftLow,
        CameraPresetLeft,
        CameraPresetLeftHigh,
        CameraPresetRightLow,
        CameraPresetRight,
        CameraPresetRightHigh,
        CameraPresetBehindLow,
        CameraPresetBehind,
        CameraPresetBehindHigh,
        CameraPresetIsometricLeft,
        CameraPresetIsometricLeftHigh,
        CameraPresetIsometricRight,
        CameraPresetIsometricRightHigh,
        CameraPresetDirectlyAbove,
        CameraPresetDirectlyAboveCW45,
        CameraPresetDirectlyAboveCCW45,
        CameraPresetFrontBelow,
        CameraPresetLeftBelow,
        CameraPresetRightBelow,
        CameraPresetBehindBelow,
        CameraPresetDirectlyBelow
    };
    Q_ENUM(CameraPreset)

    explicit Q3DCamera(QObject *parent = nullptr);
    virtual ~Q3DCamera();

    float xRotation() const;
    void setXRotation(float rotation);
    float yRotation() const;
    void setYRotation(float rotation);

    bool wrapXRotation() const;
    void setWrapXRotation(bool isEnabled);

    bool wrapYRotation() const;
    void setWrapYRotation(bool isEnabled);

    void copyValuesFrom(const Q3DObject &source) override;

    Q3DCamera::CameraPreset cameraPreset() const;
    void setCameraPreset(Q3DCamera::CameraPreset preset);

    float zoomLevel() const;
    void setZoomLevel(float zoomLevel);
    float minZoomLevel() const;
    void setMinZoomLevel(float zoomLevel);
    float maxZoomLevel() const;
    void setMaxZoomLevel(float zoomLevel);

    void setCameraPosition(float horizontal, float vertical, float zoom = 100.0f);

    QVector3D target() const;
    void setTarget(const QVector3D &target);

Q_SIGNALS:
    void xRotationChanged(float rotation);
    void yRotationChanged(float rotation);
    void zoomLevelChanged(float zoomLevel);
    void cameraPresetChanged(Q3DCamera::CameraPreset preset);
    void wrapXRotationChanged(bool isEnabled);
    void wrapYRotationChanged(bool isEnabled);
    void targetChanged(const QVector3D &target);
    void minZoomLevelChanged(float zoomLevel);
    void maxZoomLevelChanged(float zoomLevel);
    void minXRotationChanged(float rotation);
    void minYRotationChanged(float rotation);
    void maxXRotationChanged(float rotation);
    void maxYRotationChanged(float rotation);
    void viewMatrixChanged(const QMatrix4x4 &viewMatrix);
    void viewMatrixAutoUpdateChanged(bool enabled);

private:
    Q_DISABLE_COPY(Q3DCamera)

    friend class Q3DScenePrivate;
    friend class QQuickGraphsScatter;
    friend class QQuickGraphsBars;
};

QT_END_NAMESPACE

#endif
