// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include "qqmldebugtestservice.h"
#include "debugutil_p.h"
#include "qqmldebugprocess_p.h"
#include <QtQuickTestUtils/private/qmlutils_p.h>

#include <private/qqmldebugclient_p.h>
#include <private/qqmldebugconnection_p.h>
#include <private/qqmldebugconnector_p.h>

#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>
#include <QtNetwork/qhostaddress.h>
#include <QtQml/qqmlengine.h>
#include <QtCore/qtimer.h>
#include <QtCore/qdebug.h>
#include <QtCore/qthread.h>
#include <QtCore/qlibraryinfo.h>

#define PORT 3769
#define STR_PORT "3769"

class tst_QQmlDebugService : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_QQmlDebugService();

private:
    QQmlDebugConnection *m_conn;
    QQmlDebugTestService *m_service;

private slots:

    void initTestCase() override;
    void checkPortRange();
    void name();
    void version();
    void state();
    void sendMessage();
    void idForObject();
    void objectForId();
    void checkSupportForDataStreamVersion();
    void checkSupportForOldDataStreamVersion();
};

tst_QQmlDebugService::tst_QQmlDebugService()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_QQmlDebugService::initTestCase()
{
    QQmlDataTest::initTestCase();
    QQmlDebugConnector::setPluginKey(QLatin1String("QQmlDebugServer"));
    QQmlDebugConnector::setServices(QStringList()
                                    << QStringLiteral("tst_QQmlDebugService"));
    m_service = new QQmlDebugTestService("tst_QQmlDebugService", 2);

    foreach (const QString &service, QQmlDebuggingEnabler::debuggerServices())
        QCOMPARE(QQmlDebugConnector::instance()->service(service), (QQmlDebugService *)nullptr);
    foreach (const QString &service, QQmlDebuggingEnabler::inspectorServices())
        QCOMPARE(QQmlDebugConnector::instance()->service(service), (QQmlDebugService *)nullptr);
    foreach (const QString &service, QQmlDebuggingEnabler::profilerServices())
        QCOMPARE(QQmlDebugConnector::instance()->service(service), (QQmlDebugService *)nullptr);

    const QString waitingMsg = QString("QML Debugger: Waiting for connection on port %1...").arg(PORT);
    QTest::ignoreMessage(QtDebugMsg, waitingMsg.toLatin1().constData());
    QQmlDebuggingEnabler::startTcpDebugServer(PORT);

    new QQmlEngine(this);

    m_conn = new QQmlDebugConnection(this);

    for (int i = 0; i < 50; ++i) {
        // try for 5 seconds ...
        m_conn->connectToHost("127.0.0.1", PORT);
        if (m_conn->waitForConnected())
            break;
        QTest::qSleep(100);
    }
    QVERIFY(m_conn->isConnected());
}

void tst_QQmlDebugService::checkPortRange()
{
    QScopedPointer<QQmlDebugConnection> connection1(new QQmlDebugConnection());
    QScopedPointer<QQmlDebugProcess> process1(
                new QQmlDebugProcess(QLibraryInfo::path(QLibraryInfo::BinariesPath)
                                     + "/qmlscene", this));

    process1->start(QStringList() << QLatin1String("-qmljsdebugger=port:3782,3792")
                                  << testFile("test.qml"));

    if (!process1->waitForSessionStart())
        QFAIL("could not launch application, or did not get 'Waiting for connection'.");

    const int port1 = process1->debugPort();
    connection1->connectToHost("127.0.0.1", port1);
    if (!connection1->waitForConnected())
        QFAIL("could not connect to host!");

    // Second instance
    QScopedPointer<QQmlDebugConnection> connection2(new QQmlDebugConnection());
    QScopedPointer<QQmlDebugProcess> process2(
                new QQmlDebugProcess(QLibraryInfo::path(QLibraryInfo::BinariesPath)
                                     + "/qmlscene", this));

    process2->start(QStringList() << QLatin1String("-qmljsdebugger=port:3782,3792")
                                  << testFile("test.qml"));

    if (!process2->waitForSessionStart())
        QFAIL("could not launch application, or did not get 'Waiting for connection'.");

    const int port2 = process2->debugPort();
    connection2->connectToHost("127.0.0.1", port2);
    if (!connection2->waitForConnected())
        QFAIL("could not connect to host!");
}

void tst_QQmlDebugService::name()
{
    QCOMPARE(m_service->name(), QLatin1String("tst_QQmlDebugService"));
}

void tst_QQmlDebugService::version()
{
    QCOMPARE(m_service->version(), 2.0f);
}

void tst_QQmlDebugService::state()
{
    QCOMPARE(m_service->state(), QQmlDebugService::Unavailable);

    {
        QQmlDebugTestClient client("tst_QQmlDebugService", m_conn);
        QTRY_COMPARE(client.state(), QQmlDebugClient::Enabled);
        QTRY_COMPARE(m_service->state(), QQmlDebugService::Enabled);
    }

    QTRY_COMPARE(m_service->state(), QQmlDebugService::Unavailable);

    // We can do this because it will never addService()
    QTest::ignoreMessage(QtWarningMsg,
                         "QQmlDebugService: Conflicting plugin name \"tst_QQmlDebugService\"");
    QQmlDebugTestService duplicate("tst_QQmlDebugService");
    QCOMPARE(duplicate.state(), QQmlDebugService::NotConnected);
    QTest::ignoreMessage(QtWarningMsg,
                         "QQmlDebugService: Plugin \"tst_QQmlDebugService\" is not registered.");
}

void tst_QQmlDebugService::sendMessage()
{
    QQmlDebugTestClient client("tst_QQmlDebugService", m_conn);

    QByteArray msg = "hello!";

    QTRY_COMPARE(client.state(), QQmlDebugClient::Enabled);
    QTRY_COMPARE(m_service->state(), QQmlDebugService::Enabled);

    client.sendMessage(msg);
    QByteArray resp = client.waitForResponse();
    QCOMPARE(resp, msg);

    QTest::ignoreMessage(QtWarningMsg,
                         "QQmlDebugService: Conflicting plugin name \"tst_QQmlDebugService\"");
    QQmlDebugTestService duplicate("tst_QQmlDebugService");
    emit duplicate.messageToClient(duplicate.name(), "msg");
    QTest::ignoreMessage(QtWarningMsg,
                         "QQmlDebugService: Plugin \"tst_QQmlDebugService\" is not registered.");
}

void tst_QQmlDebugService::checkSupportForDataStreamVersion()
{
    QQmlDebugTestClient client("tst_QQmlDebugService", m_conn);

    QByteArray msg = "hello!";

    QTRY_COMPARE(client.state(), QQmlDebugClient::Enabled);
    QTRY_COMPARE(m_service->state(), QQmlDebugService::Enabled);

    client.sendMessage(msg);
    QByteArray resp = client.waitForResponse();
    QCOMPARE(resp, msg);
    QCOMPARE(m_conn->currentDataStreamVersion(), int(QDataStream::Qt_DefaultCompiledVersion));
}

void tst_QQmlDebugService::idForObject()
{
    QCOMPARE(QQmlDebugService::idForObject(nullptr), -1);

    QObject *objA = new QObject;

    int idA = QQmlDebugService::idForObject(objA);
    QVERIFY(idA >= 0);
    QCOMPARE(QQmlDebugService::objectForId(idA), objA);

    int idAA = QQmlDebugService::idForObject(objA);
    QCOMPARE(idAA, idA);

    QObject *objB = new QObject;
    int idB = QQmlDebugService::idForObject(objB);
    QVERIFY(idB != idA);
    QCOMPARE(QQmlDebugService::objectForId(idB), objB);

    delete objA;
    delete objB;
}

void tst_QQmlDebugService::objectForId()
{
    QCOMPARE(QQmlDebugService::objectForId(-1), static_cast<QObject*>(nullptr));
    QCOMPARE(QQmlDebugService::objectForId(1), static_cast<QObject*>(nullptr));

    QObject *obj = new QObject;
    int id = QQmlDebugService::idForObject(obj);
    QCOMPARE(QQmlDebugService::objectForId(id), obj);

    delete obj;
    QCOMPARE(QQmlDebugService::objectForId(id), static_cast<QObject*>(nullptr));
}

void tst_QQmlDebugService::checkSupportForOldDataStreamVersion()
{
    //create a new connection;
    delete m_conn;
    m_conn = new QQmlDebugConnection(this);
    m_conn->setMaximumDataStreamVersion(QDataStream::Qt_5_0);
    for (int i = 0; i < 50; ++i) {
        // try for 5 seconds ...
        m_conn->connectToHost("127.0.0.1", PORT);
        if (m_conn->waitForConnected())
            break;
        QTest::qSleep(100);
    }
    QVERIFY(m_conn->isConnected());

    QQmlDebugTestClient client("tst_QQmlDebugService", m_conn);

    QByteArray msg = "hello!";

    QTRY_COMPARE(client.state(), QQmlDebugClient::Enabled);
    QTRY_COMPARE(m_service->state(), QQmlDebugService::Enabled);

    client.sendMessage(msg);
    QByteArray resp = client.waitForResponse();
    QCOMPARE(resp, msg);
    QCOMPARE(m_conn->currentDataStreamVersion(), int(QDataStream::Qt_5_0));
}

QTEST_MAIN(tst_QQmlDebugService)

#include "tst_qqmldebugservice.moc"
