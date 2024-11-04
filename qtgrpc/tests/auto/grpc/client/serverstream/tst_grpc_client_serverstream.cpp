// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2019 Alexey Edelev <semlanik@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include <grpcclienttestbase.h>

#include <QtGrpc/QGrpcCallOptions>
#include <QtGrpc/QGrpcClientInterceptorManager>

#include <QtCore/QCryptographicHash>
#include <QtCore/QFile>
#include <QtCore/QThread>
#include <QtCore/QTimer>

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include <interceptormocks.h>
#include <message_latency_defs.h>
#include <testservice_client.grpc.qpb.h>

using namespace Qt::Literals::StringLiterals;
using namespace qtgrpc::tests;

class QtGrpcClientServerStreamTest : public GrpcClientTestBase
{
    Q_OBJECT
public:
    QtGrpcClientServerStreamTest()
        : GrpcClientTestBase(
                Channels{ GrpcClientTestBase::Channel::Qt })
    {
    }

private slots:
    void Valid();
    void Cancel();
    void DeferredCancel();
    void HugeBlob();
    void GetAsyncReply();
    void MultipleStreams();
    void MultipleStreamsCancel();
    void InThread();
    void CancelWhileErrorTimeout();
    void Deadline_data();
    void Deadline();
    void Interceptor();
    void CancelledInterceptor();
    void InterceptResponse();
};

void QtGrpcClientServerStreamTest::Valid()
{
    const int ExpectedMessageCount = 4;

    SimpleStringMessage result;
    SimpleStringMessage request;
    request.setTestFieldString("Stream");

    auto stream = client()->streamTestMethodServerStream(request);

    QSignalSpy messageReceivedSpy(stream.get(), &QGrpcServerStream::messageReceived);
    QVERIFY(messageReceivedSpy.isValid());
    QSignalSpy streamErrorSpy(stream.get(), &QGrpcServerStream::errorOccurred);
    QVERIFY(streamErrorSpy.isValid());

    QSignalSpy streamFinishedSpy(stream.get(), &QGrpcServerStream::finished);
    QVERIFY(streamFinishedSpy.isValid());

    QObject::connect(stream.get(), &QGrpcServerStream::messageReceived, this, [&result, stream] {
        SimpleStringMessage ret = stream->read<SimpleStringMessage>();
        result.setTestFieldString(result.testFieldString() + ret.testFieldString());
    });

    QTRY_COMPARE_EQ_WITH_TIMEOUT(streamFinishedSpy.count(), 1,
                                 MessageLatencyWithThreshold * ExpectedMessageCount);
    QCOMPARE(streamErrorSpy.count(), 0);

    QCOMPARE(messageReceivedSpy.count(), ExpectedMessageCount);
    QCOMPARE_EQ(result.testFieldString(), "Stream1Stream2Stream3Stream4");
}

void QtGrpcClientServerStreamTest::Cancel()
{
    const int ExpectedMessageCount = 2;

    SimpleStringMessage result;
    SimpleStringMessage request;
    request.setTestFieldString("Stream");

    auto stream = client()->streamTestMethodServerStream(request);

    QSignalSpy streamFinishedSpy(stream.get(), &QGrpcServerStream::finished);
    QVERIFY(streamFinishedSpy.isValid());
    QSignalSpy streamErrorSpy(stream.get(), &QGrpcServerStream::errorOccurred);
    QVERIFY(streamErrorSpy.isValid());

    int i = 0;
    QObject::connect(stream.get(), &QGrpcServerStream::messageReceived, this, [&] {
        SimpleStringMessage ret = stream->read<SimpleStringMessage>();
        result.setTestFieldString(result.testFieldString() + ret.testFieldString());
        if (++i == ExpectedMessageCount)
            stream->cancel();
    });

    QTRY_COMPARE_EQ_WITH_TIMEOUT(streamErrorSpy.count(), 1,
                                 MessageLatencyWithThreshold * ExpectedMessageCount);
    QCOMPARE(streamFinishedSpy.count(), 0);
    QCOMPARE(i, 2);
    QCOMPARE_EQ(result.testFieldString(), "Stream1Stream2");
}

void QtGrpcClientServerStreamTest::DeferredCancel()
{
    const int ExpectedMessageCount = 3;

    SimpleStringMessage result;
    SimpleStringMessage request;
    request.setTestFieldString("Stream");

    auto stream = client()->streamTestMethodServerStream(request);

    QSignalSpy streamFinishedSpy(stream.get(), &QGrpcServerStream::finished);
    QVERIFY(streamFinishedSpy.isValid());

    QSignalSpy streamErrorSpy(stream.get(), &QGrpcServerStream::errorOccurred);
    QVERIFY(streamErrorSpy.isValid());

    QSignalSpy messageReceivedSpy(stream.get(), &QGrpcServerStream::messageReceived);
    QVERIFY(messageReceivedSpy.isValid());

    int i = 0;
    QObject::connect(stream.get(), &QGrpcServerStream::messageReceived, this, [&] {
        SimpleStringMessage ret = stream->read<SimpleStringMessage>();
        result.setTestFieldString(result.testFieldString() + ret.testFieldString());
        if (++i == ExpectedMessageCount)
            QTimer::singleShot(MessageLatencyThreshold, stream.get(), &QGrpcServerStream::cancel);
    });

    QTRY_COMPARE_EQ_WITH_TIMEOUT(streamErrorSpy.count(), 1,
                                 MessageLatencyWithThreshold * ExpectedMessageCount);
    QCOMPARE(streamFinishedSpy.count(), 0);
    QCOMPARE(messageReceivedSpy.count(), ExpectedMessageCount);

    QCOMPARE_EQ(result.testFieldString(), "Stream1Stream2Stream3");
}

void QtGrpcClientServerStreamTest::HugeBlob()
{
    BlobMessage result;
    BlobMessage request;
    QFile testFile(":/assets/testfile");
    QVERIFY(testFile.open(QFile::ReadOnly));

    request.setTestBytes(testFile.readAll());
    QByteArray dataHash = QCryptographicHash::hash(request.testBytes(), QCryptographicHash::Sha256);

    auto stream = client()->streamTestMethodBlobServerStream(request);

    QSignalSpy streamFinishedSpy(stream.get(), &QGrpcServerStream::finished);
    QVERIFY(streamFinishedSpy.isValid());
    QSignalSpy streamErrorSpy(stream.get(), &QGrpcServerStream::errorOccurred);
    QVERIFY(streamErrorSpy.isValid());

    QObject::connect(stream.get(), &QGrpcServerStream::messageReceived, this, [&result, stream] {
        BlobMessage ret = stream->read<BlobMessage>();
        result.setTestBytes(ret.testBytes());
    });

    QTRY_COMPARE_EQ_WITH_TIMEOUT(streamFinishedSpy.count(), 1, MessageLatencyWithThreshold);
    QCOMPARE_EQ(streamErrorSpy.count(), 0);

    QVERIFY(!result.testBytes().isEmpty());
    QByteArray returnDataHash =
            QCryptographicHash::hash(result.testBytes(), QCryptographicHash::Sha256);
    QCOMPARE_EQ(returnDataHash, dataHash);
}

void QtGrpcClientServerStreamTest::GetAsyncReply()
{
    SimpleStringMessage request;
    request.setTestFieldString("Some status message");
    QGrpcStatus::StatusCode asyncStatus;
    QString statusMessage;

    auto reply = client()->testMethodStatusMessage(request);

    bool finishedCalled = false;
    reply->subscribe(
            this, [&finishedCalled] { finishedCalled = true; },
            [&asyncStatus, &statusMessage](const QGrpcStatus &status) {
                asyncStatus = status.code();
                statusMessage = status.message();
            });

    QTRY_COMPARE_WITH_TIMEOUT(statusMessage, request.testFieldString(),
                              MessageLatencyWithThreshold);
    QVERIFY(finishedCalled);

    SimpleStringMessage result;
    request.setTestFieldString("Hello Qt!");

    reply = client()->testMethod(request);
    reply->subscribe(this, [reply, &result] { result = reply->read<SimpleStringMessage>(); });

    QTRY_COMPARE_WITH_TIMEOUT(result.testFieldString(), request.testFieldString(),
                              MessageLatencyWithThreshold);

    result.setTestFieldString("");
    request.setTestFieldString("Hello Qt1!");

    reply = client()->testMethod(request);
    reply->subscribe(
            this, [reply, &result] { result = reply->read<SimpleStringMessage>(); },
            [] { QVERIFY(false); });

    QTRY_COMPARE_WITH_TIMEOUT(result.testFieldString(), request.testFieldString(),
                              MessageLatencyWithThreshold);
}

void QtGrpcClientServerStreamTest::MultipleStreams()
{
    const int ExpectedMessageCount = 4;
    SimpleStringMessage result;
    SimpleStringMessage request;
    request.setTestFieldString("Stream");

    auto stream = client()->streamTestMethodServerStream(request);
    // Ensure we're not reusing streams
    QCOMPARE_NE(stream, client()->streamTestMethodServerStream(request));

    QSignalSpy streamFinishedSpy(stream.get(), &QGrpcServerStream::finished);
    QVERIFY(streamFinishedSpy.isValid());

    QSignalSpy streamErrorSpy(stream.get(), &QGrpcServerStream::errorOccurred);
    QVERIFY(streamErrorSpy.isValid());

    QSignalSpy steamMessageRecievedSpy(stream.get(), &QGrpcServerStream::messageReceived);
    QVERIFY(steamMessageRecievedSpy.isValid());

    QObject::connect(stream.get(), &QGrpcServerStream::messageReceived, this, [&result, stream] {
        SimpleStringMessage ret = stream->read<SimpleStringMessage>();
        result.setTestFieldString(result.testFieldString() + ret.testFieldString());
    });

    QTRY_COMPARE_EQ_WITH_TIMEOUT(streamFinishedSpy.count(), 1,
                                 MessageLatencyWithThreshold * ExpectedMessageCount);
    QCOMPARE_EQ(streamErrorSpy.count(), 0);

    QCOMPARE_EQ(steamMessageRecievedSpy.count(), ExpectedMessageCount);
    QCOMPARE_EQ(result.testFieldString(), "Stream1Stream2Stream3Stream4");
}

void QtGrpcClientServerStreamTest::MultipleStreamsCancel()
{
    SimpleStringMessage result;
    SimpleStringMessage request;
    request.setTestFieldString("Stream");

    auto stream = client()->streamTestMethodServerStream(request);
    auto streamNext = client()->streamTestMethodServerStream(request);

    QCOMPARE_NE(stream, streamNext);

    QSignalSpy streamFinishedSpy(stream.get(), &QGrpcServerStream::finished);
    QVERIFY(streamFinishedSpy.isValid());
    QSignalSpy streamErrorSpy(stream.get(), &QGrpcServerStream::errorOccurred);
    QVERIFY(streamErrorSpy.isValid());

    QSignalSpy streamNextFinishedSpy(streamNext.get(), &QGrpcServerStream::finished);
    QVERIFY(streamNextFinishedSpy.isValid());
    QSignalSpy streamNextErrorSpy(streamNext.get(), &QGrpcServerStream::errorOccurred);
    QVERIFY(streamNextErrorSpy.isValid());

    streamNext->cancel();

    QCOMPARE(streamFinishedSpy.count(), 0);
    QCOMPARE(streamErrorSpy.count(), 0);

    QCOMPARE(streamNextFinishedSpy.count(), 0);
    QCOMPARE(streamNextErrorSpy.count(), 1);

    stream = client()->streamTestMethodServerStream(request);
    QCOMPARE_NE(stream, streamNext);

    streamNext = client()->streamTestMethodServerStream(request);

    QCOMPARE_NE(stream, streamNext);

    QSignalSpy otherStreamFinishedSpy(stream.get(), &QGrpcServerStream::finished);
    QVERIFY(streamFinishedSpy.isValid());
    QSignalSpy otherStreamErrorSpy(stream.get(), &QGrpcServerStream::errorOccurred);
    QVERIFY(otherStreamErrorSpy.isValid());

    QSignalSpy otherStreamNextFinishedSpy(streamNext.get(), &QGrpcServerStream::finished);
    QVERIFY(streamNextFinishedSpy.isValid());
    QSignalSpy otherStreamNextErrorSpy(streamNext.get(), &QGrpcServerStream::errorOccurred);
    QVERIFY(otherStreamNextErrorSpy.isValid());

    stream->cancel();

    QCOMPARE(otherStreamFinishedSpy.count(), 0);
    QCOMPARE_EQ(otherStreamErrorSpy.count(), 1);

    QCOMPARE(otherStreamNextFinishedSpy.count(), 0);
    QCOMPARE_EQ(otherStreamNextErrorSpy.count(), 0);
}

void QtGrpcClientServerStreamTest::InThread()
{
    SimpleStringMessage result;
    SimpleStringMessage request;
    request.setTestFieldString("Stream");

    QSignalSpy clientErrorSpy(client().get(), &TestService::Client::errorOccurred);
    QVERIFY(clientErrorSpy.isValid());

    int i = 0;
    const std::unique_ptr<QThread> thread(QThread::create([&] {
        QEventLoop waiter;
        auto stream = client()->streamTestMethodServerStream(request);
        QObject::connect(stream.get(), &QGrpcServerStream::messageReceived, &waiter,
                         [&result, &i, &waiter, stream] {
                             SimpleStringMessage ret = stream->read<SimpleStringMessage>();
                             result.setTestFieldString(result.testFieldString()
                                                       + ret.testFieldString());
                             if (++i == 4)
                                 waiter.quit();
                         });

        waiter.exec();
    }));

    thread->start();

    QTRY_COMPARE_EQ_WITH_TIMEOUT(clientErrorSpy.count(), 1, FailTimeout);
    QTRY_VERIFY(result.testFieldString().isEmpty());
    QTRY_VERIFY(
            qvariant_cast<QGrpcStatus>(clientErrorSpy.at(0).first())
                    .message()
                    .startsWith(
                            "QAbstractGrpcClient::startStream<QGrpcServerStream> is called from a "
                            "different thread."));
}

void QtGrpcClientServerStreamTest::CancelWhileErrorTimeout()
{
    SimpleStringMessage result;
    SimpleStringMessage request;
    request.setTestFieldString("Stream");

    auto stream = client()->streamTestMethodServerStream(request);

    QSignalSpy streamFinishedSpy(stream.get(), &QGrpcServerStream::finished);
    QVERIFY(streamFinishedSpy.isValid());
    QSignalSpy streamErrorSpy(stream.get(), &QGrpcServerStream::errorOccurred);
    QVERIFY(streamErrorSpy.isValid());

    stream->cancel();
    stream.reset();

    QTRY_COMPARE_EQ_WITH_TIMEOUT(streamErrorSpy.count(), 1, MessageLatencyWithThreshold);
    QCOMPARE_EQ(streamFinishedSpy.count(), 0);
}

void QtGrpcClientServerStreamTest::Deadline_data()
{
    const int ExpectedMessageCount = 4;
    QTest::addColumn<std::chrono::milliseconds>("timeout");
    QTest::addColumn<int>("ExpectedMessageCount");
    constexpr std::array<qreal, 4> messageLatencyFractions{ 0.7, 0.9, 1.0, 1.3 };
    for (const auto &fraction : messageLatencyFractions)
        QTest::newRow(QString("MessageLatency * ExpectedMessageCount * %1")
                              .arg(fraction)
                              .toStdString()
                              .c_str())
                << std::chrono::milliseconds(
                           static_cast<int64_t>((MessageLatency * fraction * ExpectedMessageCount)))
                << ExpectedMessageCount;
}

void QtGrpcClientServerStreamTest::Deadline()
{
    QFETCH(const std::chrono::milliseconds, timeout);
    QFETCH(const int, ExpectedMessageCount);

    QGrpcCallOptions opt;
    opt.withDeadline(timeout);

    SimpleStringMessage request;
    request.setTestFieldString("Stream");

    auto stream = client()->streamTestMethodServerStream(request, opt);

    QSignalSpy streamErrorSpy(stream.get(), &QGrpcServerStream::errorOccurred);
    QVERIFY(streamErrorSpy.isValid());
    QSignalSpy streamFinishedSpy(stream.get(), &QGrpcServerStream::finished);
    QVERIFY(streamFinishedSpy.isValid());

    SimpleStringMessage result;
    QObject::connect(stream.get(), &QGrpcServerStream::messageReceived, this, [&result, stream] {
        SimpleStringMessage ret = stream->read<SimpleStringMessage>();
        result.setTestFieldString(result.testFieldString() + ret.testFieldString());
    });

    if (timeout.count() < MessageLatency * ExpectedMessageCount) {
        QTRY_COMPARE_EQ_WITH_TIMEOUT(streamErrorSpy.count(), 1, FailTimeout);
        const auto code = qvariant_cast<QGrpcStatus>(streamErrorSpy.at(0).first()).code();
        // Really low timeout can trigger before service becomes available
        QVERIFY(code == QGrpcStatus::StatusCode::Cancelled
                || code == QGrpcStatus::StatusCode::Unavailable);
    } else if (timeout.count() >= MessageLatencyWithThreshold * ExpectedMessageCount) {
        QTRY_COMPARE_EQ_WITH_TIMEOUT(streamFinishedSpy.count(), 1,
                                     MessageLatencyWithThreshold * ExpectedMessageCount);
        QCOMPARE(streamErrorSpy.count(), 0);
        QCOMPARE_EQ(result.testFieldString(), "Stream1Stream2Stream3Stream4");
    } else {
        // Because we're can't be sure about the result,
        // cancel the stream, that might affect other tests.
        stream->cancel();
    }
}

void QtGrpcClientServerStreamTest::Interceptor()
{
    const int ExpectedMessageCount = 4;
    constexpr QLatin1StringView interceptor1Id = "int"_L1;
    constexpr QLatin1StringView interceptor2Id = "er"_L1;

    auto modifyFunc = [](std::shared_ptr<QGrpcChannelOperation> operation, QLatin1StringView id) {
        SimpleStringMessage oldArg;
        if (!operation->serializer()->deserialize(&oldArg, operation->argument())) {
            QFAIL("Deserialization of arg failed.");
            return;
        }
        SimpleStringMessage newArg;
        newArg.setTestFieldString(QString("%1%2").arg(oldArg.testFieldString()).arg(id));
        auto rawMessage =
            operation->serializer()->serialize<qtgrpc::tests::SimpleStringMessage>(&newArg);
        operation->setArgument(rawMessage);
    };
    auto manager = QGrpcClientInterceptorManager();
    manager.registerInterceptors({ std::make_shared<MockInterceptor>(interceptor1Id, modifyFunc),
                                   std::make_shared<MockInterceptor>(interceptor2Id, modifyFunc) });
    auto channel = client()->channel();
    channel->addInterceptorManager(manager);
    client()->attachChannel(channel);

    SimpleStringMessage result;
    SimpleStringMessage request;
    auto stream = client()->streamTestMethodServerStream(request);

    QSignalSpy messageReceivedSpy(stream.get(), &QGrpcServerStream::messageReceived);
    QVERIFY(messageReceivedSpy.isValid());
    QSignalSpy streamErrorSpy(stream.get(), &QGrpcServerStream::errorOccurred);
    QVERIFY(streamErrorSpy.isValid());

    QSignalSpy streamFinishedSpy(stream.get(), &QGrpcServerStream::finished);
    QObject::connect(stream.get(), &QGrpcServerStream::messageReceived, this, [&result, stream] {
        SimpleStringMessage ret = stream->read<SimpleStringMessage>();
        result.setTestFieldString(result.testFieldString() + ret.testFieldString());
    });

    QTRY_COMPARE_EQ_WITH_TIMEOUT(streamFinishedSpy.count(), 1,
                                 MessageLatencyWithThreshold * ExpectedMessageCount);
    QCOMPARE(streamErrorSpy.count(), 0);

    QCOMPARE(messageReceivedSpy.count(), ExpectedMessageCount);
    QCOMPARE_EQ(result.testFieldString(), "inter1inter2inter3inter4");
}

void QtGrpcClientServerStreamTest::CancelledInterceptor()
{
    constexpr QLatin1StringView interceptor1Id = "inter1"_L1;

    auto manager = QGrpcClientInterceptorManager();
    manager.registerInterceptors({ std::make_shared<NoContinuationInterceptor>(interceptor1Id) });
    auto channel = client()->channel();
    channel->addInterceptorManager(manager);
    client()->attachChannel(channel);

    SimpleStringMessage result;
    result.setTestFieldString("Result not changed by echo");
    SimpleStringMessage request;
    request.setTestFieldString("Stream");

    auto stream = client()->streamTestMethodServerStream(request);

    QSignalSpy streamFinishedSpy(stream.get(), &QGrpcServerStream::finished);
    QVERIFY(streamFinishedSpy.isValid());
    QSignalSpy streamErrorSpy(stream.get(), &QGrpcServerStream::errorOccurred);
    QVERIFY(streamErrorSpy.isValid());

    QObject::connect(stream.get(), &QGrpcServerStream::messageReceived, this, [&] {
        SimpleStringMessage ret = stream->read<SimpleStringMessage>();
        result.setTestFieldString(result.testFieldString() + ret.testFieldString());
    });

    QTRY_COMPARE_EQ_WITH_TIMEOUT(streamErrorSpy.count(), 1, MessageLatencyWithThreshold);
    QCOMPARE(streamFinishedSpy.count(), 0);
    QCOMPARE_EQ(result.testFieldString(), "Result not changed by echo");
}

void QtGrpcClientServerStreamTest::InterceptResponse()
{
    const int ExpectedMessageCount = 4;

    SimpleStringMessage serverResponse;
    auto interceptFunc =
        [this, &serverResponse](std::shared_ptr<QGrpcChannelOperation> operation,
                                std::shared_ptr<QGrpcServerStream> stream,
                                QGrpcInterceptorContinuation<QGrpcServerStream> &continuation,
                                QLatin1StringView) {
            QObject::connect(stream.get(), &QGrpcServerStream::messageReceived, this,
                             [&serverResponse, stream] {
                                 SimpleStringMessage mess = stream->read<SimpleStringMessage>();
                                 serverResponse.setTestFieldString(serverResponse.testFieldString()
                                                                   + mess.testFieldString());
                             });
            continuation(std::move(stream), operation);
        };

    auto manager = QGrpcClientInterceptorManager();
    manager.registerInterceptor(std::make_shared<ServerStreamInterceptor>("inter1"_L1,
                                                                          interceptFunc));
    auto channel = client()->channel();
    channel->addInterceptorManager(manager);
    client()->attachChannel(channel);

    SimpleStringMessage request;
    request.setTestFieldString("Stream");

    auto stream = client()->streamTestMethodServerStream(request);

    QSignalSpy streamFinishedSpy(stream.get(), &QGrpcServerStream::finished);
    QVERIFY(streamFinishedSpy.isValid());
    QSignalSpy streamErrorSpy(stream.get(), &QGrpcServerStream::errorOccurred);
    QVERIFY(streamErrorSpy.isValid());
    QSignalSpy messageReceivedSpy(stream.get(), &QGrpcServerStream::messageReceived);
    QVERIFY(messageReceivedSpy.isValid());

    SimpleStringMessage result;
    QObject::connect(stream.get(), &QGrpcServerStream::messageReceived, this, [&] {
        SimpleStringMessage ret = stream->read<SimpleStringMessage>();
        result.setTestFieldString(result.testFieldString() + ret.testFieldString());
    });

    QTRY_COMPARE_EQ_WITH_TIMEOUT(streamFinishedSpy.count(), 1,
                                 MessageLatencyWithThreshold * ExpectedMessageCount);
    QCOMPARE(streamErrorSpy.count(), 0);
    QCOMPARE(messageReceivedSpy.count(), ExpectedMessageCount);

    QCOMPARE_EQ(result.testFieldString(), "Stream1Stream2Stream3Stream4");
    QCOMPARE_EQ(serverResponse.testFieldString(), "Stream1Stream2Stream3Stream4");
}

QTEST_MAIN(QtGrpcClientServerStreamTest)

#include "tst_grpc_client_serverstream.moc"
