# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(proto_out_dir "${CMAKE_CURRENT_BINARY_DIR}/include/proto/")
set(proto_out_include "${CMAKE_CURRENT_BINARY_DIR}/include/")

qt_internal_add_executable(
    bench_qtgrpcclient
    SOURCES
        bench_qtgrpcclient.cpp
    LIBRARIES
        Qt::Grpc
    INCLUDE_DIRECTORIES
        "${proto_out_include}"
        "${qrpcbench_common_include}"
    OUTPUT_DIRECTORY
        "${CMAKE_CURRENT_BINARY_DIR}"
)

qt_add_protobuf(bench_qtgrpcclient
    PROTO_FILES
        "${proto_path}"
    OUTPUT_DIRECTORY
        "${proto_out_dir}"
)

qt_add_grpc(bench_qtgrpcclient CLIENT
    PROTO_FILES
        "${proto_path}"
    OUTPUT_DIRECTORY
        "${proto_out_dir}"
)

add_dependencies(bench_qtgrpcclient asyncbenchserver)
