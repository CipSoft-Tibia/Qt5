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
import QtWebEngine 1.2

TestWebEngineView {
    id: webEngineView
    width: 200
    height: 400

    SignalSpy {
        id: urlChangedSpy
        target: webEngineView
        signalName: "urlChanged"
    }

    TestCase {
        id: testCase
        name: "WebEngineViewLoadHtml"
        when: windowShown

        function test_loadProgressAfterLoadHtml() {
            var loadProgressChangedCount = 0;

            var handleLoadProgressChanged = function() {
                loadProgressChangedCount++;
            }

            webEngineView.loadProgressChanged.connect(handleLoadProgressChanged);
            webEngineView.loadHtml("<html><head><title>Test page 1</title></head><body>Hello.</body></html>")
            verify(webEngineView.waitForLoadSucceeded())
            compare(webEngineView.loadProgress, 100)
            verify(loadProgressChangedCount);
            webEngineView.loadProgressChanged.disconnect(handleLoadProgressChanged);
        }

        function test_dataURLFragment() {
            webEngineView.loadHtml("<html><body>" +
                                   "<a id='link' href='#anchor'>anchor</a>" +
                                   "</body></html>");
            verify(webEngineView.waitForLoadSucceeded());

            urlChangedSpy.clear();
            var center = getElementCenter("link");
            mouseClick(webEngineView, center.x, center.y);
            urlChangedSpy.wait();
            compare(webEngineView.url.toString().split("#")[1], "anchor");
        }
    }
}
