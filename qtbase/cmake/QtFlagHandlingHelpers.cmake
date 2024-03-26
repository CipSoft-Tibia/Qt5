# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Sets '${var}' to a genex that extracts the target's property.
# Sets 'have_${var}' to a genex that checks that the property has a
# non-empty value.
macro(qt_internal_genex_get_property var target property)
    set(${var} "$<TARGET_PROPERTY:${target},${property}>")
    set(have_${var} "$<BOOL:${${var}}>")
endmacro()

# Sets '${var}' to a genex that will join the given property values
# using '${glue}' and will surround the entire output with '${prefix}'
# and '${suffix}'.
macro(qt_internal_genex_get_joined_property var target property prefix suffix glue)
    qt_internal_genex_get_property("${var}" "${target}" "${property}")
    set(${var}
        "$<${have_${var}}:${prefix}$<JOIN:${${var}},${glue}>${suffix}>")
endmacro()

# This function generates LD version script for the target and uses it in the target linker line.
# Function has two modes dependending on the specified arguments.
# Arguments:
#    PRIVATE_CONTENT_FILE specifies the pre-cooked content of Qt_<version>_PRIVATE_API section.
#       Requires the content file available at build time.
function(qt_internal_add_linker_version_script target)
    if(WASM)
        return()
    endif()

    cmake_parse_arguments(PARSE_ARGV 1 arg
        ""
        "PRIVATE_CONTENT_FILE"
        "PRIVATE_HEADERS"
    )
    _qt_internal_validate_all_args_are_parsed(arg)

    if(arg_PRIVATE_CONTENT_FILE AND arg_PRIVATE_HEADERS)
        message(FATAL_ERROR "Both PRIVATE_CONTENT_FILE and PRIVATE_HEADERS are specified.")
    endif()

    if(TEST_ld_version_script)
        set(contents "Qt_${PROJECT_VERSION_MAJOR}_PRIVATE_API {\n    qt_private_api_tag*;\n")
        if(arg_PRIVATE_HEADERS)
            foreach(ph ${arg_PRIVATE_HEADERS})
                string(APPEND contents "    @FILE:${ph}@\n")
            endforeach()
        else()
            string(APPEND contents "@PRIVATE_CONTENT@")
        endif()
        string(APPEND contents "};\n")
        set(current "Qt_${PROJECT_VERSION_MAJOR}")
        string(APPEND contents "${current} {\n    *;")

        get_target_property(target_type ${target} TYPE)
        if(NOT target_type STREQUAL "INTERFACE_LIBRARY")
            set(genex_prefix "\n    ")
            set(genex_glue "$<SEMICOLON>\n    ")
            set(genex_suffix "$<SEMICOLON>")
            qt_internal_genex_get_joined_property(
                linker_exports "${target}" _qt_extra_linker_script_exports
                "${genex_prefix}" "${genex_suffix}" "${genex_glue}"
            )
            string(APPEND contents "${linker_exports}")
        endif()
        string(APPEND contents "\n};\n")

        if(NOT target_type STREQUAL "INTERFACE_LIBRARY")
            set(property_genex "$<TARGET_PROPERTY:${target},_qt_extra_linker_script_content>")
            set(check_genex "$<BOOL:${property_genex}>")
            string(APPEND contents
                "$<${check_genex}:${property_genex}>")
        endif()

        set(infile "${CMAKE_CURRENT_BINARY_DIR}/${target}.version.in")
        set(outfile "${CMAKE_CURRENT_BINARY_DIR}/${target}.version")

        file(GENERATE OUTPUT "${infile}" CONTENT "${contents}")

        if(NOT arg_PRIVATE_CONTENT_FILE)
            set(arg_PRIVATE_CONTENT_FILE "")
        endif()
        set(generator_command ${CMAKE_COMMAND}
            "-DIN_FILE=${infile}"
            "-DPRIVATE_CONTENT_FILE=${arg_PRIVATE_CONTENT_FILE}"
            "-DOUT_FILE=${outfile}"
            -P "${QT_CMAKE_DIR}/QtGenerateVersionScript.cmake"
        )
        set(generator_dependencies
            "${arg_PRIVATE_CONTENT_FILE}"
            "${QT_CMAKE_DIR}/QtGenerateVersionScript.cmake"
        )

        add_custom_command(
            OUTPUT "${outfile}"
            COMMAND ${generator_command}
            DEPENDS ${generator_dependencies}
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            COMMENT "Generating version linker script for target ${target}"
            VERBATIM
        )
        add_custom_target(${target}_version_script DEPENDS ${outfile})
        add_dependencies(${target} ${target}_version_script)
        target_link_options(${target} PRIVATE "-Wl,--version-script,${outfile}")
    endif()
endfunction()

function(qt_internal_add_link_flags_no_undefined target)
    if (NOT QT_BUILD_SHARED_LIBS OR WASM)
        return()
    endif()
    if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
        # ld64 defaults to -undefined,error, and in Xcode 15
        # passing this option is deprecated, causing a warning.
        return()
    endif()
    if ((GCC OR CLANG) AND NOT MSVC)
        if(CLANG AND QT_FEATURE_sanitizer)
            return()
        endif()
        set(previous_CMAKE_REQUIRED_LINK_OPTIONS ${CMAKE_REQUIRED_LINK_OPTIONS})

        set(CMAKE_REQUIRED_LINK_OPTIONS "-Wl,-undefined,error")
        check_cxx_source_compiles("int main() {}" HAVE_DASH_UNDEFINED_SYMBOLS)
        if(HAVE_DASH_UNDEFINED_SYMBOLS)
            set(no_undefined_flag "-Wl,-undefined,error")
        endif()

        set(CMAKE_REQUIRED_LINK_OPTIONS "-Wl,--no-undefined")
        check_cxx_source_compiles("int main() {}" HAVE_DASH_DASH_NO_UNDEFINED)
        if(HAVE_DASH_DASH_NO_UNDEFINED)
            set(no_undefined_flag "-Wl,--no-undefined")
        endif()

        set(CMAKE_REQUIRED_LINK_OPTIONS ${previous_CMAKE_REQUIRED_LINK_OPTIONS})

        if (NOT HAVE_DASH_UNDEFINED_SYMBOLS AND NOT HAVE_DASH_DASH_NO_UNDEFINED)
            message(FATAL_ERROR "Platform linker doesn't support erroring upon encountering undefined symbols. Target:\"${target}\".")
        endif()
        target_link_options("${target}" PRIVATE "${no_undefined_flag}")
    endif()
endfunction()

function(qt_internal_apply_gc_binaries_conditional target visibility)
    # Should only be applied when the feature is enabled, aka for static builds.
    if(NOT QT_FEATURE_gc_binaries)
        return()
    endif()
    qt_internal_apply_gc_binaries("${target}" "${visibility}")
endfunction()

function(qt_internal_apply_gc_binaries target visibility)
    set(possible_visibilities PRIVATE INTERFACE PUBLIC)
    if(NOT visibility IN_LIST possible_visibilities)
        message(FATAL_ERROR "Visibitily setting must be one of PRIVATE, INTERFACE or PUBLIC.")
    endif()

    string(JOIN "" clang_or_gcc_begin
        "$<$<OR:"
            "$<CXX_COMPILER_ID:GNU>,"
            "$<CXX_COMPILER_ID:Clang>,"
            "$<CXX_COMPILER_ID:AppleClang>,"
            "$<CXX_COMPILER_ID:IntelLLVM>"
        ">:"
    )
    set(clang_or_gcc_end ">")

    if ((GCC OR CLANG) AND NOT WASM AND NOT UIKIT AND NOT MSVC)
        if(APPLE)
            set(gc_sections_flag "-Wl,-dead_strip")
        elseif(SOLARIS)
            set(gc_sections_flag "-Wl,-z,ignore")
        elseif(LINUX OR BSD OR WIN32 OR ANDROID)
            set(gc_sections_flag "-Wl,--gc-sections")
        endif()

        # Save the flag value with and without genex wrapping, so we can remove the wrapping
        # when generating .pc pkgconfig files.
        set_property(GLOBAL PROPERTY _qt_internal_gc_sections_without_genex "${gc_sections_flag}")

        set(gc_sections_flag
            "${clang_or_gcc_begin}${gc_sections_flag}${clang_or_gcc_end}")

        set_property(GLOBAL PROPERTY _qt_internal_gc_sections_with_genex "${gc_sections_flag}")
    endif()
    if(gc_sections_flag)
        target_link_options("${target}" ${visibility} "${gc_sections_flag}")
    endif()

    if((GCC OR CLANG) AND NOT WASM AND NOT UIKIT AND NOT MSVC)
        set(split_sections_flags
            "${clang_or_gcc_begin}-ffunction-sections;-fdata-sections${clang_or_gcc_end}")
    endif()
    if(split_sections_flags)
        target_compile_options("${target}" ${visibility} "${split_sections_flags}")
    endif()
endfunction()

function(qt_internal_apply_intel_cet target visibility)
    if(NOT QT_FEATURE_intelcet)
        return()
    endif()

    set(possible_visibilities PRIVATE INTERFACE PUBLIC)
    if(NOT visibility IN_LIST possible_visibilities)
        message(FATAL_ERROR "Visibitily setting must be one of PRIVATE, INTERFACE or PUBLIC.")
    endif()

    if(GCC)
        string(JOIN "" flags
            "$<$<OR:"
                "$<CXX_COMPILER_ID:GNU>,"
                "$<CXX_COMPILER_ID:Clang>,"
                "$<CXX_COMPILER_ID:AppleClang>"
            ">:-mshstk>")
    endif()
    if(flags)
        target_compile_options("${target}" ${visibility} "${flags}")
    endif()
endfunction()

function(qt_internal_library_deprecation_level result)
    # QT_DISABLE_DEPRECATED_UP_TO controls which version we use as a cut-off
    # compiling in to the library. E.g. if it is set to QT_VERSION then no
    # code which was deprecated before QT_VERSION will be compiled in.
    if (NOT DEFINED QT_DISABLE_DEPRECATED_UP_TO)
        if(WIN32)
            # On Windows, due to the way DLLs work, we need to export all functions,
            # including the inlines
            list(APPEND deprecations "QT_DISABLE_DEPRECATED_UP_TO=0x040800")
        else()
            # On other platforms, Qt's own compilation does need to compile the Qt 5.0 API
            list(APPEND deprecations "QT_DISABLE_DEPRECATED_UP_TO=0x050000")
        endif()
    else()
        list(APPEND deprecations "QT_DISABLE_DEPRECATED_UP_TO=${QT_DISABLE_DEPRECATED_UP_TO}")
    endif()
    # QT_WARN_DEPRECATED_UP_TO controls the upper-bound of deprecation
    # warnings that are emitted. E.g. if it is set to 0x060500 then all use of
    # things deprecated in or before 6.5.0 will be warned against.
    list(APPEND deprecations "QT_WARN_DEPRECATED_UP_TO=0x070000")
    set("${result}" "${deprecations}" PARENT_SCOPE)
endfunction()

# Sets the exceptions flags for the given target according to exceptions_on
function(qt_internal_set_exceptions_flags target exceptions_on)
    set(_defs "")
    set(_flag "")
    if(exceptions_on)
        if(MSVC)
            set(_flag "/EHsc")
            if((MSVC_VERSION GREATER_EQUAL 1929) AND NOT CLANG)
                set(_flag ${_flag} "/d2FH4")
            endif()
        endif()
    else()
        set(_defs "QT_NO_EXCEPTIONS")
        if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
            set(_flag "/EHs-c-" "/wd4530" "/wd4577")
        elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU|AppleClang|InteLLLVM")
            set(_flag "-fno-exceptions")
        elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
            if (MSVC)
                set(_flag "/EHs-c-" "/wd4530" "/wd4577")
            else()
                set(_flag "-fno-exceptions")
            endif()
        endif()
    endif()

    target_compile_definitions("${target}" PRIVATE ${_defs})
    target_compile_options("${target}" PRIVATE ${_flag})
endfunction()

function(qt_skip_warnings_are_errors target)
    get_target_property(target_type "${target}" TYPE)
    if(target_type STREQUAL "INTERFACE_LIBRARY")
        return()
    endif()
    set_target_properties("${target}" PROPERTIES QT_SKIP_WARNINGS_ARE_ERRORS ON)
endfunction()

function(qt_skip_warnings_are_errors_when_repo_unclean target)
    if(QT_REPO_NOT_WARNINGS_CLEAN)
        qt_skip_warnings_are_errors("${target}")
    endif()
endfunction()

function(qt_disable_warnings target)
    get_target_property(target_type "${target}" TYPE)
    if(target_type STREQUAL "INTERFACE_LIBRARY")
        return()
    endif()
    set_target_properties("${target}" PROPERTIES QT_COMPILE_OPTIONS_DISABLE_WARNINGS ON)
endfunction()

function(qt_set_symbol_visibility_preset target value)
    get_target_property(target_type "${target}" TYPE)
    if(target_type STREQUAL "INTERFACE_LIBRARY")
        return()
    endif()

    set_target_properties("${target}" PROPERTIES C_VISIBILITY_PRESET "${value}")
    set_target_properties("${target}" PROPERTIES CXX_VISIBILITY_PRESET "${value}")
    set_target_properties("${target}" PROPERTIES OBJC_VISIBILITY_PRESET "${value}")
    set_target_properties("${target}" PROPERTIES OBJCXX_VISIBILITY_PRESET "${value}")
endfunction()

function(qt_set_symbol_visibility_hidden target)
    qt_set_symbol_visibility_preset("${target}" "hidden")
endfunction()

function(qt_set_language_standards)
    ## Use the latest standard the compiler supports (same as qt_common.prf)
    if (QT_FEATURE_cxx2b)
        set(CMAKE_CXX_STANDARD 23 PARENT_SCOPE)
    elseif (QT_FEATURE_cxx20)
        set(CMAKE_CXX_STANDARD 20 PARENT_SCOPE)
    else()
        set(CMAKE_CXX_STANDARD 17 PARENT_SCOPE)
    endif()
    set(CMAKE_CXX_STANDARD_REQUIRED ON PARENT_SCOPE)

    set(CMAKE_C_STANDARD 11 PARENT_SCOPE)
    set(CMAKE_C_STANDARD_REQUIRED ON PARENT_SCOPE)
endfunction()

function(qt_set_language_standards_interface_compile_features target)
    # Regardless of which C++ standard is used to build Qt itself, require C++17 when building
    # Qt applications using CMake (because the Qt header files use C++17 features).
    set(cpp_feature "cxx_std_17")
    target_compile_features("${target}" INTERFACE ${cpp_feature})
endfunction()

function(qt_set_msvc_cplusplus_options target visibility)
    # For MSVC we need to explicitly pass -Zc:__cplusplus to get correct __cplusplus.
    # Check qt_config_compile_test for more info.
    if(MSVC AND MSVC_VERSION GREATER_EQUAL 1913)
        set(flags "-Zc:__cplusplus" "-permissive-")
        target_compile_options("${target}" ${visibility}
            "$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<COMPILE_LANGUAGE:CXX>>:${flags}>")
    endif()
endfunction()

function(qt_enable_utf8_sources target)
    set(utf8_flags "")
    if(MSVC)
        list(APPEND utf8_flags "$<$<CXX_COMPILER_ID:MSVC>:-utf-8>")
    endif()

    if(utf8_flags)
        # Allow opting out by specifying the QT_NO_UTF8_SOURCE target property.
        set(opt_out_condition "$<NOT:$<BOOL:$<TARGET_PROPERTY:QT_NO_UTF8_SOURCE>>>")
        # Only set the compiler option for C and C++.
        set(language_condition "$<COMPILE_LANGUAGE:C,CXX>")
        # Compose the full condition.
        set(genex_condition "$<AND:${opt_out_condition},${language_condition}>")
        set(utf8_flags "$<${genex_condition}:${utf8_flags}>")
        target_compile_options("${target}" INTERFACE "${utf8_flags}")
    endif()
endfunction()

function(qt_internal_enable_unicode_defines)
    if(WIN32)
        set(no_unicode_condition
            "$<NOT:$<BOOL:$<TARGET_PROPERTY:QT_NO_UNICODE_DEFINES>>>")
        target_compile_definitions(Platform
            INTERFACE "$<${no_unicode_condition}:UNICODE;_UNICODE>")
    endif()
endfunction()

# Saves the list of known optimization flags for the current compiler in out_var.
#
# Mostly used for removing them before adding new ones.
function(qt_internal_get_all_possible_optimization_flag_values out_var)
    set(flag_values "")
    set(vars QT_CFLAGS_OPTIMIZE QT_CFLAGS_OPTIMIZE_FULL
             QT_CFLAGS_OPTIMIZE_DEBUG QT_CFLAGS_OPTIMIZE_SIZE)
    foreach(optimize_var ${vars})
        set(value "${${optimize_var}}")
        if(value)
            list(APPEND flag_values "${value}")
        endif()
    endforeach()

    # Additional flag values which might not be used in qmake mkspecs, but might be set by CMake,
    # aka flags that are recognized by the compile which we might want to remove.
    if(QT_CFLAGS_OPTIMIZE_VALID_VALUES)
        list(APPEND flag_values ${QT_CFLAGS_OPTIMIZE_VALID_VALUES})
    endif()

    set("${out_var}" "${flag_values}" PARENT_SCOPE)
endfunction()

# Return's the current compiler's optimize_full flags if available.
# Otherwise returns the regular optimization level flag.
function(qt_internal_get_optimize_full_flags out_var)
    set(optimize_full_flags "${QT_CFLAGS_OPTIMIZE_FULL}")
    if(NOT optimize_full_flags)
        set(optimize_full_flags "${QT_CFLAGS_OPTIMIZE}")
    endif()
    set(${out_var} "${optimize_full_flags}" PARENT_SCOPE)
endfunction()

# Prints the compiler and linker flags for each configuration, language and target type.
#
# Usually it would print the cache variables, but one may also override the variables
# in a specific directory scope, so this is useful for debugging.
#
# Basically dumps either scoped or cached
# CMAKE_<LANG>_FLAGS_CONFIG> and CMAKE_<TYPE>_LINKER_FLAGS_<CONFIG> variables.

function(qt_internal_print_optimization_flags_values)
    qt_internal_get_enabled_languages_for_flag_manipulation(languages)
    qt_internal_get_configs_for_flag_manipulation(configs)
    qt_internal_get_target_link_types_for_flag_manipulation(target_link_types)

    qt_internal_print_optimization_flags_values_helper(
        "${languages}" "${configs}" "${target_link_types}")
endfunction()

# Helper function for printing the optimization flags.
function(qt_internal_print_optimization_flags_values_helper languages configs target_link_types)
    foreach(lang ${languages})
        set(flag_var_name "CMAKE_${lang}_FLAGS")
        message(STATUS "${flag_var_name}: ${${flag_var_name}}")

        foreach(config ${configs})
            set(flag_var_name "CMAKE_${lang}_FLAGS_${config}")
            message(STATUS "${flag_var_name}: ${${flag_var_name}}")
        endforeach()
    endforeach()

    foreach(t ${target_link_types})
        set(flag_var_name "CMAKE_${t}_LINKER_FLAGS")
        message(STATUS "${flag_var_name}: ${${flag_var_name}}")

        foreach(config ${configs})
            set(flag_var_name "CMAKE_${t}_LINKER_FLAGS_${config}")
            message(STATUS "${flag_var_name}: ${${flag_var_name}}")
        endforeach()
    endforeach()
endfunction()

# Saves the list of configs for which flag manipulation will occur.
function(qt_internal_get_configs_for_flag_manipulation out_var)
    set(configs RELEASE RELWITHDEBINFO MINSIZEREL DEBUG)

    # Opt into additional non-standard configs for flag removal only.
    if(QT_ADDITIONAL_OPTIMIZATION_FLAG_CONFIGS)
        list(APPEND configs ${QT_ADDITIONAL_OPTIMIZATION_FLAG_CONFIGS})
    endif()

    set(${out_var} "${configs}" PARENT_SCOPE)
endfunction()

# Saves the list of target link types for which flag manipulation will occur.
function(qt_internal_get_target_link_types_for_flag_manipulation out_var)
    set(target_link_types EXE SHARED MODULE STATIC)
    set(${out_var} "${target_link_types}" PARENT_SCOPE)
endfunction()

# Saves list of enabled languages for which it is safe to manipulate compilation flags.
function(qt_internal_get_enabled_languages_for_flag_manipulation out_var)
    # Limit flag modification to c-like code. We don't want to accidentally add incompatible
    # flags to MSVC's RC or Swift.
    set(languages_to_process ASM C CXX OBJC OBJCXX)
    get_property(globally_enabled_languages GLOBAL PROPERTY ENABLED_LANGUAGES)
    set(enabled_languages "")
    foreach(lang ${languages_to_process})
        if(lang IN_LIST globally_enabled_languages)
            list(APPEND enabled_languages "${lang}")
        endif()
    endforeach()
    set(${out_var} "${enabled_languages}" PARENT_SCOPE)
endfunction()

# Helper function used to update compiler and linker flags further below
function(qt_internal_replace_flags_impl flag_var_name match_string replace_string IN_CACHE)
    # This must come before cache variable modification because setting the
    # cache variable with FORCE will overwrite the non-cache variable, but
    # we need to use the original value on entry to this function.

    # Handle an empty input string and an empty match string as a set().
    if(match_string STREQUAL "" AND "${${flag_var_name}}" STREQUAL "")
        set(${flag_var_name} "${replace_string}" PARENT_SCOPE)
    else()
        string(REPLACE
               "${match_string}" "${replace_string}"
               ${flag_var_name} "${${flag_var_name}}")
        string(STRIP "${${flag_var_name}}" ${flag_var_name})
        set(${flag_var_name} "${${flag_var_name}}" PARENT_SCOPE)
    endif()

    if(IN_CACHE)
        # We must not use the non-cache variable's value because toolchain files
        # might be appending things to the cache variable's value and storing it
        # in a non-cache variable (e.g. Android NDK toolchain file does this).
        # Work exclusively on cache variable value only.
        get_property(help_text CACHE "${flag_var_name}" PROPERTY HELPSTRING)

        # Handle an empty input string and an empty match string as a set().
        if(match_string STREQUAL "" AND "$CACHE{${flag_var_name}}" STREQUAL "")
            set(${flag_var_name} "${replace_string}" CACHE STRING "${help_text}" FORCE)
        else()
            set(mod_flags "$CACHE{${flag_var_name}}")
            string(REPLACE
                   "${match_string}" "${replace_string}"
                   mod_flags "${mod_flags}")
            string(STRIP "${mod_flags}" mod_flags)
            set(${flag_var_name} "${mod_flags}" CACHE STRING "${help_text}" FORCE)
        endif()
    endif()
endfunction()

# Helper function used to update compiler and linker flags further below
function(qt_internal_remove_flags_impl flag_var_name flag_values IN_CACHE)
    cmake_parse_arguments(arg "REGEX" "" "" ${ARGN})
    set(replace_type REPLACE)
    if(arg_REGEX)
        list(PREPEND replace_type REGEX)
    endif()

    # This must come before cache variable modification because setting the
    # cache variable with FORCE will overwrite the non-cache variable in this
    # function scope, but we need to use the original value before that change.
    foreach(flag_value IN LISTS flag_values)
        string(${replace_type} "${flag_value}" " " ${flag_var_name} "${${flag_var_name}}")
    endforeach()
    string(STRIP "${${flag_var_name}}" ${flag_var_name})
    set(${flag_var_name} "${${flag_var_name}}" PARENT_SCOPE)

    if(IN_CACHE)
        # We must not use the non-cache variable's value because toolchain files
        # might be appending things to the cache variable's value and storing it
        # in a non-cache variable (e.g. Android NDK toolchain file does this).
        # Work exclusively on cache variable value only.
        set(mod_flags $CACHE{${flag_var_name}})
        foreach(flag_value IN LISTS flag_values)
            string(${replace_type} "${flag_value}" " " mod_flags "${mod_flags}")
        endforeach()
        string(STRIP "${mod_flags}" mod_flags)
        get_property(help_text CACHE ${flag_var_name} PROPERTY HELPSTRING)
        set(${flag_var_name} "${mod_flags}" CACHE STRING "${help_text}" FORCE)
    endif()
endfunction()

# Helper function used to update compiler and linker flags further below
function(qt_internal_add_flags_impl flag_var_name flags IN_CACHE)
    # This must come before cache variable modification because setting the
    # cache variable with FORCE will overwrite the non-cache variable, but
    # we need to use the original value on entry to this function.
    set(${flag_var_name} "${${flag_var_name}} ${flags}")
    string(STRIP "${${flag_var_name}}" ${flag_var_name})
    set(${flag_var_name} "${${flag_var_name}}" PARENT_SCOPE)

    if(IN_CACHE)
        # We must not use the non-cache variable's value because toolchain files
        # might be appending things to the cache variable's value and storing it
        # in a non-cache variable (e.g. Android NDK toolchain file does this).
        # Work exclusively on cache variable value only.
        set(mod_flags "$CACHE{${flag_var_name}} ${flags}")
        string(STRIP "${mod_flags}" mod_flags)
        get_property(help_text CACHE ${flag_var_name} PROPERTY HELPSTRING)
        set(${flag_var_name} "${mod_flags}" CACHE STRING "${help_text}" FORCE)
    endif()
endfunction()


# Removes all known compiler optimization flags for the given CONFIGS, for all enabled 'safe'
# languages. The flag variables are always updated in the calling scope, even if they did not
# exist beforehand.
#
# IN_CACHE  - remove them from the corresponding cache variable too. Note that the cache
#             variable may have a different value to the non-cache variable.
# CONFIGS   - should be a list of upper case configs like DEBUG, RELEASE, RELWITHDEBINFO.
# LANGUAGES - optional list of languages like 'C', 'CXX', for which to remove the flags
#             if not provided, defaults to the list of enabled C-like languages
function(qt_internal_remove_known_optimization_flags)
    cmake_parse_arguments(PARSE_ARGV 0 arg
        "IN_CACHE"
        ""
        "CONFIGS;LANGUAGES")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_CONFIGS)
        message(FATAL_ERROR
            "You must specify at least one configuration for which to remove the flags.")
    endif()

    if(arg_LANGUAGES)
        set(enabled_languages "${arg_LANGUAGES}")
    else()
        qt_internal_get_enabled_languages_for_flag_manipulation(enabled_languages)
    endif()

    qt_internal_get_all_possible_optimization_flag_values(flag_values)
    set(configs ${arg_CONFIGS})

    foreach(lang ${enabled_languages})
        foreach(config ${configs})
            set(flag_var_name "CMAKE_${lang}_FLAGS_${config}")
            qt_internal_remove_flags_impl(${flag_var_name} "${flag_values}" "${arg_IN_CACHE}")
            set(${flag_var_name} "${${flag_var_name}}" PARENT_SCOPE)
        endforeach()
    endforeach()
endfunction()

# Removes specified flags from CMAKE_<LANGUAGES>_FLAGS[_CONFIGS] variables
#
# Option Arguments:
#   IN_CACHE
#       Enables flags removal from CACHE
#   REGEX
#       Enables the flag processing as a regular expression.
#
# Multi-value Arguments:
#   CONFIGS
#       List of configurations that need to clear flags. Clears all configs by default if not
#       specified.
#
#   LANGUAGES
#       List of LANGUAGES that need clear flags. Clears all languages by default if not
#       specified.
function(qt_internal_remove_compiler_flags flags)
    cmake_parse_arguments(PARSE_ARGV 1 arg
        "IN_CACHE;REGEX"
        ""
        "CONFIGS;LANGUAGES"
    )
    _qt_internal_validate_all_args_are_parsed(arg)

    if("${flags}" STREQUAL "")
        message(WARNING "qt_internal_remove_compiler_flags was called without any flags specified.")
        return()
    endif()

    if(arg_LANGUAGES)
        set(languages "${arg_LANGUAGES}")
    else()
        qt_internal_get_enabled_languages_for_flag_manipulation(languages)
    endif()

    if(arg_CONFIGS)
        set(configs "${arg_CONFIGS}")
    else()
        qt_internal_get_configs_for_flag_manipulation(configs)
    endif()

    if(arg_REGEX)
        list(APPEND extra_options "REGEX")
    endif()

    foreach(lang ${languages})
        set(flag_var_name "CMAKE_${lang}_FLAGS")
        qt_internal_remove_flags_impl(${flag_var_name}
            "${flags}"
            "${arg_IN_CACHE}"
            ${extra_options}
        )
        set(${flag_var_name} "${${flag_var_name}}" PARENT_SCOPE)
        foreach(config ${configs})
            set(flag_var_name "CMAKE_${lang}_FLAGS_${config}")
            qt_internal_remove_flags_impl(${flag_var_name}
                "${flags}"
                "${arg_IN_CACHE}"
                ${extra_options}
            )
            set(${flag_var_name} "${${flag_var_name}}" PARENT_SCOPE)
        endforeach()
    endforeach()
endfunction()

# Adds compiler flags for the given CONFIGS in the calling scope. Can also update the cache
# if asked to do so. The flag variables are always updated in the calling scope, even if they
# did not exist beforehand.
#
# FLAGS     - should be a single string of flags separated by spaces.
# IN_CACHE  - add them to the corresponding cache variable too. Note that the cache
#             variable may have a different value to the non-cache variable.
# CONFIGS   - should be a list of upper case configs like DEBUG, RELEASE, RELWITHDEBINFO.
# LANGUAGES - optional list of languages like 'C', 'CXX', for which to add the flags
#             if not provided, defaults to the list of enabled C-like languages
function(qt_internal_add_compiler_flags)
    cmake_parse_arguments(PARSE_ARGV 0 arg
        "IN_CACHE"
        "FLAGS"
        "CONFIGS;LANGUAGES")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_CONFIGS)
        message(FATAL_ERROR
            "You must specify at least one configuration for which to add the flags.")
    endif()
    if(NOT arg_FLAGS)
        message(FATAL_ERROR "You must specify at least one flag to add.")
    endif()

    if(arg_LANGUAGES)
        set(enabled_languages "${arg_LANGUAGES}")
    else()
        qt_internal_get_enabled_languages_for_flag_manipulation(enabled_languages)
    endif()

    set(configs ${arg_CONFIGS})

    foreach(lang ${enabled_languages})
        foreach(config ${configs})
            set(flag_var_name "CMAKE_${lang}_FLAGS_${config}")
            qt_internal_add_flags_impl(${flag_var_name} "${arg_FLAGS}" "${arg_IN_CACHE}")
            set(${flag_var_name} "${${flag_var_name}}" PARENT_SCOPE)
        endforeach()
    endforeach()
endfunction()

# Convenience function that adds compiler flags for all release configurations.
# The flag variables are always updated in the calling scope, even if they did not
# exist beforehand.
#
# FLAGS     - should be a single string of flags separated by spaces.
# IN_CACHE  - add them to the corresponding cache variable too. Note that the cache
#             variable may have a different value to the non-cache variable.
# LANGUAGES - optional list of languages like 'C', 'CXX', for which to add the flags
#             if not provided, defaults to the list of enabled C-like languages
function(qt_internal_add_compiler_flags_for_release_configs)
    cmake_parse_arguments(PARSE_ARGV 0 arg
        "IN_CACHE"
        "FLAGS"
        "LANGUAGES")
    _qt_internal_validate_all_args_are_parsed(arg)

    set(args "")

    if(arg_LANGUAGES)
        set(enabled_languages "${arg_LANGUAGES}")
    else()
        qt_internal_get_enabled_languages_for_flag_manipulation(enabled_languages)
    endif()

    set(configs RELEASE RELWITHDEBINFO MINSIZEREL)

    list(APPEND args CONFIGS ${configs})

    if(arg_FLAGS)
        list(APPEND args FLAGS "${arg_FLAGS}")
    endif()

    if(arg_IN_CACHE)
        list(APPEND args IN_CACHE)
    endif()
    list(APPEND args LANGUAGES ${enabled_languages})

    qt_internal_add_compiler_flags(${args})

    foreach(lang ${enabled_languages})
        foreach(config ${configs})
            set(flag_var_name "CMAKE_${lang}_FLAGS_${config}")
            set("${flag_var_name}" "${${flag_var_name}}" PARENT_SCOPE)
        endforeach()
    endforeach()
endfunction()

# Convenience function that replaces all optimization flags with the equivalent of '-O3'
# (optimize_full) flag for all release configs.
#
# This is the equivalent of qmake's CONFIG += optimize_full.
# It is meant to be called in a subdirectory scope to enable full optimizations for a particular
# Qt module, like Core or Gui.
function(qt_internal_add_optimize_full_flags)
    cmake_parse_arguments(PARSE_ARGV 0 arg
        "IN_CACHE"
        ""
        "")
    _qt_internal_validate_all_args_are_parsed(arg)

    # QT_USE_DEFAULT_CMAKE_OPTIMIZATION_FLAGS disables forced full optimization.
    if(QT_USE_DEFAULT_CMAKE_OPTIMIZATION_FLAGS)
        return()
    endif()

    # Assume that FEATURE_optimize_full has higher priority. But if FEATURE_optimize_full is OFF,
    # flags are set by FEATURE_optimize_size should remain unchanged.
    if(QT_FEATURE_optimize_size AND NOT QT_FEATURE_optimize_full)
        return()
    endif()

    set(args "")
    if(arg_IN_CACHE)
        list(APPEND args IN_CACHE)
    endif()

    qt_internal_get_enabled_languages_for_flag_manipulation(enabled_languages)
    set(configs RELEASE RELWITHDEBINFO)
    if(QT_FEATURE_optimize_full) # Assume that FEATURE_optimize_full has higher priority.
        list(APPEND configs MINSIZEREL)
    endif()

    qt_internal_remove_known_optimization_flags(${args} CONFIGS ${configs})

    # If the respective compiler doesn't have optimize_full flags, use regular optimization flags.
    # Mainly MSVC.
    qt_internal_get_optimize_full_flags(optimize_full_flags)
    list(APPEND args FLAGS "${optimize_full_flags}")

    qt_internal_add_compiler_flags_for_release_configs(${args})

    foreach(lang ${enabled_languages})
        foreach(config ${configs})
            set(flag_var_name "CMAKE_${lang}_FLAGS_${config}")
            set("${flag_var_name}" "${${flag_var_name}}" PARENT_SCOPE)
        endforeach()
    endforeach()
endfunction()

# Convenience function to replace a compiler flag with another one, for the given configurations
# for all enabled 'safe' languages.
# Essentially a glorified string(REPLACE).
# Can be used to remove compiler flags.
# The flag variables are always updated in the calling scope, even if they did not
# exist beforehand.
#
# match_string   - string to match
# replace_string - replacement string
# IN_CACHE       - replace them in the corresponding cache variable too. Note that the cache
#                  variable may have a different value to the non-cache variable.
# CONFIGS        - should be a list of upper case configs like DEBUG, RELEASE, RELWITHDEBINFO.
# LANGUAGES      - optional list of languages like 'C', 'CXX', for which to replace the flags
#                  if not provided, defaults to the list of enabled C-like languages
function(qt_internal_replace_compiler_flags match_string replace_string)
    cmake_parse_arguments(PARSE_ARGV 2 arg
        "IN_CACHE"
        ""
        "CONFIGS;LANGUAGES")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_CONFIGS)
        message(FATAL_ERROR
            "You must specify at least one configuration for which to replace the flags.")
    endif()

    if(arg_LANGUAGES)
        set(enabled_languages "${arg_LANGUAGES}")
    else()
        qt_internal_get_enabled_languages_for_flag_manipulation(enabled_languages)
    endif()
    set(configs ${arg_CONFIGS})

    foreach(lang ${enabled_languages})
        foreach(config ${configs})
            set(flag_var_name "CMAKE_${lang}_FLAGS_${config}")
            qt_internal_replace_flags_impl(${flag_var_name}
                "${match_string}" "${replace_string}" "${arg_IN_CACHE}")
            set(${flag_var_name} "${${flag_var_name}}" PARENT_SCOPE)
        endforeach()
    endforeach()
endfunction()

# Convenience function to add linker flags, for the given configurations and target link types.
# The flag variables are always updated in the calling scope, even if they did not exist beforehand.
#
# FLAGS    - should be a single string of flags separated by spaces.
# IN_CACHE - add them to the corresponding cache variable too. Note that the cache
#            variable may have a different value to the non-cache variable.
# CONFIGS  - should be a list of upper case configs like DEBUG, RELEASE, RELWITHDEBINFO.
# TYPES    - should be a list of target link types as expected by CMake's
#            CMAKE_<LINKER_TYPE>_LINKER_FLAGS_<CONFIG> cache variable.
#            e.g EXE, MODULE, SHARED, STATIC.
function(qt_internal_add_linker_flags)
    cmake_parse_arguments(PARSE_ARGV 0 arg
        "IN_CACHE"
        "FLAGS"
        "CONFIGS;TYPES")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_TYPES)
        message(FATAL_ERROR
            "You must specify at least one linker target type for which to add the flags.")
    endif()
    if(NOT arg_CONFIGS)
        message(FATAL_ERROR
            "You must specify at least one configuration for which to add the flags.")
    endif()
    if(NOT arg_FLAGS)
        message(FATAL_ERROR "You must specify at least one flag to add.")
    endif()

    set(configs ${arg_CONFIGS})
    set(target_link_types ${arg_TYPES})

    foreach(config ${configs})
        foreach(t ${target_link_types})
            set(flag_var_name "CMAKE_${t}_LINKER_FLAGS_${config}")
            qt_internal_add_flags_impl(${flag_var_name} "${arg_FLAGS}" "${arg_IN_CACHE}")
            set(${flag_var_name} "${${flag_var_name}}" PARENT_SCOPE)
        endforeach()
    endforeach()
endfunction()

# Convenience function to replace a linker flag with another one, for the given configurations
# and target link types.
# Essentially a glorified string(REPLACE).
# Can be used to remove linker flags.
# The flag variables are always updated in the calling scope, even if they did not exist beforehand.
#
# match_string   - string to match
# replace_string - replacement string
# IN_CACHE       - replace them in the corresponding cache variable too. Note that the cache
#                  variable may have a different value to the non-cache variable.
# CONFIGS        - should be a list of upper case configs like DEBUG, RELEASE, RELWITHDEBINFO.
# TYPES          - should be a list of target link types as expected by CMake's
#                  CMAKE_<LINKER_TYPE>_LINKER_FLAGS_<CONFIG> cache variable.
#                  e.g EXE, MODULE, SHARED, STATIC.
function(qt_internal_replace_linker_flags match_string replace_string)
    cmake_parse_arguments(PARSE_ARGV 2 arg
        "IN_CACHE"
        ""
        "CONFIGS;TYPES")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_TYPES)
        message(FATAL_ERROR
            "You must specify at least one linker target type for which to replace the flags.")
    endif()
    if(NOT arg_CONFIGS)
        message(FATAL_ERROR
            "You must specify at least one configuration for which to replace the flags.")
    endif()

    set(configs ${arg_CONFIGS})
    set(target_link_types ${arg_TYPES})

    foreach(config ${configs})
        foreach(t ${target_link_types})
            set(flag_var_name "CMAKE_${t}_LINKER_FLAGS_${config}")
            qt_internal_replace_flags_impl(${flag_var_name}
                "${match_string}" "${replace_string}" "${arg_IN_CACHE}")
            set(${flag_var_name} "${${flag_var_name}}" PARENT_SCOPE)
        endforeach()
    endforeach()
endfunction()

# This function finds the optimization flags set by the default CMake platform modules or toolchain
# files and replaces them with flags that Qt qmake builds expect, for all the usual
# CMAKE_BUILD_TYPE configurations.
# This normalizes things like using -O2 for both Release and RelWithDebInfo, among other compilation
# flags. Also some linker flags specific to MSVC.
# See QTBUG-85992 for details.
#
# Note that both the calling scope and the CMake cache are updated.
function(qt_internal_set_up_config_optimizations_like_in_qmake)
    # Allow opt out.
    if(QT_USE_DEFAULT_CMAKE_OPTIMIZATION_FLAGS)
        return()
    endif()

    qt_internal_get_enabled_languages_for_flag_manipulation(enabled_languages)
    qt_internal_get_configs_for_flag_manipulation(configs)
    qt_internal_get_target_link_types_for_flag_manipulation(target_link_types)

    # You can set QT_DEBUG_OPTIMIZATION_FLAGS to see the before and after results.
    if(QT_DEBUG_OPTIMIZATION_FLAGS)
        message(STATUS "")
        message(STATUS "DEBUG: Original CMake optimization flags.\n")
        qt_internal_print_optimization_flags_values_helper("${enabled_languages}" "${configs}"
                                                           "${target_link_types}")
    endif()

    # Remove known optimization flags.
    qt_internal_remove_known_optimization_flags(IN_CACHE CONFIGS ${configs})

    # Re-add optimization flags as per qmake mkspecs.
    foreach(lang ${enabled_languages})
        foreach(config ${configs})
            set(flag_var_name "CMAKE_${lang}_FLAGS_${config}")
            set(value_to_append "")

            # Release and RelWithDebInfo should get the same base optimization flags.
            if(config STREQUAL "RELEASE" AND QT_CFLAGS_OPTIMIZE)
                set(value_to_append "${QT_CFLAGS_OPTIMIZE}")
            elseif(config STREQUAL "RELWITHDEBINFO" AND QT_CFLAGS_OPTIMIZE)
                set(value_to_append "${QT_CFLAGS_OPTIMIZE}")

            # MinSizeRel should get the optimize size flag if available, otherwise the regular
            # release flag.
            elseif(config STREQUAL "MINSIZEREL")
                if(QT_CFLAGS_OPTIMIZE_SIZE)
                    set(value_to_append "${QT_CFLAGS_OPTIMIZE_SIZE}")
                else()
                    set(value_to_append "${QT_CFLAGS_OPTIMIZE}")
                endif()
            endif()

            # Debug should get the OPTIMIZE_DEBUG flag if the respective feature is ON.
            if(config STREQUAL "DEBUG" AND QT_FEATURE_optimize_debug)
                set(value_to_append "${QT_CFLAGS_OPTIMIZE_DEBUG}")
            endif()

            set(configs_for_optimize_size RELEASE RELWITHDEBINFO)
            if(QT_FEATURE_optimize_size AND config IN_LIST configs_for_optimize_size)
                set(value_to_append "${QT_CFLAGS_OPTIMIZE_SIZE}")
            endif()

            # Check if the fake 'optimize_full' feature is enabled.
            # Use the max optimization level flag for all release configs, effectively
            # overriding any previous setting.
            set(configs_for_optimize RELEASE RELWITHDEBINFO MINSIZEREL)
            if(QT_FEATURE_optimize_full AND config IN_LIST configs_for_optimize)
                qt_internal_get_optimize_full_flags(optimize_full_flags)
                set(value_to_append "${optimize_full_flags}")
            endif()

            # Assign value to the cache entry.
            if(value_to_append)
                qt_internal_add_flags_impl(${flag_var_name} "${value_to_append}" TRUE)
                # Delay updating the calling scope's variables to the end of this function
            endif()

        endforeach()
    endforeach()

    if(MSVC)
        # Handle MSVC /INCREMENTAL flag which should not be enabled for Release configurations.
        # First remove them from all configs, and re-add INCREMENTAL for Debug only.
        set(flag_values "/INCREMENTAL:YES" "/INCREMENTAL:NO" "/INCREMENTAL")
        foreach(flag_value ${flag_values})
            qt_internal_replace_linker_flags(
                    "${flag_value}" ""
                    CONFIGS ${configs}
                    TYPES ${target_link_types}
                    IN_CACHE)
        endforeach()

        set(flag_value "/INCREMENTAL:NO")
        qt_internal_add_linker_flags(
                FLAGS "${flag_value}"
                CONFIGS RELEASE RELWITHDEBINFO MINSIZEREL
                TYPES EXE SHARED MODULE # when linking static libraries, link.exe can't recognize this parameter, clang-cl will error out.
                IN_CACHE)
        qt_internal_remove_compiler_flags("(^| )/EH[scra-]*( |$)" LANGUAGES CXX CONFIGS ${configs} IN_CACHE REGEX)
    endif()

    # Allow opting into generating debug info in object files with a fake feature.
    # This would allow us to enable caching with sccache.
    # See QTQAINFRA-3934 for details.
    if(MSVC AND QT_FEATURE_msvc_obj_debug_info)
        qt_internal_replace_compiler_flags(
                "/Zi" "/Z7"
                CONFIGS RELWITHDEBINFO DEBUG
                TYPES ${target_link_types}
                IN_CACHE)
    endif()

    # Legacy Android toolchain file adds the `-g` flag to CMAKE_<LANG>_FLAGS, as a
    # result, our release build ends up containing debug symbols. To avoid that, we
    # remove the flag from CMAKE_<LANGL>_FLAGS and add
    # it to CMAKE_<LANG>_FLAGS_DEBUG.
    #
    # Note:
    #   The new `android.toolchain.cmake` file does not have this problem, but
    #   it has other issues, eg., https://github.com/android/ndk/issues/1693, so we
    #   cannot force it. While we do load the new toolchain, it automatically falls
    #   back to the legacy toolchain, ie., `android-legacy.toolchain.cmake` which
    #   has the problem described above.
    #
    # Todo:
    #   When the new toolchain is fixed, and it doesn't fall back to the legacy
    #   anymore by default, then we should be able to remove this workaround.
    if(ANDROID AND ANDROID_COMPILER_FLAGS MATCHES "(^| )-g")
        qt_internal_remove_compiler_flags("-g")
        qt_internal_add_compiler_flags(FLAGS "-g" CONFIGS DEBUG RELWITHDEBINFO)
    endif()

    # Update all relevant flags in the calling scope
    foreach(lang ${enabled_languages})
        set(flag_var_name "CMAKE_${lang}_FLAGS")
        set(${flag_var_name} "${${flag_var_name}}" PARENT_SCOPE)

        foreach(config ${configs})
            set(flag_var_name "CMAKE_${lang}_FLAGS_${config}")
            set(${flag_var_name} "${${flag_var_name}}" PARENT_SCOPE)
        endforeach()
    endforeach()

    foreach(t ${target_link_types})
        set(flag_var_name "CMAKE_${t}_LINKER_FLAGS")
        set(${flag_var_name} "${${flag_var_name}}" PARENT_SCOPE)

        foreach(config ${configs})
            set(flag_var_name "CMAKE_${t}_LINKER_FLAGS_${config}")
            set(${flag_var_name} "${${flag_var_name}}" PARENT_SCOPE)
        endforeach()
    endforeach()

    if(QT_DEBUG_OPTIMIZATION_FLAGS)
        message(STATUS "")
        message(STATUS "DEBUG: Modified optimization flags to mirror qmake mkspecs.\n")
        qt_internal_print_optimization_flags_values_helper("${enabled_languages}" "${configs}"
                                                           "${target_link_types}")
    endif()
endfunction()
