// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [gen-includes]
#include "clientguide.qpb.h"
#include "clientguide_client.grpc.qpb.h"
//! [gen-includes]

#include <QtGrpc/QGrpcHttp2Channel>
#include <QtGrpc/qgrpcstream.h>

#include <QtCore/QCommandLineParser>
#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QProcess>
#include <QtCore/QThread>
#include <QtCore/QUrl>

#include <limits>
#include <memory>

// We use part of the namespace to clarify the source.
using namespace client;

void startServerProcess();
QDebug operator<<(QDebug debug, const guide::Response &response);

class ClientGuide : public QObject
{
public:
    //! [basic-1]
    explicit ClientGuide(std::shared_ptr<QAbstractGrpcChannel> channel)
    {
        m_client.attachChannel(std::move(channel));
    }
    //! [basic-1]

    //! [basic-2]
    static guide::Request createRequest(int32_t num, bool fail = false)
    {
        guide::Request request;
        request.setNum(num);
        // The server-side logic fails the RPC if the time is in the future.
        request.setTime(fail ? std::numeric_limits<int64_t>::max()
                             : QDateTime::currentMSecsSinceEpoch());
        return request;
    }
    //! [basic-2]

    //! [unary-0]
    void unaryCall(const guide::Request &request)
    {
        std::unique_ptr<QGrpcCallReply> reply = m_client.UnaryCall(request);
        const auto *replyPtr = reply.get();
        QObject::connect(
            replyPtr, &QGrpcCallReply::finished, replyPtr,
            [reply = std::move(reply)](const QGrpcStatus &status) {
                if (status.isOk()) {
                    if (const auto response = reply->read<guide::Response>())
                        qDebug() << "Client (UnaryCall) finished, received:" << *response;
                    else
                        qDebug("Client (UnaryCall) deserialization failed");
                } else {
                    qDebug() << "Client (UnaryCall) failed:" << status;
                }
            },
            Qt::SingleShotConnection);
    }
    //! [unary-0]

    //! [sstream-0]
    void serverStreaming(const guide::Request &initialRequest)
    {
        std::unique_ptr<QGrpcServerStream> stream = m_client.ServerStreaming(initialRequest);
        const auto *streamPtr = stream.get();

        QObject::connect(
            streamPtr, &QGrpcServerStream::finished, streamPtr,
            [stream = std::move(stream)](const QGrpcStatus &status) {
                if (status.isOk())
                    qDebug("Client (ServerStreaming) finished");
                else
                    qDebug() << "Client (ServerStreaming) failed:" << status;
            },
            Qt::SingleShotConnection);
        //! [sstream-0]
        //! [sstream-1]
        QObject::connect(streamPtr, &QGrpcServerStream::messageReceived, streamPtr, [streamPtr] {
            if (const auto response = streamPtr->read<guide::Response>())
                qDebug() << "Client (ServerStream) received:" << *response;
            else
                qDebug("Client (ServerStream) deserialization failed");
        });
    }
    //! [sstream-1]

    // ! [cstream-0]
    void clientStreaming(const guide::Request &initialRequest)
    {
        m_clientStream = m_client.ClientStreaming(initialRequest);
        for (int32_t i = 1; i < 3; ++i)
            m_clientStream->writeMessage(createRequest(initialRequest.num() + i));
        m_clientStream->writesDone();

        QObject::connect(m_clientStream.get(), &QGrpcClientStream::finished, m_clientStream.get(),
                         [this](const QGrpcStatus &status) {
                             if (status.isOk()) {
                                 if (const auto response = m_clientStream->read<guide::Response>())
                                     qDebug() << "Client (ClientStreaming) finished, received:"
                                              << *response;
                                 m_clientStream.reset();
                             } else {
                                 qDebug() << "Client (ClientStreaming) failed:" << status;
                                 qDebug("Restarting the client stream");
                                 clientStreaming(createRequest(0));
                             }
                         });
    }
    // ! [cstream-0]

    // ! [bstream-1]
    void bidirectionalStreaming(const guide::Request &initialRequest)
    {
        m_bidiStream = m_client.BidirectionalStreaming(initialRequest);
        connect(m_bidiStream.get(), &QGrpcBidiStream::finished, this, &ClientGuide::bidiFinished);
        connect(m_bidiStream.get(), &QGrpcBidiStream::messageReceived, this,
                &ClientGuide::bidiMessageReceived);
    }
    // ! [bstream-1]

private slots:
    // ! [bstream-2]
    void bidiFinished(const QGrpcStatus &status)
    {
        if (status.isOk())
            qDebug("Client (BidirectionalStreaming) finished");
        else
            qDebug() << "Client (BidirectionalStreaming) failed:" << status;
        m_bidiStream.reset();
    }
    // ! [bstream-2]

    // ! [bstream-3]
    void bidiMessageReceived()
    {
        if (m_bidiStream->read(&m_bidiResponse)) {
            qDebug() << "Client (BidirectionalStreaming) received:" << m_bidiResponse;
            if (m_bidiResponse.num() > 0) {
                m_bidiStream->writeMessage(createRequest(m_bidiResponse.num() - 1));
                return;
            }
        } else {
            qDebug("Client (BidirectionalStreaming) deserialization failed");
        }
        m_bidiStream->writesDone();
    }
    // ! [bstream-3]

private:
    guide::ClientGuideService::Client m_client;
    std::unique_ptr<QGrpcClientStream> m_clientStream;
    // ! [bstream-0]
    std::unique_ptr<QGrpcBidiStream> m_bidiStream;
    guide::Response m_bidiResponse;
    // ! [bstream-0]
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // Use the -U, -S, -C, -B options to control execution
    QCommandLineParser parser;
    QCommandLineOption enableUnary("U", "Enable UnaryCalls");
    QCommandLineOption enableSStream("S", "Enable ServerStream");
    QCommandLineOption enableCStream("C", "Enable ClientStream");
    QCommandLineOption enableBStream("B", "Enable BiDiStream");

    parser.addHelpOption();
    parser.addOption(enableUnary);
    parser.addOption(enableSStream);
    parser.addOption(enableCStream);
    parser.addOption(enableBStream);
    parser.process(app);

    bool defaultRun = !parser.isSet(enableUnary) && !parser.isSet(enableSStream)
        && !parser.isSet(enableCStream) && !parser.isSet(enableBStream);

    qDebug("Welcome to the clientguide!");
    qDebug("Starting the server process ...");
    startServerProcess();

    //! [basic-0]
    auto channel = std::make_shared<QGrpcHttp2Channel>(
        QUrl("http://localhost:50056")
        /* without channel options. */
    );
    ClientGuide clientGuide(channel);
    //! [basic-0]

    if (defaultRun || parser.isSet(enableUnary)) {
        //! [unary-1]
        clientGuide.unaryCall(ClientGuide::createRequest(1));
        clientGuide.unaryCall(ClientGuide::createRequest(2, true)); // fail the RPC
        clientGuide.unaryCall(ClientGuide::createRequest(3));
        //! [unary-1]
    }

    if (defaultRun || parser.isSet(enableSStream)) {
        //! [sstream-2]
        clientGuide.serverStreaming(ClientGuide::createRequest(3));
        // ! [sstream-2]
    }

    if (defaultRun || parser.isSet(enableCStream)) {
        // ! [cstream-1]
        clientGuide.clientStreaming(ClientGuide::createRequest(0, true)); // fail the RPC
        // ! [cstream-1]
    }

    if (defaultRun || parser.isSet(enableBStream)) {
        // ! [bstream-4]
        clientGuide.bidirectionalStreaming(ClientGuide::createRequest(3));
        // ! [bstream-4]
    }

    return app.exec();
}

void startServerProcess()
{
    // For the purpose of this example, we launch the server directly from the
    // client. In a real-world scenario, the server should be running
    // independently, and this code would not be necessary. This approach is
    // used here solely for convenience in demonstrating the full interaction.
    static QProcess serverProcess;
    QObject::connect(&serverProcess, &QProcess::readyReadStandardOutput, [] {
        auto msgs = serverProcess.readAll().split('\n');
        msgs.removeIf([](const QByteArray &s) { return s.isEmpty(); });
        for (const auto &m : std::as_const(msgs)) {
            qDebug().noquote().nospace() << "    " << m;
        }
    });
    serverProcess.setProcessChannelMode(QProcess::MergedChannels);
    serverProcess.setReadChannel(QProcess::StandardOutput);
    serverProcess.start(SERVER_PATH);
    if (!serverProcess.waitForStarted()) {
        qFatal() << "Couldn't start the server: " << serverProcess.errorString();
        exit(EXIT_FAILURE);
    }
    // give the process some time to properly start up the server
    QThread::currentThread()->msleep(250);
}

QDebug operator<<(QDebug debug, const guide::Response &response)
{
    return debug << "Response( time: " << response.time() << ", num: " << response.num() << " )";
}
