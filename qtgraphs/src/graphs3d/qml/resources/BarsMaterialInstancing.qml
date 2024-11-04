// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick3D
import QtQuick

CustomMaterial {
    property TextureInput custex: TextureInput {}
    property int colorStyle
    property color uniformColor
    property bool isHighlight
    property bool instancing
    property real specularBrightness: 0.25
    readonly property real shininess: (1.0 - specularBrightness) * 100

    shadingMode: CustomMaterial.Shaded
    vertexShader: "qrc:/shaders/barsinstancingvert"
    fragmentShader: "qrc:/shaders/barsinstancingfrag"
}
