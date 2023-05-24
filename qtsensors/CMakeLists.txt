# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


cmake_minimum_required(VERSION 3.16)

include(.cmake.conf)

project(QtSensors
    VERSION "${QT_REPO_MODULE_VERSION}"
    DESCRIPTION "Qt Sensors Libraries"
    HOMEPAGE_URL "https://qt.io/"
    LANGUAGES CXX C
)

find_package(Qt6 ${PROJECT_VERSION} CONFIG REQUIRED COMPONENTS BuildInternals Core)
find_package(Qt6 ${PROJECT_VERSION} CONFIG OPTIONAL_COMPONENTS
             Xml Gui Widgets Quick Qml Svg DBus QuickTest
)
qt_internal_project_setup()

qt_build_repo()
