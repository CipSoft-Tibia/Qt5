// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QHTTPSERVERRESPONDER_P_H
#define QHTTPSERVERRESPONDER_P_H

#include <QtHttpServer/qthttpserverglobal.h>
#include <QtHttpServer/qhttpserverrequest.h>
#include <QtHttpServer/qhttpserverresponder.h>

#include <private/qhttpserverstream_p.h>

#include <QtCore/qcoreapplication.h>
#include <QtCore/qpair.h>
#include <QtCore/qpointer.h>
#include <QtCore/qsysinfo.h>

#include <type_traits>

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of QHttpServer. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

QT_BEGIN_NAMESPACE

class QHttpServerResponderPrivate
{
public:
    QHttpServerResponderPrivate(QHttpServerStream *stream) : stream(stream) { }

#if defined(QT_DEBUG)
    const QPointer<QHttpServerStream> stream;
#else
    QHttpServerStream *const stream;
#endif
    bool bodyStarted{false};
};

QT_END_NAMESPACE

#endif // QHTTPSERVERRESPONDER_P_H
