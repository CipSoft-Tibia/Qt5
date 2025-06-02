# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qmltestrunner.pro.

#####################################################################
## qmltestrunner Tool:
#####################################################################

qt_get_tool_target_name(target_name qmltestrunner)
qt_internal_add_tool(${target_name}
    TARGET_DESCRIPTION "QML Test Runner"
    TOOLS_TARGET Qml # special case
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::QuickTest
)
qt_internal_return_unless_building_tools()

if(MACOS AND NOT CMAKE_GENERATOR STREQUAL "Xcode")
    # Add com.apple.security.get-task-allow entitlement to runner,
    # so we can hook into the Swift crash handling.
    set(entitlements_file
        "${__qt_internal_cmake_apple_support_files_path}/test.entitlements.plist")
    add_custom_command(TARGET "${target_name}"
        POST_BUILD COMMAND codesign --sign -
            --entitlements "${entitlements_file}"
            "$<TARGET_FILE:${target_name}>"
        )
endif()

#### Keys ignored in scope 1:.:.:qmltestrunner.pro:<TRUE>:
# QMAKE_TARGET_DESCRIPTION = "QML" "Test" "Runner"
