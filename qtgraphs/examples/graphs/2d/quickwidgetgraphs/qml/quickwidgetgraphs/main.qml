// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtGraphs

//! [0]
Item {
    id: mainView
    width: 1280
    height: 720

    GraphsView {
        id: graphsView
        anchors.fill: parent

        theme: GraphsTheme {
            id: graphsTheme
            theme: GraphsTheme.Theme.BlueSeries
            labelBorderVisible: true
            labelBackgroundVisible: true
            backgroundColor: "black"
        }
        seriesList: pieGraph.pieSeries
    }
}
//! [0]
