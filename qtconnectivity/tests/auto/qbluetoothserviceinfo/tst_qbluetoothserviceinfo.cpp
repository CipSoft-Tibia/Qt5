// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QtTest>
#include <QUuid>

#include <QDebug>

#include <qbluetoothdeviceinfo.h>
#include <qbluetoothserviceinfo.h>
#include <qbluetoothaddress.h>
#include <qbluetoothlocaldevice.h>
#include <qbluetoothuuid.h>
#include <QtBluetooth/QBluetoothServer>

#include <QtCore/qoperatingsystemversion.h>

QT_USE_NAMESPACE

Q_DECLARE_METATYPE(QBluetoothUuid::ProtocolUuid)
Q_DECLARE_METATYPE(QUuid)
Q_DECLARE_METATYPE(QBluetoothServiceInfo::Protocol)

class tst_QBluetoothServiceInfo : public QObject
{
    Q_OBJECT

public:
    tst_QBluetoothServiceInfo();
    ~tst_QBluetoothServiceInfo();

private slots:
    void initTestCase();

    void tst_construction();

    void tst_assignment_data();
    void tst_assignment();

    void tst_serviceClassUuids();

    void tst_writeByteArray();
};

tst_QBluetoothServiceInfo::tst_QBluetoothServiceInfo()
{
}

tst_QBluetoothServiceInfo::~tst_QBluetoothServiceInfo()
{
}

void tst_QBluetoothServiceInfo::initTestCase()
{
    qRegisterMetaType<QBluetoothUuid::ProtocolUuid>();
    qRegisterMetaType<QUuid>();
    qRegisterMetaType<QBluetoothServiceInfo::Protocol>();
}

void tst_QBluetoothServiceInfo::tst_construction()
{
    const QString serviceName("My Service");
    const QString alternateServiceName("Another ServiceName");
    const QBluetoothDeviceInfo deviceInfo(QBluetoothAddress("001122334455"), "Test Device", 0);
    const QBluetoothDeviceInfo alternatedeviceInfo(QBluetoothAddress("554433221100"), "Test Device2", 0);

    QList<QBluetoothUuid::ProtocolUuid> protUuids;
    //list taken from qbluetoothuuid.h
    protUuids << QBluetoothUuid::ProtocolUuid::Sdp;
    protUuids << QBluetoothUuid::ProtocolUuid::Udp;
    protUuids << QBluetoothUuid::ProtocolUuid::Rfcomm;
    protUuids << QBluetoothUuid::ProtocolUuid::Tcp;
    protUuids << QBluetoothUuid::ProtocolUuid::TcsBin;
    protUuids << QBluetoothUuid::ProtocolUuid::TcsAt;
    protUuids << QBluetoothUuid::ProtocolUuid::Att;
    protUuids << QBluetoothUuid::ProtocolUuid::Obex;
    protUuids << QBluetoothUuid::ProtocolUuid::Ip;
    protUuids << QBluetoothUuid::ProtocolUuid::Ftp;
    protUuids << QBluetoothUuid::ProtocolUuid::Http;
    protUuids << QBluetoothUuid::ProtocolUuid::Wsp;
    protUuids << QBluetoothUuid::ProtocolUuid::Bnep;
    protUuids << QBluetoothUuid::ProtocolUuid::Upnp;
    protUuids << QBluetoothUuid::ProtocolUuid::Hidp;
    protUuids << QBluetoothUuid::ProtocolUuid::HardcopyControlChannel;
    protUuids << QBluetoothUuid::ProtocolUuid::HardcopyDataChannel;
    protUuids << QBluetoothUuid::ProtocolUuid::HardcopyNotification;
    protUuids << QBluetoothUuid::ProtocolUuid::Avctp;
    protUuids << QBluetoothUuid::ProtocolUuid::Avdtp;
    protUuids << QBluetoothUuid::ProtocolUuid::Cmtp;
    protUuids << QBluetoothUuid::ProtocolUuid::UdiCPlain;
    protUuids << QBluetoothUuid::ProtocolUuid::McapControlChannel;
    protUuids << QBluetoothUuid::ProtocolUuid::McapDataChannel;
    protUuids << QBluetoothUuid::ProtocolUuid::L2cap;

    {
        QBluetoothServiceInfo serviceInfo;

        QVERIFY(!serviceInfo.isValid());
        QVERIFY(!serviceInfo.isComplete());
        QVERIFY(!serviceInfo.isRegistered());
        QCOMPARE(serviceInfo.serviceName(), QString());
        QCOMPARE(serviceInfo.serviceDescription(), QString());
        QCOMPARE(serviceInfo.serviceProvider(), QString());
        QCOMPARE(serviceInfo.serviceUuid(), QBluetoothUuid());
        QCOMPARE(serviceInfo.serviceClassUuids().size(), 0);
        QCOMPARE(serviceInfo.attributes().size(), 0);
        QCOMPARE(serviceInfo.serverChannel(), -1);
        QCOMPARE(serviceInfo.protocolServiceMultiplexer(), -1);

        for (QBluetoothUuid::ProtocolUuid u : std::as_const(protUuids))
            QCOMPARE(serviceInfo.protocolDescriptor(u).size(), 0);
    }

    {
        QBluetoothServiceInfo serviceInfo;
        serviceInfo.setServiceName(serviceName);
        serviceInfo.setDevice(deviceInfo);

        QVERIFY(serviceInfo.isValid());
        QVERIFY(!serviceInfo.isComplete());
        QVERIFY(!serviceInfo.isRegistered());

        QCOMPARE(serviceInfo.serviceName(), serviceName);
        QCOMPARE(serviceInfo.device().address(), deviceInfo.address());

        QBluetoothServiceInfo copyInfo(serviceInfo);

        QVERIFY(copyInfo.isValid());
        QVERIFY(!copyInfo.isComplete());
        QVERIFY(!copyInfo.isRegistered());

        QCOMPARE(copyInfo.serviceName(), serviceName);
        QCOMPARE(copyInfo.device().address(), deviceInfo.address());


        copyInfo.setAttribute(QBluetoothServiceInfo::ServiceName, alternateServiceName);
        copyInfo.setDevice(alternatedeviceInfo);
        QCOMPARE(copyInfo.serviceName(), alternateServiceName);
        QCOMPARE(copyInfo.attribute(QBluetoothServiceInfo::ServiceName).toString(), alternateServiceName);
        QCOMPARE(serviceInfo.serviceName(), alternateServiceName);
        QCOMPARE(copyInfo.device().address(), alternatedeviceInfo.address());
        QCOMPARE(serviceInfo.device().address(), alternatedeviceInfo.address());

        for (QBluetoothUuid::ProtocolUuid u : std::as_const(protUuids))
            QCOMPARE(serviceInfo.protocolDescriptor(u).size(), 0);
        for (QBluetoothUuid::ProtocolUuid u : std::as_const(protUuids))
            QCOMPARE(copyInfo.protocolDescriptor(u).size(), 0);
    }
}

void tst_QBluetoothServiceInfo::tst_assignment_data()
{
    QTest::addColumn<QUuid>("uuid");
    QTest::addColumn<QBluetoothUuid::ProtocolUuid>("protocolUuid");
    QTest::addColumn<QBluetoothServiceInfo::Protocol>("serviceInfoProtocol");
    QTest::addColumn<bool>("protocolSupported");

    bool l2cpSupported = true;
    //some platforms don't support L2CP
#if defined(QT_ANDROID_BLUETOOTH) || defined(Q_OS_WIN)
    l2cpSupported = false;
#endif

#if defined(Q_OS_MACOS)
    l2cpSupported = QOperatingSystemVersion::current() < QOperatingSystemVersion::MacOSMonterey;
#endif

    QTest::newRow("assignment_data_l2cp")
        << QUuid(0x67c8770b, 0x44f1, 0x410a, 0xab, 0x9a, 0xf9, 0xb5, 0x44, 0x6f, 0x13, 0xee)
        << QBluetoothUuid::ProtocolUuid::L2cap << QBluetoothServiceInfo::L2capProtocol << l2cpSupported;
    QTest::newRow("assignment_data_rfcomm")
        << QUuid(0x67c8770b, 0x44f1, 0x410a, 0xab, 0x9a, 0xf9, 0xb5, 0x44, 0x6f, 0x13, 0xee)
        << QBluetoothUuid::ProtocolUuid::Rfcomm << QBluetoothServiceInfo::RfcommProtocol << true;

}

void tst_QBluetoothServiceInfo::tst_assignment()
{
    QFETCH(QUuid, uuid);
    QFETCH(QBluetoothUuid::ProtocolUuid, protocolUuid);
    QFETCH(QBluetoothServiceInfo::Protocol, serviceInfoProtocol);
    QFETCH(bool, protocolSupported);

    const QString serviceName("My Service");
    const QBluetoothDeviceInfo deviceInfo(QBluetoothAddress("001122334455"), "Test Device", 0);

    QBluetoothServiceInfo serviceInfo;
    serviceInfo.setServiceName(serviceName);
    serviceInfo.setDevice(deviceInfo);

    QVERIFY(serviceInfo.isValid());
    QVERIFY(!serviceInfo.isRegistered());
    QVERIFY(!serviceInfo.isComplete());

    {
        QBluetoothServiceInfo copyInfo = serviceInfo;

        QVERIFY(copyInfo.isValid());
        QVERIFY(!copyInfo.isRegistered());
        QVERIFY(!copyInfo.isComplete());

        QCOMPARE(copyInfo.serviceName(), serviceName);
        QCOMPARE(copyInfo.device().address(), deviceInfo.address());
    }

    {
        QBluetoothServiceInfo copyInfo;

        QVERIFY(!copyInfo.isValid());
        QVERIFY(!copyInfo.isRegistered());
        QVERIFY(!copyInfo.isComplete());

        copyInfo = serviceInfo;

        QVERIFY(copyInfo.isValid());
        QVERIFY(!copyInfo.isRegistered());
        QVERIFY(!copyInfo.isComplete());

        QCOMPARE(copyInfo.serviceName(), serviceName);
        QCOMPARE(copyInfo.device().address(), deviceInfo.address());
    }

    {
        QBluetoothServiceInfo copyInfo1;
        QBluetoothServiceInfo copyInfo2;

        QVERIFY(!copyInfo1.isValid());
        QVERIFY(!copyInfo1.isRegistered());
        QVERIFY(!copyInfo1.isComplete());
        QVERIFY(!copyInfo2.isValid());
        QVERIFY(!copyInfo2.isRegistered());
        QVERIFY(!copyInfo2.isComplete());

        copyInfo1 = copyInfo2 = serviceInfo;

        QVERIFY(copyInfo1.isValid());
        QVERIFY(!copyInfo1.isRegistered());
        QVERIFY(!copyInfo1.isComplete());
        QVERIFY(copyInfo2.isValid());
        QVERIFY(!copyInfo2.isRegistered());
        QVERIFY(!copyInfo2.isComplete());

        QCOMPARE(copyInfo1.serviceName(), serviceName);
        QCOMPARE(copyInfo2.serviceName(), serviceName);
        QCOMPARE(copyInfo1.device().address(), deviceInfo.address());
        QCOMPARE(copyInfo2.device().address(), deviceInfo.address());
    }

    {
        QBluetoothServiceInfo copyInfo;
        QVERIFY(!copyInfo.isValid());
        QVERIFY(!copyInfo.isRegistered());
        QVERIFY(!copyInfo.isComplete());
        copyInfo = serviceInfo;
        QVERIFY(copyInfo.contains(QBluetoothServiceInfo::ServiceName));

        copyInfo.setAttribute(QBluetoothServiceInfo::ProtocolDescriptorList, QBluetoothUuid(uuid));
        QVERIFY(copyInfo.contains(QBluetoothServiceInfo::ProtocolDescriptorList));
        QVERIFY(copyInfo.isComplete());
        QVERIFY(!copyInfo.attributes().isEmpty());

        copyInfo.removeAttribute(QBluetoothServiceInfo::ProtocolDescriptorList);
        QVERIFY(!copyInfo.contains(QBluetoothServiceInfo::ProtocolDescriptorList));
        QVERIFY(!copyInfo.isComplete());
    }

    {
        QBluetoothServiceInfo copyInfo;
        QVERIFY(!copyInfo.isValid());
        copyInfo = serviceInfo;

        QVERIFY(copyInfo.serverChannel() == -1);
        QVERIFY(copyInfo.protocolServiceMultiplexer() == -1);

        QBluetoothServiceInfo::Sequence protocolDescriptorList;
        QBluetoothServiceInfo::Sequence protocol;
        protocol << QVariant::fromValue(QBluetoothUuid(protocolUuid));
        protocolDescriptorList.append(QVariant::fromValue(protocol));
        protocol.clear();

        protocolDescriptorList.append(QVariant::fromValue(protocol));
        copyInfo.setAttribute(QBluetoothServiceInfo::ProtocolDescriptorList,
                                 protocolDescriptorList);
        if (serviceInfoProtocol == QBluetoothServiceInfo::L2capProtocol) {
            QVERIFY(copyInfo.serverChannel() == -1);
            QVERIFY(copyInfo.protocolServiceMultiplexer() != -1);
        } else if (serviceInfoProtocol == QBluetoothServiceInfo::RfcommProtocol) {
            QVERIFY(copyInfo.serverChannel() != -1);
            QVERIFY(copyInfo.protocolServiceMultiplexer() == -1);
        }

        QVERIFY(copyInfo.socketProtocol() == serviceInfoProtocol);
    }

    {
        QBluetoothServiceInfo copyInfo;

        QVERIFY(!copyInfo.isValid());
        copyInfo = serviceInfo;
        copyInfo.setServiceUuid(QBluetoothUuid::ServiceClassUuid::SerialPort);
        QVERIFY(!copyInfo.isRegistered());

        // start Bluetooth if not started
        QBluetoothLocalDevice device;
        if (device.isValid()) {
            device.powerOn();
            int waitPowerOnMs = 1000;
            while (device.hostMode() == QBluetoothLocalDevice::HostPoweredOff && waitPowerOnMs) {
                QTest::qWait(100);
                waitPowerOnMs -= 100;
            }
        }

        if (device.hostMode() == QBluetoothLocalDevice::HostPoweredOff) {
            QSKIP("Skipping test due to missing or powered OFF Bluetooth device");
        } else if (protocolSupported) {
            QBluetoothServer server(serviceInfoProtocol);
            QVERIFY(server.listen());
            QTRY_VERIFY_WITH_TIMEOUT(server.isListening(), 5000);
            QVERIFY(server.serverPort() > 0);

            QBluetoothServiceInfo::Sequence protocolDescriptorList;
            QBluetoothServiceInfo::Sequence protocol;
            protocol << QVariant::fromValue(QBluetoothUuid(QBluetoothUuid::ProtocolUuid::L2cap));

            if (serviceInfoProtocol == QBluetoothServiceInfo::L2capProtocol) {
                protocol << QVariant::fromValue(server.serverPort());
                protocolDescriptorList.append(QVariant::fromValue(protocol));
            } else if (serviceInfoProtocol == QBluetoothServiceInfo::RfcommProtocol) {
                protocolDescriptorList.append(QVariant::fromValue(protocol));
                protocol.clear();
                protocol << QVariant::fromValue(QBluetoothUuid(QBluetoothUuid::ProtocolUuid::Rfcomm))
                         << QVariant::fromValue(quint8(server.serverPort()));
                protocolDescriptorList.append(QVariant::fromValue(protocol));
            }

            serviceInfo.setAttribute(QBluetoothServiceInfo::ProtocolDescriptorList,
                                     protocolDescriptorList);

#if defined(Q_OS_MACOS)
            // bluetoothd on Monterey does not want to register a record if there is no
            // ServiceClassIDList provided.
            if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::MacOSMonterey) {
                // Nothing seems to help with L2CAP though:
                if (serviceInfoProtocol == QBluetoothServiceInfo::RfcommProtocol) {
                    QBluetoothServiceInfo::Sequence classIds;
                    classIds << QVariant::fromValue(QBluetoothUuid(QBluetoothUuid::ServiceClassUuid::SerialPort));
                    copyInfo.setAttribute(QBluetoothServiceInfo::ServiceClassIds, classIds);
                }
            }
#endif // Q_OS_MACOS

            QVERIFY(copyInfo.registerService());
            QVERIFY(copyInfo.isRegistered());
            QVERIFY(serviceInfo.isRegistered());
            QBluetoothServiceInfo secondCopy;
            secondCopy = copyInfo;
            QVERIFY(secondCopy.isRegistered());

            QVERIFY(secondCopy.unregisterService());
            QVERIFY(!copyInfo.isRegistered());
            QVERIFY(!secondCopy.isRegistered());
            QVERIFY(!serviceInfo.isRegistered());
            QVERIFY(server.isListening());
            server.close();
            QVERIFY(!server.isListening());
        }
    }
}

void tst_QBluetoothServiceInfo::tst_serviceClassUuids()
{
    QBluetoothServiceInfo info;
    QCOMPARE(info.serviceClassUuids().size(), 0);

    QBluetoothServiceInfo::Sequence classIds;
    classIds << QVariant::fromValue(QBluetoothUuid(QBluetoothUuid::ServiceClassUuid::SerialPort));
    QCOMPARE(classIds.size(), 1);

    QBluetoothUuid uuid(QString("e8e10f95-1a70-4b27-9ccf-02010264e9c8"));
    classIds.prepend(QVariant::fromValue(uuid));
    QCOMPARE(classIds.size(), 2);
    QCOMPARE(classIds.at(0).value<QBluetoothUuid>(), uuid);

    info.setAttribute(QBluetoothServiceInfo::ServiceClassIds, classIds);
    QList<QBluetoothUuid> svclids = info.serviceClassUuids();
    QCOMPARE(svclids.size(), 2);
    QCOMPARE(svclids.at(0), uuid);
    QCOMPARE(svclids.at(1), QBluetoothUuid(QBluetoothUuid::ServiceClassUuid::SerialPort));
}

static QByteArray debugOutput;

void debugHandler(QtMsgType type, const QMessageLogContext &, const QString &msg)
{
    switch (type) {
        case QtDebugMsg :
            debugOutput = msg.toLocal8Bit();
            break;
        default:
            break;
    }
}

void tst_QBluetoothServiceInfo::tst_writeByteArray()
{
    // We cannot directly test the produced XML output for Bluez
    // as there no public API to retrieve it and it would be Bluez specific.
    // However we can check the debug output.
    // It should contain a qbyteArray rather than a string. In the XML the QByteArray
    // is converted to a text tag with hex encoding.

    const QByteArray expected("\n (518)\tSequence\n (518)\t\tSequence\n (518)\t\t\tuchar 34\n (518)\t\t\tbytearray 05010906a101850105079508750119e029e7150025018102950175088103050795067508150026ff00190029ff8100050895057501190129059102950175039103c005010902a10185020901a1000509190129031500250175019503810275059501810105010930093109381581257f750895038106c0c0\n");

    const QByteArray hidDescriptor =
            QByteArray::fromHex("05010906a101850105079508750119e029e7150025018102950175088103050795067508150026FF00190029FF8100050895057501190129059102950175039103c005010902a10185020901a1000509190129031500250175019503810275059501810105010930093109381581257f750895038106c0c0");
    const QBluetoothServiceInfo::Sequence hidDescriptorList({
        QVariant::fromValue(quint8(0x22)),  // Report type
        QByteArray(hidDescriptor)           // Descriptor array
    });
    const QBluetoothServiceInfo::Sequence hidDescriptorListSeq({
        QVariant::fromValue(hidDescriptorList)
    });
    QBluetoothServiceInfo srvInfo;
    srvInfo.setAttribute(0x0206, QVariant::fromValue(hidDescriptorListSeq));

    const QVariant attribute = srvInfo.attribute(0x0206);
    debugOutput.clear();
    qInstallMessageHandler(debugHandler);
    qDebug() << srvInfo;
    qInstallMessageHandler(nullptr);
    QCOMPARE(debugOutput, expected);
}

QTEST_MAIN(tst_QBluetoothServiceInfo)

#include "tst_qbluetoothserviceinfo.moc"
