// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import Qt5Compat.GraphicalEffects

Item {
    GaussianBlur {
        anchors.fill: parent
        source: bug
        radius: 8
        samples: 16
    }
}
