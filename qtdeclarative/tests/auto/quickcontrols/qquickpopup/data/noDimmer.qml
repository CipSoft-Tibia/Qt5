// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

Window {
    T.Drawer {
        id: root
        onModalChanged: {
            if (!modal) {
                open()
            }
        }
    }
    Timer {
        interval: 100
        running: true
        repeat: false
        onTriggered: root.modal = false
    }
}
