# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(grpc_benchmarks LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

set(proto_path "${CMAKE_CURRENT_LIST_DIR}/proto/bench.proto")
set(proto_include "${CMAKE_CURRENT_LIST_DIR}/proto")
set(qrpcbench_common_include "${CMAKE_CURRENT_LIST_DIR}")

add_subdirectory(asyncbenchserver)
add_subdirectory(bench_qtgrpcclient)
add_subdirectory(bench_async_ref_client)
