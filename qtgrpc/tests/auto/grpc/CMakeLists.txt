# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Set the default message latency for the gRPC tests in milliseconds.
add_compile_definitions(QT_GRPC_TEST_MESSAGE_LATENCY=${QT_INTERNAL_GRPC_TEST_MESSAGE_LATENCY_MSEC})

if(TARGET WrapgRPC::WrapLibgRPC)
    add_subdirectory(client)
    add_subdirectory(server)
    add_subdirectory(qgrpchttp2channel)
endif()
