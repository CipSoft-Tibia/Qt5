// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

pragma Singleton
import QtQuick
import QtCore

Settings {
    property bool isDarkTheme: Qt.styleHints.colorScheme === Qt.Dark
}
