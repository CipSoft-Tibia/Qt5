// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.ToolTip {
    id: control
    objectName: "tooltip-identified"

    contentItem: Item {
        id: contentItem
        objectName: "tooltip-contentItem-identified"
    }

    background: Item {
        id: background
        objectName: "tooltip-background-identified"
    }
}
