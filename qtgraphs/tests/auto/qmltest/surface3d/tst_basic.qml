// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0
import QtGraphs
import QtTest 1.0

Item {
    id: top
    height: 150
    width: 150

    property var empty: null
    property var basic: null
    property var common: null
    property var common_init: null

    function constructEmpty() {
        empty = Qt.createQmlObject("
        import QtQuick 2.2
        import QtGraphs
        Surface3D {
        }", top)
    }

    function constructBasic() {
        basic = Qt.createQmlObject("
        import QtQuick 2.2
        import QtGraphs
        Surface3D {
            anchors.fill: parent
            flipHorizontalGrid: true
        }", top)
        basic.anchors.fill = top
    }

    function constructCommon() {
        common = Qt.createQmlObject("
        import QtQuick 2.2
        import QtGraphs
        Surface3D {
            anchors.fill: parent
        }", top)
        common.anchors.fill = top
    }

    function constructCommonInit() {
        common_init = Qt.createQmlObject("
        import QtQuick 2.2
        import QtGraphs
        Surface3D {
            anchors.fill: parent
            selectionMode: AbstractGraph3D.SelectionNone
            shadowQuality: AbstractGraph3D.ShadowQuality.Low
            msaaSamples: 2
            theme: Theme3D { }
            renderingMode: AbstractGraph3D.RenderingMode.Indirect
            measureFps: true
            orthoProjection: false
            aspectRatio: 3.0
            optimizationHint: AbstractGraph3D.OptimizationHint.Default
            polar: false
            radialLabelOffset: 2
            horizontalAspectRatio: 0.2
            locale: Qt.locale(\"UK\")
            margin: 0.2
        }", top)
        common_init.anchors.fill = top
    }

    TestCase {
        name: "Surface3D Empty"
        when: windowShown

        function test_empty() {
            constructEmpty()
            compare(empty.width, 0, "width")
            compare(empty.height, 0, "height")
            compare(empty.seriesList.length, 0, "seriesList")
            compare(empty.selectedSeries, null, "selectedSeries")
            compare(empty.flipHorizontalGrid, false, "flipHorizontalGrid")
            compare(empty.axisX.orientation, AbstractAxis3D.AxisOrientation.X)
            compare(empty.axisZ.orientation, AbstractAxis3D.AxisOrientation.Z)
            compare(empty.axisY.orientation, AbstractAxis3D.AxisOrientation.Y)
            compare(empty.axisX.type, AbstractAxis3D.AxisType.Value)
            compare(empty.axisZ.type, AbstractAxis3D.AxisType.Value)
            compare(empty.axisY.type, AbstractAxis3D.AxisType.Value)
            waitForRendering(top)
            empty.destroy()
            waitForRendering(top)
        }
    }

    TestCase {
        name: "Surface3D Basic"
        when: windowShown

        function test_1_basic() {
            constructBasic()
            compare(basic.width, 150, "width")
            compare(basic.height, 150, "height")
            compare(basic.flipHorizontalGrid, true, "flipHorizontalGrid")
        }

        function test_2_change_basic() {
            basic.flipHorizontalGrid = false
            compare(basic.flipHorizontalGrid, false, "flipHorizontalGrid")
            waitForRendering(top)
            basic.destroy()
            waitForRendering(top)
        }
    }

    TestCase {
        name: "Surface3D Common"
        when: windowShown

        function test_1_common() {
            constructCommon()
            compare(common.selectionMode, AbstractGraph3D.SelectionItem, "selectionMode")
            compare(common.shadowQuality, AbstractGraph3D.ShadowQuality.Medium, "shadowQuality")
            compare(common.msaaSamples, 4, "msaaSamples")
            compare(common.theme.type, Theme3D.Theme.Qt, "theme")
            compare(common.renderingMode, AbstractGraph3D.RenderingMode.Indirect, "renderingMode")
            compare(common.measureFps, false, "measureFps")
            compare(common.customItemList.length, 0, "customItemList")
            compare(common.orthoProjection, false, "orthoProjection")
            compare(common.selectedElement, AbstractGraph3D.ElementType.None, "selectedElement")
            compare(common.aspectRatio, 2.0, "aspectRatio")
            compare(common.optimizationHint, AbstractGraph3D.OptimizationHint.Default, "optimizationHint")
            compare(common.polar, false, "polar")
            compare(common.radialLabelOffset, 1, "radialLabelOffset")
            compare(common.horizontalAspectRatio, 0, "horizontalAspectRatio")
            compare(common.locale, Qt.locale("C"), "locale")
            compare(common.queriedGraphPosition, Qt.vector3d(0, 0, 0), "queriedGraphPosition")
            compare(common.margin, -1, "margin")

            common.destroy()
        }

        function test_2_change_common() {
            constructCommon()
            common.selectionMode = AbstractGraph3D.SelectionItem | AbstractGraph3D.SelectionRow | AbstractGraph3D.SelectionSlice
            common.shadowQuality = AbstractGraph3D.ShadowQuality.SoftHigh
            compare(common.shadowQuality, AbstractGraph3D.ShadowQuality.SoftHigh, "shadowQuality")
            common.msaaSamples = 8
            compare(common.msaaSamples, 8, "msaaSamples")
            common.theme.type = Theme3D.Theme.Retro
            // TODO: Seems to be causing crashes in testing - QTBUG-122089
            // common.renderingMode = AbstractGraph3D.RenderingMode.DirectToBackground
            common.measureFps = true
            common.orthoProjection = true
            common.aspectRatio = 1.0
            common.optimizationHint = AbstractGraph3D.OptimizationHint.Default
            common.polar = true
            common.radialLabelOffset = 2
            common.horizontalAspectRatio = 1
            common.locale = Qt.locale("FI")
            common.margin = 1.0
            compare(common.selectionMode, AbstractGraph3D.SelectionItem | AbstractGraph3D.SelectionRow | AbstractGraph3D.SelectionSlice, "selectionMode")
            compare(common.shadowQuality, AbstractGraph3D.ShadowQuality.None, "shadowQuality") // Ortho disables shadows
            // TODO: Seems to be causing crashes in testing - QTBUG-122089
            // compare(common.msaaSamples, 0, "msaaSamples") // Rendering mode changes this to zero
            compare(common.theme.type, Theme3D.Theme.Retro, "theme")
            // TODO: Seems to be causing crashes in testing - QTBUG-122089
            // compare(common.renderingMode, AbstractGraph3D.RenderingMode.DirectToBackground, "renderingMode")
            compare(common.measureFps, true, "measureFps")
            compare(common.orthoProjection, true, "orthoProjection")
            compare(common.aspectRatio, 1.0, "aspectRatio")
            compare(common.optimizationHint, AbstractGraph3D.OptimizationHint.Default, "optimizationHint")
            compare(common.polar, true, "polar")
            compare(common.radialLabelOffset, 2, "radialLabelOffset")
            compare(common.horizontalAspectRatio, 1, "horizontalAspectRatio")
            compare(common.locale, Qt.locale("FI"), "locale")
            compare(common.margin, 1.0, "margin")

            common.destroy()
        }

        function test_3_change_invalid_common() {
            constructCommon()
            common.selectionMode = AbstractGraph3D.SelectionRow | AbstractGraph3D.SelectionColumn | AbstractGraph3D.SelectionSlice
            common.theme.type = -2
            common.renderingMode = -1
            common.measureFps = false
            common.orthoProjection = false
            common.aspectRatio = -1.0
            common.polar = false
            common.horizontalAspectRatio = -2
            compare(common.selectionMode, AbstractGraph3D.SelectionItem, "selectionMode")
            compare(common.theme.type, -2/*Theme3D.Theme.Retro*/, "theme") // TODO: Fix once QTRD-3367 is done
            compare(common.renderingMode, -1/*AbstractGraph3D.RenderingMode.DirectToBackground*/, "renderingMode") // TODO: Fix once QTRD-3367 is done
            compare(common.aspectRatio, -1.0/*1.0*/, "aspectRatio") // TODO: Fix once QTRD-3367 is done
            compare(common.horizontalAspectRatio, -2/*1*/, "horizontalAspectRatio") // TODO: Fix once QTRD-3367 is done

            common.destroy()
        }

        function test_4_common_initialized() {
            constructCommonInit()

            compare(common_init.selectionMode, AbstractGraph3D.SelectionNone, "selectionMode")
            tryCompare(common_init, "shadowQuality", AbstractGraph3D.ShadowQuality.Low)
            compare(common_init.msaaSamples, 2, "msaaSamples")
            compare(common_init.theme.type, Theme3D.Theme.UserDefined, "theme")
            compare(common_init.renderingMode, AbstractGraph3D.RenderingMode.Indirect, "renderingMode")
            compare(common_init.measureFps, true, "measureFps")
            compare(common_init.customItemList.length, 0, "customItemList")
            compare(common_init.orthoProjection, false, "orthoProjection")
            compare(common_init.aspectRatio, 3.0, "aspectRatio")
            compare(common_init.optimizationHint, AbstractGraph3D.OptimizationHint.Default, "optimizationHint")
            compare(common_init.polar, false, "polar")
            compare(common_init.radialLabelOffset, 2, "radialLabelOffset")
            compare(common_init.horizontalAspectRatio, 0.2, "horizontalAspectRatio")
            compare(common_init.locale, Qt.locale("UK"), "locale")
            compare(common_init.margin, 0.2, "margin")

            common_init.destroy();
        }
    }
}
