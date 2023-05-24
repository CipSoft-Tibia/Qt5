# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT TARGET grpc_testserver)
    return()
endif()

qt_internal_add_test(tst_grpc_client_ssl
    SOURCES
        tst_grpc_client_ssl.cpp
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_SOURCE_DIR}/../../shared
    DEFINES
        TEST_GRPC_SERVER_PATH="$<TARGET_FILE:grpc_testserver>"
    LIBRARIES
        Qt::Test
        Qt::Core
        Qt::Grpc
        Qt::Network
        tst_grpc_client_qtgrpc_gen
        tst_grpc_client_test_common
)

add_dependencies(tst_grpc_client_ssl grpc_testserver)
