# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qthttpserver.pro.

cmake_minimum_required(VERSION 3.16)

include(.cmake.conf)
project(QtHttpServer
    VERSION "${QT_REPO_MODULE_VERSION}"
    DESCRIPTION "Qt HTTP Server"
    HOMEPAGE_URL "https://qt.io/"
    LANGUAGES CXX C
)

find_package(Qt6 ${PROJECT_VERSION} CONFIG REQUIRED COMPONENTS BuildInternals Core Network)
find_package(Qt6 ${PROJECT_VERSION} CONFIG OPTIONAL_COMPONENTS Concurrent WebSockets Gui)
qt_internal_project_setup()

qt_build_repo()
