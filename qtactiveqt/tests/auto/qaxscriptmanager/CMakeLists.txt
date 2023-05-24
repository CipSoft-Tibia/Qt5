# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## QAxScriptManager Test:
#####################################################################

qt_internal_add_test(tst_qaxscriptmanager
    SOURCES
        tst_qaxscriptmanager.cpp
    LIBRARIES
        Qt::AxContainer
)
