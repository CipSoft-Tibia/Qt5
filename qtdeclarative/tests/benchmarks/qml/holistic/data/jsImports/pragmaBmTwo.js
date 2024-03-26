// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// This JavaScript file is a single, large, imported script.
// It imports a shared library script.
.import "pragmaLib.js" as PragmaLibJs

function testFuncTwo(seedValue) {
    var retn = seedValue + 3;
    retn += PragmaLibJs.testFunc();
    return retn;
}
