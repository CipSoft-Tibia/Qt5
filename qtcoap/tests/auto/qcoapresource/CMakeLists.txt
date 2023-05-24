# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qcoapresource Test:
#####################################################################

qt_internal_add_test(qcoapresource
    SOURCES
        tst_qcoapresource.cpp
    LIBRARIES
        Qt::Coap
        Qt::CoapPrivate
        Qt::Network
)
