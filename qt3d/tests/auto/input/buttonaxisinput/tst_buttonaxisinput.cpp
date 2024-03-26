// Copyright (C) 2016 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QTest>
#include <qbackendnodetester.h>
#include "testdevice.h"

#include <Qt3DCore/private/qnode_p.h>
#include <Qt3DCore/private/qscene_p.h>
#include <Qt3DInput/private/buttonaxisinput_p.h>
#include <Qt3DInput/private/inputhandler_p.h>
#include <Qt3DInput/QButtonAxisInput>

class tst_ButtonAxisInput: public Qt3DCore::QBackendNodeTester
{
    Q_OBJECT

private Q_SLOTS:

    void checkPeerPropertyMirroring()
    {
        // GIVEN
        Qt3DInput::Input::ButtonAxisInput backendAxisInput;
        Qt3DInput::QButtonAxisInput axisInput;
        TestDevice sourceDevice;

        axisInput.setButtons(QList<int> { 1 << 8 });
        axisInput.setScale(0.5f);
        axisInput.setAcceleration(0.42f);
        axisInput.setDeceleration(0.43f);
        axisInput.setSourceDevice(&sourceDevice);

        // WHEN
        simulateInitializationSync(&axisInput, &backendAxisInput);

        // THEN
        QCOMPARE(backendAxisInput.peerId(), axisInput.id());
        QCOMPARE(backendAxisInput.isEnabled(), axisInput.isEnabled());
        QCOMPARE(backendAxisInput.buttons(), axisInput.buttons());
        QCOMPARE(backendAxisInput.scale(), axisInput.scale());
        QCOMPARE(backendAxisInput.acceleration(), axisInput.acceleration());
        QCOMPARE(backendAxisInput.deceleration(), axisInput.deceleration());
        QCOMPARE(backendAxisInput.sourceDevice(), sourceDevice.id());

        QCOMPARE(backendAxisInput.speedRatio(), 0.0f);
        QCOMPARE(backendAxisInput.lastUpdateTime(), 0);
    }

    void checkInitialAndCleanedUpState()
    {
        // GIVEN
        Qt3DInput::Input::ButtonAxisInput backendAxisInput;

        // THEN
        QVERIFY(backendAxisInput.peerId().isNull());
        QCOMPARE(backendAxisInput.scale(), 0.0f);
        QVERIFY(qIsInf(backendAxisInput.acceleration()));
        QVERIFY(qIsInf(backendAxisInput.deceleration()));
        QCOMPARE(backendAxisInput.speedRatio(), 0.0f);
        QCOMPARE(backendAxisInput.lastUpdateTime(), 0);
        QVERIFY(backendAxisInput.buttons().isEmpty());
        QCOMPARE(backendAxisInput.isEnabled(), false);
        QCOMPARE(backendAxisInput.sourceDevice(), Qt3DCore::QNodeId());

        // GIVEN
        Qt3DInput::QButtonAxisInput axisInput;
        TestDevice sourceDevice;

        axisInput.setButtons(QList<int> { 1 << 8 });
        axisInput.setScale(0.5f);
        axisInput.setSourceDevice(&sourceDevice);

        // WHEN
        simulateInitializationSync(&axisInput, &backendAxisInput);
        backendAxisInput.cleanup();

        // THEN
        QCOMPARE(backendAxisInput.scale(), 0.0f);
        QVERIFY(qIsInf(backendAxisInput.acceleration()));
        QVERIFY(qIsInf(backendAxisInput.deceleration()));
        QCOMPARE(backendAxisInput.speedRatio(), 0.0f);
        QCOMPARE(backendAxisInput.lastUpdateTime(), 0);
        QVERIFY(backendAxisInput.buttons().isEmpty());
        QCOMPARE(backendAxisInput.isEnabled(), false);
        QCOMPARE(backendAxisInput.sourceDevice(), Qt3DCore::QNodeId());
    }

    void checkPropertyChanges()
    {
        // GIVEN
        Qt3DInput::QButtonAxisInput axisInput;
        Qt3DInput::Input::ButtonAxisInput backendAxisInput;
        simulateInitializationSync(&axisInput, &backendAxisInput);

        // WHEN
        axisInput.setButtons(QList<int> { 64 });
        backendAxisInput.syncFromFrontEnd(&axisInput, false);

        // THEN
        QCOMPARE(backendAxisInput.buttons(), QList<int> { 64 });

        // WHEN
        axisInput.setScale(0.5f);
        backendAxisInput.syncFromFrontEnd(&axisInput, false);

        // THEN
        QCOMPARE(backendAxisInput.scale(), 0.5f);

        // WHEN
        axisInput.setEnabled(false);
        backendAxisInput.syncFromFrontEnd(&axisInput, false);

        // THEN
        QCOMPARE(backendAxisInput.isEnabled(), false);

        // WHEN
        TestDevice device;
        axisInput.setSourceDevice(&device);
        backendAxisInput.syncFromFrontEnd(&axisInput, false);

        // THEN
        QCOMPARE(backendAxisInput.sourceDevice(), device.id());

        // WHEN
        axisInput.setAcceleration(0.42f);
        backendAxisInput.syncFromFrontEnd(&axisInput, false);

        // THEN
        QCOMPARE(backendAxisInput.acceleration(), 0.42f);

        // WHEN
        axisInput.setAcceleration(-0.42f);
        backendAxisInput.syncFromFrontEnd(&axisInput, false);

        // THEN
        QVERIFY(qIsInf(backendAxisInput.acceleration()));

        // WHEN
        axisInput.setDeceleration(0.43f);
        backendAxisInput.syncFromFrontEnd(&axisInput, false);

        // THEN
        QCOMPARE(backendAxisInput.deceleration(), 0.43f);

        // WHEN
        axisInput.setDeceleration(-0.43f);
        backendAxisInput.syncFromFrontEnd(&axisInput, false);

        // THEN
        QVERIFY(qIsInf(backendAxisInput.deceleration()));
    }

    void shouldProcessAndUpdateSpeedRatioOverTime()
    {
        const qint64 s = 1000000000;

        // GIVEN
        TestDeviceIntegration deviceIntegration;
        TestDevice *device = deviceIntegration.createPhysicalDevice("keyboard");
        TestDeviceBackendNode *deviceBackend = deviceIntegration.physicalDevice(device->id());
        Qt3DInput::Input::InputHandler handler;
        handler.addInputDeviceIntegration(&deviceIntegration);

        Qt3DInput::Input::ButtonAxisInput backendAxisInput;
        Qt3DInput::QButtonAxisInput axisInput;
        axisInput.setEnabled(true);
        axisInput.setButtons(QList<int> { Qt::Key_Space });
        axisInput.setScale(-1.0f);
        axisInput.setAcceleration(0.15f);
        axisInput.setDeceleration(0.3f);
        axisInput.setSourceDevice(device);
        simulateInitializationSync(&axisInput, &backendAxisInput);
        QCOMPARE(backendAxisInput.speedRatio(), 0.0f);
        QCOMPARE(backendAxisInput.lastUpdateTime(), 0);

        // WHEN (accelerate)
        deviceBackend->setButtonPressed(Qt::Key_Space, true);

        // WHEN
        QCOMPARE(backendAxisInput.process(&handler, 30 * s), 0.0f);

        // THEN
        QCOMPARE(backendAxisInput.speedRatio(), 0.0f);
        QCOMPARE(backendAxisInput.lastUpdateTime(), 30 * s);

        // WHEN
        QCOMPARE(backendAxisInput.process(&handler, 31 * s), -0.15f);

        // THEN
        QCOMPARE(backendAxisInput.speedRatio(), 0.15f);
        QCOMPARE(backendAxisInput.lastUpdateTime(), 31 * s);

        // WHEN
        QCOMPARE(backendAxisInput.process(&handler, 32 * s), -0.3f);

        // THEN
        QCOMPARE(backendAxisInput.speedRatio(), 0.3f);
        QCOMPARE(backendAxisInput.lastUpdateTime(), 32 * s);

        // WHEN
        QCOMPARE(backendAxisInput.process(&handler, 35 * s), -0.75f);

        // THEN
        QCOMPARE(backendAxisInput.speedRatio(), 0.75f);
        QCOMPARE(backendAxisInput.lastUpdateTime(), 35 * s);

        // WHEN
        QCOMPARE(backendAxisInput.process(&handler, 37 * s), -1.0f);

        // THEN
        QCOMPARE(backendAxisInput.speedRatio(), 1.0f);
        QCOMPARE(backendAxisInput.lastUpdateTime(), 37 * s);

        // WHEN
        QCOMPARE(backendAxisInput.process(&handler, 38 * s), -1.0f);

        // THEN
        QCOMPARE(backendAxisInput.speedRatio(), 1.0f);
        QCOMPARE(backendAxisInput.lastUpdateTime(), 38 * s);

        // WHEN
        QCOMPARE(backendAxisInput.process(&handler, 42 * s), -1.0f);

        // THEN
        QCOMPARE(backendAxisInput.speedRatio(), 1.0f);
        QCOMPARE(backendAxisInput.lastUpdateTime(), 42 * s);


        // WHEN (decelerate)
        deviceBackend->setButtonPressed(Qt::Key_Space, false);

        // WHEN
        QCOMPARE(backendAxisInput.process(&handler, 43 * s), -0.7f);

        // THEN
        QCOMPARE(backendAxisInput.speedRatio(), 0.7f);
        QCOMPARE(backendAxisInput.lastUpdateTime(), 43 * s);

        // WHEN
        QCOMPARE(backendAxisInput.process(&handler, 45 * s), -0.1f);

        // THEN
        QCOMPARE(backendAxisInput.speedRatio(), 0.1f);
        QCOMPARE(backendAxisInput.lastUpdateTime(), 45 * s);

        // WHEN
        QCOMPARE(backendAxisInput.process(&handler, 46 * s), 0.0f);

        // THEN
        QCOMPARE(backendAxisInput.speedRatio(), 0.0f);
        QCOMPARE(backendAxisInput.lastUpdateTime(), 0);
    }

    void shouldNotProcessWhenDisabled()
    {
        const qint64 s = 1000000000;

        // GIVEN
        TestDeviceIntegration deviceIntegration;
        TestDevice *device = deviceIntegration.createPhysicalDevice("keyboard");
        TestDeviceBackendNode *deviceBackend = deviceIntegration.physicalDevice(device->id());
        Qt3DInput::Input::InputHandler handler;
        handler.addInputDeviceIntegration(&deviceIntegration);

        Qt3DInput::Input::ButtonAxisInput backendAxisInput;
        Qt3DInput::QButtonAxisInput axisInput;
        axisInput.setEnabled(false);
        axisInput.setButtons(QList<int> { Qt::Key_Space });
        axisInput.setScale(-1.0f);
        axisInput.setAcceleration(0.15f);
        axisInput.setDeceleration(0.3f);
        axisInput.setSourceDevice(device);
        simulateInitializationSync(&axisInput, &backendAxisInput);
        QCOMPARE(backendAxisInput.speedRatio(), 0.0f);
        QCOMPARE(backendAxisInput.lastUpdateTime(), 0);

        // WHEN (accelerate)
        deviceBackend->setButtonPressed(Qt::Key_Space, true);

        // WHEN
        QCOMPARE(backendAxisInput.process(&handler, 30 * s), 0.0f);

        // THEN
        QCOMPARE(backendAxisInput.speedRatio(), 0.0f);
        QCOMPARE(backendAxisInput.lastUpdateTime(), 0);

        // WHEN
        QCOMPARE(backendAxisInput.process(&handler, 31 * s), 0.0f);

        // THEN
        QCOMPARE(backendAxisInput.speedRatio(), 0.0f);
        QCOMPARE(backendAxisInput.lastUpdateTime(), 0);
    }
};

QTEST_APPLESS_MAIN(tst_ButtonAxisInput)

#include "tst_buttonaxisinput.moc"
