// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include <QSignalSpy>
#include <QTest>
#include <QtGrpc/QGrpcCallOptions>
#include <QtGrpc/QGrpcCallReply>
#include <QtGrpc/QGrpcChannelOptions>

#include <chrono>

#include <grpcclienttestbase.h>
#include <message_latency_defs.h>

#include "testservice_client.grpc.qpb.h"

using namespace Qt::Literals::StringLiterals;
using namespace qtgrpc::tests;

class QtGrpcClientDeadlineTest : public GrpcClientTestBase
{
    Q_OBJECT
public:
    QtGrpcClientDeadlineTest() : GrpcClientTestBase(Channels(Channel::WithChannelDeadline)) { }
private slots:
    void ChannelAndCallDeadlineTest_data();
    void ChannelAndCallDeadlineTest();
};

void QtGrpcClientDeadlineTest::ChannelAndCallDeadlineTest_data()
{
    QTest::addColumn<double>("minTimeout");
    QTest::addColumn<double>("maxTimeout");

    QTest::addRow("0.0") << double(0) << double(0.6);
    QTest::addRow("0.25") << double(0.25) << double(0.6);
}

void QtGrpcClientDeadlineTest::ChannelAndCallDeadlineTest()
{
    QFETCH(double, minTimeout);
    QFETCH(double, maxTimeout);
    const auto minTimeoutDuration = std::chrono::milliseconds(static_cast<int64_t>(MessageLatency
                                                                                   * minTimeout));
    const auto maxTimeoutDuration = std::chrono::milliseconds(static_cast<int64_t>(MessageLatency
                                                                                   * maxTimeout));
    QGrpcCallOptions callOpts;
    callOpts.withDeadline(minTimeoutDuration);

    SimpleStringMessage request;
    request.setTestFieldString("sleep");
    QSignalSpy clientErrorSpy(client().get(), &TestService::Client::errorOccurred);
    QVERIFY(clientErrorSpy.isValid());
    std::shared_ptr<QGrpcCallReply> reply;
    reply = client()->testMethod(request, callOpts);
    QSignalSpy callFinishedSpy(reply.get(), &QGrpcCallReply::finished);
    QVERIFY(callFinishedSpy.isValid());
    // Still waiting for a timeout
    QTRY_COMPARE_EQ_WITH_TIMEOUT(clientErrorSpy.count(), 0, minTimeoutDuration.count());
    // Time window to receive the timout
    QTRY_COMPARE_EQ_WITH_TIMEOUT(clientErrorSpy.count(), 1,
                                 maxTimeoutDuration.count() + MessageLatencyThreshold);

    const auto code = qvariant_cast<QGrpcStatus>(clientErrorSpy.at(0).first()).code();
    // Really low timeout can trigger before service becomes available
    QVERIFY(code == QGrpcStatus::StatusCode::Cancelled
            || code == QGrpcStatus::StatusCode::Unavailable);
}

QTEST_MAIN(QtGrpcClientDeadlineTest)
#include "tst_grpc_client_deadline.moc"
