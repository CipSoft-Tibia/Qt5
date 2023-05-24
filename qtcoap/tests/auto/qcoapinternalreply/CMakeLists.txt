# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qcoapinternalreply Test:
#####################################################################

qt_internal_add_test(qcoapinternalreply
    SOURCES
        tst_qcoapinternalreply.cpp
    LIBRARIES
        Qt::Coap
        Qt::CoapPrivate
        Qt::Network
)
