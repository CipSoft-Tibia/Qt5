# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qstringbuilder1.pro.

#####################################################################
## tst_qstringbuilder1 Test:
#####################################################################

# special case begin
qt_internal_add_test(tst_qt5compat_qstringbuilder1
# special case end
    SOURCES
        tst_qstringbuilder1.cpp
    DEFINES
        QT_USE_FAST_OPERATOR_PLUS
        QT_USE_QSTRINGBUILDER
    LIBRARIES
        Qt::Core5Compat
)
