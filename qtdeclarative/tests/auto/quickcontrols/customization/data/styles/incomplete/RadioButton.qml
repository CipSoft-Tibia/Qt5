// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.RadioButton {
    id: control
    objectName: "radiobutton-incomplete"

    indicator: Item {
        objectName: "radiobutton-indicator-incomplete"
    }

    contentItem: Item {
        objectName: "radiobutton-contentItem-incomplete"
    }
}
