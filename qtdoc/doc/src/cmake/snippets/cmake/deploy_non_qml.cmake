# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#! [basic setup]
cmake_minimum_required(VERSION 3.16)

project(MyApp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core)
qt_standard_project_setup()

qt_add_executable(MyApp main.cpp)
target_link_libraries(MyApp PRIVATE Qt6::Core)
#! [basic setup]

#! [installation]
install(TARGETS MyApp
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
#! [installation]

#! [deployment script]
qt_generate_deploy_app_script(
    TARGET MyApp
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
#! [deployment script]
