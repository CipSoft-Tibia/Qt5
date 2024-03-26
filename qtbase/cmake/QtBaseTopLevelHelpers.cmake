# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Depends on __qt6_qtbase_src_path being set in the top-level dir.
macro(qt_internal_top_level_setup_autodetect)
    # Run platform auto-detection /before/ the first project() call and thus
    # before the toolchain file is loaded.
    # Don't run auto-detection when doing standalone tests. In that case, the detection
    # results are taken from either QtBuildInternals or the qt.toolchain.cmake file.

    if(NOT QT_BUILD_STANDALONE_TESTS)
        set(__qt6_auto_detect_path "${__qt6_qtbase_src_path}/cmake/QtAutoDetect.cmake")
        if(NOT EXISTS "${__qt6_auto_detect_path}")
            message(FATAL_ERROR "Required file does not exist: '${__qt6_auto_detect_path}'")
        endif()
        include("${__qt6_auto_detect_path}")
    endif()
endmacro()

macro(qt_internal_top_level_setup_after_project)
    qt_internal_top_level_setup_testing()
endmacro()

macro(qt_internal_top_level_setup_testing)
    # Required so we can call ctest from the root build directory
    enable_testing()
endmacro()

# Depends on __qt6_qtbase_src_path being set in the top-level dir.
macro(qt_internal_top_level_setup_cmake_module_path)
    if (NOT QT_BUILD_STANDALONE_TESTS)
        set(__qt6_cmake_module_path "${__qt6_qtbase_src_path}/cmake")
        if(NOT EXISTS "${__qt6_cmake_module_path}")
            message(FATAL_ERROR "Required directory does not exist: '${__qt6_cmake_module_path}'")
        endif()

        list(APPEND CMAKE_MODULE_PATH "${__qt6_cmake_module_path}")

        list(APPEND CMAKE_MODULE_PATH
            "${__qt6_cmake_module_path}/3rdparty/extra-cmake-modules/find-modules")
        list(APPEND CMAKE_MODULE_PATH "${__qt6_cmake_module_path}/3rdparty/kwin")
    endif()
endmacro()

macro(qt_internal_top_level_before_build_submodules)
    qt_internal_top_level_setup_no_create_targets()
endmacro()

macro(qt_internal_top_level_setup_no_create_targets)
    # Also make sure the CMake config files do not recreate the already-existing targets
    if (NOT QT_BUILD_STANDALONE_TESTS)
        set(QT_NO_CREATE_TARGETS TRUE)
    endif()
endmacro()

macro(qt_internal_top_level_end)
    qt_internal_print_top_level_info()

    # Depends on QtBuildInternalsConfig being included, which is the case whenver any repo is
    # configured.
    qt_internal_qt_configure_end()
endmacro()

function(qt_internal_print_top_level_info)
    if(NOT QT_BUILD_STANDALONE_TESTS)
        # Display a summary of everything
        include(QtBuildInformation)
        include(QtPlatformSupport)
        qt_print_feature_summary()
        qt_print_build_instructions()
    endif()
endfunction()

macro(qt_internal_top_level_after_add_subdirectory)
    if(module STREQUAL "qtbase")
        if (NOT QT_BUILD_STANDALONE_TESTS)
            list(APPEND CMAKE_PREFIX_PATH "${QtBase_BINARY_DIR}/${INSTALL_LIBDIR}/cmake")
            list(APPEND CMAKE_FIND_ROOT_PATH "${QtBase_BINARY_DIR}")
        endif()
    endif()
endmacro()
