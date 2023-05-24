# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_INTERNAL_GRPC_TEST_MESSAGE_LATENCY_MSEC)
    if (WIN32)
        set(QT_INTERNAL_GRPC_TEST_MESSAGE_LATENCY_MSEC 1000)
    else()
        set(QT_INTERNAL_GRPC_TEST_MESSAGE_LATENCY_MSEC 300)
    endif()
endif()

# Set the default message latency for the gRPC tests in milliseconds.
add_compile_definitions(QT_GRPC_TEST_MESSAGE_LATENCY=${QT_INTERNAL_GRPC_TEST_MESSAGE_LATENCY_MSEC})

if(TARGET WrapgRPC::WrapLibgRPC)
    add_subdirectory(client)
    add_subdirectory(server)
endif()
