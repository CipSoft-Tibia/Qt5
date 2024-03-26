// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QSignalSpy>
#include <QTest>

class QtGrpcServerTest : public QObject
{
    Q_OBJECT
private slots:
    void TrueTest();
};

void QtGrpcServerTest::TrueTest()
{
    QVERIFY(true);
}

QTEST_MAIN(QtGrpcServerTest)
#include "tst_grpc_server.moc"
