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
        Bars3D {
        }", top)
    }

    function constructBasic() {
        basic = Qt.createQmlObject("
        import QtQuick 2.2
        import QtGraphs
        Bars3D {
            anchors.fill: parent
            multiSeriesUniform: true
            barThickness: 0.1
            barSpacing.width: 0.1
            barSpacing.height: 0.1
            barSeriesMargin.width: 0.3
            barSeriesMargin.height: 0.3
            barSpacingRelative: false
            floorLevel: 1.0
        }", top)
        basic.anchors.fill = top
    }

    function constructCommon() {
        common = Qt.createQmlObject("
        import QtQuick 2.2
        import QtGraphs
        Bars3D {
            anchors.fill: parent
        }", top)
        common.anchors.fill = top
    }

    function constructCommonInit() {
        common_init = Qt.createQmlObject("
        import QtQuick 2.2
        import QtGraphs
        Bars3D {
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
        name: "Bars3D Empty"
        when: windowShown

        function test_empty() {
            constructEmpty()
            compare(empty.width, 0, "width")
            compare(empty.height, 0, "height")
            compare(empty.multiSeriesUniform, false, "multiSeriesUniform")
            compare(empty.barThickness, 1.0, "barThickness")
            compare(empty.barSpacing, Qt.size(1.0, 1.0), "barSpacing")
            compare(empty.barSeriesMargin, Qt.size(0.0, 0.0), "barSeriesMargin")
            compare(empty.barSpacingRelative, true, "barSpacingRelative")
            compare(empty.seriesList.length, 0, "seriesList")
            compare(empty.selectedSeries, null, "selectedSeries")
            compare(empty.primarySeries, null, "primarySeries")
            compare(empty.floorLevel, 0.0, "floorLevel")
            compare(empty.columnAxis.orientation, AbstractAxis3D.AxisOrientation.X)
            compare(empty.rowAxis.orientation, AbstractAxis3D.AxisOrientation.Z)
            compare(empty.valueAxis.orientation, AbstractAxis3D.AxisOrientation.Y)
            compare(empty.columnAxis.type, AbstractAxis3D.AxisType.Category)
            compare(empty.rowAxis.type, AbstractAxis3D.AxisType.Category)
            compare(empty.valueAxis.type, AbstractAxis3D.AxisType.Value)
            waitForRendering(top)
            empty.destroy()
            waitForRendering(top)
        }
    }

    TestCase {
        name: "Bars3D Basic"
        when: windowShown

        function test_1_basic() {
            constructBasic()
            compare(basic.width, 150, "width")
            compare(basic.height, 150, "height")
            compare(basic.multiSeriesUniform, true, "multiSeriesUniform")
            compare(basic.barThickness, 0.1, "barThickness")
            compare(basic.barSpacing, Qt.size(0.1, 0.1), "barSpacing")
            compare(basic.barSeriesMargin, Qt.size(0.3, 0.3), "barSeriesMargin")
            compare(basic.barSpacingRelative, false, "barSpacingRelative")
            compare(basic.floorLevel, 1.0, "floorLevel")
            waitForRendering(top)
        }

        function test_2_basic_change() {
            basic.multiSeriesUniform = false
            basic.barThickness = 0.5
            basic.barSpacing = Qt.size(1.0, 0.0)
            basic.barSeriesMargin = Qt.size(0.5, 0.0)
            basic.barSpacingRelative = true
            basic.floorLevel = 0.2
            compare(basic.multiSeriesUniform, false, "multiSeriesUniform")
            compare(basic.barThickness, 0.5, "barThickness")
            compare(basic.barSpacing, Qt.size(1.0, 0.0), "barSpacing")
            compare(basic.barSeriesMargin, Qt.size(0.5, 0.0), "barSeriesMargin")
            compare(basic.barSpacingRelative, true, "barSpacingRelative")
            compare(basic.floorLevel, 0.2, "floorLevel")
            waitForRendering(top)
        }

        function test_3_basic_change_invalid() {
            basic.barThickness = -1
            basic.barSpacing = Qt.size(-1.0, -1.0)
            basic.barSeriesMargin = Qt.size(-1.0, -1.0)
            compare(basic.barThickness, -1/*0.5*/, "barThickness") // TODO: Fix once QTRD-3367 is done
            compare(basic.barSpacing, Qt.size(-1.0, -1.0), "barSpacing")
            compare(basic.barSeriesMargin, Qt.size(-1.0, -1.0), "barSeriesMargin")
            waitForRendering(top)
            basic.destroy()
            waitForRendering(top)
        }
    }

    TestCase {
        name: "Bars3D Common"
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
        }

        function test_2_change_common() {
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
        }

        function test_3_change_invalid_common() {
            common.selectionMode = AbstractGraph3D.SelectionRow | AbstractGraph3D.SelectionColumn | AbstractGraph3D.SelectionSlice
            common.theme.type = -2
            common.renderingMode = -1
            common.measureFps = false
            common.orthoProjection = false
            common.aspectRatio = -1.0
            common.polar = false
            common.horizontalAspectRatio = -2
            compare(common.selectionMode, AbstractGraph3D.SelectionItem | AbstractGraph3D.SelectionRow | AbstractGraph3D.SelectionSlice, "selectionMode")
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
