// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QGrpcCallOptions>
#include <QGrpcChannelOperation>
#include <QGrpcClientInterceptor>
#include <QGrpcClientInterceptorManager>

#include <QCoreApplication>
#include <QStringLiteral>
#include <QTest>
#include <QTimer>

#include <interceptormocks.h>
#include <message_latency_defs.h>

using namespace Qt::StringLiterals;

class QtGrpcClientInterceptorTest : public QObject
{
    Q_OBJECT
private slots:

    void ClientInterceptorTest();
    void ContinuationNotCalledTest();
    void BatchRegistrationOrderingTest();
    void CancelledInterceptor();
};

void QtGrpcClientInterceptorTest::ClientInterceptorTest()
{
    using Continuation = QGrpcInterceptorContinuation<QGrpcCallReply>;
    constexpr QLatin1StringView interceptor1Id = "inter1"_L1;
    constexpr QLatin1StringView interceptor2Id = "inter2"_L1;

    bool finalCallCalled = false;
    auto finalCall = Continuation([&finalCallCalled](Continuation::ReplyType response,
                                                     Continuation::ParamType) {
        finalCallCalled = true;
        return response;
    });

    std::vector<QLatin1StringView> interceptorOrder;
    auto modifyFunc = [&interceptorOrder](std::shared_ptr<QGrpcChannelOperation> operation,
                                          QLatin1StringView id) {
        interceptorOrder.push_back(id);
        operation->setServerMetadata(QGrpcMetadata{ { "value", id.latin1() } });
    };
    auto interceptor1 = std::make_shared<MockInterceptor>(interceptor1Id, modifyFunc);
    auto interceptor2 = std::make_shared<MockInterceptor>(interceptor2Id, modifyFunc);

    auto manager = QGrpcClientInterceptorManager();
    manager.registerInterceptor(interceptor1);
    manager.registerInterceptor(interceptor2);

    auto operation =
        std::make_shared<QGrpcChannelOperation>("service"_L1, "method"_L1, "arg"_L1,
                                                QGrpcCallOptions(),
                                                std::shared_ptr<QAbstractProtobufSerializer>());
    manager.run<QGrpcCallReply>(finalCall, std::make_shared<QGrpcCallReply>(operation), operation);

    // Actual call was called
    QVERIFY(finalCallCalled);
    // Verify each interceptor was called once for a call
    QCOMPARE(interceptorOrder.size(), 2u);
    QCOMPARE(interceptorOrder.at(0), interceptor2Id);
    QCOMPARE(interceptorOrder.at(1), interceptor1Id);
    // Check last value of the metadata
    const auto key = operation->serverMetadata().find("value");
    QVERIFY(key != operation->serverMetadata().end());
    QCOMPARE(key->second, interceptor1Id.latin1());
}

void QtGrpcClientInterceptorTest::ContinuationNotCalledTest()
{
    using Continuation = QGrpcInterceptorContinuation<QGrpcCallReply>;
    constexpr QLatin1StringView interceptor1Id = "inter1"_L1;
    constexpr QLatin1StringView interceptor2Id = "inter2"_L1;
    constexpr QLatin1StringView interceptor3Id = "inter3"_L1;

    bool finalCallCalled = false;
    auto finalCall = Continuation([&finalCallCalled](Continuation::ReplyType response,
                                                     Continuation::ParamType) {
        finalCallCalled = true;
        return response;
    });
    std::vector<QLatin1StringView> interceptorOrder;
    auto interceptorFunc = [&interceptorOrder](std::shared_ptr<QGrpcChannelOperation> operation,
                                               QLatin1StringView id) {
        interceptorOrder.push_back(id);
        operation->setServerMetadata(QGrpcMetadata{ { "value", id.latin1() } });
    };
    auto interceptor1 = std::make_shared<MockInterceptor>(interceptor1Id, interceptorFunc);
    auto interceptor2 = std::make_shared<NoContinuationInterceptor>(interceptor2Id,
                                                                    interceptorFunc);
    auto interceptor3 = std::make_shared<MockInterceptor>(interceptor3Id, interceptorFunc);
    auto manager = QGrpcClientInterceptorManager();
    manager.registerInterceptor(interceptor1);
    manager.registerInterceptor(interceptor2);
    manager.registerInterceptor(interceptor3);

    auto operation =
        std::make_shared<QGrpcChannelOperation>("service"_L1, "method"_L1, "arg"_L1,
                                                QGrpcCallOptions(),
                                                std::shared_ptr<QAbstractProtobufSerializer>());
    manager.run<QGrpcCallReply>(finalCall, std::make_shared<QGrpcCallReply>(operation), operation);

    // Actual call was NOT called
    QVERIFY(!finalCallCalled);
    // Verify only interceptor 3 and 2 were called
    QCOMPARE(interceptorOrder.size(), 2u);
    QCOMPARE(interceptorOrder.at(0), interceptor3Id);
    QCOMPARE(interceptorOrder.at(1), interceptor2Id);
    // Check last value of the metadata
    const auto key = operation->serverMetadata().find("value");
    QVERIFY(key != operation->serverMetadata().end());
    QCOMPARE(key->second, interceptor2Id.latin1());
}

void QtGrpcClientInterceptorTest::BatchRegistrationOrderingTest()
{
    using Continuation = QGrpcInterceptorContinuation<QGrpcCallReply>;
    constexpr QLatin1StringView interceptorIdPrefix = "inter"_L1;

    bool finalCallCalled = false;
    auto finalCall = Continuation([&finalCallCalled](Continuation::ReplyType,
                                                     Continuation::ParamType) {
        finalCallCalled = true;
    });

    std::vector<QLatin1StringView> interceptorOrder;
    auto interceptorFunc = [&interceptorOrder](std::shared_ptr<QGrpcChannelOperation>,
                                               QLatin1StringView id) {
        interceptorOrder.push_back(id);
    };
    size_t interceptorCount = 0;
    auto getNextInterceptor = [&interceptorIdPrefix, &interceptorCount, &interceptorFunc]() {
        auto id = QString("%1%2").arg(interceptorIdPrefix).arg(++interceptorCount).toLatin1();
        return std::make_shared<MockInterceptor>(id, interceptorFunc);
    };

    auto manager = QGrpcClientInterceptorManager();
    manager.registerInterceptor(getNextInterceptor());
    manager.registerInterceptor(getNextInterceptor());
    std::vector<std::shared_ptr<QGrpcClientInterceptor>> interceptorList;
    for (int i = 0; i < 2; ++i) {
        interceptorList.push_back(getNextInterceptor());
    }
    manager.registerInterceptors(interceptorList);

    auto operation =
        std::make_shared<QGrpcChannelOperation>("service"_L1, "method"_L1, "arg"_L1,
                                                QGrpcCallOptions(),
                                                std::shared_ptr<QAbstractProtobufSerializer>());
    manager.run<QGrpcCallReply>(finalCall, std::make_shared<QGrpcCallReply>(operation), operation);

    QVERIFY(finalCallCalled);
    QCOMPARE(interceptorOrder.size(), interceptorCount);
    // Vector in original order
    QCOMPARE(interceptorOrder.at(0), "inter3"_L1);
    QCOMPARE(interceptorOrder.at(1), "inter4"_L1);
    // Single interceptors in reverse order
    QCOMPARE(interceptorOrder.at(2), "inter2"_L1);
    QCOMPARE(interceptorOrder.at(3), "inter1"_L1);
}

void QtGrpcClientInterceptorTest::CancelledInterceptor()
{
    using Continuation = QGrpcInterceptorContinuation<QGrpcCallReply>;
    constexpr QLatin1StringView interceptor1Id = "inter1"_L1;

    bool finalCallCalled = false;
    auto finalCall = Continuation([&finalCallCalled](Continuation::ReplyType,
                                                     Continuation::ParamType) {
        finalCallCalled = true;
    });

    std::vector<QLatin1StringView> interceptorOrder;
    auto modifyFunc = [&interceptorOrder](std::shared_ptr<QGrpcChannelOperation>,
                                          QLatin1StringView id) { interceptorOrder.push_back(id); };
    auto interceptor1 = std::make_shared<MockInterceptor>(interceptor1Id, modifyFunc);

    auto manager = QGrpcClientInterceptorManager();
    manager.registerInterceptor(interceptor1);

    auto operation =
        std::make_shared<QGrpcChannelOperation>("service"_L1, "method"_L1, "arg"_L1,
                                                QGrpcCallOptions(),
                                                std::shared_ptr<QAbstractProtobufSerializer>());
    auto response = std::make_shared<QGrpcCallReply>(operation);

    response->cancel();
    manager.run<QGrpcCallReply>(finalCall, response, operation);

    // Nothing was called
    QVERIFY(!finalCallCalled);
    QCOMPARE(interceptorOrder.size(), 0u);
}

QTEST_MAIN(QtGrpcClientInterceptorTest)

#include "tst_grpc_client_interceptor.moc"
