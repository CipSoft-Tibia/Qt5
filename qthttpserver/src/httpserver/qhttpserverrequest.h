// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QHTTPSERVERREQUEST_H
#define QHTTPSERVERREQUEST_H

#include <QtHttpServer/qthttpserverglobal.h>

#include <QtCore/qglobal.h>
#include <QtCore/qurl.h>
#include <QtCore/qurlquery.h>
#include <QtNetwork/qhostaddress.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QRegularExpression;
class QString;

class QHttpServerRequestPrivate;
class QHttpServerRequest final
{
    friend class QHttpServerResponse;
    friend class QHttpServerStream;

    Q_GADGET_EXPORT(Q_HTTPSERVER_EXPORT)

public:
    Q_HTTPSERVER_EXPORT ~QHttpServerRequest();

    enum class Method
    {
        Unknown = 0x0000,
        Get     = 0x0001,
        Put     = 0x0002,
        Delete  = 0x0004,
        Post    = 0x0008,
        Head    = 0x0010,
        Options = 0x0020,
        Patch   = 0x0040,
        Connect = 0x0080,
        Trace   = 0x0100,

        AnyKnown = Get | Put | Delete | Post | Head | Options | Patch | Connect | Trace,
    };
    Q_ENUM(Method)
    Q_DECLARE_FLAGS(Methods, Method)
    Q_FLAG(Methods)

    Q_HTTPSERVER_EXPORT QByteArray value(const QByteArray &key) const;
    Q_HTTPSERVER_EXPORT QUrl url() const;
    Q_HTTPSERVER_EXPORT QUrlQuery query() const;
    Q_HTTPSERVER_EXPORT Method method() const;
    Q_HTTPSERVER_EXPORT QList<QPair<QByteArray, QByteArray>> headers() const;
    Q_HTTPSERVER_EXPORT QByteArray body() const;
    Q_HTTPSERVER_EXPORT QHostAddress remoteAddress() const;
    Q_HTTPSERVER_EXPORT quint16 remotePort() const;
    Q_HTTPSERVER_EXPORT QHostAddress localAddress() const;
    Q_HTTPSERVER_EXPORT quint16 localPort() const;

private:
    Q_DISABLE_COPY(QHttpServerRequest)

#if !defined(QT_NO_DEBUG_STREAM)
    friend Q_HTTPSERVER_EXPORT QDebug operator<<(QDebug debug, const QHttpServerRequest &request);
#endif

    Q_HTTPSERVER_EXPORT explicit QHttpServerRequest(const QHostAddress &remoteAddress,
                                                    quint16 remotePort,
                                                    const QHostAddress &localAddress,
                                                    quint16 localPort);

    std::unique_ptr<QHttpServerRequestPrivate> d;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QHttpServerRequest::Methods)

QT_END_NAMESPACE

#endif // QHTTPSERVERREQUEST_H
