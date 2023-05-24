# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


add_subdirectory(scxml)
add_subdirectory(statemachine)
if(TARGET Qt::Qml)
    add_subdirectory(statemachineqml)
    add_subdirectory(scxmlqml)
endif()
add_subdirectory(plugins)
