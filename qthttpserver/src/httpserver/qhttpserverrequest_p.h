// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QHTTPSERVERREQUEST_P_H
#define QHTTPSERVERREQUEST_P_H

#include <QtHttpServer/qhttpserverrequest.h>
#include <QtNetwork/private/qhttpheaderparser_p.h>
#include <QtCore/private/qbytedata_p.h>

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

class QHttp2Stream;

class QHttpServerRequestPrivate
{
public:
    QHttpServerRequestPrivate(const QHostAddress &remoteAddress, quint16 remotePort,
                              const QHostAddress &localAddress, quint16 localPort);
#if QT_CONFIG(ssl)
    QHttpServerRequestPrivate(const QHostAddress &remoteAddress, quint16 remotePort,
                              const QHostAddress &localAddress, quint16 localPort,
                              const QSslConfiguration &sslConfiguration);
#endif

    quint16 port = 0;

    enum class State {
        NothingDone,
        ReadingRequestLine,
        ReadingHeader,
        ExpectContinue,
        ReadingData,
        AllDone,
    } state = State::NothingDone;

    QUrl url;
    QHttpServerRequest::Method method;
    QHttpHeaderParser parser;

    bool parseRequestLine(QByteArrayView line);
    qsizetype readRequestLine(QIODevice *socket);
    qsizetype readHeader(QIODevice *socket);
    qsizetype sendContinue(QIODevice *socket);
    qsizetype readBodyFast(QIODevice *socket);
    qsizetype readRequestBodyRaw(QIODevice *socket, qsizetype size);
    qsizetype readRequestBodyChunked(QIODevice *socket);
    qsizetype getChunkSize(QIODevice *socket, qsizetype *chunkSize);

    bool parse(QIODevice *socket);
#if QT_CONFIG(http)
    bool parse(QHttp2Stream *socket);
#endif
    void clear();

    qint64 contentLength() const;
    QByteArray headerField(const QByteArray &name) const
    { return parser.combinedHeaderValue(name); }

    QHostAddress remoteAddress;
    quint16 remotePort;
    QHostAddress localAddress;
    quint16 localPort;
#if QT_CONFIG(ssl)
    QSslConfiguration sslConfiguration;
#endif
    bool handling{false};
    qsizetype bodyLength;
    qsizetype contentRead;
    bool chunkedTransferEncoding;
    bool lastChunkRead;
    qsizetype currentChunkRead;
    qsizetype currentChunkSize;
    bool upgrade;

    QByteArray fragment;
    QByteDataBuffer bodyBuffer;
    QByteArray body;
};

QT_END_NAMESPACE

#endif // QHTTPSERVERREQUEST_P_H
