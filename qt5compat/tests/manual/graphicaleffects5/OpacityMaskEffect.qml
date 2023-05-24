// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import Qt5Compat.GraphicalEffects

Item {
    OpacityMask {
        anchors.fill: parent
        source: bug
        maskSource: butterfly
    }
}
