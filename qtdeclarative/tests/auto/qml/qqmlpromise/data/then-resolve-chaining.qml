// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
import QtQuick 2.0

QtObject {
    property int resolveValue: 1
    property int expectedValue: resolveValue + 2;
    property bool wasTestSuccessful: false

    Component.onCompleted: {
        var promise = new Promise(function(resolve, reject) {
            resolve(resolveValue);
        });

        promise.then(function(val) {
            return val + 2;
        }).then(function(val) {
            if (val === expectedValue) {
                wasTestSuccessful = true;
            }
        });
    }
}
