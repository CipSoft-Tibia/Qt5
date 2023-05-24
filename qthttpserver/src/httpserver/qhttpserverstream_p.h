// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QHTTPSERVERSTREAM_P_H
#define QHTTPSERVERSTREAM_P_H

#include <QtCore/qobject.h>

#include <QtHttpServer/qthttpserverglobal.h>
#include <QtHttpServer/qhttpserverrequest.h>

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

class QTcpSocket;
class QAbstractHttpServer;

class QHttpServerStream : public QObject
{
    Q_OBJECT

    friend class QAbstractHttpServerPrivate;
    friend class QHttpServerResponder;

private:
    QHttpServerStream(QAbstractHttpServer *server, QTcpSocket *socket);

    void write(const QByteArray &data);
    void write(const char *body, qint64 size);

    void responderDestroyed();

    void handleReadyRead();
    void socketDisconnected();

    QAbstractHttpServer *server;
    QTcpSocket *socket;

    QHttpServerRequest request;

    // To avoid destroying the object when socket object is destroyed while
    // a request is still being handled.
    bool handlingRequest = false;
};

QT_END_NAMESPACE

#endif // QHTTPSERVERSTREAM_P_H
