// Copyright (C) 2016 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QTest>
#include <qbackendnodetester.h>
#include "testdevice.h"

#include <Qt3DInput/private/actioninput_p.h>
#include <Qt3DInput/private/inputhandler_p.h>
#include <Qt3DInput/private/inputmanagers_p.h>
#include <Qt3DInput/private/inputsequence_p.h>
#include <Qt3DInput/QActionInput>
#include <Qt3DInput/QInputSequence>

class tst_InputSequence : public Qt3DCore::QBackendNodeTester
{
    Q_OBJECT
private Q_SLOTS:
    void shouldMirrorPeerProperties()
    {
        // GIVEN
        Qt3DInput::Input::InputSequence backendInputSequence;
        Qt3DInput::QInputSequence inputSequence;
        Qt3DInput::QActionInput actionInput;

        inputSequence.setTimeout(250);
        inputSequence.setButtonInterval(100);
        inputSequence.addSequence(&actionInput);

        // WHEN
        simulateInitializationSync(&inputSequence, &backendInputSequence);

        // THEN
        QCOMPARE(backendInputSequence.peerId(), inputSequence.id());
        QCOMPARE(backendInputSequence.isEnabled(), inputSequence.isEnabled());
        QCOMPARE(backendInputSequence.timeout(), inputSequence.timeout() * 1000000);
        QCOMPARE(backendInputSequence.buttonInterval(), inputSequence.buttonInterval() * 1000000);
        QCOMPARE(backendInputSequence.sequences().size(), inputSequence.sequences().size());

        const int inputsCount = backendInputSequence.sequences().size();
        if (inputsCount > 0) {
            for (int i = 0; i < inputsCount; ++i)
                QCOMPARE(backendInputSequence.sequences().at(i), inputSequence.sequences().at(i)->id());
        }
    }

    void shouldHaveInitialAndCleanedUpStates()
    {
        // GIVEN
        Qt3DInput::Input::InputSequence backendInputSequence;

        // THEN
        QVERIFY(backendInputSequence.peerId().isNull());
        QCOMPARE(backendInputSequence.isEnabled(), false);
        QCOMPARE(backendInputSequence.timeout(), 0);
        QCOMPARE(backendInputSequence.buttonInterval(), 0);
        QCOMPARE(backendInputSequence.sequences().size(), 0);

        // GIVEN
        Qt3DInput::QInputSequence inputSequence;
        Qt3DInput::QActionInput actionInput;

        inputSequence.setTimeout(250);
        inputSequence.setButtonInterval(100);
        inputSequence.addSequence(&actionInput);

        // WHEN
        simulateInitializationSync(&inputSequence, &backendInputSequence);
        backendInputSequence.cleanup();

        // THEN
        QCOMPARE(backendInputSequence.isEnabled(), false);
        QCOMPARE(backendInputSequence.timeout(), 0);
        QCOMPARE(backendInputSequence.buttonInterval(), 0);
        QCOMPARE(backendInputSequence.sequences().size(), 0);
    }

    void shouldHandlePropertyChanges()
    {
        // GIVEN
        Qt3DInput::QInputSequence inputSequence;
        Qt3DInput::Input::InputSequence backendInputSequence;
        simulateInitializationSync(&inputSequence, &backendInputSequence);

        // WHEN
        inputSequence.setTimeout(250);
        backendInputSequence.syncFromFrontEnd(&inputSequence, false);

        // THEN
        QCOMPARE(backendInputSequence.timeout(), 250000000);

        // WHEN
        inputSequence.setButtonInterval(150);
        backendInputSequence.syncFromFrontEnd(&inputSequence, false);

        // THEN
        QCOMPARE(backendInputSequence.buttonInterval(), 150000000);

        // WHEN
        inputSequence.setEnabled(false);
        backendInputSequence.syncFromFrontEnd(&inputSequence, false);

        // THEN
        QCOMPARE(backendInputSequence.isEnabled(), false);

        // WHEN
        Qt3DInput::QActionInput input;
        const Qt3DCore::QNodeId inputId = input.id();
        inputSequence.addSequence(&input);
        backendInputSequence.syncFromFrontEnd(&inputSequence, false);

        // THEN
        QCOMPARE(backendInputSequence.sequences().size(), 1);
        QCOMPARE(backendInputSequence.sequences().first(), inputId);

        // WHEN
        inputSequence.removeSequence(&input);
        backendInputSequence.syncFromFrontEnd(&inputSequence, false);

        // THEN
        QCOMPARE(backendInputSequence.sequences().size(), 0);
    }

    void shouldActivateWhenSequenceIsConsumedInOrderOnly()
    {
        // GIVEN
        TestDeviceIntegration deviceIntegration;
        TestDevice *device = deviceIntegration.createPhysicalDevice("keyboard");
        TestDeviceBackendNode *deviceBackend = deviceIntegration.physicalDevice(device->id());
        Qt3DInput::Input::InputHandler handler;
        handler.addInputDeviceIntegration(&deviceIntegration);

        auto firstInput = new Qt3DInput::QActionInput;
        firstInput->setButtons(QList<int> { Qt::Key_Q, Qt::Key_A });
        firstInput->setSourceDevice(device);
        auto backendFirstInput = handler.actionInputManager()->getOrCreateResource(firstInput->id());
        simulateInitializationSync(firstInput, backendFirstInput);

        auto secondInput = new Qt3DInput::QActionInput;
        secondInput->setButtons(QList<int> { Qt::Key_S, Qt::Key_W });
        secondInput->setSourceDevice(device);
        auto backendSecondInput = handler.actionInputManager()->getOrCreateResource(secondInput->id());
        simulateInitializationSync(secondInput, backendSecondInput);

        auto thirdInput = new Qt3DInput::QActionInput;
        thirdInput->setButtons(QList<int> { Qt::Key_D, Qt::Key_E });
        thirdInput->setSourceDevice(device);
        auto backendThirdInput = handler.actionInputManager()->getOrCreateResource(thirdInput->id());
        simulateInitializationSync(thirdInput, backendThirdInput);

        Qt3DInput::Input::InputSequence backendInputSequence;
        Qt3DInput::QInputSequence inputSequence;
        inputSequence.setEnabled(true);
        inputSequence.setButtonInterval(150);
        inputSequence.setTimeout(450);
        inputSequence.addSequence(firstInput);
        inputSequence.addSequence(secondInput);
        inputSequence.addSequence(thirdInput);
        simulateInitializationSync(&inputSequence, &backendInputSequence);

        // WHEN
        deviceBackend->setButtonPressed(Qt::Key_Up, true);

        // THEN
        QCOMPARE(backendInputSequence.process(&handler, 1000000000), false);

        // WHEN
        deviceBackend->setButtonPressed(Qt::Key_Up, false);
        deviceBackend->setButtonPressed(Qt::Key_Q, true);

        // THEN
        QCOMPARE(backendInputSequence.process(&handler, 1100000000), false);

        // WHEN
        deviceBackend->setButtonPressed(Qt::Key_Q, false);
        deviceBackend->setButtonPressed(Qt::Key_S, true);

        // THEN
        QCOMPARE(backendInputSequence.process(&handler, 1200000000), false);

        // WHEN
        deviceBackend->setButtonPressed(Qt::Key_S, false);
        deviceBackend->setButtonPressed(Qt::Key_E, true);

        // THEN
        QCOMPARE(backendInputSequence.process(&handler, 1300000000), true);

        // WHEN
        deviceBackend->setButtonPressed(Qt::Key_E, false);

        // THEN
        QCOMPARE(backendInputSequence.process(&handler, 1400000000), false);

        // WHEN
        deviceBackend->setButtonPressed(Qt::Key_Q, true);

        // THEN
        QCOMPARE(backendInputSequence.process(&handler, 1500000000), false);

        // WHEN
        deviceBackend->setButtonPressed(Qt::Key_Q, false);
        deviceBackend->setButtonPressed(Qt::Key_S, true);

        // THEN
        QCOMPARE(backendInputSequence.process(&handler, 1600000000), false);

        // WHEN
        deviceBackend->setButtonPressed(Qt::Key_S, false);
        deviceBackend->setButtonPressed(Qt::Key_E, true);

        // THEN
        QCOMPARE(backendInputSequence.process(&handler, 1700000000), true);

        // WHEN
        deviceBackend->setButtonPressed(Qt::Key_E, false);

        // THEN
        QCOMPARE(backendInputSequence.process(&handler, 1800000000), false);


        // Now out of order

        // WHEN
        deviceBackend->setButtonPressed(Qt::Key_S, true);

        // THEN
        QCOMPARE(backendInputSequence.process(&handler, 1900000000), false);

        // WHEN
        deviceBackend->setButtonPressed(Qt::Key_S, false);
        deviceBackend->setButtonPressed(Qt::Key_Q, true);

        // THEN
        QCOMPARE(backendInputSequence.process(&handler, 2000000000), false);

        // WHEN
        deviceBackend->setButtonPressed(Qt::Key_Q, false);
        deviceBackend->setButtonPressed(Qt::Key_D, true);

        // THEN
        QCOMPARE(backendInputSequence.process(&handler, 2100000000), false);

        // WHEN
        deviceBackend->setButtonPressed(Qt::Key_D, false);

        // THEN
        QCOMPARE(backendInputSequence.process(&handler, 22000000000), false);
    }

    void shouldRespectSequenceTimeout()
    {
        // GIVEN
        TestDeviceIntegration deviceIntegration;
        TestDevice *device = deviceIntegration.createPhysicalDevice("keyboard");
        TestDeviceBackendNode *deviceBackend = deviceIntegration.physicalDevice(device->id());
        Qt3DInput::Input::InputHandler handler;
        handler.addInputDeviceIntegration(&deviceIntegration);

        auto firstInput = new Qt3DInput::QActionInput;
        firstInput->setButtons(QList<int> { Qt::Key_Q, Qt::Key_A });
        firstInput->setSourceDevice(device);
        auto backendFirstInput = handler.actionInputManager()->getOrCreateResource(firstInput->id());
        simulateInitializationSync(firstInput, backendFirstInput);

        auto secondInput = new Qt3DInput::QActionInput;
        secondInput->setButtons(QList<int> { Qt::Key_S, Qt::Key_W });
        secondInput->setSourceDevice(device);
        auto backendSecondInput = handler.actionInputManager()->getOrCreateResource(secondInput->id());
        simulateInitializationSync(secondInput, backendSecondInput);

        auto thirdInput = new Qt3DInput::QActionInput;
        thirdInput->setButtons(QList<int> { Qt::Key_D, Qt::Key_E });
        thirdInput->setSourceDevice(device);
        auto backendThirdInput = handler.actionInputManager()->getOrCreateResource(thirdInput->id());
        simulateInitializationSync(thirdInput, backendThirdInput);

        Qt3DInput::Input::InputSequence backendInputSequence;
        Qt3DInput::QInputSequence inputSequence;
        inputSequence.setEnabled(true);
        inputSequence.setButtonInterval(250);
        inputSequence.setTimeout(450);
        inputSequence.addSequence(firstInput);
        inputSequence.addSequence(secondInput);
        inputSequence.addSequence(thirdInput);
        simulateInitializationSync(&inputSequence, &backendInputSequence);

        // WHEN
        deviceBackend->setButtonPressed(Qt::Key_Q, true);

        // THEN
        QCOMPARE(backendInputSequence.process(&handler, 1100000000), false);

        // WHEN
        deviceBackend->setButtonPressed(Qt::Key_Q, false);
        deviceBackend->setButtonPressed(Qt::Key_S, true);

        // THEN
        QCOMPARE(backendInputSequence.process(&handler, 1300000000), false);

        // WHEN
        deviceBackend->setButtonPressed(Qt::Key_S, false);
        deviceBackend->setButtonPressed(Qt::Key_E, true);

        // THEN
        QCOMPARE(backendInputSequence.process(&handler, 1600000000), false); // Too late
    }

    void shouldRespectSequenceButtonInterval()
    {
        // GIVEN
        TestDeviceIntegration deviceIntegration;
        TestDevice *device = deviceIntegration.createPhysicalDevice("keyboard");
        TestDeviceBackendNode *deviceBackend = deviceIntegration.physicalDevice(device->id());
        Qt3DInput::Input::InputHandler handler;
        handler.addInputDeviceIntegration(&deviceIntegration);

        auto firstInput = new Qt3DInput::QActionInput;
        firstInput->setButtons(QList<int> { Qt::Key_Q, Qt::Key_A });
        firstInput->setSourceDevice(device);
        auto backendFirstInput = handler.actionInputManager()->getOrCreateResource(firstInput->id());
        simulateInitializationSync(firstInput, backendFirstInput);

        auto secondInput = new Qt3DInput::QActionInput;
        secondInput->setButtons(QList<int> { Qt::Key_S, Qt::Key_W });
        secondInput->setSourceDevice(device);
        auto backendSecondInput = handler.actionInputManager()->getOrCreateResource(secondInput->id());
        simulateInitializationSync(secondInput, backendSecondInput);

        auto thirdInput = new Qt3DInput::QActionInput;
        thirdInput->setButtons(QList<int> { Qt::Key_D, Qt::Key_E });
        thirdInput->setSourceDevice(device);
        auto backendThirdInput = handler.actionInputManager()->getOrCreateResource(thirdInput->id());
        simulateInitializationSync(thirdInput, backendThirdInput);

        Qt3DInput::Input::InputSequence backendInputSequence;
        Qt3DInput::QInputSequence inputSequence;
        inputSequence.setEnabled(true);
        inputSequence.setButtonInterval(100);
        inputSequence.setTimeout(450);
        inputSequence.addSequence(firstInput);
        inputSequence.addSequence(secondInput);
        inputSequence.addSequence(thirdInput);
        simulateInitializationSync(&inputSequence, &backendInputSequence);

        // WHEN
        deviceBackend->setButtonPressed(Qt::Key_Q, true);

        // THEN
        QCOMPARE(backendInputSequence.process(&handler, 1100000000), false);

        // WHEN
        deviceBackend->setButtonPressed(Qt::Key_Q, false);
        deviceBackend->setButtonPressed(Qt::Key_S, true);

        // THEN
        QCOMPARE(backendInputSequence.process(&handler, 1250000000), false); // Too late

        // WHEN
        deviceBackend->setButtonPressed(Qt::Key_S, false);
        deviceBackend->setButtonPressed(Qt::Key_E, true);

        // THEN
        QCOMPARE(backendInputSequence.process(&handler, 1300000000), false);
    }

    void shouldNotProcessWhenDisabled()
    {
        // GIVEN
        TestDeviceIntegration deviceIntegration;
        TestDevice *device = deviceIntegration.createPhysicalDevice("keyboard");
        TestDeviceBackendNode *deviceBackend = deviceIntegration.physicalDevice(device->id());
        Qt3DInput::Input::InputHandler handler;
        handler.addInputDeviceIntegration(&deviceIntegration);

        auto firstInput = new Qt3DInput::QActionInput;
        firstInput->setButtons(QList<int> { Qt::Key_Q });
        firstInput->setSourceDevice(device);
        auto backendFirstInput = handler.actionInputManager()->getOrCreateResource(firstInput->id());
        simulateInitializationSync(firstInput, backendFirstInput);

        auto secondInput = new Qt3DInput::QActionInput;
        secondInput->setButtons(QList<int> { Qt::Key_S });
        secondInput->setSourceDevice(device);
        auto backendSecondInput = handler.actionInputManager()->getOrCreateResource(secondInput->id());
        simulateInitializationSync(secondInput, backendSecondInput);

        Qt3DInput::Input::InputSequence backendInputSequence;
        Qt3DInput::QInputSequence inputSequence;
        inputSequence.setEnabled(false);
        inputSequence.setButtonInterval(150);
        inputSequence.setTimeout(450);
        inputSequence.addSequence(firstInput);
        inputSequence.addSequence(secondInput);
        simulateInitializationSync(&inputSequence, &backendInputSequence);

        // WHEN
        deviceBackend->setButtonPressed(Qt::Key_Q, true);

        // THEN
        QCOMPARE(backendInputSequence.process(&handler, 1000000000), false);

        // WHEN
        deviceBackend->setButtonPressed(Qt::Key_Q, false);
        deviceBackend->setButtonPressed(Qt::Key_S, true);

        // THEN
        QCOMPARE(backendInputSequence.process(&handler, 1100000000), false);
    }
};

QTEST_APPLESS_MAIN(tst_InputSequence)

#include "tst_inputsequence.moc"
