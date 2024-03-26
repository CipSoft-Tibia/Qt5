# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(TARGET WrapProtoc::WrapProtoc)
    set(WrapProtoc_FOUND TRUE)
    return()
endif()

set(WrapProtoc_FOUND FALSE)

set(__WrapProtoc_find_package_args "")
if(${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY)
    list(APPEND __WrapProtoc_find_package_args QUIET)
endif()

if(NOT CMAKE_CROSSCOMPILING)
    if(NOT TARGET Threads::Threads)
        find_package(Threads ${__WrapProtoc_find_package_args})
    endif()
    if(TARGET Threads::Threads)
        qt_internal_disable_find_package_global_promotion(Threads::Threads)
    endif()

    # Protobuf can be represented in the system by both modern CONFIG and old style MODULE provided
    # by CMake. The use of MODULE with new versions of protoc in PATH causes issues, so CONFIG
    # should be preferred, but we still need to support MODULE. CMAKE_FIND_PACKAGE_PREFER_CONFIG
    # gives this possibility.
    set(__WrapProtoc_CMAKE_FIND_PACKAGE_PREFER_CONFIG_save ${CMAKE_FIND_PACKAGE_PREFER_CONFIG})
    set(CMAKE_FIND_PACKAGE_PREFER_CONFIG TRUE)
    find_package(Protobuf ${WrapProtoc_FIND_VERSION} ${__WrapProtoc_find_package_args})
    set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ${__WrapProtoc_CMAKE_FIND_PACKAGE_PREFER_CONFIG_save})
    unset(__WrapProtoc_CMAKE_FIND_PACKAGE_PREFER_CONFIG_save)

    if(TARGET protobuf::libprotobuf)
        qt_internal_disable_find_package_global_promotion(protobuf::libprotobuf)
    endif()
    if(TARGET protobuf::libprotoc)
        qt_internal_disable_find_package_global_promotion(protobuf::libprotoc)
    endif()

    if(Protobuf_FOUND AND TARGET protobuf::protoc)
        get_target_property(__WrapProtoc_is_protoc_imported protobuf::protoc IMPORTED)
        if(__WrapProtoc_is_protoc_imported)
            foreach(config IN ITEMS _RELWITHDEBINFO "" _RELEASE _MINSIZEREL _DEBUG)
                get_target_property(__WrapProtoc_protoc_imported_location protobuf::protoc
                    IMPORTED_LOCATION${config})
                if(__WrapProtoc_protoc_imported_location)
                    break()
                endif()
            endforeach()
        endif()
    endif()
endif()

if(NOT __WrapProtoc_protoc_imported_location)
    if(CMAKE_CROSSCOMPILING)
        set(__WrapProtoc_extra_prefix_paths "${QT_ADDITIONAL_HOST_PACKAGES_PREFIX_PATH}")
    endif()
    find_program(__WrapProtoc_protoc_imported_location
        NAMES protoc protoc.exe
        PATHS "${__WrapProtoc_extra_prefix_paths}" "$ENV{Protobuf_ROOT}"
        PATH_SUFFIXES bin
    )
endif()

if(__WrapProtoc_protoc_imported_location)
    add_executable(WrapProtoc::WrapProtoc IMPORTED)
    set_target_properties(WrapProtoc::WrapProtoc PROPERTIES
        IMPORTED_LOCATION "${__WrapProtoc_protoc_imported_location}"
        _qt_internal_protobuf_version "${Protobuf_VERSION}"
    )
    set(WrapProtoc_FOUND TRUE)
endif()

unset(__WrapProtoc_find_package_args)
