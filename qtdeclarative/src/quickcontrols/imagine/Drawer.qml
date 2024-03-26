// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.Imagine
import QtQuick.Controls.Imagine.impl

T.Drawer {
    id: control

    parent: T.Overlay.overlay

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding)

    topPadding: background ? background.topPadding : 0
    leftPadding: background ? background.leftPadding : 0
    rightPadding: background ? background.rightPadding : 0
    bottomPadding: background ? background.bottomPadding : 0

    topInset: background ? -background.topInset || 0 : 0
    leftInset: background ? -background.leftInset || 0 : 0
    rightInset: background ? -background.rightInset || 0 : 0
    bottomInset: background ? -background.bottomInset || 0 : 0

    enter: Transition { SmoothedAnimation { velocity: 5 } }
    exit: Transition { SmoothedAnimation { velocity: 5 } }

    background: NinePatchImage {
        source: Imagine.url + "drawer-background"
        NinePatchImageSelector on source {
            states: [
                {"modal": control.modal},
                {"dim": control.dim},
                {"top": control.edge === Qt.TopEdge},
                {"left": control.edge === Qt.LeftEdge},
                {"right": control.edge === Qt.RightEdge},
                {"bottom": control.edge === Qt.BottomEdge}
            ]
        }
    }

    T.Overlay.modal: NinePatchImage {
        source: Imagine.url + "drawer-overlay"
        NinePatchImageSelector on source {
            states: [
                {"modal": true}
            ]
        }
    }

    T.Overlay.modeless: NinePatchImage {
        source: Imagine.url + "drawer-overlay"
        NinePatchImageSelector on source {
            states: [
                {"modal": false}
            ]
        }
    }
}
