# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

function(qt_internal_multimedia_set_ffmpeg_link_directory directory)
    foreach (lib ${ffmpeg_libs} FFmpeg)
        set_target_properties(${lib} PROPERTIES INTERFACE_LINK_DIRECTORIES ${directory})
    endforeach()
endfunction()

function(qt_internal_multimedia_copy_or_install_ffmpeg)
    if (WIN32)
        set(install_dir ${INSTALL_BINDIR})
    else()
        set(install_dir ${INSTALL_LIBDIR})
    endif()

    if (QT_WILL_INSTALL)
        qt_install(FILES "${FFMPEG_SHARED_LIBRARIES}" DESTINATION ${install_dir})
    else()
        # elseif(NOT WIN32) actually we can just drop the coping for unix platforms
        #                   However, it makes sense to copy anyway for consistency:
        #                   in order to have the same configuration for developer builds.

        set(ffmpeg_output_dir "${QT_BUILD_DIR}/${install_dir}")
        file(MAKE_DIRECTORY ${ffmpeg_output_dir})

        foreach(lib_path ${FFMPEG_SHARED_LIBRARIES})
            get_filename_component(lib_name ${lib_path} NAME)
            if(NOT EXISTS "${ffmpeg_output_dir}/${lib_name}")
                file(COPY ${lib_path} DESTINATION ${ffmpeg_output_dir})
            endif()
        endforeach()

        # On Windows, shared linking goes through 'integration' static libs,
        # otherwise we should link the directory with copied libs
        if (NOT WIN32)
            qt_internal_multimedia_set_ffmpeg_link_directory(${ffmpeg_output_dir})
        endif()
    endif()

    # Should we set the compile definition for the plugin or for the QtMM module?
    # target_compile_definitions(QFFmpegMediaPlugin PRIVATE FFMPEG_DEPLOY_FOLDER="${FFMPEG_DEPLOY_FOLDER}")
endfunction()
