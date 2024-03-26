// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtLocation

Item {
    id: testIcon
    property icon icon: ({ parameters: { singleUrl: "http://www.example.com/test-icon.png" }})
}
