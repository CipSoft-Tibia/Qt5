// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
var ba = new ByteArray();    // constructs an empty ByteArray
var ba2 = new ByteArray(10); // constructs a ByteArray of length 10 (all bytes initialized to 0)
//! [0]


//! [1]
for (var i = 0; i < ba.length; ++i)
    ba[i] = 123;
//! [1]


//! [2]
ba[0] = 257;
print(ba[0]);  // 1
//! [2]


//! [3]
var ba3 = new ByteArray();
print(ba3.length); // 0
ba[0] = 64;
print(ba3.length); // 1
//! [3]


//! [4]
ba["foo"] = "Hello";
//! [4]


//! [5]
var ba64 = ba.toBase64();
print(ba64.toLatin1String());
//! [5]
