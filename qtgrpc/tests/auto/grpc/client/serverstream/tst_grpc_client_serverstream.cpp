// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2019 Alexey Edelev <semlanik@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include <grpcclienttestbase.h>

#include <QtGrpc/QGrpcCallOptions>

#include <QtCore/QCryptographicHash>
#include <QtCore/QFile>
#include <QtCore/QThread>
#include <QtCore/QTimer>

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include <testservice_client.grpc.qpb.h>
#include <message_latency_defs.h>

using namespace Qt::Literals::StringLiterals;
using namespace qtgrpc::tests;

class QtGrpcClientServerStreamTest : public GrpcClientTestBase
{
    Q_OBJECT
public:
    QtGrpcClientServerStreamTest()
        : GrpcClientTestBase(
                { GrpcClientTestBase::Channel::Qt, GrpcClientTestBase::Channel::Native })
    {
    }

private slots:
    void Valid();
    void Abort();
    void DeferredAbort();
    void HugeBlob();
    void GetAsyncReply();
    void MultipleStreams();
    void MultipleStreamsAbort();
    void InThread();
    void AbortWhileErrorTimeout();
    void Deadline_data();
    void Deadline();
};

void QtGrpcClientServerStreamTest::Valid()
{
    const int ExpectedMessageCount = 4;

    SimpleStringMessage result;
    SimpleStringMessage request;
    request.setTestFieldString("Stream");

    auto stream = client()->streamTestMethodServerStream(request);

    QSignalSpy messageReceivedSpy(stream.get(), &QGrpcStream::messageReceived);
    QVERIFY(messageReceivedSpy.isValid());
    QSignalSpy streamErrorSpy(stream.get(), &QGrpcStream::errorOccurred);
    QVERIFY(streamErrorSpy.isValid());

    QSignalSpy streamFinishedSpy(stream.get(), &QGrpcStream::finished);
    QObject::connect(stream.get(), &QGrpcStream::messageReceived, this, [&result, stream] {
        SimpleStringMessage ret = stream->read<SimpleStringMessage>();
        result.setTestFieldString(result.testFieldString() + ret.testFieldString());
    });

    QTRY_COMPARE_EQ_WITH_TIMEOUT(streamFinishedSpy.count(), 1,
                                 MessageLatencyWithThreshold * ExpectedMessageCount);
    QCOMPARE(streamErrorSpy.count(), 0);

    QCOMPARE(messageReceivedSpy.count(), ExpectedMessageCount);
    QCOMPARE_EQ(result.testFieldString(), "Stream1Stream2Stream3Stream4");
}

void QtGrpcClientServerStreamTest::Abort()
{
    const int ExpectedMessageCount = 2;

    SimpleStringMessage result;
    SimpleStringMessage request;
    request.setTestFieldString("Stream");

    auto stream = client()->streamTestMethodServerStream(request);

    QSignalSpy streamFinishedSpy(stream.get(), &QGrpcStream::finished);
    QVERIFY(streamFinishedSpy.isValid());
    QSignalSpy streamErrorSpy(stream.get(), &QGrpcStream::errorOccurred);
    QVERIFY(streamErrorSpy.isValid());

    int i = 0;
    QObject::connect(stream.get(), &QGrpcStream::messageReceived, this, [&] {
        SimpleStringMessage ret = stream->read<SimpleStringMessage>();
        result.setTestFieldString(result.testFieldString() + ret.testFieldString());
        if (++i == ExpectedMessageCount)
            stream->abort();
    });

    QTRY_COMPARE_EQ_WITH_TIMEOUT(streamFinishedSpy.count(), 1,
                                 MessageLatencyWithThreshold * ExpectedMessageCount);
    QCOMPARE(streamErrorSpy.count(), 0);
    QCOMPARE(i, 2);
    QCOMPARE_EQ(result.testFieldString(), "Stream1Stream2");
}

void QtGrpcClientServerStreamTest::DeferredAbort()
{
    const int ExpectedMessageCount = 3;

    SimpleStringMessage result;
    SimpleStringMessage request;
    request.setTestFieldString("Stream");

    auto stream = client()->streamTestMethodServerStream(request);

    QSignalSpy streamFinishedSpy(stream.get(), &QGrpcStream::finished);
    QVERIFY(streamFinishedSpy.isValid());

    QSignalSpy streamErrorSpy(stream.get(), &QGrpcStream::errorOccurred);
    QVERIFY(streamErrorSpy.isValid());

    QSignalSpy messageReceivedSpy(stream.get(), &QGrpcStream::messageReceived);
    QVERIFY(messageReceivedSpy.isValid());

    int i = 0;
    QObject::connect(stream.get(), &QGrpcStream::messageReceived, this, [&] {
        SimpleStringMessage ret = stream->read<SimpleStringMessage>();
        result.setTestFieldString(result.testFieldString() + ret.testFieldString());
        if (++i == ExpectedMessageCount)
            QTimer::singleShot(MessageLatencyThreshold, stream.get(), &QGrpcStream::abort);
    });

    QTRY_COMPARE_EQ_WITH_TIMEOUT(streamFinishedSpy.count(), 1,
                                 MessageLatencyWithThreshold * ExpectedMessageCount);
    QCOMPARE(streamErrorSpy.count(), 0);
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

    QSignalSpy streamFinishedSpy(stream.get(), &QGrpcStream::finished);
    QVERIFY(streamFinishedSpy.isValid());
    QSignalSpy streamErrorSpy(stream.get(), &QGrpcStream::errorOccurred);
    QVERIFY(streamErrorSpy.isValid());

    QObject::connect(stream.get(), &QGrpcStream::messageReceived, this, [&result, stream] {
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

    reply->subscribe(
            this, [] { QVERIFY(false); },
            [&asyncStatus, &statusMessage](const QGrpcStatus &status) {
                asyncStatus = status.code();
                statusMessage = status.message();
            });

    QTRY_COMPARE_WITH_TIMEOUT(statusMessage, request.testFieldString(),
                              MessageLatencyWithThreshold);

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

    QSignalSpy streamFinishedSpy(stream.get(), &QGrpcStream::finished);
    QVERIFY(streamFinishedSpy.isValid());

    QSignalSpy streamErrorSpy(stream.get(), &QGrpcStream::errorOccurred);
    QVERIFY(streamErrorSpy.isValid());

    QSignalSpy steamMessageRecievedSpy(stream.get(), &QGrpcStream::messageReceived);
    QVERIFY(steamMessageRecievedSpy.isValid());

    QObject::connect(stream.get(), &QGrpcStream::messageReceived, this, [&result, stream] {
        SimpleStringMessage ret = stream->read<SimpleStringMessage>();
        result.setTestFieldString(result.testFieldString() + ret.testFieldString());
    });

    QTRY_COMPARE_EQ_WITH_TIMEOUT(streamFinishedSpy.count(), 1,
                                 MessageLatencyWithThreshold * ExpectedMessageCount);
    QCOMPARE_EQ(streamErrorSpy.count(), 0);

    QCOMPARE_EQ(steamMessageRecievedSpy.count(), ExpectedMessageCount);
    QCOMPARE_EQ(result.testFieldString(), "Stream1Stream2Stream3Stream4");
}

void QtGrpcClientServerStreamTest::MultipleStreamsAbort()
{
    SimpleStringMessage result;
    SimpleStringMessage request;
    request.setTestFieldString("Stream");

    auto stream = client()->streamTestMethodServerStream(request);
    auto streamNext = client()->streamTestMethodServerStream(request);

    QCOMPARE_NE(stream, streamNext);

    QSignalSpy streamFinishedSpy(stream.get(), &QGrpcStream::finished);
    QVERIFY(streamFinishedSpy.isValid());
    QSignalSpy streamErrorSpy(stream.get(), &QGrpcStream::errorOccurred);
    QVERIFY(streamErrorSpy.isValid());

    QSignalSpy streamNextFinishedSpy(streamNext.get(), &QGrpcStream::finished);
    QVERIFY(streamNextFinishedSpy.isValid());
    QSignalSpy streamNextErrorSpy(streamNext.get(), &QGrpcStream::errorOccurred);
    QVERIFY(streamNextErrorSpy.isValid());

    streamNext->abort();

    QCOMPARE(streamFinishedSpy.count(), 0);
    QCOMPARE(streamErrorSpy.count(), 0);

    QCOMPARE(streamNextFinishedSpy.count(), 1);
    QCOMPARE(streamNextErrorSpy.count(), 0);

    stream = client()->streamTestMethodServerStream(request);
    QCOMPARE_NE(stream, streamNext);

    streamNext = client()->streamTestMethodServerStream(request);

    QCOMPARE_NE(stream, streamNext);

    QSignalSpy otherStreamFinishedSpy(stream.get(), &QGrpcStream::finished);
    QVERIFY(streamFinishedSpy.isValid());
    QSignalSpy otherStreamErrorSpy(stream.get(), &QGrpcStream::errorOccurred);
    QVERIFY(otherStreamErrorSpy.isValid());

    QSignalSpy otherStreamNextFinishedSpy(streamNext.get(), &QGrpcStream::finished);
    QVERIFY(streamNextFinishedSpy.isValid());
    QSignalSpy otherStreamNextErrorSpy(streamNext.get(), &QGrpcStream::errorOccurred);
    QVERIFY(otherStreamNextErrorSpy.isValid());

    stream->abort();

    QCOMPARE(otherStreamFinishedSpy.count(), 1);
    QCOMPARE_EQ(otherStreamErrorSpy.count(), 0);

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
        QObject::connect(stream.get(), &QGrpcStream::messageReceived, &waiter,
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
                            "QAbstractGrpcClient::startStream is called from a different thread."));
}

void QtGrpcClientServerStreamTest::AbortWhileErrorTimeout()
{
    SimpleStringMessage result;
    SimpleStringMessage request;
    request.setTestFieldString("Stream");

    auto stream = client()->streamTestMethodServerStream(request);

    QSignalSpy streamFinishedSpy(stream.get(), &QGrpcStream::finished);
    QVERIFY(streamFinishedSpy.isValid());
    QSignalSpy streamErrorSpy(stream.get(), &QGrpcStream::errorOccurred);
    QVERIFY(streamErrorSpy.isValid());

    stream->abort();
    stream.reset();

    QTRY_COMPARE_EQ_WITH_TIMEOUT(streamFinishedSpy.count(), 1, MessageLatencyWithThreshold);
    QCOMPARE_EQ(streamErrorSpy.count(), 0);
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

    QSignalSpy streamErrorSpy(stream.get(), &QGrpcStream::errorOccurred);
    QVERIFY(streamErrorSpy.isValid());
    QSignalSpy streamFinishedSpy(stream.get(), &QGrpcStream::finished);
    QVERIFY(streamFinishedSpy.isValid());

    SimpleStringMessage result;
    QObject::connect(stream.get(), &QGrpcStream::messageReceived, this, [&result, stream] {
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
        stream->abort();
    }
}

QTEST_MAIN(QtGrpcClientServerStreamTest)

#include "tst_grpc_client_serverstream.moc"
