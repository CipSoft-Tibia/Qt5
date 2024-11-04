# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

function(__qt_internal_get_supported_min_cmake_version_for_using_qt out_var)
    # This is recorded in Qt6ConfigExtras.cmake
    set(supported_version "${QT_SUPPORTED_MIN_CMAKE_VERSION_FOR_USING_QT}")
    set(${out_var} "${supported_version}" PARENT_SCOPE)
endfunction()

function(__qt_internal_get_computed_min_cmake_version_for_using_qt out_var)
    # Allow override when configuring user project.
    if(QT_FORCE_MIN_CMAKE_VERSION_FOR_USING_QT)
        set(computed_min_version "${QT_FORCE_MIN_CMAKE_VERSION_FOR_USING_QT}")

    # Set in QtConfigExtras.cmake.
    elseif(QT_COMPUTED_MIN_CMAKE_VERSION_FOR_USING_QT)
        set(computed_min_version "${QT_COMPUTED_MIN_CMAKE_VERSION_FOR_USING_QT}")
    else()
        message(FATAL_ERROR
            "Qt Developer error: Can't compute the minimum CMake version required to use this Qt.")
    endif()

    set(${out_var} "${computed_min_version}" PARENT_SCOPE)
endfunction()

function(__qt_internal_warn_if_min_cmake_version_not_met)
    __qt_internal_get_supported_min_cmake_version_for_using_qt(min_supported_version)
    __qt_internal_get_computed_min_cmake_version_for_using_qt(computed_min_version)

    if(NOT min_supported_version STREQUAL computed_min_version
            AND computed_min_version VERSION_LESS min_supported_version)
        message(WARNING
               "The minimum required CMake version to use Qt is: '${min_supported_version}'. "
               "You have explicitly chosen to require a lower minimum CMake version: '${computed_min_version}'. "
               "Using Qt with this CMake version is not officially supported. Use at your own risk."
               )
    endif()
endfunction()

function(__qt_internal_require_suitable_cmake_version_for_using_qt)
    # Skip the public project check if we're building a Qt repo because it's too early to do
    # it at find_package(Qt6) time.
    # Instead, a separate check is done in qt_build_repo_begin.
    # We detect a Qt repo by the presence of the QT_REPO_MODULE_VERSION variable set in .cmake.conf
    # of each repo.
    if(QT_REPO_MODULE_VERSION)
        return()
    endif()

    # Only do the setup once per directory scope, because Qt6 is a dependency for many packages,
    # and a recursive call will show the warning multiple times.
    if(__qt_internal_set_up_cmake_minimum_required_version_already_done)
        return()
    endif()
    set(__qt_internal_set_up_cmake_minimum_required_version_already_done TRUE PARENT_SCOPE)

    # Check the overall minimum required CMake version when consuming any Qt CMake package.
    __qt_internal_warn_if_min_cmake_version_not_met()
    __qt_internal_get_computed_min_cmake_version_for_using_qt(computed_min_version)

    if(CMAKE_VERSION VERSION_LESS computed_min_version)
        set(major_minor "${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}")
        message(FATAL_ERROR
            "CMake ${computed_min_version} or higher is required to use Qt. "
            "You are running version ${CMAKE_VERSION} "
            "Qt requires newer CMake features to work correctly. You can lower the minimum "
            "required version by passing "
            "-DQT_FORCE_MIN_CMAKE_VERSION_FOR_USING_QT=${major_minor} when configuring the "
            "project. Using Qt with this CMake version is not officially supported. "
            "Use at your own risk.")
    endif()
endfunction()

function(__qt_internal_set_cmp0156)
    if(POLICY CMP0156)
        if(QT_FORCE_CMP0156_TO_NEW)
            cmake_policy(SET CMP0156 "NEW")
        else()
            cmake_policy(GET CMP0156 policy_value)
            if(NOT "${policy_value}" STREQUAL "OLD")
                if("${policy_value}" STREQUAL "NEW" AND NOT QT_BUILDING_QT)
                    message(WARNING "CMP0156 is set to '${policy_value}'. Qt forces the 'OLD'"
                        " behavior of this policy by default. Set QT_FORCE_CMP0156_TO_NEW=ON to"
                        " force the 'NEW' behavior for the Qt commands that create either"
                        " library or executable targets.")
                endif()
                cmake_policy(SET CMP0156 "OLD")
            endif()
        endif()
    endif()
endfunction()
