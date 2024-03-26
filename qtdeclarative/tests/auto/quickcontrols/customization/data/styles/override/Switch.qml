// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import "../simple" as Simple

Simple.Switch {
    id: control
    objectName: "switch-override"

    indicator: Item {
        objectName: "switch-indicator-override"
    }

    contentItem: Item {
        objectName: "switch-contentItem-override"
    }
}
