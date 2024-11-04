// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtQuickTest>

#include <server_proc_runner.h>

class GrpcClientUnarycallQml : public QObject
{
    Q_OBJECT
private:
    ServerProcRunner m_serverProccess{ TEST_GRPC_SERVER_PATH };
};

QUICK_TEST_MAIN_WITH_SETUP(tst_client_qml, GrpcClientUnarycallQml)

#include "tst_client_qml.moc"
