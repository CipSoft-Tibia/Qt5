# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## conversion Test:
#####################################################################

qt_internal_add_test(conversion
    SOURCES
        tst_conversion.cpp
    INCLUDE_DIRECTORIES
        ../../../src/activeqt/shared/
    LIBRARIES
        Qt::AxContainer
        Qt::Gui
        Qt::AxBasePrivate
)
