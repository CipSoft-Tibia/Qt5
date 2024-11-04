// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQuick
import QtGraphs
import QtTest

Item {
    id: top
    height: 150
    width: 150

    GraphsView {
        id: initial
    }

    GraphTheme {
        id: myTheme
        colorTheme: GraphTheme.ColorTheme.Light
        shadowEnabled: true
        gridSmoothing: 5
    }

    GraphTheme {
        id: newTheme
    }

    GraphsView {
        id: initialized
        height: top.height
        width: top.width
        theme: myTheme
        backgroundColor: "#fafade"
        marginBottom: 5
        marginLeft: 10
        marginRight: 10
        marginTop: 5

        BarSeries {
            id: barInitial
        }

        LineSeries {
            id: lineInitial
        }

        AreaSeries {
            id: areaInitial
        }
    }

    TestCase {
        name: "GraphsView Initial"

        function test_1_initial() {
            compare(initial.height, 0)
            compare(initial.width, 0)
            compare(initial.backgroundColor, "#000000")
            compare(initial.marginTop, 20)
            compare(initial.marginBottom, 20)
            compare(initial.marginLeft, 20)
            compare(initial.marginRight, 20)
            compare(initial.seriesList, [])
            // compare some of the contents of the initial theme, as theme itself cannot be
            compare(initial.theme.colorTheme, GraphTheme.ColorTheme.Dark)
            compare(initial.theme.shadowEnabled, false)
            compare(initial.theme.gridSmoothing, 1)
        }

        function test_1_initial_change() {
            initial.height = 100
            initial.width = 100
            initial.backgroundColor = "#222222"
            initial.marginTop = 10
            initial.marginBottom = 11
            initial.marginLeft = 12
            initial.marginRight = 13
            initial.theme = myTheme
            initial.addSeries(barInitial)

            waitForRendering(top)

            compare(initial.height, 100)
            compare(initial.width, 100)
            compare(initial.backgroundColor, "#222222")
            compare(initial.marginTop, 10)
            compare(initial.marginBottom, 11)
            compare(initial.marginLeft, 12)
            compare(initial.marginRight, 13)
            compare(initial.seriesList, [barInitial])
            compare(initial.theme, myTheme)
        }
    }

    TestCase {
        name: "GraphsView Initialized"

        function test_1_initialized() {
            compare(initialized.height, top.height)
            compare(initialized.width, top.width)
            compare(initialized.backgroundColor, "#fafade")
            compare(initialized.marginTop, 5)
            compare(initialized.marginBottom, 5)
            compare(initialized.marginLeft, 10)
            compare(initialized.marginRight, 10)
            compare(initialized.seriesList, [barInitial, lineInitial, areaInitial])
            compare(initialized.theme, myTheme)
        }

        function test_2_initialized_change() {
            initialized.height = 100
            initialized.width = 100
            initialized.backgroundColor = "#222222"
            initialized.marginTop = 10
            initialized.marginBottom = 11
            initialized.marginLeft = 12
            initialized.marginRight = 13
            initialized.theme = newTheme
            initialized.removeSeries(barInitial)

            waitForRendering(top)

            compare(initialized.height, 100)
            compare(initialized.width, 100)
            compare(initialized.backgroundColor, "#222222")
            compare(initialized.marginTop, 10)
            compare(initialized.marginBottom, 11)
            compare(initialized.marginLeft, 12)
            compare(initialized.marginRight, 13)
            compare(initialized.seriesList, [lineInitial, areaInitial])
            compare(initialized.theme, newTheme)
        }

        function test_3_initialized_change_to_invalid() {
            // TODO: Crashes - QTBUG-124503
            // initialized.theme = null
            initialized.addSeries(null)
            initialized.addSeries(myTheme)

            waitForRendering(top)

            compare(initialized.seriesList, [lineInitial, areaInitial])
            compare(initialized.theme, newTheme)
        }
    }
}
