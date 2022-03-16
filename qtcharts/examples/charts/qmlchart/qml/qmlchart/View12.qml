/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Charts module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0
import QtCharts 2.0

Item {
    anchors.fill: parent

    //![1]
    ChartView {
        id: chart
        title: "Production costs"
        anchors.fill: parent
        legend.visible: false
        antialiasing: true

        PieSeries {
            id: pieOuter
            size: 0.96
            holeSize: 0.7
            PieSlice { id: slice; label: "Alpha"; value: 19511; color: "#99CA53" }
            PieSlice { label: "Epsilon"; value: 11105; color: "#209FDF" }
            PieSlice { label: "Psi"; value: 9352; color: "#F6A625" }
        }

        PieSeries {
            size: 0.7
            id: pieInner
            holeSize: 0.25

            PieSlice { label: "Materials"; value: 10334; color: "#B9DB8A" }
            PieSlice { label: "Employee"; value: 3066; color: "#DCEDC4" }
            PieSlice { label: "Logistics"; value: 6111; color: "#F3F9EB" }

            PieSlice { label: "Materials"; value: 7371; color: "#63BCE9" }
            PieSlice { label: "Employee"; value: 2443; color: "#A6D9F2" }
            PieSlice { label: "Logistics"; value: 1291; color: "#E9F5FC" }

            PieSlice { label: "Materials"; value: 4022; color: "#F9C36C" }
            PieSlice { label: "Employee"; value: 3998; color: "#FCE1B6" }
            PieSlice { label: "Logistics"; value: 1332; color: "#FEF5E7" }
        }
    }

    Component.onCompleted: {
        // Set the common slice properties dynamically for convenience
        for (var i = 0; i < pieOuter.count; i++) {
            pieOuter.at(i).labelPosition = PieSlice.LabelOutside;
            pieOuter.at(i).labelVisible = true;
            pieOuter.at(i).borderWidth = 3;
        }
        for (var i = 0; i < pieInner.count; i++) {
            pieInner.at(i).labelPosition = PieSlice.LabelInsideNormal;
            pieInner.at(i).labelVisible = true;
            pieInner.at(i).borderWidth = 2;
        }
    }
    //![1]
}
