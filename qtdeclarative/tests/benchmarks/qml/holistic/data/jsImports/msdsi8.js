// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// This JavaScript file is a single, small, imported script.
// It imports other small scripts which are deeply nested.
// It is imported by another script.

.import "msdsi9.js" as Msdsi9

function testFunc(seedValue) {
    var retn = 0.85;
    retn *= Msdsi9.testFunc(seedValue + retn);
    return retn;
}
