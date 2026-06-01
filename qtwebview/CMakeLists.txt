# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

include(.cmake.conf)
project(QtWebView
    VERSION "${QT_REPO_MODULE_VERSION}"
    DESCRIPTION "Qt WebView Libraries"
    HOMEPAGE_URL "https://qt.io/"
    LANGUAGES CXX C
)

find_package(Qt6 ${PROJECT_VERSION} CONFIG REQUIRED COMPONENTS BuildInternals Core)
find_package(Qt6 ${PROJECT_VERSION} CONFIG OPTIONAL_COMPONENTS Gui Qml Quick WebEngineCore WebEngineQuick)
qt_internal_project_setup()

# Make sure we use the fixed BASE argument of qt_add_resource.
set(QT_USE_FIXED_QT_ADD_RESOURCE_BASE TRUE)

if(QNX)
    message(NOTICE "Skipping the build as the condition \"NOT QNX\" is not met.")
    return()
endif()

if(NOT QT_FEATURE_gui)
    message(NOTICE "Skipping the build as the condition \"QT_FEATURE_gui\" is not met.")
    return()
endif()
qt_build_repo()
