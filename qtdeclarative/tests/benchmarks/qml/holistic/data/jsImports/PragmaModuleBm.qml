// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

import "pragmaModuleBm.js" as PragmaModuleBmJs

Item {
    id: testQtObject

    // value = 20 + (Qt.test.Enum3 == 2) + 9 + (nbr times shared testFunc has been called previously = 0) + 9 + (nbr times shared testFunc has been called previously = 1)
    property int importedScriptFunctionValue: PragmaModuleBmJs.testFuncThree(20)
}
