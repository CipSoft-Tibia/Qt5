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
        id: svSettingsContainer

        anchors.fill: parent

        SwipeViewPage {
            id: settingsPage1

            property alias bluetoothSwitch: bluetoothSwitch
            property alias wirelessSwitch: wirelessSwitch

            Column {
                anchors.centerIn: parent
                spacing: 25

                Row {
                    spacing: 50
                    Image {
                        anchors.verticalCenter: parent.verticalCenter
                        source: UIStyle.themeImagePath("images/bluetooth")
                    }
                    QQC2.Switch {
                        id: bluetoothSwitch
                        anchors.verticalCenter: parent.verticalCenter
                        checked: settings.bluetooth
                        onToggled: settings.bluetooth = checked
                    }
                }
                Row {
                    spacing: 50
                    Image {
                        anchors.verticalCenter: parent.verticalCenter
                        source: UIStyle.themeImagePath("images/wifi")
                    }
                    QQC2.Switch {
                        id: wirelessSwitch
                        anchors.verticalCenter: parent.verticalCenter
                        checked: settings.wireless
                        onToggled: settings.wireless = checked
                    }
                }
            }
        }

        SwipeViewPage {
            id: settingsPage2

            property alias brightnessSlider: brightnessSlider
            property alias darkThemeSwitch: darkThemeSwitch

            Column {
                anchors.centerIn: parent
                spacing: 2

                Column {
                    Image {
                        anchors.horizontalCenter: parent.horizontalCenter
                        source: UIStyle.themeImagePath("images/brightness")
                    }
                    QQC2.Slider {
                        id: brightnessSlider
                        anchors.horizontalCenter: parent.horizontalCenter
                        from: 0
                        to: 5
                        stepSize: 1
                        value: settings.brightness
                        onMoved: settings.brightness = value
                    }
                }
                Column {
                    anchors.horizontalCenter: parent.horizontalCenter

                    Image {
                        anchors.horizontalCenter: parent.horizontalCenter
                        source: UIStyle.themeImagePath("images/theme")
                    }
                    QQC2.Switch {
                        id: darkThemeSwitch
                        anchors.horizontalCenter: parent.horizontalCenter
                        checked: settings.darkTheme
                        onToggled: settings.darkTheme = checked
                    }
                }
            }
        }

        SwipeViewPage {
            id: settingsPage3

            Column {
                anchors.centerIn: parent

                Column {
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 6

                    Image {
                        anchors.horizontalCenter: parent.horizontalCenter
                        source: UIStyle.themeImagePath("images/demo-mode")
                    }
                    QQC2.Switch {
                        id: demoModeSwitch
                        anchors.horizontalCenter: parent.horizontalCenter
                        checked: settings.demoMode
                        onToggled: settings.demoMode = checked
                    }
                }
            }
        }
    }

    QQC2.PageIndicator {
        count: svSettingsContainer.count
        currentIndex: svSettingsContainer.currentIndex

        anchors.bottom: svSettingsContainer.bottom
        anchors.horizontalCenter: parent.horizontalCenter
    }
}
