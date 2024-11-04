// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include <grpcclienttestbase.h>

#include <QtCore/QTimer>
#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

#include <testservice_client.grpc.qpb.h>
#include <message_latency_defs.h>

using namespace Qt::Literals::StringLiterals;
using namespace qtgrpc::tests;

class QtGrpcClientClientStreamTest : public GrpcClientTestBase
{
    Q_OBJECT
public:
    QtGrpcClientClientStreamTest()
        : GrpcClientTestBase(
                Channels{ GrpcClientTestBase::Channel::Qt })
    {
    }

private slots:
    void Valid();
};

void QtGrpcClientClientStreamTest::Valid()
{
    const int ExpectedMessageCount = 4;

    SimpleStringMessage request;
    request.setTestFieldString("Stream");

    auto stream = client()->streamTestMethodClientStream(request);

    int i = 0;
    QTimer sendTimer;
    QObject::connect(&sendTimer, &QTimer::timeout, this, [&]() {
        stream->sendMessage(request);
        if (++i == ExpectedMessageCount)
            sendTimer.stop();
    });

    sendTimer.start(MessageLatency);

    QSignalSpy streamFinishedSpy(stream.get(), &QGrpcClientStream::finished);
    QVERIFY(streamFinishedSpy.isValid());
    QSignalSpy streamErrorSpy(stream.get(), &QGrpcClientStream::errorOccurred);
    QVERIFY(streamErrorSpy.isValid());

    QTRY_COMPARE_EQ_WITH_TIMEOUT(streamFinishedSpy.count(), 1,
                                 MessageLatencyWithThreshold * ExpectedMessageCount);
    QCOMPARE(streamErrorSpy.count(), 0);

    SimpleStringMessage result = stream->read<SimpleStringMessage>();
    QCOMPARE_EQ(result.testFieldString(), "Stream1Stream2Stream3Stream4");
}

QTEST_MAIN(QtGrpcClientClientStreamTest)

#include "tst_grpc_client_clientstream.moc"
