# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qcoapoption Test:
#####################################################################

qt_internal_add_test(qcoapoption
    SOURCES
        tst_qcoapoption.cpp
    LIBRARIES
        Qt::Coap
        Qt::CoapPrivate
)
