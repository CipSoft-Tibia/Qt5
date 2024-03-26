// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.Dial {
    id: control
    objectName: "dial-simple"

    implicitWidth: Math.max(handle.implicitWidth, background.implicitWidth)
    implicitHeight: Math.max(handle.implicitHeight, background.implicitHeight)

    handle: Rectangle {
        objectName: "dial-handle-simple"
        color: control.pressed ? "red" : "green"
    }

    background: Rectangle {
        objectName: "dial-background-simple"
        implicitWidth: 200
        implicitHeight: 20
    }
}
