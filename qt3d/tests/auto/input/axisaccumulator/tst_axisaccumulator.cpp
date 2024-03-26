// Copyright (C) 2015 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QTest>
#include <qbackendnodetester.h>
#include <Qt3DCore/private/qnode_p.h>
#include <Qt3DCore/private/qscene_p.h>
#include <Qt3DInput/private/axis_p.h>
#include <Qt3DInput/private/axisaccumulator_p.h>
#include <Qt3DInput/private/qabstractaxisinput_p.h>
#include <Qt3DInput/private/inputmanagers_p.h>
#include <Qt3DInput/QAnalogAxisInput>
#include <Qt3DInput/QAxis>
#include <Qt3DInput/QAxisAccumulator>
#include <Qt3DCore/private/qbackendnode_p.h>
#include "testarbiter.h"

class tst_AxisAccumulator: public Qt3DCore::QBackendNodeTester
{
    Q_OBJECT

private Q_SLOTS:

    void checkPeerPropertyMirroring()
    {
        // GIVEN
        Qt3DInput::Input::AxisAccumulator backendAccumulator;
        Qt3DInput::QAxisAccumulator axisAccumulator;
        Qt3DInput::QAxis axis;

        axisAccumulator.setSourceAxis(&axis);
        axisAccumulator.setSourceAxisType(Qt3DInput::QAxisAccumulator::Velocity);
        axisAccumulator.setScale(2.0f);

        // WHEN
        simulateInitializationSync(&axisAccumulator, &backendAccumulator);

        // THEN
        QCOMPARE(backendAccumulator.peerId(), axisAccumulator.id());
        QCOMPARE(backendAccumulator.isEnabled(), axisAccumulator.isEnabled());
        QCOMPARE(backendAccumulator.sourceAxisType(), axisAccumulator.sourceAxisType());
        QCOMPARE(backendAccumulator.sourceAxisId(), axisAccumulator.sourceAxis()->id());
        QCOMPARE(backendAccumulator.scale(), axisAccumulator.scale());
    }

    void checkInitialAndCleanedUpState()
    {
        // GIVEN
        Qt3DInput::Input::AxisAccumulator backendAxisAccumulator;

        // THEN
        QVERIFY(backendAxisAccumulator.peerId().isNull());
        QCOMPARE(backendAxisAccumulator.isEnabled(), false);
        QCOMPARE(backendAxisAccumulator.value(), 0.0f);
        QCOMPARE(backendAxisAccumulator.velocity(), 0.0f);
        QCOMPARE(backendAxisAccumulator.scale(), 1.0f);
        QCOMPARE(backendAxisAccumulator.sourceAxisId(), Qt3DCore::QNodeId());
        QCOMPARE(backendAxisAccumulator.sourceAxisType(), Qt3DInput::QAxisAccumulator::Velocity);

        // GIVEN
        Qt3DInput::QAxisAccumulator axisAccumulator;
        Qt3DInput::QAxis axis;

        axisAccumulator.setSourceAxis(&axis);
        axisAccumulator.setScale(2.0f);
        axisAccumulator.setSourceAxisType(Qt3DInput::QAxisAccumulator::Acceleration);
        axisAccumulator.setEnabled(true);

        // WHEN
        simulateInitializationSync(&axisAccumulator, &backendAxisAccumulator);
        backendAxisAccumulator.cleanup();

        // THEN
        QCOMPARE(backendAxisAccumulator.isEnabled(), false);
        QCOMPARE(backendAxisAccumulator.value(), 0.0f);
        QCOMPARE(backendAxisAccumulator.velocity(), 0.0f);
        QCOMPARE(backendAxisAccumulator.scale(), 1.0f);
        QCOMPARE(backendAxisAccumulator.sourceAxisId(), Qt3DCore::QNodeId());
        QCOMPARE(backendAxisAccumulator.sourceAxisType(), Qt3DInput::QAxisAccumulator::Velocity);
    }

    void checkPropertyChanges()
    {
        // GIVEN
        Qt3DInput::QAxisAccumulator axisAccumulator;
        Qt3DInput::Input::AxisAccumulator backendAxisAccumulator;
        simulateInitializationSync(&axisAccumulator, &backendAxisAccumulator);

        // WHEN
        axisAccumulator.setEnabled(false);
        backendAxisAccumulator.syncFromFrontEnd(&axisAccumulator, false);

        // THEN
        QCOMPARE(backendAxisAccumulator.isEnabled(), false);

        // WHEN
        Qt3DInput::QAxis axis;
        const Qt3DCore::QNodeId axisId = axis.id();
        axisAccumulator.setSourceAxis(&axis);
        backendAxisAccumulator.syncFromFrontEnd(&axisAccumulator, false);

        // THEN
        QCOMPARE(backendAxisAccumulator.sourceAxisId(), axisId);

        // WHEN
        axisAccumulator.setSourceAxisType(Qt3DInput::QAxisAccumulator::Acceleration);
        backendAxisAccumulator.syncFromFrontEnd(&axisAccumulator, false);

        // THEN
        QCOMPARE(backendAxisAccumulator.sourceAxisType(), Qt3DInput::QAxisAccumulator::Acceleration);

        // WHEN
        axisAccumulator.setScale(3.f);
        backendAxisAccumulator.syncFromFrontEnd(&axisAccumulator, false);

        // THEN
        QCOMPARE(backendAxisAccumulator.scale(), 3.0f);
    }

    void shouldNotChangeValueWhenDisabled()
    {
        // GIVEN
        TestArbiter arbiter;
        Qt3DInput::Input::AxisAccumulator backendAxisAccumulator;
        backendAxisAccumulator.setEnabled(false);

        // WHEN
        backendAxisAccumulator.setValue(454.0f);

        // THEN
        QCOMPARE(backendAxisAccumulator.value(), 0.0f);
        QCOMPARE(arbiter.dirtyNodes().size(), 0);
    }

    void checkIntegration_data()
    {
        QTest::addColumn<Qt3DInput::QAxisAccumulator::SourceAxisType>("sourceAxisType");
        QTest::addColumn<float>("axisValue");
        QTest::addColumn<float>("scale");
        QTest::addColumn<float>("dt");
        QTest::addColumn<float>("valueResult");
        QTest::addColumn<float>("velocityResult");

        QTest::newRow("velocity=10, axis=1, dt=1") << Qt3DInput::QAxisAccumulator::Velocity
                                                   << 1.0f
                                                   << 10.0f
                                                   << 1.0f
                                                   << 10.0f
                                                   << 0.0f;

        QTest::newRow("velocity=10, axis=1, dt=0.2") << Qt3DInput::QAxisAccumulator::Velocity
                                                   << 1.0f
                                                   << 10.0f
                                                   << 0.2f
                                                   << 2.0f
                                                   << 0.0f;

        QTest::newRow("velocity=20, axis=1, dt=0.1") << Qt3DInput::QAxisAccumulator::Velocity
                                                   << 1.0f
                                                   << 20.0f
                                                   << 0.1f
                                                   << 2.0f
                                                   << 0.0f;

        QTest::newRow("velocity=10, axis=0.5, dt=1") << Qt3DInput::QAxisAccumulator::Velocity
                                                   << 0.5f
                                                   << 10.0f
                                                   << 1.0f
                                                   << 5.0f
                                                   << 0.0f;

        QTest::newRow("acceleration=10, axis=1, dt=1") << Qt3DInput::QAxisAccumulator::Acceleration
                                                       << 1.0f
                                                       << 10.0f
                                                       << 1.0f
                                                       << 10.0f
                                                       << 10.0f;
    }

    void checkIntegration()
    {
        // GIVEN
        QFETCH(Qt3DInput::QAxisAccumulator::SourceAxisType, sourceAxisType);
        QFETCH(float, axisValue);
        QFETCH(float, scale);
        QFETCH(float, dt);
        QFETCH(float, valueResult);
        QFETCH(float, velocityResult);

        Qt3DInput::QAxis *axis = new Qt3DInput::QAxis;
        Qt3DInput::Input::AxisManager axisManager;
        Qt3DInput::Input::Axis *backendAxis = axisManager.getOrCreateResource(axis->id());
        Qt3DInput::QAxisAccumulator axisAccumulator;
        Qt3DInput::Input::AxisAccumulator backendAxisAccumulator;

        // WHEN
        backendAxis->setEnabled(true);
        backendAxis->setAxisValue(axisValue);
        axisAccumulator.setSourceAxis(axis);
        axisAccumulator.setScale(scale);
        axisAccumulator.setSourceAxisType(sourceAxisType);
        axisAccumulator.setEnabled(true);
        simulateInitializationSync(&axisAccumulator, &backendAxisAccumulator);

        backendAxisAccumulator.stepIntegration(&axisManager, dt);

        // THEN
        switch (sourceAxisType) {
        case Qt3DInput::QAxisAccumulator::Velocity:
            QCOMPARE(backendAxisAccumulator.value(), valueResult);
            break;

        case Qt3DInput::QAxisAccumulator::Acceleration:
            QCOMPARE(backendAxisAccumulator.velocity(), velocityResult);
            QCOMPARE(backendAxisAccumulator.value(), valueResult);
            break;
        }
    }
};

QTEST_APPLESS_MAIN(tst_AxisAccumulator)

#include "tst_axisaccumulator.moc"
