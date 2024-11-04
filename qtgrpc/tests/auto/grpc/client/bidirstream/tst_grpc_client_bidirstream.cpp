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

class QtGrpcClientBidirStreamTest : public GrpcClientTestBase
{
    Q_OBJECT
public:
    QtGrpcClientBidirStreamTest()
        : GrpcClientTestBase(
                Channels{ GrpcClientTestBase::Channel::Qt })
    {
    }

private slots:
    void Valid();
};

void QtGrpcClientBidirStreamTest::Valid()
{
    const int ExpectedMessageCount = 4;

    SimpleStringMessage request;
    request.setTestFieldString("Stream");

    auto stream = client()->streamTestMethodBiStream(request);

    QString fullResponse;
    int i = 0;
    QObject::connect(stream.get(), &QGrpcBidirStream::messageReceived, this,
                     [stream, &request, &fullResponse, &i]() {
                         SimpleStringMessage rsp = stream->read<SimpleStringMessage>();
                         fullResponse += rsp.testFieldString() + QString::number(++i);
                         stream->sendMessage(request);
                     });

    QSignalSpy streamFinishedSpy(stream.get(), &QGrpcBidirStream::finished);
    QVERIFY(streamFinishedSpy.isValid());
    QSignalSpy streamErrorSpy(stream.get(), &QGrpcBidirStream::errorOccurred);
    QVERIFY(streamErrorSpy.isValid());

    QTRY_COMPARE_EQ_WITH_TIMEOUT(streamFinishedSpy.count(), 1,
                                 MessageLatencyWithThreshold * ExpectedMessageCount);
    QCOMPARE(streamErrorSpy.count(), 0);

    QCOMPARE_EQ(fullResponse, "Stream11Stream22Stream33Stream44");
}

QTEST_MAIN(QtGrpcClientBidirStreamTest)

#include "tst_grpc_client_bidirstream.moc"
