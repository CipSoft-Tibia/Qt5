// Copyright (C) 2023 The Qt Company Ltd.
// Copyright (C) 2019 Alexey Edelev <semlanik@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

TextField {
    id: _inputField
    width: 200
    color: "#cecfd5"
    placeholderTextColor: "#9d9faa"
    font.pointSize: 14
    padding: 10
    background: Rectangle {
        radius: 5
        border {
            width: 1
            color: _inputField.focus ? "#41cd52" : "#f3f3f4"
        }
        color: "#222840"
    }
}
