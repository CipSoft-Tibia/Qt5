# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qcoaprequest Test:
#####################################################################

qt_internal_add_test(qcoaprequest
    SOURCES
        tst_qcoaprequest.cpp
    LIBRARIES
        Qt::Coap
        Qt::CoapPrivate
        Qt::Network
)
