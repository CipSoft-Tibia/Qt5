# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# This function creates a CMake target for a generic console or GUI binary.
# Please consider to use a more specific version target like the one created
# by qt_add_test or qt_add_tool below.
# One-value Arguments:
#     CORE_LIBRARY
#         The argument accepts 'Bootstrap' or 'None' values. If the argument value is set to
#         'Bootstrap' the Qt::Bootstrap library is linked to the executable instead of Qt::Core.
#         The 'None' value points that core library is not necessary and avoids linking neither
#         Qt::Core or Qt::Bootstrap libraries. Otherwise the Qt::Core library will be publicly
#         linked to the executable target by default.
function(qt_internal_add_executable name)
    cmake_parse_arguments(PARSE_ARGV 1 arg
        "${__qt_internal_add_executable_optional_args}"
        "${__qt_internal_add_executable_single_args}"
        "${__qt_internal_add_executable_multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if ("x${arg_OUTPUT_DIRECTORY}" STREQUAL "x")
        set(arg_OUTPUT_DIRECTORY "${QT_BUILD_DIR}/${INSTALL_BINDIR}")
    endif()

    get_filename_component(arg_OUTPUT_DIRECTORY "${arg_OUTPUT_DIRECTORY}"
        ABSOLUTE BASE_DIR "${QT_BUILD_DIR}")

    if ("x${arg_INSTALL_DIRECTORY}" STREQUAL "x")
        set(arg_INSTALL_DIRECTORY "${INSTALL_BINDIR}")
    endif()

    _qt_internal_create_executable(${name})
    qt_internal_mark_as_internal_target(${name})
    if(ANDROID)
        _qt_internal_android_executable_finalizer(${name})
    endif()

    if(arg_QT_APP AND QT_FEATURE_debug_and_release AND CMAKE_VERSION VERSION_GREATER_EQUAL "3.19.0")
        set_property(TARGET "${name}"
            PROPERTY EXCLUDE_FROM_ALL "$<NOT:$<CONFIG:${QT_MULTI_CONFIG_FIRST_CONFIG}>>")
    endif()

    if (arg_VERSION)
        if(arg_VERSION MATCHES "[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+")
            # nothing to do
        elseif(arg_VERSION MATCHES "[0-9]+\\.[0-9]+\\.[0-9]+")
            set(arg_VERSION "${arg_VERSION}.0")
        elseif(arg_VERSION MATCHES "[0-9]+\\.[0-9]+")
            set(arg_VERSION "${arg_VERSION}.0.0")
        elseif (arg_VERSION MATCHES "[0-9]+")
            set(arg_VERSION "${arg_VERSION}.0.0.0")
        else()
            message(FATAL_ERROR "Invalid version format")
        endif()
    endif()

    if(arg_DELAY_TARGET_INFO)
        # Delay the setting of target info properties if requested. Needed for scope finalization
        # of Qt apps.
        set_target_properties("${name}" PROPERTIES
            QT_DELAYED_TARGET_VERSION "${arg_VERSION}"
            QT_DELAYED_TARGET_PRODUCT "${arg_TARGET_PRODUCT}"
            QT_DELAYED_TARGET_DESCRIPTION "${arg_TARGET_DESCRIPTION}"
            QT_DELAYED_TARGET_COMPANY "${arg_TARGET_COMPANY}"
            QT_DELAYED_TARGET_COPYRIGHT "${arg_TARGET_COPYRIGHT}"
            )
    else()
        if(NOT arg_TARGET_DESCRIPTION)
            set(arg_TARGET_DESCRIPTION "Qt ${name}")
        endif()
        qt_set_target_info_properties(${name} ${ARGN}
            TARGET_DESCRIPTION ${arg_TARGET_DESCRIPTION}
            TARGET_VERSION ${arg_VERSION})
    endif()

    if (WIN32 AND NOT arg_DELAY_RC)
        _qt_internal_generate_win32_rc_file(${name})
    endif()

    qt_set_common_target_properties(${name})

    qt_internal_add_repo_local_defines(${name})

    if(ANDROID)
        # The above call to qt_set_common_target_properties() sets the symbol
        # visibility to hidden, but for Android, we need main() to not be hidden
        # because it has to be loadable at runtime using dlopen().
        set_property(TARGET ${name} PROPERTY C_VISIBILITY_PRESET default)
        set_property(TARGET ${name} PROPERTY CXX_VISIBILITY_PRESET default)
    endif()

    qt_autogen_tools_initial_setup(${name})
    qt_skip_warnings_are_errors_when_repo_unclean("${name}")

    set(extra_libraries "")
    if(arg_CORE_LIBRARY STREQUAL "Bootstrap")
        list(APPEND extra_libraries ${QT_CMAKE_EXPORT_NAMESPACE}::Bootstrap)
    elseif(NOT arg_CORE_LIBRARY STREQUAL "None")
        list(APPEND extra_libraries ${QT_CMAKE_EXPORT_NAMESPACE}::Core)
    endif()

    set(private_includes
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}"
         ${arg_INCLUDE_DIRECTORIES}
    )

    if(arg_PUBLIC_LIBRARIES)
        message(WARNING
            "qt_internal_add_executable's PUBLIC_LIBRARIES option is deprecated, and will be "
            "removed in a future Qt version. Use the LIBRARIES option instead.")
    endif()

    if(arg_NO_UNITY_BUILD)
        set(arg_NO_UNITY_BUILD "NO_UNITY_BUILD")
    else()
        set(arg_NO_UNITY_BUILD "")
    endif()

    qt_internal_extend_target("${name}"
        ${arg_NO_UNITY_BUILD}
        SOURCES ${arg_SOURCES}
        NO_PCH_SOURCES ${arg_NO_PCH_SOURCES}
        NO_UNITY_BUILD_SOURCES ${arg_NO_UNITY_BUILD_SOURCES}
        INCLUDE_DIRECTORIES ${private_includes}
        DEFINES ${arg_DEFINES}
        LIBRARIES
            ${arg_LIBRARIES}
            ${arg_PUBLIC_LIBRARIES}
            Qt::PlatformCommonInternal
            ${extra_libraries}
        DBUS_ADAPTOR_SOURCES ${arg_DBUS_ADAPTOR_SOURCES}
        DBUS_ADAPTOR_FLAGS ${arg_DBUS_ADAPTOR_FLAGS}
        DBUS_INTERFACE_SOURCES ${arg_DBUS_INTERFACE_SOURCES}
        DBUS_INTERFACE_FLAGS ${arg_DBUS_INTERFACE_FLAGS}
        COMPILE_OPTIONS ${arg_COMPILE_OPTIONS}
        LINK_OPTIONS ${arg_LINK_OPTIONS}
        MOC_OPTIONS ${arg_MOC_OPTIONS}
        ENABLE_AUTOGEN_TOOLS ${arg_ENABLE_AUTOGEN_TOOLS}
        DISABLE_AUTOGEN_TOOLS ${arg_DISABLE_AUTOGEN_TOOLS}
    )
    set_target_properties("${name}" PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${arg_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${arg_OUTPUT_DIRECTORY}"
        WIN32_EXECUTABLE "${arg_GUI}"
        MACOSX_BUNDLE "${arg_GUI}"
    )

    if(WASM)
        # WASM unconditionally sets DISABLE_EXCEPTION_CATCHING=1
        qt_internal_set_exceptions_flags("${name}" FALSE)
    else()
        qt_internal_set_exceptions_flags("${name}" ${arg_EXCEPTIONS})
    endif()

    if(WASM)
        qt_internal_wasm_add_finalizers("${name}")
    endif()

    # Check if target needs to be excluded from all target. Also affects qt_install.
    # Set by qt_exclude_tool_directories_from_default_target.
    set(exclude_from_all FALSE)
    if(__qt_exclude_tool_directories)
        foreach(absolute_dir ${__qt_exclude_tool_directories})
            string(FIND "${CMAKE_CURRENT_SOURCE_DIR}" "${absolute_dir}" dir_starting_pos)
            if(dir_starting_pos EQUAL 0)
                set(exclude_from_all TRUE)
                set_target_properties("${name}" PROPERTIES EXCLUDE_FROM_ALL TRUE)
                break()
            endif()
        endforeach()
    endif()

    if(NOT arg_NO_INSTALL)
        set(additional_install_args "")
        if(exclude_from_all)
            list(APPEND additional_install_args EXCLUDE_FROM_ALL COMPONENT "ExcludedExecutables")
        endif()

        qt_get_cmake_configurations(cmake_configs)
        foreach(cmake_config ${cmake_configs})
            qt_get_install_target_default_args(
                OUT_VAR install_targets_default_args
                CMAKE_CONFIG "${cmake_config}"
                ALL_CMAKE_CONFIGS "${cmake_configs}"
                RUNTIME "${arg_INSTALL_DIRECTORY}"
                LIBRARY "${arg_INSTALL_DIRECTORY}"
                BUNDLE "${arg_INSTALL_DIRECTORY}")

            # Make installation optional for targets that are not built by default in this config
            if(NOT exclude_from_all AND arg_QT_APP AND QT_FEATURE_debug_and_release
                    AND NOT (cmake_config STREQUAL QT_MULTI_CONFIG_FIRST_CONFIG))
                set(install_optional_arg "OPTIONAL")
            else()
                unset(install_optional_arg)
            endif()

            qt_install(TARGETS "${name}"
                       ${additional_install_args} # Needs to be before the DESTINATIONS.
                       ${install_optional_arg}
                       CONFIGURATIONS ${cmake_config}
                       ${install_targets_default_args})
        endforeach()

        if(NOT exclude_from_all AND arg_QT_APP AND QT_FEATURE_debug_and_release)
            set(separate_debug_info_executable_arg "QT_EXECUTABLE")
        else()
            unset(separate_debug_info_executable_arg)
        endif()
        qt_enable_separate_debug_info(${name} "${arg_INSTALL_DIRECTORY}"
                                      ${separate_debug_info_executable_arg}
                                      ADDITIONAL_INSTALL_ARGS ${additional_install_args})
        qt_internal_install_pdb_files(${name} "${arg_INSTALL_DIRECTORY}")
    endif()

    # If linking against Gui, make sure to also build the default QPA plugin.
    # This makes the experience of an initial Qt configuration to build and run one single
    # test / executable nicer.
    get_target_property(linked_libs "${name}" LINK_LIBRARIES)
    if("Qt::Gui" IN_LIST linked_libs AND TARGET qpa_default_plugins)
        add_dependencies("${name}" qpa_default_plugins)
    endif()

    # For static plugins, we need to explicitly link to plugins we want to be
    # loaded with the executable. User projects get that automatically, but
    # for tools built as part of Qt, we can't use that mechanism because it
    # would pollute the targets we export as part of an install and lead to
    # circular dependencies. The logic here is a simpler equivalent of the
    # more dynamic logic in QtPlugins.cmake.in, but restricted to only
    # adding plugins that are provided by the same module as the module
    # libraries the executable links to.
    set(libs
        ${arg_LIBRARIES}
        ${arg_PUBLIC_LIBRARIES}
        ${extra_libraries}
        Qt::PlatformCommonInternal
    )

    set(deduped_libs "")
    foreach(lib IN LISTS libs)
        if(NOT TARGET "${lib}")
            continue()
        endif()

        # Normalize module by stripping any leading "Qt::", because properties are set on the
        # versioned target (either Gui when building the module, or Qt6::Gui when it's
        # imported).
        if(lib MATCHES "Qt::([-_A-Za-z0-9]+)")
            set(new_lib "${QT_CMAKE_EXPORT_NAMESPACE}::${CMAKE_MATCH_1}")
            if(TARGET "${new_lib}")
                set(lib "${new_lib}")
            endif()
        endif()

        # Unalias the target.
        get_target_property(aliased_target ${lib} ALIASED_TARGET)
        if(aliased_target)
            set(lib ${aliased_target})
        endif()

        list(APPEND deduped_libs "${lib}")
    endforeach()

    list(REMOVE_DUPLICATES deduped_libs)

    foreach(lib IN LISTS deduped_libs)
        string(MAKE_C_IDENTIFIER "${name}_plugin_imports_${lib}" out_file)
        string(APPEND out_file .cpp)

        # Initialize plugins that are built in the same repository as the Qt module 'lib'.
        set(class_names_regular
            "$<GENEX_EVAL:$<TARGET_PROPERTY:${lib},_qt_initial_repo_plugin_class_names>>")

        # Initialize plugins that are built in the current Qt repository, but are associated
        # with a Qt module from a different repository (qtsvg's QSvgPlugin associated with
        # qtbase's QtGui).
        string(MAKE_C_IDENTIFIER "${PROJECT_NAME}" current_project_name)
        set(prop_prefix "_qt_repo_${current_project_name}")
        set(class_names_current_project
            "$<GENEX_EVAL:$<TARGET_PROPERTY:${lib},${prop_prefix}_plugin_class_names>>")

        # Only add separator if first list is not empty, so we don't trigger the file generation
        # when all lists are empty.
        set(class_names_separator "$<$<NOT:$<STREQUAL:${class_names_regular},>>:;>" )
        set(class_names
            "${class_names_regular}${class_names_separator}${class_names_current_project}")

        set(out_file_path "${CMAKE_CURRENT_BINARY_DIR}/${out_file}")

        file(GENERATE OUTPUT "${out_file_path}" CONTENT
"// This file is auto-generated. Do not edit.
#include <QtPlugin>

Q_IMPORT_PLUGIN($<JOIN:${class_names},)\nQ_IMPORT_PLUGIN(>)
"
            CONDITION "$<NOT:$<STREQUAL:${class_names},>>"
        )

        # CMake versions earlier than 3.18.0 can't find the generated file for some reason,
        # failing at generation phase.
        # Explicitly marking the file as GENERATED fixes the issue.
        set_source_files_properties("${out_file_path}" PROPERTIES GENERATED TRUE)

        target_sources(${name} PRIVATE
            "$<$<NOT:$<STREQUAL:${class_names},>>:${out_file_path}>"
        )
        target_link_libraries(${name} PRIVATE
            "$<TARGET_PROPERTY:${lib},_qt_initial_repo_plugins>"
            "$<TARGET_PROPERTY:${lib},${prop_prefix}_plugins>")
    endforeach()

endfunction()

# This function compiles the target at configure time the very first time and creates the custom
# ${target}_build that re-runs compilation at build time if necessary. The resulting executable is
# imported under the provided target name. This function should only be used to compile tiny
# executables with system dependencies only.
# One-value Arguments:
#     CMAKELISTS_TEMPLATE
#         The CMakeLists.txt templated that is used to configure the project
#         for an executable. By default the predefined template from the Qt installation is used.
#     INSTALL_DIRECTORY
#         installation directory of the executable. Ignored if NO_INSTALL is set.
#     OUTPUT_NAME
#         the output name of an executable
#     CONFIG
#         the name of configuration that tool needs to be build with.
# Multi-value Arguments:
#     PACKAGES
#         list of system packages are required to successfully build the project.
#     INCLUDES
#         list of include directories are required to successfully build the project.
#     DEFINES
#         list of definitions are required to successfully build the project.
#     COMPILE_OPTIONS
#         list of compiler options are required to successfully build the project.
#     LINK_OPTIONS
#         list of linker options are required to successfully build the project.
#     SOURCES
#         list of project sources.
#     CMAKE_FLAGS
#         specify flags of the form -DVAR:TYPE=VALUE to be passed to the cmake command-line used to
#         drive the test build.
# Options:
#     WIN32
#         reflects the corresponding add_executable argument.
#     MACOSX_BUNDLE
#         reflects the corresponding add_executable argument.
#     NO_INSTALL
#         avoids installing the tool.
function(qt_internal_add_configure_time_executable target)
    set(one_value_args
        CMAKELISTS_TEMPLATE
        INSTALL_DIRECTORY
        OUTPUT_NAME
        CONFIG
    )
    set(multi_value_args
        PACKAGES
        INCLUDES
        DEFINES
        COMPILE_OPTIONS
        LINK_OPTIONS
        SOURCES
        CMAKE_FLAGS
    )
    set(option_args WIN32 MACOSX_BUNDLE NO_INSTALL)
    cmake_parse_arguments(PARSE_ARGV 1 arg
        "${option_args}" "${one_value_args}" "${multi_value_args}")

    set(target_binary_dir "${CMAKE_CURRENT_BINARY_DIR}/configure_time_bins")
    if(arg_CONFIG)
        set(CMAKE_TRY_COMPILE_CONFIGURATION "${arg_CONFIG}")
        string(TOUPPER "_${arg_CONFIG}" config_suffix)
    endif()

    get_cmake_property(is_multi_config GENERATOR_IS_MULTI_CONFIG)
    if(is_multi_config AND CMAKE_TRY_COMPILE_CONFIGURATION)
        set(configuration_path "${CMAKE_TRY_COMPILE_CONFIGURATION}/")
        set(config_build_arg "--config" "${CMAKE_TRY_COMPILE_CONFIGURATION}")
    endif()

    set(configure_time_target "${target}")
    if(arg_OUTPUT_NAME)
        set(configure_time_target "${arg_OUTPUT_NAME}")
    endif()
    set(target_binary "${configure_time_target}${CMAKE_EXECUTABLE_SUFFIX}")

    set(install_dir "${INSTALL_BINDIR}")
    if(arg_INSTALL_DIRECTORY)
        set(install_dir "${arg_INSTALL_DIRECTORY}")
    endif()
    set(output_directory "${QT_BUILD_DIR}/${install_dir}")
    set(target_binary_path
        "${output_directory}/${configuration_path}${target_binary}")
    get_filename_component(target_binary_path "${target_binary_path}" ABSOLUTE)

    if(NOT DEFINED arg_SOURCES)
        message(FATAL_ERROR "No SOURCES given to target: ${target}")
    endif()
    set(sources "${arg_SOURCES}")

    # Timestamp file is required because CMake ignores 'add_custom_command' if we use only the
    # binary file as the OUTPUT.
    set(timestamp_file "${target_binary_dir}/${target_binary}_timestamp")
    add_custom_command(OUTPUT "${target_binary_path}" "${timestamp_file}"
        COMMAND
            ${CMAKE_COMMAND} --build "${target_binary_dir}" --clean-first ${config_build_arg}
        COMMAND
            ${CMAKE_COMMAND} -E touch "${timestamp_file}"
        DEPENDS
            ${sources}
        COMMENT
            "Compiling ${target}"
        VERBATIM
    )

    add_custom_target(${target}_build ALL
        DEPENDS
            "${target_binary_path}"
            "${timestamp_file}"
    )

    set(should_build_at_configure_time TRUE)
    if(QT_INTERNAL_HAVE_CONFIGURE_TIME_${target} AND
        EXISTS "${target_binary_path}" AND EXISTS "${timestamp_file}")
        set(last_ts 0)
        foreach(source IN LISTS sources)
            file(TIMESTAMP "${source}" ts "%s")
            if(${ts} GREATER ${last_ts})
                set(last_ts ${ts})
            endif()
        endforeach()

        file(TIMESTAMP "${target_binary_path}" ts "%s")
        if(${ts} GREATER_EQUAL ${last_ts})
            set(should_build_at_configure_time FALSE)
        endif()
    endif()

    set(cmake_flags_arg "")
    if(arg_CMAKE_FLAGS)
        set(cmake_flags_arg CMAKE_FLAGS "${arg_CMAKE_FLAGS}")
    endif()

    qt_internal_get_enabled_languages_for_flag_manipulation(enabled_languages)
    foreach(lang IN LISTS enabled_languages)
        set(compiler_flags_var "CMAKE_${lang}_FLAGS")
        list(APPEND cmake_flags_arg "-D${compiler_flags_var}:STRING=${${compiler_flags_var}}")
        if(arg_CONFIG)
            set(compiler_flags_var_config "${compiler_flags_var}${config_suffix}")
            list(APPEND cmake_flags_arg
                "-D${compiler_flags_var_config}:STRING=${${compiler_flags_var_config}}")
        endif()
    endforeach()

    qt_internal_get_target_link_types_for_flag_manipulation(target_link_types)
    foreach(linker_type IN LISTS target_link_types)
        set(linker_flags_var "CMAKE_${linker_type}_LINKER_FLAGS")
        list(APPEND cmake_flags_arg "-D${linker_flags_var}:STRING=${${linker_flags_var}}")
        if(arg_CONFIG)
            set(linker_flags_var_config "${linker_flags_var}${config_suffix}")
            list(APPEND cmake_flags_arg
                "-D${linker_flags_var_config}:STRING=${${linker_flags_var_config}}")
        endif()
    endforeach()

    if(NOT "${QT_INTERNAL_CMAKE_FLAGS_CONFIGURE_TIME_TOOL_${target}}" STREQUAL "${cmake_flags_arg}")
        set(should_build_at_configure_time TRUE)
    endif()

    if(should_build_at_configure_time)
        foreach(arg IN LISTS multi_value_args)
            string(TOLOWER "${arg}" template_arg_name)
            set(${template_arg_name} "")
            if(DEFINED arg_${arg})
                set(${template_arg_name} "${arg_${arg}}")
            endif()
        endforeach()

        foreach(arg IN LISTS option_args)
            string(TOLOWER "${arg}" template_arg_name)
            set(${template_arg_name} "")
            if(arg_${arg})
                set(${template_arg_name} "${arg}")
            endif()
        endforeach()

        file(MAKE_DIRECTORY "${target_binary_dir}")
        set(template "${QT_CMAKE_DIR}/QtConfigureTimeExecutableCMakeLists.txt.in")
        if(DEFINED arg_CMAKELISTS_TEMPLATE)
            set(template "${arg_CMAKELISTS_TEMPLATE}")
        endif()

        configure_file("${template}" "${target_binary_dir}/CMakeLists.txt" @ONLY)

        if(EXISTS "${target_binary_dir}/CMakeCache.txt")
            file(REMOVE "${target_binary_dir}/CMakeCache.txt")
        endif()

        try_compile(result
            "${target_binary_dir}"
            "${target_binary_dir}"
            ${target}
            ${cmake_flags_arg}
            OUTPUT_VARIABLE try_compile_output
        )

        set(QT_INTERNAL_CMAKE_FLAGS_CONFIGURE_TIME_TOOL_${target}
            "${cmake_flags_arg}" CACHE INTERNAL "")

        file(WRITE "${timestamp_file}" "")
        set(QT_INTERNAL_HAVE_CONFIGURE_TIME_${target} ${result} CACHE INTERNAL
            "Indicates that the configure-time target ${target} was built")
        if(NOT result)
            message(FATAL_ERROR "Unable to build ${target}: ${try_compile_output}")
        endif()
    endif()

    add_executable(${target} IMPORTED GLOBAL)
    add_executable(${QT_CMAKE_EXPORT_NAMESPACE}::${target} ALIAS ${target})
    set_target_properties(${target} PROPERTIES
        _qt_internal_configure_time_target TRUE
        IMPORTED_LOCATION "${target_binary_path}")

    if(NOT arg_NO_INSTALL)
        set_target_properties(${target} PROPERTIES
            _qt_internal_configure_time_target_install_location
                "${install_dir}/${target_binary}"
        )
        qt_path_join(target_install_dir ${QT_INSTALL_DIR} ${install_dir})
        qt_install(PROGRAMS "${target_binary_path}" DESTINATION "${target_install_dir}")
    endif()
endfunction()
