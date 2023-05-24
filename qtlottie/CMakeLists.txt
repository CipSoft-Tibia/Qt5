# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

include(.cmake.conf)
project(QtLottie
    VERSION "${QT_REPO_MODULE_VERSION}"
    DESCRIPTION "Qt Lottie Libraries"
    HOMEPAGE_URL "https://qt.io/"
    LANGUAGES CXX C
)

find_package(Qt6 ${PROJECT_VERSION} CONFIG REQUIRED COMPONENTS BuildInternals Core)
find_package(Qt6 ${PROJECT_VERSION} QUIET CONFIG OPTIONAL_COMPONENTS Gui Qml Quick QuickTest)
qt_internal_project_setup()

if(NOT TARGET Qt6::Gui)
    message(NOTICE "Skipping the build as Qt6::Gui cannot be found.")
    return()
endif()

qt_build_repo()
