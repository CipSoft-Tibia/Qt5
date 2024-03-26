// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.AbstractButton {
    id: control
    objectName: "abstractbutton-incomplete"

    indicator: Item {
        objectName: "abstractbutton-indicator-incomplete"
    }

    contentItem: Item {
        objectName: "abstractbutton-contentItem-incomplete"
    }

    background: Item {
        objectName: "abstractbutton-background-incomplete"
    }
}
