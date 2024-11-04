// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2019 Alexey Edelev <semlanik@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <grpcclienttestbase.h>

#include <QtCore/QThread>
#include <QtCore/QTimer>

#include <QtGrpc/QGrpcCallOptions>
#include <QtGrpc/QGrpcClientInterceptorManager>
#include <QtGrpc/QGrpcMetadata>

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include <memory>

#include <interceptormocks.h>
#include <message_latency_defs.h>
#include <server_proc_runner.h>
#include <testservice_client.grpc.qpb.h>

using namespace Qt::Literals::StringLiterals;
using namespace qtgrpc::tests;

class QtGrpcClientUnaryCallTest : public GrpcClientTestBase
{
    Q_OBJECT
public:
    QtGrpcClientUnaryCallTest()
        : GrpcClientTestBase(GrpcClientTestBase::Channels{
                GrpcClientTestBase::Channel::Qt,
#if !defined(Q_OS_DARWIN) && !defined(Q_OS_WIN32)
                GrpcClientTestBase::Channel::Ssl,
#endif
                            })
    {
    }

private slots:
    void AsyncWithSubscribe();
    void AsyncWithLambda();
    void ImmediateCancel();
    void DeferredCancel();
    void AsyncClientStatusMessage();
    void AsyncStatusMessage();
    void InThread();
    void AsyncInThread();
    void Metadata();
    void Deadline_data();
    void Deadline();
    void Interceptor();
    void CancelledInterceptor();
    void InterceptResponse();
    void CacheIntercept();
};

void QtGrpcClientUnaryCallTest::AsyncWithSubscribe()
{
    SimpleStringMessage request;
    SimpleStringMessage result;
    request.setTestFieldString("Hello Qt!");

    bool waitForReply = false;
    std::shared_ptr<QGrpcCallReply> reply = client()->testMethod(request);
    reply->subscribe(this, [reply, &result, &waitForReply] {
        result = reply->read<SimpleStringMessage>();
        waitForReply = true;
    });

    QTRY_COMPARE_EQ_WITH_TIMEOUT(waitForReply, true, MessageLatency);
    QCOMPARE_EQ(result.testFieldString(), "Hello Qt!");
}

void QtGrpcClientUnaryCallTest::AsyncWithLambda()
{
    SimpleStringMessage result;
    SimpleStringMessage request;
    request.setTestFieldString("Hello Qt!");
    bool waitForReply = false;
    client()->testMethod(request, this,
                         [&result, &waitForReply](std::shared_ptr<QGrpcCallReply> reply) {
                             result = reply->read<SimpleStringMessage>();
                             waitForReply = true;
                         });

    QTRY_COMPARE_EQ_WITH_TIMEOUT(waitForReply, true, MessageLatency);
    QCOMPARE_EQ(result.testFieldString(), "Hello Qt!");
}

void QtGrpcClientUnaryCallTest::ImmediateCancel()
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

    reply->cancel();

    QTRY_COMPARE_EQ_WITH_TIMEOUT(replyErrorSpy.count(), 1, FailTimeout);
    QTRY_COMPARE_EQ_WITH_TIMEOUT(clientErrorSpy.count(), 1, FailTimeout);
    QTRY_COMPARE_EQ_WITH_TIMEOUT(replyFinishedSpy.count(), 0, FailTimeout);

    QCOMPARE_EQ(result.testFieldString(), "Result not changed by echo");
    QCOMPARE_EQ(qvariant_cast<QGrpcStatus>(clientErrorSpy.at(0).first()).code(),
                QGrpcStatus::Cancelled);
}

void QtGrpcClientUnaryCallTest::DeferredCancel()
{
    SimpleStringMessage request;
    request.setTestFieldString("sleep");

    SimpleStringMessage result;
    result.setTestFieldString("Result not changed by echo");
    std::shared_ptr<QGrpcCallReply> reply = client()->testMethod(request);

    QObject::connect(reply.get(), &QGrpcCallReply::finished, this, [reply, &result] {
        QVERIFY(false);
        result = reply->read<SimpleStringMessage>();
    });

    QSignalSpy replyErrorSpy(reply.get(), &QGrpcCallReply::errorOccurred);
    QVERIFY(replyErrorSpy.isValid());

    QTimer::singleShot(MessageLatencyThreshold, reply.get(), &QGrpcCallReply::cancel);

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

void QtGrpcClientUnaryCallTest::InThread()
{
    SimpleStringMessage request;

    request.setTestFieldString("Hello Qt from thread!");

    QSignalSpy clientErrorSpy(client().get(), &TestService::Client::errorOccurred);
    QVERIFY(clientErrorSpy.isValid());

    std::shared_ptr<QGrpcCallReply> reply;
    const std::unique_ptr<QThread> thread(QThread::create(
        [&] { reply = client()->testMethod(request); }));

    thread->start();

    QTRY_COMPARE_EQ_WITH_TIMEOUT(clientErrorSpy.count(), 1, MessageLatencyWithThreshold);
    QVERIFY(reply == nullptr);
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
        reply->cancel();
    }
}

void QtGrpcClientUnaryCallTest::Interceptor()
{
    constexpr QLatin1StringView clientMetadataKey = "client_header"_L1;
    constexpr QLatin1StringView serverMetadataKey = "server_header"_L1;
    constexpr QLatin1StringView interceptor1Id = "inter1"_L1;
    constexpr QLatin1StringView interceptor2Id = "inter2"_L1;

    auto modifyFunc = [clientMetadataKey](std::shared_ptr<QGrpcChannelOperation> operation,
                                          QLatin1StringView id) {
        auto metadata = operation->options().metadata();
        if (auto it = metadata.find(clientMetadataKey.latin1()); it != metadata.end()) {
            it->second += id;
        } else {
            metadata.insert({ clientMetadataKey.latin1(), id.latin1() });
        }
        operation->setClientMetadata(metadata);
    };
    auto manager = QGrpcClientInterceptorManager();
    manager.registerInterceptors({ std::make_shared<MockInterceptor>(interceptor1Id, modifyFunc),
                                   std::make_shared<MockInterceptor>(interceptor2Id, modifyFunc) });
    auto channel = client()->channel();
    channel->addInterceptorManager(manager);
    client()->attachChannel(channel);

    auto reply = client()->testMetadata({});

    QSignalSpy callFinishedSpy(reply.get(), &QGrpcCallReply::finished);
    QVERIFY(callFinishedSpy.isValid());

    QTRY_COMPARE_EQ_WITH_TIMEOUT(callFinishedSpy.count(), 1, MessageLatencyWithThreshold);
    const auto serverMetadata = reply->metadata();
    const auto it = serverMetadata.find(serverMetadataKey.latin1());

    QVERIFY(it != serverMetadata.end());
    QCOMPARE_EQ(it->second, "inter1inter2"_ba);
}

void QtGrpcClientUnaryCallTest::CancelledInterceptor()
{
    constexpr QLatin1StringView interceptor1Id = "inter1"_L1;

    auto manager = QGrpcClientInterceptorManager();
    manager.registerInterceptors({ std::make_shared<NoContinuationInterceptor>(interceptor1Id) });
    auto channel = client()->channel();
    channel->addInterceptorManager(manager);
    client()->attachChannel(channel);

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

    QTRY_COMPARE_EQ_WITH_TIMEOUT(replyErrorSpy.count(), 1, FailTimeout);
    QTRY_COMPARE_EQ_WITH_TIMEOUT(clientErrorSpy.count(), 1, FailTimeout);
    QTRY_COMPARE_EQ_WITH_TIMEOUT(replyFinishedSpy.count(), 0, FailTimeout);

    QCOMPARE_EQ(result.testFieldString(), "Result not changed by echo");
}

void QtGrpcClientUnaryCallTest::InterceptResponse()
{
    SimpleStringMessage serverResponse;
    auto interceptFunc =
        [this, &serverResponse](std::shared_ptr<QGrpcChannelOperation> operation,
                                std::shared_ptr<QGrpcCallReply> response,
                                QGrpcInterceptorContinuation<QGrpcCallReply> &continuation,
                                QLatin1StringView) {
            QObject::connect(response.get(), &QGrpcCallReply::finished, this,
                             [&serverResponse, response] {
                                 serverResponse = response->read<SimpleStringMessage>();
                             });
            continuation(std::move(response), std::move(operation));
        };

    auto manager = QGrpcClientInterceptorManager();
    manager.registerInterceptor(std::make_shared<CallInterceptor>("inter1"_L1, interceptFunc));
    auto channel = client()->channel();
    channel->addInterceptorManager(manager);
    client()->attachChannel(channel);

    SimpleStringMessage request;
    SimpleStringMessage result;
    request.setTestFieldString("Hello Qt!");
    client()->testMethod(request, client().get(), [&result](std::shared_ptr<QGrpcCallReply> reply) {
        result = reply->read<SimpleStringMessage>();
    });

    QTRY_COMPARE_EQ_WITH_TIMEOUT(serverResponse.testFieldString(),
                                 "Hello Qt!", MessageLatencyWithThreshold);
    QCOMPARE_EQ(result.testFieldString(), "Hello Qt!");
}

void QtGrpcClientUnaryCallTest::CacheIntercept()
{
    SimpleStringMessage serverResponse;
    auto interceptFunc =
        [](std::shared_ptr<QGrpcChannelOperation> operation, std::shared_ptr<QGrpcCallReply>,
           QGrpcInterceptorContinuation<QGrpcCallReply> &, QLatin1StringView id) {
            SimpleStringMessage deserializedArg;
            if (!operation->serializer()->deserialize(&deserializedArg, operation->argument())) {
                QFAIL("Deserialization of arg failed.");
                return;
            }
            SimpleStringMessage cachedValue;
            cachedValue.setTestFieldString(id);
            const auto serializedValue =
                operation->serializer()->serialize<SimpleStringMessage>(&cachedValue);
            emit operation->dataReady(serializedValue);
            emit operation->finished();
        };

    auto manager = QGrpcClientInterceptorManager();
    manager.registerInterceptor(std::make_shared<CallInterceptor>("inter1"_L1, interceptFunc));
    auto channel = client()->channel();
    channel->addInterceptorManager(manager);
    client()->attachChannel(channel);

    SimpleStringMessage request;
    SimpleStringMessage result;
    request.setTestFieldString("Hello Qt!");
    client()->testMethod(request, client().get(), [&result](std::shared_ptr<QGrpcCallReply> reply) {
        result = reply->read<SimpleStringMessage>();
    });

    QTRY_COMPARE_EQ_WITH_TIMEOUT(result.testFieldString(),
                                 "inter1", MessageLatencyWithThreshold);
}

QTEST_MAIN(QtGrpcClientUnaryCallTest)

#include "tst_grpc_client_unarycall.moc"
