// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.GroupBox {
    id: control
    objectName: "groupbox-incomplete"

    label: Text {
        objectName: "groupbox-label-incomplete"
    }

    contentItem: Item {
        objectName: "groupbox-contentItem-incomplete"
    }

    background: Item {
        objectName: "groupbox-background-incomplete"
    }
}
