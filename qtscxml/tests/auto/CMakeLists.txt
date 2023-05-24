# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


add_subdirectory(scxmlcoutput)
if(TARGET Qt::Gui AND TARGET Qt::Qml)
    if(TARGET Qt::Scxml)
        add_subdirectory(compiled)
        add_subdirectory(dynamicmetaobject)
        add_subdirectory(parser)
        add_subdirectory(scion)
        add_subdirectory(statemachine)
        add_subdirectory(statemachineinfo)
        add_subdirectory(cmake)
    endif()
    # More checks inside
    add_subdirectory(qml)
    add_subdirectory(qmltest)
endif()
if(NOT UIKIT AND TARGET Qt::StateMachine)
    add_subdirectory(qstatemachine)
endif()
