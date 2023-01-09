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

import QtQuick 2.0
import QtTest 1.0
import QtWebEngine 1.5

TestWebEngineView {
    id: webEngineView
    width: 200
    height: 400

    property var newViewRequest: null
    property var dialog: null
    property string viewType: ""
    property var loadRequestArray: []

    onLoadingChanged: {
        loadRequestArray.push({
            "status": loadRequest.status,
        });
    }

    SignalSpy {
        id: newViewRequestedSpy
        target: webEngineView
        signalName: "newViewRequested"
    }

    onNewViewRequested: {
        newViewRequest = {
            "destination": request.destination,
            "userInitiated": request.userInitiated,
            "requestedUrl": request.requestedUrl
        };

        dialog = Qt.createQmlObject(
            "import QtQuick.Window 2.0\n" +
            "Window {\n" +
            "    width: 100; height: 100\n" +
            "    visible: true; flags: Qt.Dialog\n" +
            "    property alias webEngineView: webView\n" +
            "    TestWebEngineView { id: webView; anchors.fill: parent }\n" +
            "}", webEngineView);

        if (viewType === "dialog")
            request.openIn(dialog.webEngineView);
        else if (viewType === "null")
            request.openIn(0);
        else if (viewType === "webEngineView")
            request.openIn(webEngineView);
    }

    TestCase {
        id: testCase
        name: "NewViewRequest"
        when: windowShown

        function init() {
            webEngineView.url = Qt.resolvedUrl("about:blank");
            verify(webEngineView.waitForLoadSucceeded());

            newViewRequestedSpy.clear();
            newViewRequest = null;
            viewType = "";
            loadRequestArray = [];
        }

        function cleanup() {
            if (dialog)
                dialog.destroy();
        }

        function test_loadNewViewRequest_data() {
            return [
                   { tag: "dialog", viewType: "dialog" },
                   { tag: "invalid", viewType: "null" },
                   { tag: "unhandled", viewType: "" },
                   { tag: "webEngineView", viewType: "webEngineView" },
            ];
        }

        function test_loadNewViewRequest(row) {
            viewType = row.viewType;
            var url = 'data:text/html,%3Chtml%3E%3Cbody%3ETest+Page%3C%2Fbody%3E%3C%2Fhtml%3E';

            // Open an empty page in a new tab
            webEngineView.loadHtml(
                "<html><head><script>" +
                "   function popup() { window.open(''); }" +
                "</script></head>" +
                "<body onload='popup()'></body></html>");
            verify(webEngineView.waitForLoadSucceeded());
            tryCompare(newViewRequestedSpy, "count", 1);

            compare(newViewRequest.destination, WebEngineView.NewViewInTab);
            verify(!newViewRequest.userInitiated);

            if (viewType === "dialog") {
                verify(dialog.webEngineView.waitForLoadSucceeded());
                compare(dialog.webEngineView.url, "");
                dialog.destroy();
            }
            // https://chromium-review.googlesource.com/c/chromium/src/+/1300395
            compare(newViewRequest.requestedUrl, 'about:blank#blocked');
            newViewRequestedSpy.clear();

            // Open a page in a new dialog
            webEngineView.loadHtml(
                "<html><head><script>" +
                "   function popup() { window.open('" + url + "', '_blank', 'width=200,height=100'); }" +
                "</script></head>" +
                "<body onload='popup()'></body></html>");
            verify(webEngineView.waitForLoadSucceeded());
            tryCompare(newViewRequestedSpy, "count", 1);

            compare(newViewRequest.destination, WebEngineView.NewViewInDialog);
            compare(newViewRequest.requestedUrl, url);
            verify(!newViewRequest.userInitiated);
            if (viewType === "dialog") {
                verify(dialog.webEngineView.waitForLoadSucceeded());
                dialog.destroy();
            }
            newViewRequestedSpy.clear();

            if (viewType !== "webEngineView") {
                // Open a page in a new dialog by user
                webEngineView.loadHtml(
                    "<html><head><script>" +
                    "   function popup() { window.open('" + url + "', '_blank', 'width=200,height=100'); }" +
                    "</script></head>" +
                    "<body onload=\"document.getElementById('popupButton').focus();\">" +
                    "   <button id='popupButton' onclick='popup()'>Pop Up!</button>" +
                    "</body></html>");
                verify(webEngineView.waitForLoadSucceeded());
                webEngineView.verifyElementHasFocus("popupButton");
                keyPress(Qt.Key_Enter);
                tryCompare(newViewRequestedSpy, "count", 1);
                compare(newViewRequest.requestedUrl, url);

                compare(newViewRequest.destination, WebEngineView.NewViewInDialog);
                verify(newViewRequest.userInitiated);
                if (viewType === "dialog") {
                    verify(dialog.webEngineView.waitForLoadSucceeded());
                    dialog.destroy();
                }
                newViewRequestedSpy.clear();
            }

            loadRequestArray = [];
            compare(loadRequestArray.length, 0);
            webEngineView.url = Qt.resolvedUrl("test2.html");
            verify(webEngineView.waitForLoadSucceeded());
            var center = webEngineView.getElementCenter("link");
            mouseClick(webEngineView, center.x, center.y, Qt.LeftButton, Qt.ControlModifier);
            tryCompare(newViewRequestedSpy, "count", 1);
            compare(newViewRequest.requestedUrl, Qt.resolvedUrl("test1.html"));
            compare(newViewRequest.destination, WebEngineView.NewViewInBackgroundTab);
            verify(newViewRequest.userInitiated);
            if (viewType === "" || viewType === "null") {
                compare(loadRequestArray[0].status, WebEngineView.LoadStartedStatus);
                compare(loadRequestArray[1].status, WebEngineView.LoadSucceededStatus);
                compare(loadRequestArray.length, 2);
            }
            newViewRequestedSpy.clear();
        }
    }
}
