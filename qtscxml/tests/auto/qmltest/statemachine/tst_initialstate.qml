// Copyright (C) 2016 Ford Motor Company
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtTest
import QtQml.StateMachine

TestCase {
    StateMachine {
        id: myStateMachine
        initialState: myState;
        running: true
        State {
            id: myState
        }
    }

    name: "initialStateTest"
    function test_initialState() {
        tryCompare(myStateMachine, "running", true);
        compare(myState.active, true);
        myStateMachine.running = false;
        tryCompare(myStateMachine, "running", false);
        myStateMachine.running = true;
        tryCompare(myStateMachine, "running", true);
        tryCompare(myState, "active", true);
    }
}
