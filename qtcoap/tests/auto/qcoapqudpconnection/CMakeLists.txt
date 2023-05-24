# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qcoapqudpconnection Test:
#####################################################################

set (QT_TESTSERVER_COMPOSE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../../testserver/docker-compose.yml")

qt_internal_add_test(qcoapqudpconnection
    SOURCES
        ../coapnetworksettings.h
        tst_qcoapqudpconnection.cpp
    LIBRARIES
        Qt::Coap
        Qt::CoapPrivate
        Qt::Network
    QT_TEST_SERVER_LIST "californium"
)

## Scopes:
#####################################################################

qt_internal_extend_target(qcoapqudpconnection CONDITION DEFINED ENV{COAP_TEST_SERVER_IP}
    DEFINES
        COAP_TEST_SERVER_IP="$ENV{COAP_TEST_SERVER_IP}"
)
