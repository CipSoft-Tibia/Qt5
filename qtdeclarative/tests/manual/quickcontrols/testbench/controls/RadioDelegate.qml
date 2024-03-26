// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

QtObject {
    property var supportedStates: [
        [],
        ["disabled"],
        ["pressed"],
        ["checked"],
        ["checked", "disabled"],
        ["checked", "pressed"]
    ]

    property Component component: Component {
        RadioDelegate {
            text: "RadioDelegate"
            enabled: !is("disabled")
            checked: is("checked")
            // Only set it if it's pressed, or the non-pressed examples will have no press effects
            down: is("pressed") ? true : undefined
            focusPolicy: Qt.StrongFocus
        }
    }

    property Component exampleComponent: ListView {
        implicitWidth: 200
        implicitHeight: 200
        clip: true
        model: 20
        delegate: RadioDelegate {
            width: parent.width
            text: "RadioDelegate"
            focusPolicy: Qt.StrongFocus
        }
    }
}
