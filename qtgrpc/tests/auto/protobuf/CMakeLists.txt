# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Basic functionality
if(TARGET Qt6::qtprotobufgen)
    add_subdirectory(basic)
    add_subdirectory(syntax)
    add_subdirectory(nopackage)
    add_subdirectory(enums)
    add_subdirectory(externalpackage)
    add_subdirectory(sequence)
    add_subdirectory(extranamespace)
    add_subdirectory(nested)
    add_subdirectory(converters)
    add_subdirectory(duplicated_metatypes)
    add_subdirectory(recursive)
    add_subdirectory(optional)
    if(TARGET Qt6::ProtobufWellKnownTypes)
        add_subdirectory(wellknown)
    endif()
endif()
add_subdirectory(qprotobuflazymessagepointer)
add_subdirectory(qprotobufoneof)
