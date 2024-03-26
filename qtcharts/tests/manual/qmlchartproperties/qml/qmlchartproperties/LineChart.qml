// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0
import QtCharts 2.0

ChartView {
    property variant series: lineSeries

    title: "line series"
    anchors.fill: parent
    animationOptions: ChartView.SeriesAnimations

    LineSeries {
        id: lineSeries
        name: "line 1"
        XYPoint { x: 0; y: 0 }
        XYPoint { x: 1.1; y: 2.1 }
        XYPoint { x: 1.9; y: 3.3 }
        XYPoint { x: 2.1; y: 2.1 }
        XYPoint { x: 2.9; y: 4.9 }
        XYPoint { x: 3.4; y: 3.0 }
        XYPoint { x: 4.1; y: 3.3 }

        pointLabelsFormat: "@xPoint, @yPoint";

        onNameChanged:              console.log("lineSeries.onNameChanged: " + name);
        onVisibleChanged:           console.log("lineSeries.onVisibleChanged: " + visible);
        onOpacityChanged:           console.log(name + ".onOpacityChanged: " + opacity);
        onClicked:                  console.log(name + ".onClicked: " + point.x + ", " + point.y);
        onPointReplaced:            console.log("lineSeries.onPointReplaced: " + index);
        onPointRemoved:             console.log("lineSeries.onPointRemoved: " + index);
        onPointAdded:               console.log("lineSeries.onPointAdded: " + series.at(index).x + ", " + series.at(index).y);
        onColorChanged:             console.log("lineSeries.onColorChanged: " + color);
        onWidthChanged:             console.log("lineSeries.onWidthChanged: " + width);
        onStyleChanged:             console.log("lineSeries.onStyleChanged: " + style);
        onCapStyleChanged:          console.log("lineSeries.onCapStyleChanged: " + capStyle);
        onCountChanged:             console.log("lineSeries.onCountChanged: " + count);
        onHovered:                  console.log(name + ".onHovered:" + point.x + "," + point.y + " " + state);
        onPointLabelsVisibilityChanged:  console.log("lineSeries.onPointLabelsVisibilityChanged: "
                                                     + visible);
        onPointLabelsFormatChanged:      console.log("lineSeries.onPointLabelsFormatChanged: "
                                                     + format);
        onPointLabelsFontChanged:        console.log("lineSeries.onPointLabelsFontChanged: "
                                                     + font.family);
        onPointLabelsColorChanged:       console.log("lineSeries.onPointLabelsColorChanged: "
                                                     + color);
        onPointLabelsClippingChanged:    console.log("lineSeries.onPointLabelsClippingChanged: "
                                                     + clipping);
        onPressed:          console.log(name + ".onPressed: " + point.x + ", " + point.y);
        onReleased:         console.log(name + ".onReleased: " + point.x + ", " + point.y);
        onDoubleClicked:    console.log(name + ".onDoubleClicked: " + point.x + ", " + point.y);
    }

    LineSeries {
        name: "line 2"
        XYPoint { x: 1.1; y: 1.1 }
        XYPoint { x: 1.9; y: 2.3 }
        XYPoint { x: 2.1; y: 1.1 }
        XYPoint { x: 2.9; y: 3.9 }
        XYPoint { x: 3.4; y: 2.0 }
        XYPoint { x: 4.1; y: 2.3 }
        onClicked:                  console.log(name + ".onClicked: " + point.x + ", " + point.y);
        onHovered:                  console.log(name + ".onHovered: " + point.x + ", " + point.y);
        onPressed:          console.log(name + ".onPressed: " + point.x + ", " + point.y);
        onReleased:         console.log(name + ".onReleased: " + point.x + ", " + point.y);
        onDoubleClicked:    console.log(name + ".onDoubleClicked: " + point.x + ", " + point.y);
    }
}
