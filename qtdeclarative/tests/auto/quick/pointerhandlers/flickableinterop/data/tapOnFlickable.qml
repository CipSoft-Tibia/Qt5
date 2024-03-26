// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.12

Flickable {
    id: root
    objectName: "root"
    width: 800
    height: 480
    contentWidth: 1000
    contentHeight: 600

    // faster rebound to speed up test runs
    rebound: Transition {
        NumberAnimation {
            properties: "x,y"
            duration: 30
            easing.type: Easing.OutBounce
        }
    }

    Rectangle {
        objectName: "button"
        anchors.centerIn: parent
        border.color: "tomato"
        border.width: 10
        color: innerTap.pressed ? "wheat" : "transparent"
        width: 100
        height: 100
        TapHandler {
            id: innerTap
            objectName: "buttonTap"
        }
    }

    TapHandler {
        objectName: "contentItemTap"
    }
    Component.onCompleted: contentItem.objectName = "Flickable's contentItem"
}
