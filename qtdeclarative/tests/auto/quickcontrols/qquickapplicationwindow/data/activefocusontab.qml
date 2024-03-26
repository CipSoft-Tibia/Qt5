// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    title: "Test Application Window"
    width: 100
    height: 100
    Item {
        id: main
        objectName: "main"
        width: 100
        height: 100
        //focus: true
        Column {
            anchors.fill: parent
            id: column
            objectName: "column"
            Item {
                id: sub1
                objectName: "sub1"
                activeFocusOnTab: true
                Accessible.role: Accessible.Table
                width: 100
                height: 50
                Rectangle {
                    anchors.fill: parent
                    color: parent.activeFocus ? "red" : "black"
                }
            }
            Item {
                id: sub2
                objectName: "sub2"
                activeFocusOnTab: true
                Accessible.role: Accessible.Table
                width: 100
                height: 50
                Rectangle {
                    anchors.fill: parent
                    color: parent.activeFocus ? "red" : "black"
                }
            }
        }
    }
}
