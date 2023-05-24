# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

include(.cmake.conf)
project(QtTranslations
    VERSION "${QT_REPO_MODULE_VERSION}"
    DESCRIPTION "Qt Translations"
    HOMEPAGE_URL "https://qt.io/"
    LANGUAGES CXX C
)

# Make sure we use the fixed BASE argument of qt_add_resource.
set(QT_USE_FIXED_QT_ADD_RESOURCE_BASE TRUE)

set(QT_REPO_NOT_WARNINGS_CLEAN TRUE)
find_package(Qt6 ${PROJECT_VERSION} CONFIG REQUIRED COMPONENTS BuildInternals Core)
find_package(Qt6 ${PROJECT_VERSION} QUIET CONFIG OPTIONAL_COMPONENTS Linguist)

include(QtSetup) # for qt_install

if(TARGET ${QT_CMAKE_EXPORT_NAMESPACE}::lupdate AND TARGET ${QT_CMAKE_EXPORT_NAMESPACE}::lconvert)
    add_subdirectory(translations)
else()
    message(STATUS "Skipping adding translation custom targets due to missing lupdate or lconvert.")
endif()
