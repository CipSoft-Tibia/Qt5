// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2019 Alexey Edelev <semlanik@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGrpc/qtgrpcglobal.h>

#include <QSignalSpy>
#include <QTest>
#include <QtNetwork/QSslConfiguration>

#include <grpcclienttestbase.h>
#include <message_latency_defs.h>

#include "testservice_client.grpc.qpb.h"

using namespace qtgrpc::tests;

class QtGrpcSslClientTest : public GrpcClientTestBase
{
    Q_OBJECT
public:
    QtGrpcSslClientTest()
        : GrpcClientTestBase(Channels(GrpcClientTestBase::Channel::SslNoCredentials))
    {
    }
private slots:
    void IncorrectSecureCredentialsTest();
};

void QtGrpcSslClientTest::IncorrectSecureCredentialsTest()
{
    SimpleStringMessage req;
    req.setTestFieldString("Hello Qt!");

    QSignalSpy errorSpy(client().get(), &TestService::Client::errorOccurred);
    client()->testMethod(req);

    QTRY_COMPARE_EQ_WITH_TIMEOUT(errorSpy.count(), 1, MessageLatencyWithThreshold);
}

QTEST_MAIN(QtGrpcSslClientTest)
#include "tst_grpc_client_ssl.moc"
