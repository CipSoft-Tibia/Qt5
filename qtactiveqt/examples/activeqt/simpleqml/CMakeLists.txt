# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(simpleqmlax LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/activeqt/simpleqml")

find_package(Qt6 REQUIRED COMPONENTS AxServer Core Gui Quick QuickWidgets Widgets)
qt6_add_axserver_library(simpleqmlax
    main.cpp
    simpleqml.def
    simpleqml.rc
)

target_link_libraries(simpleqmlax PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
    Qt::QuickWidgets
    Qt::Widgets
)

# Resources:
set(simpleqml_resource_files
    "main.qml"
)

qt6_add_resources(simpleqmlax "simpleqml"
    PREFIX
        "/"
    FILES
        ${simpleqml_resource_files}
)

install(TARGETS simpleqmlax
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
