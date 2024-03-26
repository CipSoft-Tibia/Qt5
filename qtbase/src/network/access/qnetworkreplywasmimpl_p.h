// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QNETWORKREPLYWASMIMPL_H
#define QNETWORKREPLYWASMIMPL_H

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

#include "qnetworkreply.h"
#include "qnetworkreply_p.h"
#include "qnetworkaccessmanager.h"

#include <QtCore/qfile.h>

#include <private/qtnetworkglobal_p.h>
#include <private/qabstractfileengine_p.h>

#include <emscripten.h>
#include <emscripten/fetch.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QIODevice;

class QNetworkReplyWasmImplPrivate;
class QNetworkReplyWasmImpl: public QNetworkReply
{
    Q_OBJECT
public:
    QNetworkReplyWasmImpl(QObject *parent = nullptr);
    ~QNetworkReplyWasmImpl();
    virtual void abort() override;

    // reimplemented from QNetworkReply
    virtual void close() override;
    virtual qint64 bytesAvailable() const override;
    virtual bool isSequential () const override;
    qint64 size() const override;

    virtual qint64 readData(char *data, qint64 maxlen) override;

    void setup(QNetworkAccessManager::Operation op, const QNetworkRequest &request,
               QIODevice *outgoingData);

    Q_DECLARE_PRIVATE(QNetworkReplyWasmImpl)

    Q_PRIVATE_SLOT(d_func(), void emitReplyError(QNetworkReply::NetworkError errorCode, const QString &errorString))
    Q_PRIVATE_SLOT(d_func(), void emitDataReadProgress(qint64 done, qint64 total))
    Q_PRIVATE_SLOT(d_func(), void dataReceived(char *buffer, int bufferSize))

private:
    QByteArray methodName() const;
};

class QNetworkReplyWasmImplPrivate: public QNetworkReplyPrivate
{
public:
    QNetworkReplyWasmImplPrivate();
    ~QNetworkReplyWasmImplPrivate();

    QNetworkAccessManagerPrivate *managerPrivate;
    void doSendRequest();
    static void setReplyAttributes(quintptr data, int statusCode, const QString &statusReason);

    void emitReplyError(QNetworkReply::NetworkError errorCode, const QString &);
    void emitDataReadProgress(qint64 done, qint64 total);
    void dataReceived(const QByteArray &buffer, int bufferSize);
    void headersReceived(const QByteArray &buffer);

    void setStatusCode(int status, const QByteArray &statusText);

    void setup(QNetworkAccessManager::Operation op, const QNetworkRequest &request,
               QIODevice *outgoingData);

    State state;
    void _q_bufferOutgoingData();
    void _q_bufferOutgoingDataFinished();

    std::shared_ptr<QAtomicInt> pendingDownloadData;
    std::shared_ptr<QAtomicInt> pendingDownloadProgress;

    qint64 bytesDownloaded;
    qint64 bytesBuffered;

    qint64 downloadBufferReadPosition;
    qint64 downloadBufferCurrentSize;
    qint64 totalDownloadSize;
    qint64 percentFinished;
    QByteArray downloadBuffer;

    QIODevice *outgoingData;
    std::shared_ptr<QRingBuffer> outgoingDataBuffer;
    QByteArray requestData;

    static void downloadProgress(emscripten_fetch_t *fetch);
    static void downloadFailed(emscripten_fetch_t *fetch);
    static void downloadSucceeded(emscripten_fetch_t *fetch);
    static void stateChange(emscripten_fetch_t *fetch);

    static QNetworkReply::NetworkError statusCodeFromHttp(int httpStatusCode, const QUrl &url);

    emscripten_fetch_t *m_fetch;
    void setReplyFinished();
    void setCanceled();

    Q_DECLARE_PUBLIC(QNetworkReplyWasmImpl)
};

QT_END_NAMESPACE

#endif // QNETWORKREPLYWASMIMPL_H
