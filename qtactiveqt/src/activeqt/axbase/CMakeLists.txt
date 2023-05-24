# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## AxBase Module:
#####################################################################

qt_internal_add_module(AxBasePrivate
    STATIC
    INTERNAL_MODULE
    SOURCES
        qaxtypefunctions.cpp qaxtypefunctions_p.h
        qaxutils.cpp qaxutils_p.h
    LIBRARIES
        advapi32
        gdi32
        ole32
        oleaut32
        user32
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Widgets
)

## Scopes:
#####################################################################

qt_internal_extend_target(AxBasePrivate CONDITION MINGW
    LIBRARIES
        uuid
)
