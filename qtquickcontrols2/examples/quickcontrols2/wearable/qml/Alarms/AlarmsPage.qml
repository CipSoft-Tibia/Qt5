/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.12
import QtQuick.Controls 2.3 as QQC2
import ".."
import "../Style"

Item {
    QQC2.SwipeView {
        id: svAlarmsContainer

        anchors.fill: parent

        Repeater {
            model: ListModel {
                ListElement { name: qsTr("Week Days"); state: true; time: "06:00 AM" }
                ListElement { name: qsTr("Week Ends"); state: false; time: "07:30 AM" }
            }

            SwipeViewPage {
                property alias stateSwitch: stateSwitch

                Column {
                    spacing: 30
                    anchors.centerIn: parent

                    QQC2.Switch {
                        id: stateSwitch
                        checked: model.state
                        anchors.left: nameLabel.right
                    }

                    Text {
                        text: model.time
                        anchors.horizontalCenter: parent.horizontalCenter
                        verticalAlignment: Text.AlignVCenter
                        height: UIStyle.fontSizeXL
                        font.bold: stateSwitch.checked
                        font.pixelSize: stateSwitch.checked ? UIStyle.fontSizeXL : UIStyle.fontSizeL
                        font.letterSpacing: 4
                        color: UIStyle.themeColorQtGray1
                    }

                    Text {
                        id: nameLabel
                        text: model.name
                        anchors.horizontalCenter: parent.horizontalCenter
                        font.pixelSize: UIStyle.fontSizeS
                        font.italic: true
                        font.bold: true
                        font.letterSpacing: 1
                        color: UIStyle.themeColorQtGray2
                    }
                }
            }
        }
    }

    QQC2.PageIndicator {
        count: svAlarmsContainer.count
        currentIndex: svAlarmsContainer.currentIndex

        anchors.bottom: svAlarmsContainer.bottom
        anchors.horizontalCenter: parent.horizontalCenter
    }
}
