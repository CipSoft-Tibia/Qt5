// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <proto/bench_client.grpc.qpb.h>
#include <qrpcbench_common.h>

#include <QtGrpc/qgrpchttp2channel.h>

#include <QtCore/qcoreapplication.h>
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qeventloop.h>
#include <QtCore/qobject.h>
#include <QtCore/qtimer.h>

using namespace std::chrono_literals;

class QtGrpcClientBenchmark : public QObject
{
    Q_OBJECT
public:
    explicit QtGrpcClientBenchmark(quint64 calls, qsizetype payload = 0) : mCalls(calls)
    {
        if (payload > 0)
            sData = QByteArray(payload, 'x');

        QUrl uri(QString("http://") + HostUri.data());
        mClient.attachChannel(std::make_shared<QGrpcHttp2Channel>(std::move(uri)));
    }
    ~QtGrpcClientBenchmark() override = default;

    QtGrpcClientBenchmark(const QtGrpcClientBenchmark &) = delete;
    QtGrpcClientBenchmark &operator=(const QtGrpcClientBenchmark &) = delete;

    QtGrpcClientBenchmark(QtGrpcClientBenchmark &&) = delete;
    QtGrpcClientBenchmark &operator=(QtGrpcClientBenchmark &&) = delete;

    void unaryCall();
    void serverStreaming();
    void clientStreaming();
    void bidiStreaming();

private:
    void unaryCallHelper(qt::bench::UnaryCallRequest &request, quint64 &writes);

private:
    qt::bench::BenchmarkService::Client mClient;
    QEventLoop mLoop;
    QElapsedTimer mTimer;
    uint64_t mCalls;

    inline static QByteArray sData;
};

void QtGrpcClientBenchmark::unaryCall()
{
    quint64 writes = 0;
    qt::bench::UnaryCallRequest request;
    unaryCallHelper(request, writes);
    mLoop.exec();
}

void QtGrpcClientBenchmark::unaryCallHelper(qt::bench::UnaryCallRequest &request, quint64 &writes)
{
    // recursively enqueue a message after the previous message finished
    request.setPing(writes);
    auto reply = mClient.UnaryCall(request);
    auto *replyPtr = reply.get();

    auto connection = std::make_shared<QMetaObject::Connection>();
    *connection = QObject::connect(replyPtr, &QGrpcCallReply::finished, &mClient,
                                   [connection, reply = std::move(reply), this, &request,
                                    &writes](const QGrpcStatus &status) {
                                       if (writes == 0)
                                           mTimer.start();
                                       if (status.isOk()) {
                                           if (++writes < mCalls) {
                                               unaryCallHelper(request, writes);
                                           } else {
                                               Client::printRpcResult("UnaryCall",
                                                                      mTimer.nsecsElapsed(),
                                                                      writes);
                                               mLoop.quit();
                                           }
                                       } else {
                                           qDebug() << "FAILED: " << status;
                                           mLoop.quit();
                                       }
                                       QObject::disconnect(*connection);
                                   });
}

void QtGrpcClientBenchmark::serverStreaming()
{
    quint64 counter = 0;
    quint64 recvBytes = 0;

    qt::bench::ServerStreamingRequest request;
    if (!sData.isEmpty())
        request.setPayload(sData);
    request.setPing(mCalls);
    auto stream = mClient.ServerStreaming(request);
    auto *streamPtr = stream.get();

    QObject::connect(streamPtr, &QGrpcServerStream::messageReceived, this,
                     [this, stream = streamPtr, &counter, &recvBytes]() {
                         if (counter == 0)
                             mTimer.start();
                         const auto response = stream->read<qt::bench::ServerStreamingResponse>();
                         if (response->hasPayload())
                             recvBytes += static_cast<quint64>(response->payload().size());
                         ++counter;
                     });

    QObject::connect(streamPtr, &QGrpcServerStream::finished, this,
                     [this, &counter, &recvBytes](const QGrpcStatus &status) {
                         if (status.isOk()) {
                             Client::printRpcResult("ServerStreaming", mTimer.nsecsElapsed(),
                                                    counter, recvBytes, sData.size());
                         } else {
                             qDebug() << "FAILED: " << status;
                         }
                         mLoop.quit();
                     });
    mLoop.exec();
}

void QtGrpcClientBenchmark::clientStreaming()
{
    quint64 counter = 0;
    quint64 sendBytes = 0;

    qt::bench::ClientStreamingRequest request;
    if (!sData.isEmpty())
        request.setPayload(sData);
    request.setPing(10);

    const auto stream = mClient.ClientStreaming(request);

    QTimer::singleShot(0, [this, &stream, &counter, &request, &sendBytes]() {
        // Run on event loop
        mTimer.start();
        for (; counter < mCalls; ++counter) {
            if (request.hasPayload())
                sendBytes += static_cast<quint64>(request.payload().size());
            request.setPing(counter);
            stream->writeMessage(request);
        }
        stream->writesDone();
    });

    QObject::connect(stream.get(), &QGrpcServerStream::finished, this,
                     [this, &stream, &counter, &sendBytes](const QGrpcStatus &status) {
                         if (status.isOk()) {
                             quint64 recvBytes = 0;
                             const auto resp = stream->read<qt::bench::ClientStreamingResponse>();
                             if (resp->hasPayload())
                                recvBytes = static_cast<quint64>(resp->payload().size());
                             Client::printRpcResult("ClientStreaming", mTimer.nsecsElapsed(),
                                                    counter, recvBytes, sendBytes);
                         } else {
                             qDebug() << "FAILED: " << status;
                         }
                         mLoop.quit();
                     });
    mLoop.exec();
}

void QtGrpcClientBenchmark::bidiStreaming()
{
    quint64 counter = 0;
    quint64 recvBytes = 0;
    quint64 sendBytes = 0;

    qt::bench::BiDiStreamingRequest request;
    if (!sData.isEmpty())
        request.setPayload(sData);
    qt::bench::BiDiStreamingResponse response;
    auto stream = mClient.BiDiStreaming(request);
    auto streamPtr = stream.get();

    QObject::connect(streamPtr, &QGrpcBidiStream::messageReceived, this,
                     [this, stream = streamPtr, &counter, &response, &request, &recvBytes,
                      &sendBytes]() {
                         if (counter == 0)
                             mTimer.start();
                         if (stream->read(&response)) {
                             if (counter < mCalls) {
                                 if (response.hasPayload())
                                     recvBytes += static_cast<quint64>(response.payload().size());
                                 request.setPing(counter);
                                 stream->writeMessage(request);
                                 if (request.hasPayload())
                                     sendBytes += static_cast<quint64>(request.payload().size());
                                 ++counter;
                             } else {
                                 stream->writesDone();
                             }
                         } else {
                             qDebug() << "FAILED: read()";
                             mLoop.quit();
                         }
                     });

    QObject::connect(streamPtr, &QGrpcServerStream::finished, this,
                     [this, &counter, &recvBytes, &sendBytes](const QGrpcStatus &status) {
                         if (status.isOk()) {
                             Client::printRpcResult("BidiStreaming", mTimer.nsecsElapsed(), counter,
                                                    recvBytes, sendBytes);
                         } else {
                             qDebug() << "FAILED: " << status;
                         }
                         mLoop.quit();
                     });
    mLoop.exec();
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    Client::benchmarkMain<QtGrpcClientBenchmark>("QtGrpcClient", argc, argv);
}

#include "bench_qtgrpcclient.moc"
