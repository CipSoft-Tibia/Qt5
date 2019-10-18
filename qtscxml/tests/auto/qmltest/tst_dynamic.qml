/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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

import QtTest 1.15
import QtScxml 5.15

TestCase {
    id: testCase

    StateMachineLoader {
        id: loader
        source: "qrc:///statemachine.scxml"
    }

    function test_overloaded_calls_with_dynamic_statemachine()
    {
        // This test calls "submitEvent" invokable function which has 3
        // overloads, differentiated both by parameter types and amounts.
        // Test verifies that the overloads are callable while using
        // a dynamic statemachine which has a dynamic metaobject under the hood
        tryVerify(() => loader.stateMachine.activeStateNames()[0] === "red", 200)
        loader.stateMachine.submitEvent("step")
        tryVerify(() => loader.stateMachine.activeStateNames()[0] === "yellow", 200)
        loader.stateMachine.submitEvent("step", "somedata")
        tryVerify(() => loader.stateMachine.activeStateNames()[0] === "green", 200)
    }
}
