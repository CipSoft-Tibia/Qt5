// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// This JavaScript file is a single, large, imported script.
// It imports a QML module and two other scripts.
// Each of those two other scripts imports a single script,
// which is a .pragma library script (shared between the two).

.import Qt.test 1.0 as QtTest
.import "pragmaBmOne.js" as PragmaBmOneJs
.import "pragmaBmTwo.js" as PragmaBmTwoJs

function testFuncThree(seedValue) {
    var retn = seedValue + QtTest.EnumValue3;
    retn += PragmaBmOneJs.testFuncOne(seedValue);
    retn += PragmaBmTwoJs.testFuncTwo(seedValue);
    return retn;
}

