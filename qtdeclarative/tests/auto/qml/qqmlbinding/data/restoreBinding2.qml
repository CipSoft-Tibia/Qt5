// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0
import QtQml 2.14

Rectangle {
    height: 400
    width: 400

    Rectangle {
        property bool when: true

        id: myItem
        objectName:  "myItem"
        height: 300
        width: 200
    }

    property var boundValue: 100

    Binding {
        restoreMode: Binding.RestoreValue
        objectName: "theBinding"
        target: myItem
        property: "height"
        when: myItem.when
        value: boundValue
    }
}
