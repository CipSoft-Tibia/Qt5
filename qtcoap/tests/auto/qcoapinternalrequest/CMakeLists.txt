# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qcoapinternalrequest Test:
#####################################################################

qt_internal_add_test(qcoapinternalrequest
    SOURCES
        tst_qcoapinternalrequest.cpp
    LIBRARIES
        Qt::Coap
        Qt::CoapPrivate
        Qt::Network
)
