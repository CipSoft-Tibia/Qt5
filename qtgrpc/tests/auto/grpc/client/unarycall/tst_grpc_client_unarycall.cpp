// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2019 Alexey Edelev <semlanik@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <grpcclienttestbase.h>

#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtCore/qsystemdetection.h>

#include <QtGrpc/QGrpcCallOptions>
#include <QtGrpc/QGrpcMetadata>

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include <memory>

#if QT_CONFIG(native_grpc)
#  include <grpcpp/security/credentials.h>
#endif

#include <testservice_client.grpc.qpb.h>
#include <message_latency_defs.h>
#include <server_proc_runner.h>

using namespace Qt::Literals::StringLiterals;
using namespace qtgrpc::tests;

class QtGrpcClientUnaryCallTest : public GrpcClientTestBase
{
    Q_OBJECT
public:
    QtGrpcClientUnaryCallTest()
        : GrpcClientTestBase(GrpcClientTestBase::Channels{
                GrpcClientTestBase::Channel::Qt,
                GrpcClientTestBase::Channel::Native,
#if !defined(Q_OS_DARWIN) && !defined(Q_OS_WIN32)
                GrpcClientTestBase::Channel::Ssl,
#endif
                            })
    {
    }

private slots:
    void Blocking();
    void AsyncWithSubscribe();
    void AsyncWithLambda();
    void ImmediateAbort();
    void DeferredAbort();
    void AsyncClientStatusMessage();
    void BlockingClientStatusMessage();
    void AsyncStatusMessage();
    void BlockingStatusMessage();
    void NonCompatibleArgRet();
    void InThread();
    void AsyncInThread();
    void Metadata();
    void Deadline_data();
    void Deadline();
};

void QtGrpcClientUnaryCallTest::Blocking()
{
    SimpleStringMessage request;
    auto result = std::make_shared<SimpleStringMessage>();
    request.setTestFieldString("Hello Qt!");
    QCOMPARE_EQ(client()->testMethod(request, result.get()), QGrpcStatus::Ok);
    QCOMPARE_EQ(result->testFieldString(), "Hello Qt!");
}

void QtGrpcClientUnaryCallTest::AsyncWithSubscribe()
{
    SimpleStringMessage request;
    SimpleStringMessage result;
    request.setTestFieldString("Hello Qt!");
    QEventLoop waiter;

    std::shared_ptr<QGrpcCallReply> reply = client()->testMethod(request);
    reply->subscribe(this, [reply, &result, &waiter] {
        result = reply->read<SimpleStringMessage>();
        waiter.quit();
    });

    waiter.exec();
    QCOMPARE_EQ(result.testFieldString(), "Hello Qt!");
}

void QtGrpcClientUnaryCallTest::AsyncWithLambda()
{
    SimpleStringMessage result;
    SimpleStringMessage request;
    request.setTestFieldString("Hello Qt!");
    QEventLoop waiter;
    client()->testMethod(request, this, [&result, &waiter](std::shared_ptr<QGrpcCallReply> reply) {
        result = reply->read<SimpleStringMessage>();
        waiter.quit();
    });

    waiter.exec();
    QCOMPARE_EQ(result.testFieldString(), "Hello Qt!");
}

void QtGrpcClientUnaryCallTest::ImmediateAbort()
{
    SimpleStringMessage result;
    SimpleStringMessage request;
    request.setTestFieldString("sleep");

    std::shared_ptr<QGrpcCallReply> reply = client()->testMethod(request);

    result.setTestFieldString("Result not changed by echo");
    QObject::connect(reply.get(), &QGrpcCallReply::finished, this,
                     [&result, reply] { result = reply->read<SimpleStringMessage>(); });

    QSignalSpy replyErrorSpy(reply.get(), &QGrpcCallReply::errorOccurred);
    QSignalSpy replyFinishedSpy(reply.get(), &QGrpcCallReply::finished);
    QSignalSpy clientErrorSpy(client().get(), &TestService::Client::errorOccurred);

    QVERIFY(replyErrorSpy.isValid());
    QVERIFY(replyFinishedSpy.isValid());
    QVERIFY(clientErrorSpy.isValid());

    reply->abort();

    QTRY_COMPARE_EQ_WITH_TIMEOUT(replyErrorSpy.count(), 1, FailTimeout);
    QTRY_COMPARE_EQ_WITH_TIMEOUT(clientErrorSpy.count(), 1, FailTimeout);
    QTRY_COMPARE_EQ_WITH_TIMEOUT(replyFinishedSpy.count(), 0, FailTimeout);

    QCOMPARE_EQ(result.testFieldString(), "Result not changed by echo");
}

void QtGrpcClientUnaryCallTest::DeferredAbort()
{
    SimpleStringMessage request;
    request.setTestFieldString("sleep");

    SimpleStringMessage result;
    result.setTestFieldString("Result not changed by echo");
    std::shared_ptr<QGrpcCallReply> reply = client()->testMethod(request);

    QObject::connect(reply.get(), &QGrpcCallReply::finished, this,
                     [reply, &result] { result = reply->read<SimpleStringMessage>(); });

    QSignalSpy replyErrorSpy(reply.get(), &QGrpcCallReply::errorOccurred);
    QVERIFY(replyErrorSpy.isValid());

    QTimer::singleShot(MessageLatencyThreshold, reply.get(), &QGrpcCallReply::abort);

    QTRY_COMPARE_EQ_WITH_TIMEOUT(replyErrorSpy.count(), 1, FailTimeout);
    QCOMPARE_EQ(result.testFieldString(), "Result not changed by echo");
}

void QtGrpcClientUnaryCallTest::AsyncClientStatusMessage()
{
    SimpleStringMessage request;
    request.setTestFieldString("Some status message");

    QSignalSpy clientErrorSpy(client().get(), &TestService::Client::errorOccurred);
    QVERIFY(clientErrorSpy.isValid());

    client()->testMethodStatusMessage(request);

    QTRY_COMPARE_EQ_WITH_TIMEOUT(clientErrorSpy.count(), 1, FailTimeout);

    QCOMPARE(qvariant_cast<QGrpcStatus>(clientErrorSpy.at(0).first()).message(),
             request.testFieldString());
}

void QtGrpcClientUnaryCallTest::BlockingClientStatusMessage()
{
    SimpleStringMessage request;
    request.setTestFieldString("Some status message");
    auto result = std::make_shared<SimpleStringMessage>();

    QSignalSpy clientErrorSpy(client().get(), &TestService::Client::errorOccurred);
    QVERIFY(clientErrorSpy.isValid());

    client()->testMethodStatusMessage(request, result.get());

    QTRY_COMPARE_GE_WITH_TIMEOUT(clientErrorSpy.count(), 1, FailTimeout);

    QCOMPARE(qvariant_cast<QGrpcStatus>(clientErrorSpy.at(0).first()).message(),
             request.testFieldString());
}

void QtGrpcClientUnaryCallTest::AsyncStatusMessage()
{
    SimpleStringMessage request;
    request.setTestFieldString("Some status message");

    std::shared_ptr<QGrpcCallReply> reply = client()->testMethodStatusMessage(request);

    QSignalSpy replyErrorSpy(reply.get(), &QGrpcCallReply::errorOccurred);
    QVERIFY(replyErrorSpy.isValid());

    QTRY_COMPARE_EQ_WITH_TIMEOUT(replyErrorSpy.count(), 1, FailTimeout);
    QCOMPARE(qvariant_cast<QGrpcStatus>(replyErrorSpy.at(0).first()).message(),
             request.testFieldString());
}


void QtGrpcClientUnaryCallTest::BlockingStatusMessage()
{
    SimpleStringMessage request;
    request.setTestFieldString("Some status message");
    auto result = std::make_shared<SimpleStringMessage>();

    QGrpcStatus status = client()->testMethodStatusMessage(request, result.get());

    QCOMPARE_EQ(status.message(), request.testFieldString());
}

void QtGrpcClientUnaryCallTest::NonCompatibleArgRet()
{
    const QtProtobuf::sint32 TestValue = 2048;
    const QString TestValueString = QString::number(TestValue);

    SimpleIntMessage request;
    request.setTestField(TestValue);
    auto result = std::make_shared<SimpleStringMessage>();
    QCOMPARE_EQ(client()->testMethodNonCompatibleArgRet(request, result.get()), QGrpcStatus::Ok);
    QCOMPARE_EQ(result->testFieldString(), TestValueString);
}

void QtGrpcClientUnaryCallTest::InThread()
{
    SimpleStringMessage request;
    auto result = std::make_shared<SimpleStringMessage>();

    request.setTestFieldString("Hello Qt from thread!");

    QSignalSpy clientErrorSpy(client().get(), &TestService::Client::errorOccurred);
    QVERIFY(clientErrorSpy.isValid());

    bool ok = false;
    const std::unique_ptr<QThread> thread(QThread::create(
            [&] { ok = client()->testMethod(request, result.get()) == QGrpcStatus::Ok; }));

    thread->start();

    QTRY_COMPARE_EQ_WITH_TIMEOUT(clientErrorSpy.count(), 1, MessageLatencyWithThreshold);
    QVERIFY(!ok);
    QVERIFY(result->testFieldString().isEmpty());
    QVERIFY(qvariant_cast<QGrpcStatus>(clientErrorSpy.at(0).first())
                    .message()
                    .startsWith("QAbstractGrpcClient::call is called from a different thread."));
}

void QtGrpcClientUnaryCallTest::AsyncInThread()
{
    SimpleStringMessage request;
    SimpleStringMessage result;
    request.setTestFieldString("Hello Qt from thread!");

    QSignalSpy clientErrorSpy(client().get(), &TestService::Client::errorOccurred);
    QVERIFY(clientErrorSpy.isValid());

    const std::unique_ptr<QThread> thread(QThread::create([&] {
        QEventLoop waiter;
        std::shared_ptr<QGrpcCallReply> reply = client()->testMethod(request);
        QObject::connect(reply.get(), &QGrpcCallReply::finished, &waiter,
                         [reply, &result, &waiter] {
                             result = reply->read<SimpleStringMessage>();
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
                    .startsWith("QAbstractGrpcClient::call is called from a different thread."));
}

void QtGrpcClientUnaryCallTest::Metadata()
{
    QSignalSpy clientErrorSpy(client().get(), &TestService::Client::errorOccurred);
    QVERIFY(clientErrorSpy.isValid());

    QGrpcCallOptions opt;
    opt.withMetadata({ { "client_header", "1" }, { "client_return_header", "valid_value" } });
    auto reply = client()->testMetadata({}, opt);

    QGrpcMetadata metadata;
    QEventLoop waiter;
    reply->subscribe(this, [reply, &metadata, &waiter] {
        metadata = reply->metadata();
        waiter.quit();
    });

    waiter.exec();

    int serverHeaderCount = 0;
    QByteArray clientReturnHeader;
    for (const auto &header : reply->metadata()) {
        if (header.first == "server_header") {
            QCOMPARE_EQ(QString::fromLatin1(header.second), QString::number(++serverHeaderCount));
        } else if (header.first == "client_return_header") {
            clientReturnHeader = header.second;
        }
    }

    QCOMPARE_EQ(clientErrorSpy.count(), 0);
    if (channelType().testFlag(GrpcClientTestBase::Channel::Native))
        QEXPECT_FAIL("", "Unimplemented in the reference gRPC channel", Abort);
    QCOMPARE_EQ(serverHeaderCount, 1);
    QCOMPARE_EQ(clientReturnHeader, "valid_value"_ba);
}

void QtGrpcClientUnaryCallTest::Deadline_data()
{
    QTest::addColumn<std::chrono::milliseconds>("timeout");
    constexpr std::array<qreal, 4> messageLatencyFractions{ 0.7, 0.9, 1.0, 1.3 };
    for (const auto &fraction : messageLatencyFractions)
        QTest::newRow(QString("MessageLatency * %1").arg(fraction).toStdString().c_str())
                << std::chrono::milliseconds(static_cast<int64_t>(MessageLatency * fraction));
}

void QtGrpcClientUnaryCallTest::Deadline()
{
    QFETCH(const std::chrono::milliseconds, timeout);

    QGrpcCallOptions opt;
    opt.withDeadline(timeout);

    SimpleStringMessage request;
    request.setTestFieldString("sleep");

    QSignalSpy clientErrorSpy(client().get(), &TestService::Client::errorOccurred);
    QVERIFY(clientErrorSpy.isValid());

    auto reply = client()->testMethod(request, opt);
    QSignalSpy callFinishedSpy(reply.get(), &QGrpcCallReply::finished);
    QVERIFY(callFinishedSpy.isValid());

    if (timeout.count() < MessageLatency) {
        QTRY_COMPARE_EQ_WITH_TIMEOUT(clientErrorSpy.count(), 1, FailTimeout);
        const auto code = qvariant_cast<QGrpcStatus>(clientErrorSpy.at(0).first()).code();
        // Really low timeout can trigger before service becomes available
        QVERIFY(code == QGrpcStatus::StatusCode::Cancelled
                || code == QGrpcStatus::StatusCode::Unavailable);
    } else if (timeout.count() >= MessageLatencyWithThreshold) {
        QTRY_COMPARE_EQ_WITH_TIMEOUT(callFinishedSpy.count(), 1, MessageLatencyWithThreshold);
        QCOMPARE(reply->read<SimpleStringMessage>().testFieldString(), request.testFieldString());
    } else {
        // Because we're can't be sure about the result,
        // cancel the call, that might affect other tests.
        reply->abort();
    }
}

QTEST_MAIN(QtGrpcClientUnaryCallTest)

#include "tst_grpc_client_unarycall.moc"
