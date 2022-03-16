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
import QtTest 1.1
import QtWebEngine 1.7

WebEngineView {
    property var loadStatus: null
    property bool windowCloseRequestedSignalEmitted: false
    settings.focusOnNavigationEnabled: true

    function waitForLoadSucceeded(timeout) {
        var success = _waitFor(function() { return loadStatus == WebEngineView.LoadSucceededStatus }, timeout)
        loadStatus = null
        return success
    }
    function waitForLoadFailed(timeout) {
        var failure = _waitFor(function() { return loadStatus == WebEngineView.LoadFailedStatus }, timeout)
        loadStatus = null
        return failure
    }
    function waitForLoadStopped(timeout) {
        var stop = _waitFor(function() { return loadStatus == WebEngineView.LoadStoppedStatus }, timeout)
        loadStatus = null
        return stop
    }
    function waitForWindowCloseRequested() {
        return _waitFor(function() { return windowCloseRequestedSignalEmitted; });
    }
    function _waitFor(predicate, timeout) {
        if (timeout === undefined)
            timeout = 12000;
        var i = 0
        while (i < timeout && !predicate()) {
            testResult.wait(50)
            i += 50
        }
        return predicate()
    }

    function getActiveElementId() {
        var activeElementId;
        runJavaScript("document.activeElement.id", function(result) {
            activeElementId = result;
        });
        testCase.tryVerify(function() { return activeElementId != undefined });
        return activeElementId;
    }

    function verifyElementHasFocus(element) {
        testCase.tryVerify(function() { return getActiveElementId() == element; }, 5000,
            "Element \"" + element + "\" has focus");
    }

    function setFocusToElement(element) {
        runJavaScript("document.getElementById('" + element + "').focus()");
        verifyElementHasFocus(element);
    }

    function getElementCenter(element) {
            var center;
            runJavaScript("(function() {" +
                          "   var elem = document.getElementById('" + element + "');" +
                          "   var rect = elem.getBoundingClientRect();" +
                          "   return { 'x': (rect.left + rect.right) / 2, 'y': (rect.top + rect.bottom) / 2 };" +
                          "})();", function(result) { center = result } );
            testCase.tryVerify(function() { return center !== undefined; });
            return center;
    }

    function getTextSelection() {
        var textSelection;
        runJavaScript("window.getSelection().toString()", function(result) { textSelection = result });
        testCase.tryVerify(function() { return textSelection !== undefined; });
        return textSelection;
    }

    TestResult { id: testResult }
    TestCase { id: testCase }

    onLoadingChanged: {
        loadStatus = loadRequest.status
    }

    onWindowCloseRequested: {
        windowCloseRequestedSignalEmitted = true;
    }
}

