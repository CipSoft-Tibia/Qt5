// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T
import QtQuick.Controls.Universal
import QtQuick.Controls.Universal.impl

T.HorizontalHeaderView {
    id: control

    implicitWidth: syncView ? syncView.width : 0
    // The contentHeight of TableView will be zero at start-up, until the delegate
    // items have been loaded. This means that even if the implicit height of
    // HorizontalHeaderView should be the same as the content height in the end, we
    // need to ensure that it has at least a height of 1 at start-up, otherwise
    // TableView won't bother loading any delegates at all.
    implicitHeight: Math.max(1, contentHeight)

    delegate: Rectangle {
        id: delegate

        required property var model

        // Qt6: add cellPadding (and font etc) as public API in headerview
        readonly property real cellPadding: 8

        implicitWidth: text.implicitWidth + (cellPadding * 2)
        implicitHeight: Math.max(control.height, text.implicitHeight + (cellPadding * 2))
        color: control.Universal.background

        Label {
            id: text
            text: delegate.model[control.textRole]
            width: delegate.width
            height: delegate.height
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            color: Color.transparent(control.Universal.foreground, enabled ? 1.0 : 0.2)
        }
    }
}
