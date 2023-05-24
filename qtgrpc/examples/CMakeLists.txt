# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_examples_build_begin(EXTERNAL_BUILD)

add_subdirectory(protobuf)

if(TARGET Qt6::Grpc)
    add_subdirectory(grpc)
endif()
qt_examples_build_end()
