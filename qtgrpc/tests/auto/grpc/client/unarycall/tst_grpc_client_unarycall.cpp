// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2019 Alexey Edelev <semlanik@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <grpcclienttestbase.h>

#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtCore/qbytearray.h>
#include <QtCore/qhash.h>

#include <QtGrpc/QGrpcCallOptions>

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include <memory>

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
            GrpcClientTestBase::Channel::Json,
            #if !defined(Q_OS_DARWIN) && !defined(Q_OS_WIN32)
                            GrpcClientTestBase::Channel::Ssl,
            #endif
        })
    {
    }

private Q_SLOTS:
    void callIsValid();
    void immediateCancel();
    void deferredCancel();
    void asyncClientStatusMessage();
    void asyncStatusMessage();
    void metadata();
};

void QtGrpcClientUnaryCallTest::callIsValid()
{
    SimpleStringMessage request;
    request.setTestFieldString("Hello Qt!");

    std::shared_ptr<SimpleStringMessage> result = std::make_shared<SimpleStringMessage>();
    bool waitForReply = false;
    std::shared_ptr<QGrpcCallReply> reply = client()->testMethod(request);
    connect(
        reply.get(), &QGrpcOperation::finished, this,
        [&reply, resultWeak = std::weak_ptr(result), &waitForReply](const QGrpcStatus &status) {
            QCOMPARE_EQ(status.code(), QtGrpc::StatusCode::Ok);
            auto resultOpt = reply->read<SimpleStringMessage>();
            QVERIFY(resultOpt.has_value());
            if (auto resultVal = resultWeak.lock(); resultVal)
                *resultVal = resultOpt.value();
            waitForReply = true;
        },
        Qt::SingleShotConnection);

    QTRY_COMPARE_EQ_WITH_TIMEOUT(waitForReply, true, MessageLatency);
    QCOMPARE_EQ(result->testFieldString(), "Hello Qt!");
}

void QtGrpcClientUnaryCallTest::immediateCancel()
{
    SimpleStringMessage request;
    request.setTestFieldString("sleep");

    std::shared_ptr<QGrpcCallReply> reply = client()->testMethod(request);

    QSignalSpy replyFinishedSpy(reply.get(), &QGrpcCallReply::finished);
    QVERIFY(replyFinishedSpy.isValid());

    reply->cancel();

    QTRY_COMPARE_EQ_WITH_TIMEOUT(replyFinishedSpy.count(), 1, FailTimeout);

    QCOMPARE_EQ(qvariant_cast<QGrpcStatus>(replyFinishedSpy.at(0).first()).code(),
                QtGrpc::StatusCode::Cancelled);
}

void QtGrpcClientUnaryCallTest::deferredCancel()
{
    SimpleStringMessage request;
    request.setTestFieldString("sleep");

    std::shared_ptr<QGrpcCallReply> reply = client()->testMethod(request);

    QSignalSpy replyFinishedSpy(reply.get(), &QGrpcCallReply::finished);
    QVERIFY(replyFinishedSpy.isValid());

    QTimer::singleShot(MessageLatencyThreshold, reply.get(), &QGrpcCallReply::cancel);

    QTRY_COMPARE_EQ_WITH_TIMEOUT(replyFinishedSpy.count(), 1, FailTimeout);
    auto args = replyFinishedSpy.first();
    QCOMPARE(args.count(), 1);
    QCOMPARE_EQ(args.first().value<QGrpcStatus>().code(), QtGrpc::StatusCode::Cancelled);
}

void QtGrpcClientUnaryCallTest::asyncClientStatusMessage()
{
    SimpleStringMessage request;
    request.setTestFieldString("Some status message");

    auto reply = client()->testMethodStatusMessage(request);

    QSignalSpy replyFinishedSpy(reply.get(), &QGrpcCallReply::finished);
    QVERIFY(replyFinishedSpy.isValid());

    QTRY_COMPARE_EQ_WITH_TIMEOUT(replyFinishedSpy.count(), 1, FailTimeout);

    QCOMPARE(qvariant_cast<QGrpcStatus>(replyFinishedSpy.at(0).first()).message(),
             request.testFieldString());
}

void QtGrpcClientUnaryCallTest::asyncStatusMessage()
{
    SimpleStringMessage request;
    request.setTestFieldString("Some status message");

    std::shared_ptr<QGrpcCallReply> reply = client()->testMethodStatusMessage(request);

    QSignalSpy replyFinishedSpy(reply.get(), &QGrpcCallReply::finished);
    QVERIFY(replyFinishedSpy.isValid());

    QTRY_COMPARE_EQ_WITH_TIMEOUT(replyFinishedSpy.count(), 1, FailTimeout);
    auto args = replyFinishedSpy.first();
    QCOMPARE(args.count(), 1);
    QCOMPARE(args.first().value<QGrpcStatus>().message(), request.testFieldString());
}

void QtGrpcClientUnaryCallTest::metadata()
{
    QGrpcCallOptions opt;
    opt.setMetadata({
        { "client_header",        "1"           },
        { "client_return_header", "valid_value" }
    });
    auto reply = client()->testMetadata({}, opt);

    QHash<QByteArray, QByteArray> metadata;
    QEventLoop waiter;

    connect(
        reply.get(), &QGrpcOperation::finished, this,
        [&reply, &metadata, &waiter](const QGrpcStatus &) {
            metadata = reply->metadata();
            waiter.quit();
        },
        Qt::SingleShotConnection);

    waiter.exec();

    int serverHeaderCount = 0;
    QByteArray clientReturnHeader;
    for (const auto &[key, value] : reply->metadata().asKeyValueRange()) {
        if (key == "server_header") {
            QCOMPARE_EQ(QString::fromLatin1(value), QString::number(++serverHeaderCount));
        } else if (key == "client_return_header") {
            clientReturnHeader = value;
        }
    }

    QCOMPARE_EQ(serverHeaderCount, 1);
    QCOMPARE_EQ(clientReturnHeader, "valid_value"_ba);
}

QTEST_MAIN(QtGrpcClientUnaryCallTest)

#include "tst_grpc_client_unarycall.moc"
