# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Collection of auto detection routines to improve the user experience when
# building Qt from source.
#
# Make sure to not run detection when building standalone tests, because the detection was already
# done when initially configuring qtbase.

function(qt_internal_ensure_static_qt_config)
    if(NOT DEFINED BUILD_SHARED_LIBS)
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build Qt statically or dynamically" FORCE)
    endif()

    if(BUILD_SHARED_LIBS)
        message(FATAL_ERROR
            "Building Qt for ${CMAKE_SYSTEM_NAME} as shared libraries is not supported.")
    endif()
endfunction()

function(qt_auto_detect_wasm)
    if("${QT_QMAKE_TARGET_MKSPEC}" STREQUAL "wasm-emscripten"
            OR "${QT_QMAKE_TARGET_MKSPEC}" STREQUAL "wasm-emscripten-64")
        if (NOT DEFINED ENV{EMSDK})
            message(FATAL_ERROR
                "Can't find an Emscripten SDK! Make sure the EMSDK environment variable is "
                "available by activating and sourcing the emscripten sdk. Also ensure emcc is in "
                "your path.")
        endif()
        if(NOT DEFINED QT_AUTODETECT_WASM_IS_DONE)
            message(STATUS "Extracting Emscripten SDK info from EMSDK env var: $ENV{EMSDK}")
            __qt_internal_get_emroot_path_suffix_from_emsdk_env(EMROOT_PATH)

            __qt_internal_query_emsdk_version("${EMROOT_PATH}" TRUE CMAKE_EMSDK_REGEX_VERSION)
            set(EMCC_VERSION "${CMAKE_EMSDK_REGEX_VERSION}" CACHE STRING INTERNAL FORCE)

            if(NOT DEFINED BUILD_SHARED_LIBS)
                qt_internal_ensure_static_qt_config()
            endif()

            # Find toolchain file
            if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
                __qt_internal_get_emscripten_cmake_toolchain_file_path_from_emsdk_env(
                    "${EMROOT_PATH}" wasm_toolchain_file)
                set(CMAKE_TOOLCHAIN_FILE "${wasm_toolchain_file}" CACHE STRING "" FORCE)
            endif()

            if(EXISTS "${CMAKE_TOOLCHAIN_FILE}")
                message(STATUS
                    "Emscripten ${EMCC_VERSION} toolchain file detected at ${CMAKE_TOOLCHAIN_FILE}")
            else()
                __qt_internal_show_error_no_emscripten_toolchain_file_found_when_building_qt()
            endif()

            __qt_internal_get_emcc_recommended_version(recommended_version)
            set(QT_EMCC_RECOMMENDED_VERSION "${recommended_version}" CACHE STRING INTERNAL FORCE)

            set(QT_AUTODETECT_WASM_IS_DONE TRUE CACHE BOOL "")
        else()
            message(STATUS
                "Reusing cached Emscripten ${EMCC_VERSION} toolchain file detected at "
                "${CMAKE_TOOLCHAIN_FILE}")
        endif()
    endif()
endfunction()

function(qt_auto_detect_android)
    # We assume an Android build if any of the ANDROID_* cache variables are set.
    if(DEFINED ANDROID_SDK_ROOT
            OR DEFINED ANDROID_NDK_ROOT
            OR DEFINED ANDROID_ABI
            OR DEFINED ANDROID_NATIVE_ABI_LEVEL
            OR DEFINED ANDROID_STL)
        set(android_detected TRUE)
    else()
        set(android_detected FALSE)
    endif()

    # Auto-detect NDK root
    if(NOT DEFINED ANDROID_NDK_ROOT AND DEFINED ANDROID_SDK_ROOT)
        file(GLOB ndk_versions LIST_DIRECTORIES true RELATIVE "${ANDROID_SDK_ROOT}/ndk"
            "${ANDROID_SDK_ROOT}/ndk/*")
        unset(ndk_root)
        if(NOT ndk_versions STREQUAL "")
            # Use the NDK with the highest version number.
            if(CMAKE_VERSION VERSION_LESS 3.18)
                list(SORT ndk_versions)
                list(REVERSE ndk_versions)
            else()
                list(SORT ndk_versions COMPARE NATURAL ORDER DESCENDING)
            endif()
            list(GET ndk_versions 0 ndk_root)
            string(PREPEND ndk_root "${ANDROID_SDK_ROOT}/ndk/")
        else()
            # Fallback: use the deprecated "ndk-bundle" directory within the SDK root.
            set(ndk_root "${ANDROID_SDK_ROOT}/ndk-bundle")
            if(NOT IS_DIRECTORY "${ndk_root}")
                unset(ndk_root)
            endif()
        endif()
        if(DEFINED ndk_root)
            message(STATUS "Android NDK detected: ${ndk_root}")
            set(ANDROID_NDK_ROOT "${ndk_root}" CACHE STRING "")
        endif()
    endif()

    # Auto-detect toolchain file
    if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND DEFINED ANDROID_NDK_ROOT)
        set(toolchain_file "${ANDROID_NDK_ROOT}/build/cmake/android.toolchain.cmake")
        if(EXISTS "${toolchain_file}")
            message(STATUS "Android toolchain file within NDK detected: ${toolchain_file}")
            set(CMAKE_TOOLCHAIN_FILE "${toolchain_file}" CACHE STRING "")
        else()
            message(FATAL_ERROR "Cannot find the toolchain file '${toolchain_file}'. "
                "Please specify the toolchain file with -DCMAKE_TOOLCHAIN_FILE=<file>.")
        endif()
    endif()

    if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND android_detected)
        message(FATAL_ERROR "An Android build was requested, but no Android toolchain file was "
            "specified nor detected.")
    endif()

    if(DEFINED CMAKE_TOOLCHAIN_FILE AND NOT DEFINED QT_AUTODETECT_ANDROID)
        # Peek into the toolchain file and check if it looks like an Android one.
        if(NOT android_detected)
            file(READ ${CMAKE_TOOLCHAIN_FILE} toolchain_file_content OFFSET 0 LIMIT 80)
            string(FIND "${toolchain_file_content}" "The Android Open Source Project"
                find_result REVERSE)
            if(NOT ${find_result} EQUAL -1)
                set(android_detected TRUE)
            endif()
        endif()

        if(android_detected)
            message(STATUS "Android build detected, checking configuration defaults...")
            # ANDROID_NATIVE_API_LEVEL is an just an alias to ANDROID_PLATFORM, check for both
            if(NOT DEFINED ANDROID_PLATFORM AND NOT DEFINED ANDROID_NATIVE_API_LEVEL)
                message(STATUS "Neither ANDROID_PLATFORM nor ANDROID_NATIVE_API_LEVEL"
                    " were specified, using API level 23 as default")
                set(ANDROID_PLATFORM "android-23" CACHE STRING "")
                set(ANDROID_NATIVE_API_LEVEL 23 CACHE STRING "")
            endif()
            if(NOT DEFINED ANDROID_STL)
                set(ANDROID_STL "c++_shared" CACHE STRING "")
            endif()
        endif()
        set(QT_AUTODETECT_ANDROID ${android_detected} CACHE STRING "")
    elseif (QT_AUTODETECT_ANDROID)
        message(STATUS "Android build detected")
    endif()
endfunction()

function(qt_auto_detect_vcpkg)
    if(QT_USE_VCPKG AND DEFINED ENV{VCPKG_ROOT})
        set(vcpkg_toolchain_file "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
        get_filename_component(vcpkg_toolchain_file "${vcpkg_toolchain_file}" ABSOLUTE)

        if(DEFINED CMAKE_TOOLCHAIN_FILE)
            get_filename_component(supplied_toolchain_file "${CMAKE_TOOLCHAIN_FILE}" ABSOLUTE)
            if(NOT supplied_toolchain_file STREQUAL vcpkg_toolchain_file)
                set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "${supplied_toolchain_file}" CACHE STRING "")
            endif()
            unset(supplied_toolchain_file)
        endif()
        set(CMAKE_TOOLCHAIN_FILE "${vcpkg_toolchain_file}" CACHE STRING "" FORCE)
        message(STATUS "Using vcpkg from $ENV{VCPKG_ROOT}")
        if(DEFINED ENV{QT_VCPKG_TARGET_TRIPLET} AND NOT DEFINED VCPKG_TARGET_TRIPLET)
            set(VCPKG_TARGET_TRIPLET "$ENV{QT_VCPKG_TARGET_TRIPLET}" CACHE STRING "")
            message(STATUS "Using vcpkg triplet ${VCPKG_TARGET_TRIPLET}")
        endif()
        unset(vcpkg_toolchain_file)
        message(STATUS "CMAKE_TOOLCHAIN_FILE is: ${CMAKE_TOOLCHAIN_FILE}")
        if(DEFINED VCPKG_CHAINLOAD_TOOLCHAIN_FILE)
            message(STATUS "VCPKG_CHAINLOAD_TOOLCHAIN_FILE is: ${VCPKG_CHAINLOAD_TOOLCHAIN_FILE}")
        endif()
    endif()
endfunction()

function(qt_auto_detect_ios)
    if(CMAKE_SYSTEM_NAME STREQUAL iOS)
        message(STATUS "Using internal CMake ${CMAKE_SYSTEM_NAME} toolchain file.")

        # The QT_UIKIT_SDK check simulates the input.sdk condition for simulator_and_device in
        # configure.json.
        # If the variable is explicitly provided, assume simulator_and_device to be off.
        if(QT_UIKIT_SDK)
            set(simulator_and_device OFF)
        else()
            # Default to simulator_and_device when an explicit sdk is not requested.
            # Requires CMake 3.17.0+.
            set(simulator_and_device ON)
        endif()

        message(STATUS "simulator_and_device set to: \"${simulator_and_device}\".")

        # Choose relevant architectures.
        # Using a non Xcode generator requires explicit setting of the
        # architectures, otherwise compilation fails with unknown defines.
        if(simulator_and_device)
            set(osx_architectures "arm64;x86_64")
        elseif(QT_UIKIT_SDK STREQUAL "iphoneos")
            set(osx_architectures "arm64")
        elseif(QT_UIKIT_SDK STREQUAL "iphonesimulator")
            set(osx_architectures "x86_64")
        else()
            if(NOT DEFINED QT_UIKIT_SDK)
                message(FATAL_ERROR "Please provide a value for -DQT_UIKIT_SDK."
                    " Possible values: iphoneos, iphonesimulator.")
            else()
                message(FATAL_ERROR
                        "Unknown SDK argument given to QT_UIKIT_SDK: ${QT_UIKIT_SDK}.")
            endif()
        endif()

        # For non simulator_and_device builds, we need to explicitly set the SYSROOT aka the sdk
        # value.
        if(QT_UIKIT_SDK)
            set(CMAKE_OSX_SYSROOT "${QT_UIKIT_SDK}" CACHE STRING "")
        endif()
        set(CMAKE_OSX_ARCHITECTURES "${osx_architectures}" CACHE STRING "")

        if(NOT DEFINED BUILD_SHARED_LIBS)
            qt_internal_ensure_static_qt_config()
        endif()

        # Disable qt rpaths for iOS, just like mkspecs/common/uikit.conf does, due to those
        # bundles not being able to use paths outside the app bundle. Not sure this is strictly
        # needed though.
        set(QT_DISABLE_RPATH "OFF" CACHE BOOL "Disable automatic Qt rpath handling." FORCE)
    endif()
endfunction()

function(qt_auto_detect_cmake_config)
    # If CMAKE_CONFIGURATION_TYPES are not set for the multi-config generator use Release and
    # Debug configurations by default, instead of those are proposed by the CMake internal logic.
    get_property(is_multi GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    if(is_multi)
        if(NOT CMAKE_CONFIGURATION_TYPES)
            set(CMAKE_CONFIGURATION_TYPES Release Debug)
            set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" PARENT_SCOPE)
        endif()

        # Allow users to specify this option.
        if(NOT QT_MULTI_CONFIG_FIRST_CONFIG)
            list(GET CMAKE_CONFIGURATION_TYPES 0 first_config_type)
            set(QT_MULTI_CONFIG_FIRST_CONFIG "${first_config_type}")
            set(QT_MULTI_CONFIG_FIRST_CONFIG "${first_config_type}" PARENT_SCOPE)
        endif()

        set(CMAKE_TRY_COMPILE_CONFIGURATION "${QT_MULTI_CONFIG_FIRST_CONFIG}" PARENT_SCOPE)
        if(CMAKE_GENERATOR STREQUAL "Ninja Multi-Config")
            # Create build-<config>.ninja files for all specified configurations.
            set(CMAKE_CROSS_CONFIGS "all" CACHE STRING "")

            # The configuration that will be considered the main one (for example when
            # configuring standalone tests with a single-config generator like Ninja).
            set(CMAKE_DEFAULT_BUILD_TYPE "${QT_MULTI_CONFIG_FIRST_CONFIG}" CACHE STRING "")

            # By default when ninja is called without parameters, it will build all configurations.
            set(CMAKE_DEFAULT_CONFIGS "all" CACHE STRING "")
        endif()
    endif()
endfunction()

function(qt_auto_detect_cyclic_toolchain)
    if(CMAKE_TOOLCHAIN_FILE AND CMAKE_TOOLCHAIN_FILE MATCHES "/qt\\.toolchain\\.cmake$")
        message(FATAL_ERROR
                "Woah there! You can't use the Qt generated qt.toolchain.cmake file to configure "
                "qtbase, because that will create a toolchain file that includes itself!\n"
                "Did you accidentally use qt-cmake to configure qtbase? Make sure to remove the "
                "CMakeCache.txt file, and configure qtbase with 'cmake' instead of 'qt-cmake'.")
    endif()
endfunction()

function(qt_auto_detect_darwin)
    if(APPLE)
        # If no CMAKE_OSX_DEPLOYMENT_TARGET is provided, default to a value that Qt defines.
        # This replicates the behavior in mkspecs/common/macx.conf where
        # QMAKE_MACOSX_DEPLOYMENT_TARGET is set.
        set(description
            "Minimum OS X version to target for deployment (at runtime); newer APIs weak linked."
            " Set to empty string for default value.")
        if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
            if(NOT CMAKE_SYSTEM_NAME)
                # macOS
                set(version "11.0")
            elseif(CMAKE_SYSTEM_NAME STREQUAL iOS)
                set(version "14.0")
            endif()
            if(version)
                set(CMAKE_OSX_DEPLOYMENT_TARGET "${version}" CACHE STRING "${description}")
            endif()
        endif()

        _qt_internal_get_apple_sdk_version(apple_sdk_version)
        set(QT_MAC_SDK_VERSION "${apple_sdk_version}" CACHE STRING "Darwin SDK version.")

        _qt_internal_get_xcode_version_raw(xcode_version_raw)
        set(QT_MAC_XCODE_VERSION "${xcode_version_raw}" CACHE STRING "Xcode version.")

        list(LENGTH CMAKE_OSX_ARCHITECTURES arch_count)
        if(NOT CMAKE_SYSTEM_NAME STREQUAL iOS AND arch_count GREATER 0)
            foreach(arch ${CMAKE_OSX_ARCHITECTURES})
                if(arch STREQUAL "arm64e")
                    message(WARNING "Applications built against an arm64e Qt architecture will "
                                     "likely fail to run on Apple Silicon. Consider targeting "
                                     "'arm64' instead.")
                endif()
            endforeach()
        endif()
    endif()
endfunction()

function(qt_auto_detect_macos_universal)
    if(APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL iOS)
        list(LENGTH CMAKE_OSX_ARCHITECTURES arch_count)

        set(is_universal "OFF")
        if(arch_count GREATER 1)
            set(is_universal "ON")
        endif()

        set(QT_IS_MACOS_UNIVERSAL "${is_universal}" CACHE INTERNAL "Build universal Qt for macOS")
    endif()
endfunction()

function(qt_auto_detect_pch)
    set(default_value "ON")

    if(CMAKE_OSX_ARCHITECTURES AND CMAKE_VERSION VERSION_LESS 3.18.0 AND NOT QT_FORCE_PCH)
        list(LENGTH CMAKE_OSX_ARCHITECTURES arch_count)
        # CMake versions lower than 3.18 don't support PCH when multiple architectures are set.
        # This is the case for simulator_and_device builds.
        if(arch_count GREATER 1)
            set(default_value "OFF")
            message(WARNING "PCH support disabled due to usage of multiple architectures.")
        endif()
    endif()

    option(BUILD_WITH_PCH "Build Qt using precompiled headers?" "${default_value}")
endfunction()

function(qt_auto_detect_win32_arm)
    if("${QT_QMAKE_TARGET_MKSPEC}" STREQUAL "win32-arm64-msvc")
        set(CMAKE_SYSTEM_NAME "Windows" CACHE STRING "")
        set(CMAKE_SYSTEM_VERSION "10" CACHE STRING "")
        set(CMAKE_SYSTEM_PROCESSOR "arm64" CACHE STRING "")
    endif()
endfunction()

function(qt_auto_detect_linux_x86)
    if("${QT_QMAKE_TARGET_MKSPEC}" STREQUAL "linux-g++-32" AND NOT QT_NO_AUTO_DETECT_LINUX_X86)

        # Add flag to ensure code is compiled for 32bit x86 ABI aka i386 or its flavors.
        set(__qt_toolchain_common_flags_init "-m32")

        if(NOT QT_NO_OVERRIDE_LANG_FLAGS_INIT)
            set(CMAKE_C_FLAGS_INIT "${__qt_toolchain_common_flags_init}" PARENT_SCOPE)
            set(CMAKE_CXX_FLAGS_INIT "${__qt_toolchain_common_flags_init}" PARENT_SCOPE)
            set(CMAKE_ASM_FLAGS_INIT "${__qt_toolchain_common_flags_init}" PARENT_SCOPE)
        endif()

        # Each distro places arch-specific libraries according to its own file system layout.
        #
        # https://wiki.debian.org/Multiarch/TheCaseForMultiarch
        # https://wiki.ubuntu.com/MultiarchSpec
        # https://wiki.gentoo.org/wiki/Project:AMD64/Multilib_layout
        # https://wiki.archlinux.org/title/official_repositories#multilib
        # https://documentation.suse.com/sles/15-SP3/html/SLES-all/cha-64bit.html
        # https://pilotlogic.com/sitejoom/index.php/wiki?id=398
        # https://unix.stackexchange.com/questions/458069/multilib-and-multiarch
        #
        # CMake can usually find 32 bit libraries just fine on its own.
        # find_library will use prefixes from CMAKE_PREFIX_PATH / CMAKE_SYSTEM_PREFIX_PATH
        # and add arch-specific lib folders like 'lib/i386-linux-gnu' on debian based systems
        # or lib32/lib64 on other distros.
        # The problem is that if no 32 bit library is found, a 64 bit one might get picked up.
        # That's why we need to specify additional ignore paths.
        #
        # The paths used in the code below are Ubuntu specific.
        # You can opt out of using them if you are using a different distro, but then you need to
        # specify appropriate paths yourself in your own CMake toolchain file.
        #
        # Note that to absolutely ensure no x86_64 library is picked up on a multiarch /
        # multilib-enabled system, you might need to specify extra directories in
        # CMAKE_INGORE_PATH for each sub-directory containing a library.
        #
        # For example to exclude /usr/lib/x86_64-linux-gnu/mit-krb5/libgssapi_krb5.so
        # you need to add /usr/lib/x86_64-linux-gnu/mit-krb5 explicitly to CMAKE_IGNORE_PATH.
        # Adding just /usr/lib/x86_64-linux-gnu to either CMAKE_IGNORE_PATH or
        # CMAKE_IGNORE_PREFIX_PATH is not enough.
        #
        # Another consideration are results returned by CMake's pkg_check_modules which uses
        # pkg-config.
        # CMAKE_IGNORE_PATH is not read by pkg_check_modules, but CMAKE_PREFIX_PATH
        # values are passed as additional prefixes to look for .pc files, IN ADDITION to the default
        # prefixes searched by pkg-config of each specific distro.
        # For example on Ubuntu, the default searched paths on an x86_64 host are:
        #   /usr/local/lib/x86_64-linux-gnu/pkgconfig
        #   /usr/local/lib/pkgconfig
        #   /usr/local/share/pkgconfig
        #   /usr/lib/x86_64-linux-gnu/pkgconfig
        #   /usr/lib/pkgconfig
        #   /usr/share/pkgconfig
        # To ensure the x86_64 packages are not picked up, the PKG_CONFIG_LIBDIR environment
        # variable can be overridden with an explicit list of prefixes.
        # Again, the paths below are Ubuntu specific.
        if(NOT QT_NO_OVERRIDE_CMAKE_IGNORE_PATH)
            set(linux_x86_ignore_path "/usr/lib/x86_64-linux-gnu;/lib/x86_64-linux-gnu")
            set(CMAKE_IGNORE_PATH "${linux_x86_ignore_path}" PARENT_SCOPE)
            set_property(GLOBAL PROPERTY
                _qt_internal_linux_x86_ignore_path "${linux_x86_ignore_path}")
        endif()
        if(NOT QT_NO_OVERRIDE_PKG_CONFIG_LIBDIR)
            set(pc_config_libdir "")
            list(APPEND pc_config_libdir "/usr/local/lib/i386-linux-gnu/pkgconfig")
            list(APPEND pc_config_libdir "/usr/local/lib/pkgconfig")
            list(APPEND pc_config_libdir "/usr/local/share/pkgconfig")
            list(APPEND pc_config_libdir "/usr/lib/i386-linux-gnu/pkgconfig")
            list(APPEND pc_config_libdir "/usr/lib/pkgconfig")
            list(APPEND pc_config_libdir "/usr/share/pkgconfig")
            list(JOIN pc_config_libdir ":" pc_config_libdir)

            set_property(GLOBAL PROPERTY
                _qt_internal_linux_x86_pc_config_libdir "${pc_config_libdir}")

            # Overrides the default prefix list.
            set(ENV{PKG_CONFIG_LIBDIR} "${pc_config_libdir}")

            # Overrides the additional prefixes list.
            set(ENV{PKG_CONFIG_DIR} "")
        endif()
    endif()
endfunction()

function(qt_auto_detect_integrity)
    if(
       # Qt's custom CMake toolchain file sets this value.
       CMAKE_SYSTEM_NAME STREQUAL "Integrity" OR

       # Upstream CMake expects this name, but we don't currently use it in Qt.
       CMAKE_SYSTEM_NAME STREQUAL "GHS-MULTI"
    )
        qt_internal_ensure_static_qt_config()
    endif()
endfunction()

# Save the build type before project() might set one.
# This allows us to determine if the user has set an explicit build type that we should use.
function(qt_auto_detect_cmake_build_type)
    set(__qt_auto_detect_cmake_build_type_before_project_call "${CMAKE_BUILD_TYPE}" PARENT_SCOPE)
endfunction()

macro(qt_internal_setup_autodetect)
    # This needs to be here because QtAutoDetect loads before any other modules
    option(QT_USE_VCPKG "Enable the use of vcpkg" OFF)

    include("${CMAKE_CURRENT_LIST_DIR}/QtPublicAppleHelpers.cmake")
    include("${CMAKE_CURRENT_LIST_DIR}/QtPublicWasmToolchainHelpers.cmake")

    # Let CMake load our custom platform modules.
    # CMake-provided platform modules take precedence.
    if(NOT QT_AVOID_CUSTOM_PLATFORM_MODULES)
        list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/platforms")
    endif()

    qt_auto_detect_cyclic_toolchain()
    qt_auto_detect_cmake_config()
    qt_auto_detect_darwin()
    qt_auto_detect_macos_universal()
    qt_auto_detect_ios()
    qt_auto_detect_android()
    qt_auto_detect_pch()
    qt_auto_detect_wasm()
    qt_auto_detect_win32_arm()
    qt_auto_detect_linux_x86()
    qt_auto_detect_integrity()
    qt_auto_detect_cmake_build_type()
    qt_auto_detect_vcpkg()
endmacro()
