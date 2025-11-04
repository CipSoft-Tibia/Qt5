# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(graphprinting LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS Quick3D)
find_package(Qt6 COMPONENTS Graphs)
find_package(Qt6 COMPONENTS PrintSupport)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(graphprinting
    main.cpp
)
set_target_properties(graphprinting PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(graphprinting PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Quick
    Qt::Quick3D
    Qt::Graphs
    Qt::PrintSupport
    Qt::GuiPrivate
)

qt6_add_qml_module(graphprinting
    URI GraphPrintingExample
    NO_RESOURCE_TARGET_PATH
    SOURCES
        graphprinter.h graphprinter.cpp
    QML_FILES
        qml/graphprinting/main.qml
        qml/graphprinting/Graph2D.qml
        qml/graphprinting/Graph3D.qml
    RESOURCES
        qml/graphprinting/box_left.svg
        qml/graphprinting/box_left_fill.svg
        qml/graphprinting/documents.svg
        qml/graphprinting/documents_fill.svg
        qml/graphprinting/flatten.svg
        qml/graphprinting/flatten_square_fill.svg
        qml/graphprinting/folder.svg
        qml/graphprinting/folder_fill.svg
        qml/graphprinting/print.svg
        qml/graphprinting/print_fill.svg
)

install(TARGETS graphprinting
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
