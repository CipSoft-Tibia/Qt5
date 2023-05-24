# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(sensorsdocsnippet LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Core Sensors)

qt_add_executable(sensorsdocsnippet
    creating.cpp
    main.cpp
    mybackend.h
    plugin.cpp
    start.cpp
)

target_link_libraries(sensorsdocsnippet PUBLIC
        Qt::Sensors
)

if (GCC AND UNIX)
    target_link_libraries(sensorsdocsnippet PUBLIC
        rdynamic
        log
    )
endif()
