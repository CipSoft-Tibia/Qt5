# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16.0)

include(.cmake.conf)
project(QtLanguageServer
    VERSION "${QT_REPO_MODULE_VERSION}"
    DESCRIPTION "Qt Language Server"
    HOMEPAGE_URL "https://qt.io/"
    LANGUAGES CXX C
)

find_package(Qt6 ${PROJECT_VERSION} CONFIG REQUIRED COMPONENTS BuildInternals Core)
find_package(Qt6 ${PROJECT_VERSION} QUIET CONFIG OPTIONAL_COMPONENTS Network Concurrent Test)
qt_internal_project_setup()

if(ANDROID)
    # Android tests needs to link against Qt6::Gui
    find_package(Qt6 ${PROJECT_VERSION} QUIET CONFIG OPTIONAL_COMPONENTS Gui)
endif()

qt_build_repo()
