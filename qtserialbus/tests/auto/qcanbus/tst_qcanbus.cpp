/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtSerialBus module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtSerialBus/qcanbus.h>
#include <QtSerialBus/qcanbusfactory.h>

#include <QtTest/qtest.h>

class tst_QCanBus : public QObject
{
    Q_OBJECT
public:
    explicit tst_QCanBus();

private slots:
    void initTestCase();
    void plugins();
    void interfaces();
    void createDevice();

private:
    QCanBus *bus = nullptr;
};

tst_QCanBus::tst_QCanBus()
{
}

void tst_QCanBus::initTestCase()
{
#if QT_CONFIG(library)
    /*
     * Set custom path since CI doesn't install test plugins
     */
    QCoreApplication::addLibraryPath(QCoreApplication::applicationDirPath()
                                     + QStringLiteral("/../../../plugins"));
#ifdef Q_OS_WIN
    QCoreApplication::addLibraryPath(QCoreApplication::applicationDirPath()
                                     + QStringLiteral("/../../../../plugins"));
#endif
#endif // QT_CONFIG(library)
    bus = QCanBus::instance();
    QVERIFY(bus);
    QCanBus *sameInstance = QCanBus::instance();
    QCOMPARE(bus, sameInstance);
}

void tst_QCanBus::plugins()
{
    const QStringList pluginList = bus->plugins();
    QVERIFY(!pluginList.isEmpty());
    QVERIFY(pluginList.contains("generic"));
    QVERIFY(pluginList.contains("genericv1"));
}

void tst_QCanBus::interfaces()
{
    // Plugins derived from QCanBusFactory(V1) don't have availableDevices()
    const QList<QCanBusDeviceInfo> deviceListV1 = bus->availableDevices("genericV1");
    QVERIFY(deviceListV1.isEmpty());

    const QList<QCanBusDeviceInfo> pluginList = bus->availableDevices("generic");
    QCOMPARE(pluginList.size(), 1);
    QCOMPARE(pluginList.at(0).name(), QStringLiteral("can0"));
    QVERIFY(pluginList.at(0).isVirtual());
    QVERIFY(pluginList.at(0).hasFlexibleDataRate());
}

void tst_QCanBus::createDevice()
{
    // Assure we can still create plugins derived from QCanBusFactory(V1)
    QCanBusDevice *dummyV1 = bus->createDevice("genericv1", "unused");
    QVERIFY(dummyV1);
    delete dummyV1;

    QString error, error2;
    QCanBusDevice *dummy = bus->createDevice("generic", "unused");
    QCanBusDevice *dummy2 = bus->createDevice("generic", "unused");
    QCanBusDevice *faulty = bus->createDevice("generic", "invalid", &error);
    QCanBusDevice *faulty2 = bus->createDevice("faulty", "faulty", &error2);
    QVERIFY(dummy);
    QVERIFY(dummy2);

    QVERIFY(!faulty);
    QCOMPARE(error, tr("No such interface: 'invalid'"));

    QVERIFY(!faulty2);
    QCOMPARE(error2, tr("No such plugin: 'faulty'"));

    delete dummy;
    delete dummy2;
}

QTEST_MAIN(tst_QCanBus)

#include "tst_qcanbus.moc"
