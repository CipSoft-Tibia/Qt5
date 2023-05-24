// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2019 Alexey Edelev <semlanik@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QGrpcHttp2Channel>

#include <QCoreApplication>
#include <QSignalSpy>
#include <QTest>
#include <QtCore/QThread>

#include "testservice_client.grpc.qpb.h"

namespace {
using namespace qtgrpc::tests;
}

class QtGrpcUnattachedChannelClientTest : public QObject
{
    Q_OBJECT
private slots:
    void CheckMethodsGeneration();
    void ClientSyncTestUnattachedChannel();
    void ClientSyncTestUnattachedChannelSignal();
    void AttachChannelThreadTest();
};

void QtGrpcUnattachedChannelClientTest::CheckMethodsGeneration()
{
    // Dummy compile time check of functions generation and interface compatibility
    TestService::Client client;
    QGrpcChannelOptions channelOptions{ QUrl() };
    client.attachChannel(std::make_shared<QGrpcHttp2Channel>(channelOptions));
    SimpleStringMessage request;
    auto result = std::make_shared<SimpleStringMessage>();
    client.testMethod(request, result.get());
    client.testMethod(request);
    client.testMethod(request, &client, [](std::shared_ptr<QGrpcCallReply>) {});
}

void QtGrpcUnattachedChannelClientTest::ClientSyncTestUnattachedChannel()
{
    TestService::Client client;
    SimpleStringMessage request;
    request.setTestFieldString("Some status message");
    auto result = std::make_shared<SimpleStringMessage>();

    QGrpcStatus status = client.testMethodStatusMessage(request, result.get());

    QCOMPARE_EQ(status.code(), QGrpcStatus::Unknown);
    QCOMPARE_EQ("Serializing failed. Serializer is not ready.", status.message());
}

void QtGrpcUnattachedChannelClientTest::ClientSyncTestUnattachedChannelSignal()
{
    TestService::Client client;
    SimpleStringMessage request;
    request.setTestFieldString("Some status message");
    auto result = std::make_shared<SimpleStringMessage>();

    QSignalSpy clientErrorSpy(&client, &TestService::Client::errorOccurred);
    QVERIFY(clientErrorSpy.isValid());

    client.testMethodStatusMessage(request, result.get());

    QTRY_COMPARE_EQ(clientErrorSpy.count(), 1);
    QCOMPARE(qvariant_cast<QGrpcStatus>(clientErrorSpy.at(0).first()).code(), QGrpcStatus::Unknown);
    QCOMPARE(qvariant_cast<QGrpcStatus>(clientErrorSpy.at(0).first()).message(),
             "Serializing failed. Serializer is not ready.");
}

void QtGrpcUnattachedChannelClientTest::AttachChannelThreadTest()
{
    std::shared_ptr<QGrpcHttp2Channel> channel;
    QGrpcChannelOptions channelOptions(QUrl("http://localhost:50051", QUrl::StrictMode));

    std::shared_ptr<QThread> thread(QThread::create([&] {
        channel = std::make_shared<QGrpcHttp2Channel>(channelOptions);
    }));
    thread->start();
    thread->wait();

    TestService::Client client;

    QSignalSpy clientErrorSpy(&client, &TestService::Client::errorOccurred);
    QVERIFY(clientErrorSpy.isValid());

    client.attachChannel(channel);

    QTRY_COMPARE_EQ(clientErrorSpy.count(), 1);
    QCOMPARE(qvariant_cast<QGrpcStatus>(clientErrorSpy.at(0).first()).code(), QGrpcStatus::Unknown);
    QVERIFY(qvariant_cast<QGrpcStatus>(clientErrorSpy.at(0).first())
                    .message()
                    .startsWith("QAbstractGrpcClient::attachChannel is called from a different "
                                "thread."));
}

QTEST_MAIN(QtGrpcUnattachedChannelClientTest)

#include "tst_grpc_unattached_channel.moc"
