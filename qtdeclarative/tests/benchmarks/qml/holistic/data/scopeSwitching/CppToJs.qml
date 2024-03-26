// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0
import "cppToJs.js" as CppToJs

Item {
    id: jsConsumer
    property int sideEffect: 10

    function callJsFunction() {
        jsConsumer.sideEffect = jsConsumer.sideEffect + CppToJs.nextValue;
    }
}
