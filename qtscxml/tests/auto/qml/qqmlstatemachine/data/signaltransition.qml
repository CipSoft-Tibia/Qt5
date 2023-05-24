// Copyright (C) 2021 The Qt Company
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQml.StateMachine

Item {
    id: root

    signal signal1()
    signal signal2()

    function getSignal1() { return root.signal1 }
    function getSignal2() { return root.signal2 }

    SignalTransition {
        objectName: "st1"
        guard: 1 + 1
    }

    SignalTransition {
        objectName: "st2"
        guard: 2 + 2
    }

    SignalTransition {
        // Do not crash on SignalTransition without signal
        onTriggered: () => {}
    }
}
