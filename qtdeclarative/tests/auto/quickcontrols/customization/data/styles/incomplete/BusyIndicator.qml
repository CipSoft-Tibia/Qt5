// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.BusyIndicator {
    id: control
    objectName: "busyindicator-incomplete"

    contentItem: Item {
        objectName: "busyindicator-contentItem-incomplete"
    }

    background: Item {
        objectName: "busyindicator-background-incomplete"
    }
}
