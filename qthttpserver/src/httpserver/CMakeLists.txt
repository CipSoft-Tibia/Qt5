# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from httpserver.pro.

#####################################################################
## HttpServer Module:
#####################################################################

qt_internal_add_module(HttpServer
    SOURCES
        qabstracthttpserver.cpp qabstracthttpserver.h qabstracthttpserver_p.h
        qhttpserver.cpp qhttpserver.h qhttpserver_p.h
        qhttpserverliterals.cpp qhttpserverliterals_p.h
        qhttpserverrequest.cpp qhttpserverrequest.h qhttpserverrequest_p.h
        qhttpserverresponder.cpp qhttpserverresponder.h qhttpserverresponder_p.h
        qhttpserverresponse.cpp qhttpserverresponse.h qhttpserverresponse_p.h
        qhttpserverrouter.cpp qhttpserverrouter.h qhttpserverrouter_p.h
        qhttpserverrouterrule.cpp qhttpserverrouterrule.h qhttpserverrouterrule_p.h
        qhttpserverrouterviewtraits.h
        qhttpserverstream.cpp qhttpserverstream_p.h
        qhttpserverviewtraits.h
        qhttpserverviewtraits_impl.h
        qthttpserverglobal.h
    LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Network
    PRIVATE_MODULE_INTERFACE
        Qt::CorePrivate
        Qt::NetworkPrivate
    GENERATE_CPP_EXPORTS
)

## Scopes:
#####################################################################

qt_internal_extend_target(HttpServer CONDITION TARGET Qt::WebSockets
    LIBRARIES
        Qt::WebSocketsPrivate
    PUBLIC_LIBRARIES
        Qt::WebSockets
    PRIVATE_MODULE_INTERFACE
        Qt::WebSocketsPrivate
)

qt_internal_add_docs(HttpServer
                     ../../doc/config/qthttpserver.qdocconf
)
