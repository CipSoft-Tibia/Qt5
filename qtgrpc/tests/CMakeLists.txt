# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(QT_BUILD_STANDALONE_TESTS)
    # Gui is need to build and run tests on Android Platforms.
    find_package(Qt6 ${PROJECT_VERSION} CONFIG REQUIRED COMPONENTS Protobuf Grpc)
    find_package(Qt6 ${PROJECT_VERSION} QUIET CONFIG OPTIONAL_COMPONENTS Gui Quick QuickTest)
    find_package(WrapgRPC QUIET)
endif()
set(QT_BUILD_TESTS_BATCHED FALSE)
qt_build_tests()
