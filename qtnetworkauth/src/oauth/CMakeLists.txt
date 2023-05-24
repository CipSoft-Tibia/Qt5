# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from oauth.pro.

#####################################################################
## NetworkAuth Module:
#####################################################################

qt_internal_add_module(NetworkAuth
    SOURCES
        qabstractoauth.cpp qabstractoauth.h qabstractoauth_p.h
        qabstractoauth2.cpp qabstractoauth2.h qabstractoauth2_p.h
        qabstractoauthreplyhandler.cpp qabstractoauthreplyhandler.h qabstractoauthreplyhandler_p.h
        qoauth1.cpp qoauth1.h qoauth1_p.h
        qoauth1signature.cpp qoauth1signature.h qoauth1signature_p.h
        qoauth2authorizationcodeflow.cpp qoauth2authorizationcodeflow.h qoauth2authorizationcodeflow_p.h
        qoauthglobal.h
        qoauthhttpserverreplyhandler.cpp qoauthhttpserverreplyhandler.h qoauthhttpserverreplyhandler_p.h
        qoauthoobreplyhandler.cpp qoauthoobreplyhandler.h
    LIBRARIES
        Qt::CorePrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Network
    PRIVATE_MODULE_INTERFACE
        Qt::CorePrivate
)

#### Keys ignored in scope 1:.:.:oauth.pro:<TRUE>:
# MODULE = "networkauth"
qt_internal_add_docs(NetworkAuth
    doc/qtnetworkauth.qdocconf
)

