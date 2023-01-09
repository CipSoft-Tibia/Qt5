/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWebEngine module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
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
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.1
import QtWebEngine 1.3
import Qt.labs.settings 1.0

ApplicationWindow {
    width: 1300
    height: 900
    visible: true

    Item {
        id: bookmarkUrls

        property url multiTest: Qt.resolvedUrl("qrc:/test/favicon-multi-gray.html")
        property url candidatesTest: Qt.resolvedUrl("qrc:/test/favicon-candidates-gray.html")
        property url aboutBlank: Qt.resolvedUrl("about:blank")
        property url qtHome: Qt.resolvedUrl("http://www.qt.io/")
    }

    Settings {
        id: appSettings

        property alias autoLoadIconsForPage: autoLoadIconsForPage.checked
        property alias touchIconsEnabled: touchIconsEnabled.checked
    }

    SplitView {
        anchors.fill: parent
        orientation: Qt.Vertical

        FaviconPanel {
            id: faviconPanel

            Layout.fillWidth: true
            Layout.minimumHeight: 200

            Layout.margins: 2

            iconUrl: webEngineView && webEngineView.icon
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: 2

            AddressBar {
                id: addressBar

                Layout.fillWidth: true
                Layout.leftMargin: 5
                Layout.rightMargin: 5
                height: 25

                color: "white"
                radius: 4

                progress: webEngineView && webEngineView.loadProgress
                iconUrl: webEngineView && webEngineView.icon
                pageUrl: webEngineView && webEngineView.url

                onAccepted: webEngineView.url = addressUrl
            }

            Rectangle {
                id: toolBar

                Layout.fillWidth: true
                Layout.leftMargin: 5
                Layout.rightMargin: 5
                Layout.preferredHeight: 25

                RowLayout {
                    anchors.verticalCenter: parent.verticalCenter

                    Button {
                        text: "Multi-sized Favicon Test"
                        onClicked: webEngineView.url = bookmarkUrls.multiTest
                        enabled: webEngineView.url != bookmarkUrls.multiTest
                    }

                    Button {
                        text: "Candidate Favicons Test"
                        onClicked: webEngineView.url = bookmarkUrls.candidatesTest
                        enabled: webEngineView.url != bookmarkUrls.candidatesTest
                    }

                    Button {
                        text: "About Blank"
                        onClicked: webEngineView.url = bookmarkUrls.aboutBlank
                        enabled: webEngineView.url != bookmarkUrls.aboutBlank
                    }

                    Button {
                        text: "Qt Home Page"
                        onClicked: webEngineView.url = bookmarkUrls.qtHome
                        enabled: webEngineView.url != bookmarkUrls.qtHome
                    }
                }

                ToolButton {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right

                    menu: Menu {
                        MenuItem {
                            id: autoLoadIconsForPage
                            text: "Icons On"
                            checkable: true
                            checked: WebEngine.settings.autoLoadIconsForPage

                            onCheckedChanged: webEngineView.reload()
                        }

                        MenuItem {
                            id: touchIconsEnabled
                            text: "Touch Icons On"
                            checkable: true
                            checked: WebEngine.settings.touchIconsEnabled
                            enabled: autoLoadIconsForPage.checked

                            onCheckedChanged: webEngineView.reload()
                        }
                    }
                }
            }

            WebEngineView {
                id: webEngineView

                Layout.fillWidth: true
                Layout.fillHeight: true

                settings.autoLoadIconsForPage: appSettings.autoLoadIconsForPage
                settings.touchIconsEnabled: appSettings.touchIconsEnabled

                Component.onCompleted: webEngineView.url = bookmarkUrls.multiTest
            }
        }
    }
}
