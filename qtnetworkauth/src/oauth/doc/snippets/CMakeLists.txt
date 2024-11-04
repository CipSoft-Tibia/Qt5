# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

project(networkauth_cppsnippets)

find_package(Qt6 REQUIRED COMPONENTS Core NetworkAuth Gui)

qt_standard_project_setup(REQUIRES 6.5)

add_executable(networkauth_cppsnippets
    src_oauth_replyhandlers.cpp
)

target_link_libraries(networkauth_cppsnippets PRIVATE Qt6::Core Qt6::NetworkAuth Qt6::Gui)

