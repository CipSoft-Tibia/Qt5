// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtGraphs

Rectangle {
    anchors.fill: parent
    gradient: Gradient {
        GradientStop { position: 0.0; color: "#d0d020" }
        GradientStop { position: 1.0; color: "#406020" }
    }

    GraphsView {
        id: chartView
        anchors.fill: parent
        anchors.margins: 20 * px
        anchors.rightMargin: settingsView.posX + 20 * px
        theme: GraphTheme {
            id: myTheme
            colorTheme: GraphTheme.ColorThemeDark
            axisXLabelsFont.pixelSize: 20
            gridMajorBarsColor: "#ffffff"
            gridMinorBarsColor: "#eeeeee"
            axisYMajorColor: "#ffffff"
            axisYMinorColor: "#eeeeee"
            axisXMajorColor: "#ffffff"
            axisXMinorColor: "#eeeeee"
            shadowEnabled: checkBoxShadowEnabled.checked
            shadowColor: "#80404040"
        }
        SeriesTheme {
            id: customSeriesTheme
            colors: ["#dd444444", "#dd555555", "#dd666666", "#dd777777", "#dd888888"]
            borderColors: ["#111111", "#222222", "#333333", "#444444", "#555555"]
            borderWidth: 2
        }
        BarSeries {
            id: mySeries
            theme: customSeriesTheme
            axisX: BarCategoryAxis {
                id: xAxis
                visible: checkBoxAxisXVisible.checked
                lineVisible: checkBoxAxisXLineVisible.checked
                labelsVisible: checkBoxAxisXLabelsVisible.checked
                categories: [1, 2, 3, 4, 5, 6]
                gridVisible: checkBoxGridXMajor.checked
                minorGridVisible: checkBoxGridXMinor.checked
            }
            axisY: ValueAxis {
                id: yAxis
                visible: checkBoxAxisYVisible.checked
                lineVisible: checkBoxAxisYLineVisible.checked
                labelsVisible: checkBoxAxisYLabelsVisible.checked
                max: 10
                minorTickCount: 1
                // Alternative tick formatting
                //tickInterval: 1.0
                //labelFormat: "g"
                //labelDecimals: 3
                gridVisible: checkBoxGridYMajor.checked
                minorGridVisible: checkBoxGridYMinor.checked
            }

            BarSet { id: set1; label: "Bob"; values: [1, 2, 3, 4, 5, 6] }
            BarSet { id: set2; label: "Frank"; values: [8, 8, 6, 0, 5, 3] }
            BarSet { id: set3; label: "James"; values: [4, 3, 2, 6, 4, 2] }
            BarSet { id: set5; label: "Frank"; values: [8, 4, 3, 1, 8, 5] }
            BarSet { id: set6; label: "James"; values: [5, 2, 5, 7, 1, 2] }

        }
    }

    SettingsView {
        id: settingsView
        CustomLabel {
            text: "Margin: Left"
        }
        CustomSlider {
            sliderValue: chartView.marginLeft
            fromValue: 0
            toValue: 60
            onSliderValueChanged: chartView.marginLeft = sliderValue;
        }
        CustomLabel {
            text: "Margin: Right"
        }
        CustomSlider {
            sliderValue: chartView.marginRight
            fromValue: 0
            toValue: 60
            onSliderValueChanged: chartView.marginRight = sliderValue;
        }
        CustomLabel {
            text: "Margin: Top"
        }
        CustomSlider {
            sliderValue: chartView.marginTop
            fromValue: 0
            toValue: 60
            onSliderValueChanged: chartView.marginTop = sliderValue;
        }
        CustomLabel {
            text: "Margin: Bottom"
        }
        CustomSlider {
            sliderValue: chartView.marginBottom
            fromValue: 0
            toValue: 60
            onSliderValueChanged: chartView.marginBottom = sliderValue;
        }
        CustomLabel {
            text: "Y-coordinate: Max"
        }
        CustomSlider {
            id: sliderYMaxValue
            sliderValue: yAxis.max
            fromValue: 0.1
            toValue: 110
            onSliderValueChanged: yAxis.max = sliderValue;
        }
        CustomLabel {
            text: "Y-coordinate: Min"
        }
        CustomSlider {
            id: sliderYMinValue
            sliderValue: yAxis.min
            fromValue: -3
            toValue: 3
            onSliderValueChanged: yAxis.min = sliderValue;
        }
        CustomLabel {
            text: "Y-coordinate: Anchor"
        }
        CustomSlider {
            id: sliderYAnchorValue
            sliderValue: yAxis.tickAnchor
            fromValue: -10
            toValue: 10
            onSliderValueChanged: yAxis.tickAnchor = sliderValue;
        }
        CustomLabel {
            text: "Y-coordinate: Decimals"
        }
        CustomSlider {
            id: sliderYAnchorDecimals
            sliderValue: yAxis.labelDecimals
            sliderStepSize: 1
            fromValue: -1
            toValue: 4
            onSliderValueChanged: yAxis.labelDecimals = sliderValue;
        }
        CustomCheckBox {
            id: checkBoxAxisXVisible
            text: "Axis X: Visible"
            checked: true
        }
        CustomCheckBox {
            id: checkBoxAxisXLineVisible
            text: "Axis X: Line visible"
            checked: true
        }
        CustomCheckBox {
            id: checkBoxAxisXLabelsVisible
            text: "Axis X: Labels visible"
            checked: true
        }
        CustomCheckBox {
            id: checkBoxAxisYVisible
            text: "Axis Y: Visible"
            checked: true
        }
        CustomCheckBox {
            id: checkBoxAxisYLineVisible
            text: "Axis Y: Line visible"
            checked: true
        }
        CustomCheckBox {
            id: checkBoxAxisYLabelsVisible
            text: "Axis Y: Labels visible"
            checked: true
        }
        CustomCheckBox {
            id: checkBoxGridYMajor
            text: "Grid Y: Major lines"
            checked: true
        }
        CustomCheckBox {
            id: checkBoxGridYMinor
            text: "Grid Y: Minor lines"
            checked: true
        }
        CustomCheckBox {
            id: checkBoxGridXMajor
            text: "Grid X: Major lines"
            checked: false
        }
        CustomCheckBox {
            id: checkBoxGridXMinor
            text: "Grid X: Minor lines"
            checked: false
        }

        CustomLabel {
            text: "Y-coordinate: Minor Ticks"
        }
        CustomSlider {
            id: sliderYTicksValue
            sliderValue: yAxis.minorTickCount
            sliderStepSize: 1
            fromValue: 0
            toValue: 9
            onSliderValueChanged: yAxis.minorTickCount = sliderValue;
        }
        CustomLabel {
            text: "Major bars width"
        }
        CustomSlider {
            sliderValue: myTheme.gridMajorBarsWidth
            fromValue: 1.0
            toValue: 4.0
            onSliderValueChanged: {
                myTheme.gridMajorBarsWidth = sliderValue;
                myTheme.axisYMajorBarWidth = sliderValue;
                myTheme.axisXMajorBarWidth = sliderValue;
            }
        }
        CustomLabel {
            text: "Minor bars width"
        }
        CustomSlider {
            sliderValue: myTheme.gridMinorBarsWidth
            fromValue: 1.0
            toValue: 4.0
            onSliderValueChanged: {
                myTheme.gridMinorBarsWidth = sliderValue;
                myTheme.axisYMinorBarWidth = sliderValue;
                myTheme.axisXMinorBarWidth = sliderValue;
            }
        }
        CustomCheckBox {
            id: checkBoxShadowEnabled
            text: "Shadow: Enabled"
            checked: true
        }
        CustomLabel {
            text: "Shadow: Opacity"
        }
        CustomSlider {
            id: sliderShadowOpacity
            sliderValue: myTheme.shadowColor.a
            fromValue: 0
            toValue: 1
            onSliderValueChanged: myTheme.shadowColor.a = sliderValue;
        }
        CustomLabel {
            text: "Shadow: Smoothing"
        }
        CustomSlider {
            id: sliderShadowSmoothing
            sliderValue: myTheme.shadowSmoothing
            fromValue: 0
            toValue: 10
            onSliderValueChanged: myTheme.shadowSmoothing = sliderValue;
        }
        CustomLabel {
            text: "Shadow: OffsetX"
        }
        CustomSlider {
            id: sliderShadowOffsetX
            sliderValue: myTheme.shadowXOffset
            fromValue: -2
            toValue: 2
            onSliderValueChanged: myTheme.shadowXOffset = sliderValue;
        }
        CustomLabel {
            text: "Shadow: OffsetY"
        }
        CustomSlider {
            id: sliderShadowOffsetY
            sliderValue: myTheme.shadowYOffset
            fromValue: -2
            toValue: 2
            onSliderValueChanged: myTheme.shadowYOffset = sliderValue;
        }
    }
}
