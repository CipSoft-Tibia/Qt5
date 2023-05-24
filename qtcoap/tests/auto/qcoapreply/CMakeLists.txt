# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qcoapreply Test:
#####################################################################

qt_internal_add_test(qcoapreply
    SOURCES
        tst_qcoapreply.cpp
    LIBRARIES
        Qt::Coap
        Qt::CoapPrivate
        Qt::Network
)
