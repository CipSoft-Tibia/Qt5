// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the Network Access API.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#ifndef QOAUTHHTTPSERVERREPLYHANDLER_P_H
#define QOAUTHHTTPSERVERREPLYHANDLER_P_H

#ifndef QT_NO_HTTP

#include <QtNetworkAuth/qoauthglobal.h>
#include <QtNetworkAuth/qoauthhttpserverreplyhandler.h>

#include <private/qobject_p.h>

#include <QtNetwork/qtcpserver.h>

QT_BEGIN_NAMESPACE

class QOAuthHttpServerReplyHandlerPrivate
{
    Q_DECLARE_PUBLIC(QOAuthHttpServerReplyHandler)

public:
    explicit QOAuthHttpServerReplyHandlerPrivate(QOAuthHttpServerReplyHandler *p);
    ~QOAuthHttpServerReplyHandlerPrivate();

    QTcpServer httpServer;
    QString text;
    QHostAddress listenAddress = QHostAddress::LocalHost;
    QString path;

private:
    void _q_clientConnected();
    void _q_readData(QTcpSocket *socket);
    void _q_answerClient(QTcpSocket *socket, const QUrl &url);

    struct QHttpRequest {
        quint16 port = 0;

        bool readMethod(QTcpSocket *socket);
        bool readUrl(QTcpSocket *socket);
        bool readStatus(QTcpSocket *socket);
        bool readHeader(QTcpSocket *socket);

        enum class State {
            ReadingMethod,
            ReadingUrl,
            ReadingStatus,
            ReadingHeader,
            ReadingBody,
            AllDone
        } state = State::ReadingMethod;
        QByteArray fragment;

        enum class Method {
            Unknown,
            Head,
            Get,
            Put,
            Post,
            Delete,
        } method = Method::Unknown;
        QUrl url;
        QPair<quint8, quint8> version;
        QMap<QByteArray, QByteArray> headers;
    };

    QMap<QTcpSocket *, QHttpRequest> clients;

    QOAuthHttpServerReplyHandler *q_ptr;
};

QT_END_NAMESPACE

#endif // QT_NO_HTTP

#endif // QOAUTHHTTPSERVERREPLYHANDLER_P_H
