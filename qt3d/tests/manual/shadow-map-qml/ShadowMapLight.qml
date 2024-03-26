// Copyright (C) 2014 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import Qt3D.Core 2.0
import Qt3D.Render 2.0

Entity {
    id: root

    property vector3d lightPosition: Qt.vector3d(30.0, 30.0, 0.0)
    property vector3d lightIntensity: Qt.vector3d(1.0, 1.0, 1.0)

    readonly property Camera lightCamera: lightCamera
    readonly property matrix4x4 lightViewProjection: lightCamera.projectionMatrix.times(lightCamera.viewMatrix)

    Camera {
        id: lightCamera
        objectName: "lightCameraLens"
        projectionType: CameraLens.PerspectiveProjection
        fieldOfView: 45
        aspectRatio: 1
        nearPlane : 0.1
        farPlane : 200.0
        position: root.lightPosition
        viewCenter: Qt.vector3d(0.0, 0.0, 0.0)
        upVector: Qt.vector3d(0.0, 1.0, 0.0)
    }
}
