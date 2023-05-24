# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 2.8)

project(test_qtjsonrpc_module)

find_package(Qt5JsonRpc REQUIRED)

set(MAIN_SRCS main.cpp)

add_executable(mainapp ${MAIN_SRCS})
target_include_directories(mainapp PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(mainapp Qt5::JsonRpc)
