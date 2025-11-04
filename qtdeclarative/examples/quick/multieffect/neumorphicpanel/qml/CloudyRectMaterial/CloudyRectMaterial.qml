// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
// Created with Qt Quick Effect Maker (version 0.43), Tue Oct 8 10:23:11 2024

import QtQuick

Item {
    id: rootItem

    // Enable this to animate iTime property
    property bool timeRunning: false
    // When timeRunning is false, this can be used to control iTime manually
    property real animatedTime: frameAnimation.elapsedTime

    // The levels of details for the electic clouds. Bigger value means more detailed rending which also requires more processing power. The default value is 6 and practical range is between 1 and 10.
    property int electricCloudLevels: 4
    // The color used for the clouds. Alpha channel defines the amount of opacity this effect has.
    property color electricCloudColor: Qt.rgba(1, 1, 1, 0.6)
    // Color of the shadow/glow.
    property color color: Qt.rgba(0, 0, 0, 1)
    // The size of the shadow rectangle.
    property point rectSize: Qt.point(100, 100)
    // Corner radius amount of the shadow rectangle.
    property real radius: 0
    // Blur radius amount of the shadow rectangle.
    property real blur: 10

    FrameAnimation {
        id: frameAnimation
        running: rootItem.timeRunning
    }

    ShaderEffect {
        readonly property alias iTime: rootItem.animatedTime
        readonly property vector3d iResolution: Qt.vector3d(width, height, 1.0)
        readonly property alias electricCloudLevels: rootItem.electricCloudLevels
        readonly property alias electricCloudColor: rootItem.electricCloudColor
        readonly property alias color: rootItem.color
        readonly property alias rectSize: rootItem.rectSize
        readonly property alias radius: rootItem.radius
        readonly property alias blur: rootItem.blur

        vertexShader: 'cloudyrectmaterial.vert.qsb'
        fragmentShader: 'cloudyrectmaterial.frag.qsb'
        anchors.fill: parent
    }
}
