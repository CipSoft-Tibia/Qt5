// Copyright (C) 2023 The Qt Company
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQml.StateMachine

SignalTransition {
    // Do not crash on SignalTransition without signal
    onTriggered: () => {}
}
