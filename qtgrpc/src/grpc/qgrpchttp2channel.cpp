// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2019 Alexey Edelev <semlanik@gmail.com>, Viktor Kopp <vifactor@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/QEventLoop>
#include <QtCore/QMetaObject>
#include <QtCore/QTimer>
#include <QtCore/QUrl>
#include <QtGrpc/qabstractgrpcclient.h>
#include <QtGrpc/qgrpccallreply.h>
#include <QtGrpc/qgrpcstream.h>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QtProtobuf/qprotobufserializer.h>
#include <qtgrpcglobal_p.h>

#include <unordered_map>

#include "qgrpchttp2channel.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*!
    \class QGrpcHttp2Channel
    \inmodule QtGrpc

    \brief The QGrpcHttp2Channel class is an HTTP/2 implementation of
    QAbstractGrpcChannel interface.

    Uses \l QGrpcChannelOptions and \l QGrpcCallOptions
    to control the HTTP/2 communication with the server.

    Use \l QGrpcChannelOptions to set the SSL configuration,
    application-specific HTTP/2 headers, and connection timeouts.

    \l QGrpcCallOptions control channel parameters for the
    specific unary call or gRPC stream.

    \sa QGrpcChannelOptions, QGrpcCallOptions, QSslConfiguration
*/

// This QNetworkReply::NetworkError -> QGrpcStatus::StatusCode mapping should be kept in sync
// with original https://github.com/grpc/grpc/blob/master/doc/statuscodes.md
const static std::unordered_map<QNetworkReply::NetworkError, QGrpcStatus::StatusCode>
        StatusCodeMap = {
            { QNetworkReply::ConnectionRefusedError, QGrpcStatus::Unavailable },
            { QNetworkReply::RemoteHostClosedError, QGrpcStatus::Unavailable },
            { QNetworkReply::HostNotFoundError, QGrpcStatus::Unavailable },
            { QNetworkReply::TimeoutError, QGrpcStatus::DeadlineExceeded },
            { QNetworkReply::OperationCanceledError, QGrpcStatus::Unavailable },
            { QNetworkReply::SslHandshakeFailedError, QGrpcStatus::PermissionDenied },
            { QNetworkReply::TemporaryNetworkFailureError, QGrpcStatus::Unknown },
            { QNetworkReply::NetworkSessionFailedError, QGrpcStatus::Unavailable },
            { QNetworkReply::BackgroundRequestNotAllowedError, QGrpcStatus::Unknown },
            { QNetworkReply::TooManyRedirectsError, QGrpcStatus::Unavailable },
            { QNetworkReply::InsecureRedirectError, QGrpcStatus::PermissionDenied },
            { QNetworkReply::UnknownNetworkError, QGrpcStatus::Unknown },
            { QNetworkReply::ProxyConnectionRefusedError, QGrpcStatus::Unavailable },
            { QNetworkReply::ProxyConnectionClosedError, QGrpcStatus::Unavailable },
            { QNetworkReply::ProxyNotFoundError, QGrpcStatus::Unavailable },
            { QNetworkReply::ProxyTimeoutError, QGrpcStatus::DeadlineExceeded },
            { QNetworkReply::ProxyAuthenticationRequiredError, QGrpcStatus::Unauthenticated },
            { QNetworkReply::UnknownProxyError, QGrpcStatus::Unknown },
            { QNetworkReply::ContentAccessDenied, QGrpcStatus::PermissionDenied },
            { QNetworkReply::ContentOperationNotPermittedError, QGrpcStatus::PermissionDenied },
            { QNetworkReply::ContentNotFoundError, QGrpcStatus::NotFound },
            { QNetworkReply::AuthenticationRequiredError, QGrpcStatus::PermissionDenied },
            { QNetworkReply::ContentReSendError, QGrpcStatus::DataLoss },
            { QNetworkReply::ContentConflictError, QGrpcStatus::InvalidArgument },
            { QNetworkReply::ContentGoneError, QGrpcStatus::DataLoss },
            { QNetworkReply::UnknownContentError, QGrpcStatus::Unknown },
            { QNetworkReply::ProtocolUnknownError, QGrpcStatus::Unknown },
            { QNetworkReply::ProtocolInvalidOperationError, QGrpcStatus::Unimplemented },
            { QNetworkReply::ProtocolFailure, QGrpcStatus::Unknown },
            { QNetworkReply::InternalServerError, QGrpcStatus::Internal },
            { QNetworkReply::OperationNotImplementedError, QGrpcStatus::Unimplemented },
            { QNetworkReply::ServiceUnavailableError, QGrpcStatus::Unavailable },
            { QNetworkReply::UnknownServerError, QGrpcStatus::Unknown }
        };

constexpr char GrpcAcceptEncodingHeader[] = "grpc-accept-encoding";
constexpr char AcceptEncodingHeader[] = "accept-encoding";
constexpr char TEHeader[] = "te";
constexpr char GrpcStatusHeader[] = "grpc-status";
constexpr char GrpcStatusMessageHeader[] = "grpc-message";
constexpr qsizetype GrpcMessageSizeHeaderSize = 5;

static void addMetadataToRequest(QNetworkRequest *request, const QGrpcMetadata &channelMetadata,
                                 const QGrpcMetadata &callMetadata)
{
    auto iterateMetadata = [&request](const auto &metadata) {
        for (const auto &[key, value] : std::as_const(metadata)) {
            request->setRawHeader(key, value);
        }
    };

    iterateMetadata(channelMetadata);
    iterateMetadata(callMetadata);
}

static QGrpcMetadata collectMetadata(QNetworkReply *networkReply)
{
    return QGrpcMetadata(networkReply->rawHeaderPairs().begin(),
                         networkReply->rawHeaderPairs().end());
}

static std::optional<std::chrono::milliseconds> deadlineForCall(
        const QGrpcChannelOptions &channelOptions, const QGrpcCallOptions &callOptions)
{
    if (callOptions.deadline())
        return *callOptions.deadline();
    if (channelOptions.deadline())
        return *channelOptions.deadline();
    return std::nullopt;
}

struct QGrpcHttp2ChannelPrivate
{
    struct ExpectedData
    {
        qsizetype expectedSize;
        QByteArray container;
    };

    QNetworkAccessManager nm;
    QGrpcChannelOptions channelOptions;
#if QT_CONFIG(ssl)
    QSslConfiguration sslConfig;
#endif
    std::unordered_map<QNetworkReply *, ExpectedData> activeStreamReplies;
    QObject lambdaContext;

    QNetworkReply *post(QLatin1StringView method, QLatin1StringView service, QByteArrayView args,
                        const QGrpcCallOptions &callOptions)
    {
        QUrl callUrl = channelOptions.host();
        callUrl.setPath("/%1/%2"_L1.arg(service, method));

        qGrpcDebug() << "Service call url:" << callUrl;
        QNetworkRequest request(callUrl);
        request.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/grpc"_L1));
        request.setRawHeader(GrpcAcceptEncodingHeader, "identity,deflate,gzip");
        request.setRawHeader(AcceptEncodingHeader, "identity,gzip");
        request.setRawHeader(TEHeader, "trailers");
#if QT_CONFIG(ssl)
        request.setSslConfiguration(sslConfig);
#endif

        addMetadataToRequest(&request, channelOptions.metadata(), callOptions.metadata());

        request.setAttribute(QNetworkRequest::Http2DirectAttribute, true);

        QByteArray msg(GrpcMessageSizeHeaderSize, '\0');
        // Args must be 4-byte unsigned int to fit into 4-byte big endian
        qToBigEndian(static_cast<quint32>(args.size()), msg.data() + 1);
        msg += args;
        qGrpcDebug() << "SEND msg with size:" << msg.size();

        QNetworkReply *networkReply = nm.post(request, msg);
#if QT_CONFIG(ssl)
        QObject::connect(networkReply, &QNetworkReply::sslErrors, networkReply,
                         [networkReply](const QList<QSslError> &errors) {
                             qGrpcCritical() << errors;
                             // TODO: filter out noncritical SSL handshake errors
                             // FIXME: error due to ssl failure is not transferred to the client:
                             // last error will be Operation canceled
                             QGrpcHttp2ChannelPrivate::abortNetworkReply(networkReply);
                         });
#endif
        if (auto deadline = deadlineForCall(channelOptions, callOptions)) {
            QTimer::singleShot(*deadline, networkReply, [networkReply] {
                QGrpcHttp2ChannelPrivate::abortNetworkReply(networkReply);
            });
        }
        return networkReply;
    }

    static void abortNetworkReply(QNetworkReply *networkReply)
    {
        if (networkReply->isRunning())
            networkReply->abort();
        else
            networkReply->deleteLater();
    }

    static QByteArray processReply(QNetworkReply *networkReply, QGrpcStatus::StatusCode &statusCode)
    {
        // Check if no network error occurred
        if (networkReply->error() != QNetworkReply::NoError) {
            statusCode = StatusCodeMap.at(networkReply->error());
            return {};
        }

        // Check if server answer with error
        statusCode = static_cast<QGrpcStatus::StatusCode>(
                networkReply->rawHeader(GrpcStatusHeader).toInt());
        if (statusCode != QGrpcStatus::StatusCode::Ok)
            return {};

        // Message size doesn't matter for now
        return networkReply->readAll().mid(GrpcMessageSizeHeaderSize);
    }

    QGrpcHttp2ChannelPrivate(const QGrpcChannelOptions &options) : channelOptions(options)
    {
#if QT_CONFIG(ssl)
        if (channelOptions.host().scheme() == "https"_L1) {
            // HTTPS connection requested but not ssl configuration provided.
            Q_ASSERT(channelOptions.sslConfiguration());
            sslConfig = *channelOptions.sslConfiguration();
        } else if (channelOptions.host().scheme().isEmpty()) {
            auto tmpHost = channelOptions.host();
            tmpHost.setScheme("http"_L1);
            channelOptions.withHost(tmpHost);
        }
#else
        auto tmpHost = channelOptions.host();
        tmpHost.setScheme("http"_L1);
        channelOptions.withHost(tmpHost);
#endif
    }

    static int getExpectedDataSize(QByteArrayView container)
    {
        return qFromBigEndian(*reinterpret_cast<const quint32 *>(container.data() + 1))
                + GrpcMessageSizeHeaderSize;
    }
};

/*!
    Constructs QGrpcHttp2Channel with \a options.
*/
QGrpcHttp2Channel::QGrpcHttp2Channel(const QGrpcChannelOptions &options)
    : QAbstractGrpcChannel(), dPtr(std::make_unique<QGrpcHttp2ChannelPrivate>(options))
{
}

/*!
    Destroys the QGrpcHttp2Channel object.
*/
QGrpcHttp2Channel::~QGrpcHttp2Channel() = default;

/*!
    Synchronously calls the RPC method and writes the result to the output parameter \a ret.

    The RPC method name is constructed by concatenating the \a method
    and \a service parameters and called with the \a args argument.
    Uses \a options argument to set additional parameter for the call.
*/
QGrpcStatus QGrpcHttp2Channel::call(QLatin1StringView method, QLatin1StringView service,
                                    QByteArrayView args, QByteArray &ret,
                                    const QGrpcCallOptions &options)
{
    QEventLoop loop;

    QNetworkReply *networkReply = dPtr->post(method, service, args, options);
    QObject::connect(networkReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

    // If reply was finished in same stack it doesn't make sense to start event loop
    if (!networkReply->isFinished())
        loop.exec();

    QGrpcStatus::StatusCode grpcStatus = QGrpcStatus::StatusCode::Unknown;
    ret = dPtr->processReply(networkReply, grpcStatus);

    networkReply->deleteLater();
    qGrpcDebug() << __func__ << "RECV:" << ret.toHex() << "grpcStatus" << grpcStatus;
    return { grpcStatus, QString::fromUtf8(networkReply->rawHeader(GrpcStatusMessageHeader)) };
}

/*!
    Asynchronously calls the RPC method.

    The RPC method name is constructed by concatenating the \a method
    and \a service parameters and called with the \a args argument.
    Uses \a options argument to set additional parameter for the call.
    The method can emit QGrpcCallReply::finished() and QGrpcCallReply::errorOccurred()
    signals on a QGrpcCallReply returned object.
*/
std::shared_ptr<QGrpcCallReply> QGrpcHttp2Channel::call(QLatin1StringView method,
                                                        QLatin1StringView service,
                                                        QByteArrayView args,
                                                        const QGrpcCallOptions &options)
{
    std::shared_ptr<QGrpcCallReply> reply(new QGrpcCallReply(serializer()),
                                          [](QGrpcCallReply *reply) { reply->deleteLater(); });

    QNetworkReply *networkReply = dPtr->post(method, service, args, options);

    auto connection = std::make_shared<QMetaObject::Connection>();
    auto abortConnection = std::make_shared<QMetaObject::Connection>();

    *connection = QObject::connect(
            networkReply, &QNetworkReply::finished, reply.get(),
            [reply, networkReply, connection, abortConnection] {
                QGrpcStatus::StatusCode grpcStatus = QGrpcStatus::StatusCode::Unknown;
                QByteArray data = QGrpcHttp2ChannelPrivate::processReply(networkReply, grpcStatus);
                reply->setMetadata(collectMetadata(networkReply));
                QObject::disconnect(*connection);
                QObject::disconnect(*abortConnection);

                qGrpcDebug() << "RECV:" << data;
                if (QGrpcStatus::StatusCode::Ok == grpcStatus) {
                    reply->setData(data);
                    emit reply->finished();
                } else {
                    reply->setData({});
                    emit reply->errorOccurred({ grpcStatus,
                                                QLatin1StringView(networkReply->rawHeader(
                                                        GrpcStatusMessageHeader)) });
                }
                networkReply->deleteLater();
            });

    *abortConnection = QObject::connect(reply.get(), &QGrpcCallReply::errorOccurred, networkReply,
                                        [networkReply, connection,
                                         abortConnection](const QGrpcStatus &status) {
                                            if (status.code() == QGrpcStatus::Aborted) {
                                                QObject::disconnect(*connection);
                                                QObject::disconnect(*abortConnection);

                                                networkReply->deleteLater();
                                            }
                                        });
    return reply;
}

/*!
    Creates and starts a stream to the RPC method.

    The RPC method name is constructed by concatenating the \a method
    and \a service parameters and called with the \a arg argument.
    Returns a shared pointer to the QGrpcStream. Uses \a options argument
    to set additional parameter for the stream.

    Calls QGrpcStream::updateData() when the stream receives data from the server.
    The method may emit QGrpcStream::errorOccurred() when the stream has terminated with an error.
*/
std::shared_ptr<QGrpcStream> QGrpcHttp2Channel::startStream(QLatin1StringView method,
                                                            QLatin1StringView service,
                                                            QByteArrayView arg,
                                                            const QGrpcCallOptions &options)
{
    QNetworkReply *networkReply = dPtr->post(method, service, arg, options);

    std::shared_ptr<QGrpcStream> grpcStream(new QGrpcStream(method, arg, serializer()));
    auto finishConnection = std::make_shared<QMetaObject::Connection>();
    auto abortConnection = std::make_shared<QMetaObject::Connection>();
    auto readConnection = std::make_shared<QMetaObject::Connection>();

    *readConnection = QObject::connect(
            networkReply, &QNetworkReply::readyRead, grpcStream.get(),
            [networkReply, grpcStream, this] {
                auto replyIt = dPtr->activeStreamReplies.find(networkReply);

                const QByteArray data = networkReply->readAll();
                qGrpcDebug() << "RECV data size:" << data.size();

                if (replyIt == dPtr->activeStreamReplies.end()) {
                    qGrpcDebug() << data.toHex();
                    int expectedDataSize = QGrpcHttp2ChannelPrivate::getExpectedDataSize(data);
                    qGrpcDebug() << "First chunk received:" << data.size()
                                 << "expectedDataSize:" << expectedDataSize;

                    if (expectedDataSize == 0) {
                        grpcStream->updateData(QByteArray());
                        return;
                    }

                    QGrpcHttp2ChannelPrivate::ExpectedData dataContainer{ expectedDataSize,
                                                                          QByteArray{} };
                    replyIt = dPtr->activeStreamReplies.insert({ networkReply, dataContainer })
                                      .first;
                }

                QGrpcHttp2ChannelPrivate::ExpectedData &dataContainer = replyIt->second;
                dataContainer.container.append(data);

                qGrpcDebug() << "Processeded chunk:" << data.size()
                             << "dataContainer:" << dataContainer.container.size()
                             << "capacity:" << dataContainer.expectedSize;
                while (dataContainer.container.size() >= dataContainer.expectedSize
                       && !networkReply->isFinished()) {
                    qGrpcDebug() << "Full data received:" << data.size()
                                 << "dataContainer:" << dataContainer.container.size()
                                 << "capacity:" << dataContainer.expectedSize;
                    grpcStream->setMetadata(collectMetadata(networkReply));
                    grpcStream->updateData(dataContainer.container.mid(
                            GrpcMessageSizeHeaderSize,
                            dataContainer.expectedSize - GrpcMessageSizeHeaderSize));
                    dataContainer.container.remove(0, dataContainer.expectedSize);
                    if (dataContainer.container.size() > GrpcMessageSizeHeaderSize) {
                        dataContainer.expectedSize = QGrpcHttp2ChannelPrivate::getExpectedDataSize(
                                dataContainer.container);
                    } else if (dataContainer.container.size() > 0) {
                        qGrpcWarning("Invalid container size received, size header is less than 5 "
                                     "bytes");
                    }
                }

                if (dataContainer.container.size() < GrpcMessageSizeHeaderSize
                    || networkReply->isFinished()) {
                    dPtr->activeStreamReplies.erase(replyIt);
                }
            });

    *finishConnection = QObject::connect(
            networkReply, &QNetworkReply::finished, grpcStream.get(),
            [grpcStream, service, networkReply, abortConnection, readConnection,
             finishConnection, this]() {
                QObject::disconnect(*finishConnection);
                QObject::disconnect(*readConnection);
                QObject::disconnect(*abortConnection);

                dPtr->activeStreamReplies.erase(networkReply);
                QGrpcHttp2ChannelPrivate::abortNetworkReply(networkReply);
                networkReply->deleteLater();

                const QString errorString = networkReply->errorString();
                const QNetworkReply::NetworkError networkError = networkReply->error();
                qGrpcWarning() << grpcStream->method() << "call" << service
                               << "stream finished:" << errorString;
                grpcStream->setMetadata(collectMetadata(networkReply));
                switch (networkError) {
                case QNetworkReply::NoError: {
                    // Reply is closed without network error, but may contain an unhandled data
                    // TODO: processReply returns the data, that might need the processing. It's
                    // should be taken into account in new HTTP/2 channel implementation.
                    QGrpcStatus::StatusCode grpcStatus;
                    QGrpcHttp2ChannelPrivate::processReply(networkReply, grpcStatus);
                    if (grpcStatus != QGrpcStatus::StatusCode::Ok) {
                        emit grpcStream->errorOccurred(
                                QGrpcStatus{ grpcStatus,
                                             QLatin1StringView(networkReply->rawHeader(
                                                     GrpcStatusMessageHeader)) });
                    }
                    break;
                }
                default:
                    emit grpcStream->errorOccurred(
                            QGrpcStatus{ StatusCodeMap.at(networkError),
                                         "%1 call %2 stream failed: %3"_L1.arg(
                                                 service, grpcStream->method(), errorString) });
                    break;
                }
                emit grpcStream->finished();
            });

    *abortConnection = QObject::connect(
            grpcStream.get(), &QGrpcStream::finished, networkReply,
            [networkReply, finishConnection, abortConnection, readConnection] {
                QObject::disconnect(*finishConnection);
                QObject::disconnect(*readConnection);
                QObject::disconnect(*abortConnection);

                QGrpcHttp2ChannelPrivate::abortNetworkReply(networkReply);
                networkReply->deleteLater();
            });

    return grpcStream;
}

/*!
    Returns the newly created QProtobufSerializer shared pointer.
*/
std::shared_ptr<QAbstractProtobufSerializer> QGrpcHttp2Channel::serializer() const
{
    // TODO: make selection based on credentials or channel settings
    return std::make_shared<QProtobufSerializer>();
}

QT_END_NAMESPACE
