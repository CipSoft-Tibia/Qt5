# Populates $out_module_list with all subdirectories that have a CMakeLists.txt file
function(qt_internal_find_modules out_module_list)
    set(module_list "")
    file(GLOB directories LIST_DIRECTORIES true RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" *)
    foreach(directory IN LISTS directories)
        if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${directory}"
           AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${directory}/CMakeLists.txt")
            list(APPEND module_list "${directory}")
        endif()
    endforeach()
    message(DEBUG "qt_internal_find_modules: ${module_list}")
    set(${out_module_list} "${module_list}" PARENT_SCOPE)
endfunction()

# poor man's yaml parser, populating $out_dependencies with all dependencies
# in the $depends_file
# Each entry will be in the format dependency/sha1/required
function(qt_internal_parse_dependencies depends_file out_dependencies)
    file(STRINGS "${depends_file}" lines)
    set(eof_marker "---EOF---")
    list(APPEND lines "${eof_marker}")
    set(required_default TRUE)
    set(dependencies "")
    set(dependency "")
    set(revision "")
    set(required "${required_default}")
    foreach(line IN LISTS lines)
        if(line MATCHES "^  (.+):$" OR line STREQUAL "${eof_marker}")
            # Found a repo entry or end of file. Add the last seen dependency.
            if(NOT dependency STREQUAL "")
                if(revision STREQUAL "")
                    message(FATAL_ERROR "Format error in ${depends_file} - ${dependency} does not specify revision!")
                endif()
                list(APPEND dependencies "${dependency}/${revision}/${required}")
            endif()
            # Remember the current dependency
            if(NOT line STREQUAL "${eof_marker}")
                set(dependency "${CMAKE_MATCH_1}")
                set(revision "")
                set(required "${required_default}")
                # dependencies are specified with relative path to this module
                string(REPLACE "../" "" dependency ${dependency})
            endif()
        elseif(line MATCHES "^    ref: (.+)$")
            set(revision "${CMAKE_MATCH_1}")
        elseif(line MATCHES "^    required: (.+)$")
            string(TOUPPER "${CMAKE_MATCH_1}" required)
        endif()
    endforeach()
    message(DEBUG
        "qt_internal_parse_dependencies for ${depends_file}\n    dependencies: ${dependencies}")
    set(${out_dependencies} "${dependencies}" PARENT_SCOPE)
endfunction()

# Helper macro for qt_internal_resolve_module_dependencies.
macro(qt_internal_resolve_module_dependencies_set_skipped value)
    if(DEFINED arg_SKIPPED_VAR)
        set(${arg_SKIPPED_VAR} ${value} PARENT_SCOPE)
    endif()
endmacro()

# Strips tqtc- prefix from a repo name.
function(qt_internal_normalize_repo_name repo_name out_var)
    string(REGEX REPLACE "^tqtc-" "" normalized "${repo_name}")
    set(${out_var} "${normalized}" PARENT_SCOPE)
endfunction()

# Checks if a directory with the given repo name exists in the current
# source / working directory. If it doesn't, it strips the tqtc- prefix.
function(qt_internal_use_normalized_repo_name_if_needed repo_name out_var)
    set(base_dir "${CMAKE_CURRENT_SOURCE_DIR}")
    set(repo_dir "${base_dir}/${repo_name}")
    if(NOT IS_DIRECTORY "${repo_dir}")
        qt_internal_normalize_repo_name("${repo_name}" repo_name)
    endif()
    set(${out_var} "${repo_name}" PARENT_SCOPE)
endfunction()


# Resolve the dependencies of the given module.
# "Module" in the sense of Qt repository.
#
# Side effects: Sets the global properties QT_DEPS_FOR_${module} and QT_REQUIRED_DEPS_FOR_${module}
# with the direct (required) dependencies of module.
#
#
# Positional arguments:
#
# module is the Qt repository.
#
# out_ordered is where the result is stored. This is a list of all dependencies, including
# transitive ones, in topologically sorted order. Note that ${module} itself is also part of
# out_ordered.
#
# out_revisions is a list of git commit IDs for each of the dependencies in ${out_ordered}. This
# list has the same length as ${out_ordered}.
#
#
# Keyword arguments:
#
# PARSED_DEPENDENCIES is a list of dependencies of module in the format that
# qt_internal_parse_dependencies returns. If this argument is not provided, dependencies.yaml of the
# module is parsed.
#
# IN_RECURSION is an internal option that is set when the function is in recursion.
#
# REVISION is an internal value with the git commit ID that belongs to ${module}.
#
# SKIPPED_VAR is an output variable name that is set to TRUE if the module was skipped, to FALSE
# otherwise.
#
# NORMALIZE_REPO_NAME_IF_NEEDED Will remove 'tqtc-' from the beginning of submodule dependencies
# if a tqtc- named directory does not exist.
function(qt_internal_resolve_module_dependencies module out_ordered out_revisions)
    set(options IN_RECURSION NORMALIZE_REPO_NAME_IF_NEEDED)
    set(oneValueArgs REVISION SKIPPED_VAR)
    set(multiValueArgs PARSED_DEPENDENCIES)
    cmake_parse_arguments(arg "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Clear the property that stores the repositories we've already seen.
    if(NOT arg_IN_RECURSION)
        set_property(GLOBAL PROPERTY _qt_internal_seen_repos)
    endif()

    # Bail out if we've seen the module already.
    qt_internal_resolve_module_dependencies_set_skipped(FALSE)
    get_property(seen GLOBAL PROPERTY _qt_internal_seen_repos)
    if(module IN_LIST seen)
        qt_internal_resolve_module_dependencies_set_skipped(TRUE)
        return()
    endif()

    set_property(GLOBAL APPEND PROPERTY _qt_internal_seen_repos ${module})

    # Set a default REVISION.
    if("${arg_REVISION}" STREQUAL "")
        set(arg_REVISION HEAD)
    endif()

    # Retrieve the dependencies.
    if(DEFINED arg_PARSED_DEPENDENCIES)
        set(dependencies "${arg_PARSED_DEPENDENCIES}")
    else()
        set(depends_file "${CMAKE_CURRENT_SOURCE_DIR}/${module}/dependencies.yaml")
        set(dependencies "")
        if(EXISTS "${depends_file}")
            qt_internal_parse_dependencies("${depends_file}" dependencies)
        endif()
    endif()

    # Traverse the dependencies.
    set(ordered)
    set(revisions)
    foreach(dependency IN LISTS dependencies)
        if(dependency MATCHES "(.*)/([^/]+)/([^/]+)")
            set(dependency "${CMAKE_MATCH_1}")
            set(revision "${CMAKE_MATCH_2}")
            set(required "${CMAKE_MATCH_3}")
        else()
            message(FATAL_ERROR "Internal Error: wrong dependency format ${dependency}")
        endif()

        set(normalize_arg "")
        if(arg_NORMALIZE_REPO_NAME_IF_NEEDED)
            qt_internal_use_normalized_repo_name_if_needed("${dependency}" dependency)
            set(normalize_arg "NORMALIZE_REPO_NAME_IF_NEEDED")
        endif()

        set_property(GLOBAL APPEND PROPERTY QT_DEPS_FOR_${module} ${dependency})
        if(required)
            set_property(GLOBAL APPEND PROPERTY QT_REQUIRED_DEPS_FOR_${module} ${dependency})
        endif()

        qt_internal_resolve_module_dependencies(${dependency} dep_ordered dep_revisions
            REVISION "${revision}"
            SKIPPED_VAR skipped
            IN_RECURSION
            ${normalize_arg}
        )
        if(NOT skipped)
            list(APPEND ordered ${dep_ordered})
            list(APPEND revisions ${dep_revisions})
        endif()
    endforeach()

    list(APPEND ordered ${module})
    list(APPEND revisions ${arg_REVISION})
    set(${out_ordered} "${ordered}" PARENT_SCOPE)
    set(${out_revisions} "${revisions}" PARENT_SCOPE)
endfunction()

# Resolves the dependencies of the given modules.
# "Module" is here used in the sense of Qt repository.
#
# Returns all dependencies, including transitive ones, in topologically sorted order.
#
# Arguments:
# modules is the initial list of repos.
# out_all_ordered is the variable name where the result is stored.
#
# See qt_internal_resolve_module_dependencies for side effects.
function(qt_internal_sort_module_dependencies modules out_all_ordered)

    # Create a fake repository "all_selected_repos" that has all repositories from the input as
    # required dependency. The format must match what qt_internal_parse_dependencies produces.
    set(all_selected_repos_as_parsed_dependencies)
    foreach(module IN LISTS modules)
        list(APPEND all_selected_repos_as_parsed_dependencies "${module}/HEAD/FALSE")
    endforeach()

    qt_internal_resolve_module_dependencies(all_selected_repos ordered unused_revisions
        PARSED_DEPENDENCIES ${all_selected_repos_as_parsed_dependencies}
        NORMALIZE_REPO_NAME_IF_NEEDED
    )

    # Drop "all_selected_repos" from the output. It depends on all selected repos, thus it must be
    # the last element in the topologically sorted list.
    list(REMOVE_AT ordered -1)

    message(DEBUG
        "qt_internal_sort_module_dependencies
    input modules: ${modules}\n    topo-sorted:   ${ordered}")
    set(${out_all_ordered} "${ordered}" PARENT_SCOPE)
endfunction()

# does what it says, but also updates submodules
function(qt_internal_checkout module revision)
    set(swallow_output "") # unless VERBOSE, eat git output, show it in case of error
    if (NOT VERBOSE)
        list(APPEND swallow_output "OUTPUT_VARIABLE" "git_output" "ERROR_VARIABLE" "git_output")
    endif()
    message(NOTICE "Checking '${module}' out to revision '${revision}'")
    execute_process(
        COMMAND "git" "checkout" "${revision}"
        WORKING_DIRECTORY "./${module}"
        RESULT_VARIABLE git_result
        ${swallow_output}
    )
    if (git_result EQUAL 128)
        message(WARNING "${git_output}, trying detached checkout")
        execute_process(
            COMMAND "git" "checkout" "--detach" "${revision}"
            WORKING_DIRECTORY "./${module}"
            RESULT_VARIABLE git_result
            ${swallow_output}
        )
    endif()
    if (git_result)
        message(FATAL_ERROR "Failed to check '${module}' out to '${revision}': ${git_output}")
    endif()
    execute_process(
        COMMAND "git" "submodule" "update"
        WORKING_DIRECTORY "./${module}"
        RESULT_VARIABLE git_result
        OUTPUT_VARIABLE git_stdout
        ERROR_VARIABLE git_stderr
    )
endfunction()

# clones or creates a worktree for $dependency, using the source of $dependent
function(qt_internal_get_dependency dependent dependency)
    set(swallow_output "") # unless VERBOSE, eat git output, show it in case of error
    if (NOT VERBOSE)
        list(APPEND swallow_output "OUTPUT_VARIABLE" "git_output" "ERROR_VARIABLE" "git_output")
    endif()

    set(gitdir "")
    set(remote "")

    # try to read the worktree source
    execute_process(
        COMMAND "git" "rev-parse" "--git-dir"
        WORKING_DIRECTORY "./${dependent}"
        RESULT_VARIABLE git_result
        OUTPUT_VARIABLE git_stdout
        ERROR_VARIABLE git_stderr
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    string(FIND "${git_stdout}" "${module}" index)
    string(SUBSTRING "${git_stdout}" 0 ${index} gitdir)
    string(FIND "${gitdir}" ".git/modules" index)
    if(index GREATER -1) # submodules have not been absorbed
        string(SUBSTRING "${gitdir}" 0 ${index} gitdir)
    endif()
    message(DEBUG "Will look for clones in ${gitdir}")

    execute_process(
        COMMAND "git" "remote" "get-url" "origin"
        WORKING_DIRECTORY "./${dependent}"
        RESULT_VARIABLE git_result
        OUTPUT_VARIABLE git_stdout
        ERROR_VARIABLE git_stderr
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    string(FIND "${git_stdout}" "${dependent}.git" index)
    string(SUBSTRING "${git_stdout}" 0 ${index} remote)
    message(DEBUG "Will clone from ${remote}")

    if(EXISTS "${gitdir}.gitmodules" AND NOT EXISTS "${gitdir}${dependency}/.git")
        # super repo exists, but the submodule we need does not - try to initialize
        message(NOTICE "Initializing submodule '${dependency}' from ${gitdir}")
        execute_process(
            COMMAND "git" "submodule" "update" "--init" "${dependency}"
            WORKING_DIRECTORY "${gitdir}"
            RESULT_VARIABLE git_result
            ${swallow_output}
        )
        if (git_result)
            # ignore errors, fall back to an independent clone instead
            message(WARNING "Failed to initialize submodule '${dependency}' from ${gitdir}")
        endif()
    endif()

    if(EXISTS "${gitdir}${dependency}")
        # for the module we want, there seems to be a clone parallel to what we have
        message(NOTICE "Adding worktree for ${dependency} from ${gitdir}${dependency}")
        execute_process(
            COMMAND "git" "worktree" "add" "--detach" "${CMAKE_CURRENT_SOURCE_DIR}/${dependency}"
            WORKING_DIRECTORY "${gitdir}/${dependency}"
            RESULT_VARIABLE git_result
            ${swallow_output}
        )
        if (git_result)
            message(FATAL_ERROR "Failed to check '${module}' out to '${revision}': ${git_output}")
        endif()
    else()
        # we don't find the existing clone, so clone from the same remote
        message(NOTICE "Cloning ${dependency} from ${remote}${dependency}.git")
        execute_process(
            COMMAND "git" "clone" "${remote}${dependency}.git"
            WORKING_DIRECTORY "."
            RESULT_VARIABLE git_result
            ${swallow_output}
        )
        if (git_result)
            message(FATAL_ERROR "Failed to check '${module}' out to '${revision}': ${git_output}")
        endif()
    endif()
endfunction()

# evaluates the dependencies for $module, and checks all dependencies
# out so that it is a consistent set
function(qt_internal_sync_to module)
    if(ARGN)
        set(revision "${ARGV1}")
        # special casing "." as the target module - checkout all out to $revision
        if("${module}" STREQUAL ".")
            qt_internal_find_modules(modules)
            foreach(module IN LISTS modules)
                qt_internal_checkout("${module}" "${revision}")
            endforeach()
            return()
        endif()
    else()
        set(revision "HEAD")
    endif()
    qt_internal_checkout("${module}" "${revision}")

    qt_internal_resolve_module_dependencies(${module} initial_dependencies initial_revisions)
    if(initial_dependencies)
        foreach(dependency ${initial_dependencies})
            if(dependency MATCHES "^tqtc-")
                message(WARNING
                    "Handling of tqtc- repos will likely fail. Fixing this is non-trivial.")
                break()
            endif()
        endforeach()
    endif()

    set(revision "")
    set(checkedout "1")
    # Load all dependencies for $module, then iterate over the dependencies in reverse order,
    # and check out the first that isn't already at the required revision.
    # Repeat everything (we need to reload dependencies after each checkout) until no more checkouts
    # are done.
    while(${checkedout})
        qt_internal_resolve_module_dependencies(${module} dependencies revisions)
        message(DEBUG "${module} dependencies: ${dependencies}")
        message(DEBUG "${module} revisions   : ${revisions}")

        list(LENGTH dependencies count)
        if (count EQUAL "0")
            message(NOTICE "Module ${module} has no dependencies")
            return()
        endif()

        math(EXPR count "${count} - 1")
        set(checkedout 0)
        foreach(i RANGE ${count} 0 -1 )
            list(GET dependencies ${i} dependency)
            list(GET revisions ${i} revision)
            if ("${revision}" STREQUAL "HEAD")
                message(DEBUG "Not changing checked out revision of ${dependency}")
                continue()
            endif()

            if(NOT EXISTS "./${dependency}")
                message(DEBUG "No worktree for '${dependency}' found in '${CMAKE_CURRENT_SOURCE_DIR}'")
                qt_internal_get_dependency("${module}" "${dependency}")
            endif()

            execute_process(
                COMMAND "git" "rev-parse" "HEAD"
                WORKING_DIRECTORY "./${dependency}"
                RESULT_VARIABLE git_result
                OUTPUT_VARIABLE git_stdout
                ERROR_VARIABLE git_stderr
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )
            if (git_result)
                message(WARNING "${git_stdout}")
                message(FATAL_ERROR "Failed to get current HEAD of '${dependency}': ${git_stderr}")
            endif()
            if ("${git_stdout}" STREQUAL "${revision}")
                continue()
            endif()

            qt_internal_checkout("${dependency}" "${revision}")
            set(checkedout 1)
            break()
        endforeach()
    endwhile()
endfunction()

# Runs user specified command for all qt repositories in qt directory.
# Similar to git submodule foreach, except without relying on .gitmodules existing.
# Useful for worktree checkouts.
function(qt_internal_foreach_repo_run)
    cmake_parse_arguments(PARSE_ARGV 0 arg
                          ""
                          ""
                          "ARGS"
    )
    if(NOT arg_ARGS)
        message(FATAL_ERROR "No arguments specified to qt_internal_foreach_repo_run")
    endif()
    separate_arguments(args NATIVE_COMMAND "${arg_ARGS}")

    # Find the qt repos
    qt_internal_find_modules(modules)

    # Hack to support color output on unix systems
    # https://stackoverflow.com/questions/18968979/how-to-make-colorized-message-with-cmake
    execute_process(COMMAND
        /usr/bin/tty
        OUTPUT_VARIABLE tty_name
        RESULT_VARIABLE tty_exit_code
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    set(color_supported FALSE)
    set(output_goes_where "")
    if(NOT tty_exit_CODE AND tty_name)
        set(color_supported TRUE)
        set(output_goes_where "OUTPUT_FILE" "${tty_name}")
    endif()

    # Count successes and failures.
    set(count_success "0")
    set(count_failure "0")

    # Show colored error markers.
    set(color "--normal")
    if(color_supported)
        set(color "--red")
    endif()

    foreach(module IN LISTS modules)
        message("Entering '${module}'")
        execute_process(
            COMMAND ${args}
            WORKING_DIRECTORY "${module}"
            ${output_goes_where}
            RESULT_VARIABLE cmd_result
        )
        if(cmd_result)
            math(EXPR count_failure "${count_failure}+1")
            # cmake_echo_color is undocumented, but lets us output colors and control newlines.
            execute_process(
                COMMAND
                ${CMAKE_COMMAND} -E env CLICOLOR_FORCE=1
                ${CMAKE_COMMAND} -E cmake_echo_color "${color}"
                "Process execution failed here ^^^^^^^^^^^^^^^^^^^^"
            )
        else()
            math(EXPR count_success "${count_success}+1")
        endif()
    endforeach()

    # Show summary with colors.
    set(color "--normal")
    if(count_failure AND color_supported)
        set(color "--red")
    endif()

    message("\nSummary\n=======\n")
    execute_process(
        COMMAND
            ${CMAKE_COMMAND} -E cmake_echo_color --normal --no-newline "Failures: "
    )
    execute_process(
        COMMAND
            ${CMAKE_COMMAND} -E env CLICOLOR_FORCE=1
            ${CMAKE_COMMAND} -E cmake_echo_color "${color}" "${count_failure}"
    )
    message("Successes: ${count_success}")
endfunction()
