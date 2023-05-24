// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QHTTPSERVERRESPONDER_H
#define QHTTPSERVERRESPONDER_H

#include <QtHttpServer/qthttpserverglobal.h>

#include <QtCore/qstringfwd.h>
#include <QtCore/qmetatype.h>

#include <memory>
#include <utility>
#include <initializer_list>

QT_BEGIN_NAMESPACE

class QHttpServerStream;
class QHttpServerRequest;
class QHttpServerResponse;

class QHttpServerResponderPrivate;
class Q_HTTPSERVER_EXPORT QHttpServerResponder final
{
    Q_GADGET
    Q_DECLARE_PRIVATE(QHttpServerResponder)

    friend class QHttpServerStream;

public:
    enum class StatusCode {
        // 1xx: Informational
        Continue = 100,
        SwitchingProtocols,
        Processing,

        // 2xx: Success
        Ok = 200,
        Created,
        Accepted,
        NonAuthoritativeInformation,
        NoContent,
        ResetContent,
        PartialContent,
        MultiStatus,
        AlreadyReported,
        IMUsed = 226,

        // 3xx: Redirection
        MultipleChoices = 300,
        MovedPermanently,
        Found,
        SeeOther,
        NotModified,
        UseProxy,
        // 306: not used, was proposed as "Switch Proxy" but never standardized
        TemporaryRedirect = 307,
        PermanentRedirect,

        // 4xx: Client Error
        BadRequest = 400,
        Unauthorized,
        PaymentRequired,
        Forbidden,
        NotFound,
        MethodNotAllowed,
        NotAcceptable,
        ProxyAuthenticationRequired,
        RequestTimeout,
        Conflict,
        Gone,
        LengthRequired,
        PreconditionFailed,
        PayloadTooLarge,
        UriTooLong,
        UnsupportedMediaType,
        RequestRangeNotSatisfiable,
        ExpectationFailed,
        ImATeapot,
        MisdirectedRequest = 421,
        UnprocessableEntity,
        Locked,
        FailedDependency,
        UpgradeRequired = 426,
        PreconditionRequired = 428,
        TooManyRequests,
        RequestHeaderFieldsTooLarge = 431,
        UnavailableForLegalReasons = 451,

        // 5xx: Server Error
        InternalServerError = 500,
        NotImplemented,
        BadGateway,
        ServiceUnavailable,
        GatewayTimeout,
        HttpVersionNotSupported,
        VariantAlsoNegotiates,
        InsufficientStorage,
        LoopDetected,
        NotExtended = 510,
        NetworkAuthenticationRequired,
        NetworkConnectTimeoutError = 599,
    };
    Q_ENUM(StatusCode)

    using HeaderList = std::initializer_list<std::pair<QByteArray, QByteArray>>;

    QHttpServerResponder(QHttpServerResponder &&other);
    ~QHttpServerResponder();

    void write(QIODevice *data, HeaderList headers, StatusCode status = StatusCode::Ok);

    void write(QIODevice *data, const QByteArray &mimeType, StatusCode status = StatusCode::Ok);

    void write(const QJsonDocument &document,
               HeaderList headers,
               StatusCode status = StatusCode::Ok);

    void write(const QJsonDocument &document,
               StatusCode status = StatusCode::Ok);

    void write(const QByteArray &data,
               HeaderList headers,
               StatusCode status = StatusCode::Ok);

    void write(const QByteArray &data,
               const QByteArray &mimeType,
               StatusCode status = StatusCode::Ok);

    void write(HeaderList headers, StatusCode status = StatusCode::Ok);
    void write(StatusCode status = StatusCode::Ok);


    void writeStatusLine(StatusCode status = StatusCode::Ok);

    void writeHeader(const QByteArray &key, const QByteArray &value);
    void writeHeaders(HeaderList headers);

    void writeBody(const char *body, qint64 size);
    void writeBody(const char *body);
    void writeBody(const QByteArray &body);

    void sendResponse(const QHttpServerResponse &response);

private:
    QHttpServerResponder(QHttpServerStream *stream);

    std::unique_ptr<QHttpServerResponderPrivate> d_ptr;
};

QT_END_NAMESPACE

QT_DECL_METATYPE_EXTERN_TAGGED(QHttpServerResponder::StatusCode, QHttpServerResponder__StatusCode,
                               Q_HTTPSERVER_EXPORT)

#endif // QHTTPSERVERRESPONDER_H
