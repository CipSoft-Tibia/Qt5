// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QHTTPSERVERRESPONSE_H
#define QHTTPSERVERRESPONSE_H

#include <QtHttpServer/qhttpserverresponder.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QJsonObject;

class QHttpServerResponsePrivate;
class Q_HTTPSERVER_EXPORT QHttpServerResponse final
{
    Q_DECLARE_PRIVATE(QHttpServerResponse)
    Q_DISABLE_COPY(QHttpServerResponse)

    friend class QHttpServerResponder;
public:
    using StatusCode = QHttpServerResponder::StatusCode;

    QHttpServerResponse(QHttpServerResponse &&other) noexcept;
    QHttpServerResponse& operator=(QHttpServerResponse &&other) noexcept;

    QHttpServerResponse(const StatusCode statusCode);

    QHttpServerResponse(const char *data, const StatusCode status = StatusCode::Ok);

    QHttpServerResponse(const QString &data, const StatusCode status = StatusCode::Ok);

    explicit QHttpServerResponse(const QByteArray &data, const StatusCode status = StatusCode::Ok);
    explicit QHttpServerResponse(QByteArray &&data, const StatusCode status = StatusCode::Ok);

    QHttpServerResponse(const QJsonObject &data, const StatusCode status = StatusCode::Ok);
    QHttpServerResponse(const QJsonArray &data, const StatusCode status = StatusCode::Ok);

    QHttpServerResponse(const QByteArray &mimeType,
                        const QByteArray &data,
                        const StatusCode status = StatusCode::Ok);
    QHttpServerResponse(QByteArray &&mimeType,
                        const QByteArray &data,
                        const StatusCode status = StatusCode::Ok);
    QHttpServerResponse(const QByteArray &mimeType,
                        QByteArray &&data,
                        const StatusCode status = StatusCode::Ok);
    QHttpServerResponse(QByteArray &&mimeType,
                        QByteArray &&data,
                        const StatusCode status = StatusCode::Ok);

    ~QHttpServerResponse();
    static QHttpServerResponse fromFile(const QString &fileName);

    QByteArray data() const;

    QByteArray mimeType() const;

    StatusCode statusCode() const;

    void addHeader(QByteArray &&name, QByteArray &&value);
    void addHeader(QByteArray &&name, const QByteArray &value);
    void addHeader(const QByteArray &name, QByteArray &&value);
    void addHeader(const QByteArray &name, const QByteArray &value);

    void addHeaders(QHttpServerResponder::HeaderList headers);

    template<typename Container>
    void addHeaders(const Container &headerList)
    {
        for (const auto &header : headerList)
            addHeader(header.first, header.second);
    }

    void clearHeader(const QByteArray &name);
    void clearHeaders();

    void setHeader(QByteArray &&name, QByteArray &&value);
    void setHeader(QByteArray &&name, const QByteArray &value);
    void setHeader(const QByteArray &name, QByteArray &&value);
    void setHeader(const QByteArray &name, const QByteArray &value);

    void setHeaders(QHttpServerResponder::HeaderList headers);

    bool hasHeader(const QByteArray &name) const;
    bool hasHeader(const QByteArray &name, const QByteArray &value) const;

    QList<QByteArray> headers(const QByteArray &name) const;

private:
    std::unique_ptr<QHttpServerResponsePrivate> d_ptr;
};

QT_END_NAMESPACE

#endif   // QHTTPSERVERRESPONSE_H
