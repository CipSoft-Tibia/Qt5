// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.Drawer {
    id: control
    objectName: "drawer-identified"

    contentItem: Item {
        id: contentItem
        objectName: "drawer-contentItem-identified"
    }

    background: Item {
        id: background
        objectName: "drawer-background-identified"
    }
}
