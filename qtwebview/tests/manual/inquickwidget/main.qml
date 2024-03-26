// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.0
import QtWebView 1.1

Rectangle {
    anchors.fill: parent
    color: "green"

    WebView {
       anchors.fill: parent
       url: "https://qt.io"
   }
}
