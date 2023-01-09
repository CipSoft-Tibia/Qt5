/****************************************************************************
**
** Copyright (C) 2017 Ford Motor Company
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtRemoteObjects module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "rep_MyInterface_replica.h"

#include <QCoreApplication>
#include <QtRemoteObjects/qremoteobjectnode.h>
#include <QtTest/QtTest>

class tst_Client_Process : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        m_repNode.reset(new QRemoteObjectNode);
        m_repNode->connectToNode(QUrl(QStringLiteral("tcp://127.0.0.1:65213")));
        m_rep.reset(m_repNode->acquire<MyInterfaceReplica>());
        connect(m_rep.data(), &MyInterfaceReplica::notified, [&]() { m_notified = true; });
        connect(m_rep.data(), &MyInterfaceReplica::initialValueChanged, [&]() {
            // this value is only set when the replica first connects to the source
            QCOMPARE(m_notified, false);
            QCOMPARE(m_rep->initialValue(), 18);
        });
        QVERIFY(m_rep->waitForSource());
    }

    void testRun()
    {
        auto reply = m_rep->start();
        QVERIFY(reply.waitForFinished());

        // BEGIN: Testing
        QSignalSpy advanceSpy(m_rep.data(), &MyInterfaceReplica::advance);

        QSignalSpy spy(m_rep.data(), &MyInterfaceReplica::enum1Changed);
        QVERIFY(advanceSpy.wait());

        QCOMPARE(spy.count(), 2);
        // END: Testing

        reply = m_rep->stop();
        QVERIFY(reply.waitForFinished());
    }

    void testEnumDetails()
    {
        QHash<QByteArray, int> kvs = {{"First", 0}, {"Second", 1}, {"Third", 2}};
        QScopedPointer<QRemoteObjectDynamicReplica> rep(m_repNode->acquireDynamic("MyInterface"));
        QVERIFY(rep->waitForSource());

        auto mo = rep->metaObject();
        int enumIdx = mo->indexOfEnumerator("Enum1");
        QVERIFY(enumIdx != -1);
        auto enumerator = mo->enumerator(enumIdx);
        QCOMPARE(enumerator.name(), "Enum1");
        QCOMPARE(enumerator.keyCount(), 3);
        for (int i = 0; i < 3; ++i) {
            auto key = enumerator.key(i);
            auto val = enumerator.value(i);
            auto it = kvs.find(key);
            QVERIFY(it != kvs.end());
            QCOMPARE(*it, val);
            kvs.erase(it);
        }

        int propIdx = mo->indexOfProperty("enum1");
        QVERIFY(propIdx != -1);
        auto property = mo->property(propIdx);
        property.write(rep.data(), 1);
        QTRY_COMPARE(property.read(rep.data()).toInt(), 1);
    }

    void testMethodSignalParamDetails()
    {
        QScopedPointer<QRemoteObjectDynamicReplica> rep(m_repNode->acquireDynamic("MyInterface"));
        QVERIFY(rep->waitForSource());

        auto mo = rep->metaObject();
        int signalIdx = mo->indexOfSignal("testEnumParamsInSignals(MyInterfaceReplica::Enum1,bool,QString)");
        QVERIFY(signalIdx != -1);
        auto simm = mo->method(signalIdx);
        {
            QCOMPARE(simm.parameterCount(), 3);
            auto paramNames = simm.parameterNames();
            QCOMPARE(paramNames.size(), 3);
            QCOMPARE(paramNames.at(0), QByteArrayLiteral("enumSignalParam"));
            QCOMPARE(paramNames.at(1), QByteArrayLiteral("signalParam2"));
            QCOMPARE(paramNames.at(2), QByteArrayLiteral("__repc_variable_1"));
            QCOMPARE(simm.parameterType(0), QMetaType::type("MyInterfaceReplica::Enum1"));
            QCOMPARE(simm.parameterType(1), int(QMetaType::Bool));
            QCOMPARE(simm.parameterType(2), int(QMetaType::QString));
        }

        int slotIdx = mo->indexOfSlot("testEnumParamsInSlots(MyInterfaceReplica::Enum1,bool,int)");
        QVERIFY(slotIdx != -1);
        auto slmm = mo->method(slotIdx);
        {
            QCOMPARE(slmm .parameterCount(), 3);
            auto paramNames = slmm .parameterNames();
            QCOMPARE(paramNames.size(), 3);
            QCOMPARE(paramNames.at(0), QByteArrayLiteral("enumSlotParam"));
            QCOMPARE(paramNames.at(1), QByteArrayLiteral("slotParam2"));
            QCOMPARE(paramNames.at(2), QByteArrayLiteral("__repc_variable_1"));
        }

        int enumVal = 0;
        mo->invokeMethod(rep.data(), "testEnumParamsInSlots",
                                    QGenericArgument("MyInterfaceReplica::Enum1", &enumVal),
                                    Q_ARG(bool, true), Q_ARG(int, 1234));

        int enumIdx = mo->indexOfProperty("enum1");
        QVERIFY(enumIdx != -1);
        QTRY_COMPARE(mo->property(enumIdx).read(rep.data()).toInt(), 0);

        int startedIdx = mo->indexOfProperty("started");
        QVERIFY(startedIdx != -1);
        QTRY_COMPARE(mo->property(startedIdx).read(rep.data()).toBool(), true);
    }

    void testMethodSignal()
    {
        QScopedPointer<MyInterfaceReplica> rep(new MyInterfaceReplica());
        rep->setNode(m_repNode.get());
        QVERIFY(rep->waitForSource());

        rep->testEnumParamsInSlots(MyInterfaceReplica::Second, false, 74);

        connect(rep.data(), &MyInterfaceReplica::testEnumParamsInSignals,
                [](MyInterfaceReplica::Enum1 enumSignalParam) { QCOMPARE(enumSignalParam, MyInterfaceReplica::Second); });

        QTRY_COMPARE(rep->enum1(), MyInterfaceReplica::Second);
        QTRY_COMPARE(rep->started(), false);
    }

    void testPod()
    {
        QScopedPointer<QRemoteObjectDynamicReplica> podRep(m_repNode->acquireDynamic("PodInterface"));
        QVERIFY(podRep->waitForSource());
        QVariant value = podRep->property("myPod");
        const QMetaObject *mo = QMetaType::metaObjectForType(value.userType());
        const void *gadget = value.constData();

        QMetaProperty iProp = mo->property(mo->indexOfProperty("i"));
        QVariant iValue = iProp.readOnGadget(gadget);
        QCOMPARE(iValue.toInt(), 1);

        QMetaProperty fProp = mo->property(mo->indexOfProperty("f"));
        QVariant fValue = fProp.readOnGadget(gadget);
        QCOMPARE(fValue.toFloat(), 5.0f);

        QMetaProperty sProp = mo->property(mo->indexOfProperty("s"));
        QVariant sValue = sProp.readOnGadget(gadget);
        QCOMPARE(sValue.toString(), QString(QLatin1String("test")));
    }

    void cleanupTestCase()
    {
        auto reply = m_rep->quit();
        QVERIFY(reply.waitForFinished());
        m_rep.reset();
        QVERIFY(QMetaType::type("MyPOD") != QMetaType::UnknownType);
        m_repNode.reset();
        QVERIFY(QMetaType::type("MyPOD") == QMetaType::UnknownType);
    }

private:
    QScopedPointer<QRemoteObjectNode> m_repNode;
    QScopedPointer<MyInterfaceReplica> m_rep;
    bool m_notified = false;
};

QTEST_MAIN(tst_Client_Process)

#include "main.moc"
