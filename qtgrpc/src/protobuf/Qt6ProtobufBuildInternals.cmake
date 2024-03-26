# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause
set(__qt_protobuf_build_internals_base_dir "${CMAKE_CURRENT_LIST_DIR}" CACHE INTERNAL "")

macro(qt_internal_get_internal_add_protobuf_module_keywords option_args single_args multi_args)
    set(${option_args} "")
    set(${single_args}
        TYPE_REGISTRATION_FUNCTION
    )
    set(${multi_args}
        PROTO_FILES
    )
endmacro()

function(qt_internal_add_protobuf_module target)
    qt_internal_get_internal_add_module_keywords(module_option_args module_single_args
        module_multi_args)

    qt_internal_get_internal_add_protobuf_module_keywords(
            protobuf_module_option_args
            protobuf_module_single_args
            protobuf_module_multi_args
    )
    set(protobuf_module_single_args TYPE_REGISTRATION_FUNCTION)

    set(option_args ${module_option_args} ${protobuf_module_option_args})
    set(single_args ${module_single_args} ${protobuf_module_single_args})
    set(multi_args ${module_multi_args} ${protobuf_module_multi_args})

    cmake_parse_arguments(PARSE_ARGV 1 arg
        "${option_args}"
        "${single_args}"
        "${multi_args}"
    )
    _qt_internal_validate_all_args_are_parsed(arg)

    qt_remove_args(module_args
        ARGS_TO_REMOVE
            ${protobuf_module_option_args}
            ${protobuf_module_single_args}
            ${protobuf_module_multi_args}
            EXTRA_CMAKE_INCLUDES
        ALL_ARGS
            ${option_args}
            ${single_args}
            ${multi_args}
        ARGS
            ${ARGN}
    )

    if(NOT arg_EXTRA_CMAKE_INCLUDES)
        set(arg_EXTRA_CMAKE_INCLUDES "")
    endif()

    if(arg_PROTO_FILES)
        set(module_extra_properties_filename
            "${INSTALL_CMAKE_NAMESPACE}${target}ProtobufProperties.cmake")
    endif()

    qt_internal_add_module(${target}
        EXTRA_CMAKE_INCLUDES
            ${module_extra_properties_filename}
            ${arg_EXTRA_CMAKE_INCLUDES}
        ${module_args}
    )

    if(arg_PROTO_FILES)
        unset(generated_targets)
        qt6_add_protobuf(${target}
            PROTO_FILES
                ${arg_PROTO_FILES}
            OUTPUT_TARGETS generated_targets
            GENERATE_PACKAGE_SUBFOLDERS
        )
        qt_internal_module_info(module ${target})

        if(arg_PROTO_FILES_BASE_DIR)
            set(proto_files_base_dir "${arg_PROTO_FILES_BASE_DIR}")
        else()
            set(proto_files_base_dir "${CMAKE_CURRENT_SOURCE_DIR}")
        endif()

        foreach(f IN LISTS arg_PROTO_FILES)
            if(IS_ABSOLUTE "${f}")
                file(RELATIVE_PATH f_rel "${proto_files_base_dir}" "${f}")
            else()
                set(f_rel "${f}")
            endif()
            get_filename_component(relative_directory "${f_rel}" DIRECTORY)
            qt_copy_or_install(
                FILES
                    ${f}
                DESTINATION
                    "${module_install_interface_include_dir}/${relative_directory}"
            )
        endforeach()

        set_target_properties(${target} PROPERTIES QT_PROTO_INCLUDES "${proto_files_base_dir}")

        set(proto_include_dirs "${module_install_interface_include_dir}")

        set(export_name "${INSTALL_CMAKE_NAMESPACE}${target}")
        qt_path_join(config_build_dir ${QT_CONFIG_BUILD_DIR} ${export_name})
        qt_path_join(config_install_dir ${QT_CONFIG_INSTALL_DIR} ${export_name})
        set(module_extra_properties_file "${config_build_dir}/${module_extra_properties_filename}")
        configure_file("${__qt_protobuf_build_internals_base_dir}/QtProtobufProperties.cmake.in"
            "${module_extra_properties_file}" @ONLY)
        qt_install(FILES "${module_extra_properties_file}" DESTINATION "${config_install_dir}")
        if(generated_targets)
            list(REMOVE_DUPLICATES generated_targets)
            qt_install(TARGETS ${generated_targets}
                EXPORT "${export_name}Targets"
                DESTINATION "${INSTALL_LIBDIR}"
            )

            qt_internal_add_targets_to_additional_targets_export_file(
                TARGETS ${generated_targets}
                EXPORT_NAME_PREFIX "${export_name}"
            )
        endif()
    endif()

    get_target_property(target_type ${target} TYPE)
    if(arg_TYPE_REGISTRATION_FUNCTION)
        set(registration_file
            "${CMAKE_CURRENT_BINARY_DIR}/${target}_protobuf_module_registration.cpp")
        set(content "#include <QtProtobuf/qtprotobufglobal.h>\n\
QT_BEGIN_NAMESPACE\n\
extern Q_DECL_IMPORT void ${arg_TYPE_REGISTRATION_FUNCTION}();\n\
Q_CONSTRUCTOR_FUNCTION(${arg_TYPE_REGISTRATION_FUNCTION})\n\
QT_END_NAMESPACE\n")
        qt_internal_get_main_cmake_configuration(main_config)
        file(GENERATE
            OUTPUT "${registration_file}"
            CONTENT "${content}"
            CONDITION "$<CONFIG:${main_config}>")
        if(target_type STREQUAL "STATIC_LIBRARY"
            OR (WIN32 AND NOT target_type STREQUAL "EXECUTABLE"))
            set(object_library ${target}_protobuf_module_registration)

            add_library(${object_library} OBJECT ${registration_file})
            qt_internal_link_internal_platform_for_object_library(${object_library})
            _qt_internal_copy_dependency_properties(${object_library}
                ${target} PRIVATE_ONLY)

            target_link_libraries(${target}
                INTERFACE "$<TARGET_OBJECTS:$<TARGET_NAME:${object_library}>>")
            add_dependencies(${target} ${object_library})

            qt_install(TARGETS ${object_library}
                EXPORT "${INSTALL_CMAKE_NAMESPACE}${target}Targets"
                DESTINATION "${INSTALL_LIBDIR}"
            )

            qt_internal_add_targets_to_additional_targets_export_file(
                TARGETS ${object_library}
                EXPORT_NAME_PREFIX "${INSTALL_CMAKE_NAMESPACE}${target}"
            )
        else()
            target_sources(${target} PRIVATE "${registration_file}")
        endif()
    endif()
endfunction()
