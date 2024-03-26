// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0
import QtCharts 2.0

ChartView {
    title: "Bar series"
    anchors.fill: parent
    theme: ChartView.ChartThemeLight
    legend.alignment: Qt.AlignBottom
    animationOptions: ChartView.SeriesAnimations

    property variant series: mySeries

    HorizontalBarSeries {
        id: mySeries
        name: "bar"
        labelsFormat: "@value";
        axisY: BarCategoryAxis { categories: ["2007", "2008", "2009", "2010", "2011", "2012" ] }
        BarSet { label: "Bob"; values: [2, 2, 3, 4, 5, 6]
            onClicked:                  console.log("barset.onClicked: " + index);
            onHovered:                  console.log("barset.onHovered: " + status + " " + index);
            onPenChanged:               console.log("barset.onPenChanged: " + pen);
            onBrushChanged:             console.log("barset.onBrushChanged: " + brush);
            onLabelChanged:             console.log("barset.onLabelChanged: " + label);
            onLabelBrushChanged:        console.log("barset.onLabelBrushChanged: " + labelBrush);
            onLabelFontChanged:         console.log("barset.onLabelFontChanged: " + labelFont);
            onColorChanged:             console.log("barset.onColorChanged: " + color);
            onBorderColorChanged:       console.log("barset.onBorderColorChanged: " + color);
            onLabelColorChanged:        console.log("barset.onLabelColorChanged: " + color);
            onCountChanged:             console.log("barset.onCountChanged: " + count);
            onValuesAdded:              console.log("barset.onValuesAdded: " + index + ", " + count);
            onValuesRemoved:            console.log("barset.onValuesRemoved: " + index + ", " + count);
            onValueChanged:             console.log("barset.onValuesChanged: " + index);
            onPressed:                  console.log("barset.onPressed: " + index);
            onReleased:                 console.log("barset.onReleased: " + index);
            onDoubleClicked:            console.log("barset.onDoubleClicked: " + index);
        }
        BarSet { label: "Susan"; values: [5, 1, 2, 4, 1, 7] }
        BarSet { label: "James"; values: [3, 5, 8, 13, 5, 8] }

        onNameChanged:              console.log("horizontalBarSeries.onNameChanged: " + series.name);
        onVisibleChanged:           console.log("horizontalBarSeries.onVisibleChanged: " + series.visible);
        onOpacityChanged:           console.log("horizontalBarSeries.onOpacityChanged: " + opacity);
        onClicked:                  console.log("horizontalBarSeries.onClicked: " + barset + " " + index);
        onHovered:                  console.log("horizontalBarSeries.onHovered: " + barset + " "
                                                + status + " " + index);
        onLabelsVisibleChanged:     console.log("horizontalBarSeries.onLabelsVisibleChanged: " + series.labelsVisible);
        onCountChanged:             console.log("horizontalBarSeries.onCountChanged: " + count);
        onLabelsFormatChanged:      console.log("horizontalBarSeries.onLabelsFormatChanged: "
                                                + format);
        onLabelsPositionChanged:    console.log("horizontalBarSeries.onLabelsPositionChanged: "
                                                + series.labelsPosition);
        onLabelsPrecisionChanged:   console.log(
                                        "horizontalBarSeries.onLabelsPrecisionChanged: "
                                        + series.labelsPrecision);
        onPressed:          console.log("horizontalBarSeries.onPressed: " + barset + " " + index);
        onReleased:         console.log("horizontalBarSeries.onReleased: " + barset + " " + index);
        onDoubleClicked:    console.log("horizontalBarSeries.onDoubleClicked: " + barset + " " + index);

        function changeLabelsPosition() {
            if (labelsPosition === BarSeries.LabelsCenter)
                labelsPosition = BarSeries.LabelsInsideEnd;
            else
                labelsPosition = BarSeries.LabelsCenter;
        }
    }
}
