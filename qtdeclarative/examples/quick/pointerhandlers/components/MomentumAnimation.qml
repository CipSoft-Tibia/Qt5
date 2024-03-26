// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

ParallelAnimation {
    id: root
    property Item target: null
    property int duration: 500
    property vector2d velocity: Qt.vector2d(0,0)

    function restart(vel) {
        stop()
        velocity = vel
        start()
    }

    NumberAnimation {
        id: xAnim
        target: root.target
        property: "x"
        to: target.x + velocity.x / duration * 100
        duration: root.duration
        easing.type: Easing.OutQuad
    }
    NumberAnimation {
        id: yAnim
        target: root.target
        property: "y"
        to: target.y + velocity.y / duration * 100
        duration: root.duration
        easing.type: Easing.OutQuad
    }
}
