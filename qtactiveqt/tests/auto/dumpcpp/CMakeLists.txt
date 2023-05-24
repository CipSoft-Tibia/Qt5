# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_dumpcpp Test:
#####################################################################

qt_internal_add_test(tst_dumpcpp
    SOURCES
        tst_dumpcpp.cpp
    LIBRARIES
        Qt::AxContainer
        Qt::Gui
        Qt::Widgets
)

qt6_target_typelibs(tst_dumpcpp LIBRARIES "ieframe.dll")
qt6_target_typelibs(tst_dumpcpp LIBRARIES "msxml6.dll")
