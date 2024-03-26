// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.SwipeView {
    id: control
    objectName: "swipeview-identified"

    contentItem: Item {
        id: contentItem
        objectName: "swipeview-contentItem-identified"
    }

    background: Item {
        id: background
        objectName: "swipeview-background-identified"
    }
}
