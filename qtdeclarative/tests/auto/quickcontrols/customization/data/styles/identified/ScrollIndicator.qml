// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.ScrollIndicator {
    id: control
    objectName: "scrollindicator-identified"

    contentItem: Item {
        id: contentItem
        objectName: "scrollindicator-contentItem-identified"
    }

    background: Item {
        id: background
        objectName: "scrollindicator-background-identified"
    }
}
