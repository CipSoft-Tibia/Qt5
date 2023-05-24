# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

include(.cmake.conf)
project(QtCoap
    VERSION "${QT_REPO_MODULE_VERSION}"
    DESCRIPTION "Qt Coap Libraries"
    HOMEPAGE_URL "https://qt.io/"
    LANGUAGES CXX
)

find_package(Qt6 ${PROJECT_VERSION} CONFIG REQUIRED COMPONENTS BuildInternals Core Network)
find_package(Qt6 ${PROJECT_VERSION} CONFIG OPTIONAL_COMPONENTS Gui Widgets Quick Qml)
qt_internal_project_setup()

qt_build_repo()
