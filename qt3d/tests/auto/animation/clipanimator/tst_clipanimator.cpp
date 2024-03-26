// Copyright (C) 2017 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QTest>
#include <Qt3DAnimation/private/clipanimator_p.h>
#include <Qt3DAnimation/qanimationcliploader.h>
#include <Qt3DAnimation/qchannelmapper.h>
#include <Qt3DAnimation/qclipanimator.h>
#include <Qt3DAnimation/qclock.h>
#include <Qt3DCore/private/qnode_p.h>
#include <Qt3DCore/private/qscene_p.h>
#include <Qt3DCore/private/qbackendnode_p.h>
#include <qbackendnodetester.h>
#include <testarbiter.h>

class tst_ClipAnimator: public Qt3DCore::QBackendNodeTester
{
    Q_OBJECT

private Q_SLOTS:
    void checkPeerPropertyMirroring()
    {
        // GIVEN
        Qt3DAnimation::Animation::Handler handler;
        Qt3DAnimation::Animation::ClipAnimator backendAnimator;
        backendAnimator.setHandler(&handler);
        Qt3DAnimation::QClipAnimator animator;
        auto clip = new Qt3DAnimation::QAnimationClipLoader();
        auto clock = new Qt3DAnimation::QClock();

        animator.setClip(clip);
        animator.setClock(clock);
        animator.setLoopCount(10);
        animator.setNormalizedTime(0.5f);

        // WHEN
        simulateInitializationSync(&animator, &backendAnimator);

        // THEN
        QCOMPARE(backendAnimator.peerId(), animator.id());
        QCOMPARE(backendAnimator.isEnabled(), animator.isEnabled());
        QCOMPARE(backendAnimator.clipId(), clip->id());
        QCOMPARE(backendAnimator.clockId(), clock->id());
        QCOMPARE(backendAnimator.isRunning(), animator.isRunning());
        QCOMPARE(backendAnimator.loops(), animator.loopCount());
        QCOMPARE(backendAnimator.normalizedLocalTime(), animator.normalizedTime());
    }

    void checkInitialAndCleanedUpState()
    {
        // GIVEN
        Qt3DAnimation::Animation::Handler handler;
        Qt3DAnimation::Animation::ClipAnimator backendAnimator;
        backendAnimator.setHandler(&handler);

        // THEN
        QVERIFY(backendAnimator.peerId().isNull());
        QCOMPARE(backendAnimator.isEnabled(), false);
        QCOMPARE(backendAnimator.clipId(), Qt3DCore::QNodeId());
        QCOMPARE(backendAnimator.clockId(), Qt3DCore::QNodeId());
        QCOMPARE(backendAnimator.isRunning(), false);
        QCOMPARE(backendAnimator.loops(), 1);
        QCOMPARE(backendAnimator.normalizedLocalTime(), -1.0);

        // GIVEN
        Qt3DAnimation::QClipAnimator animator;
        auto clip = new Qt3DAnimation::QAnimationClipLoader();
        auto clock = new Qt3DAnimation::QClock();
        animator.setClip(clip);
        animator.setClock(clock);
        animator.setRunning(true);
        animator.setLoopCount(25);
        animator.setNormalizedTime(1.0f);

        // WHEN
        simulateInitializationSync(&animator, &backendAnimator);
        backendAnimator.setClipId(Qt3DCore::QNodeId::createId());
        backendAnimator.setClockId(Qt3DCore::QNodeId::createId());
        backendAnimator.cleanup();

        // THEN
        QCOMPARE(backendAnimator.clipId(), Qt3DCore::QNodeId());
        QCOMPARE(backendAnimator.clockId(), Qt3DCore::QNodeId());
        QCOMPARE(backendAnimator.isEnabled(), false);
        QCOMPARE(backendAnimator.isRunning(), false);
        QCOMPARE(backendAnimator.loops(), 1);
        QCOMPARE(backendAnimator.normalizedLocalTime(), -1.0f);
    }

    void checkPropertyChanges()
    {
        // GIVEN
        Qt3DAnimation::QClipAnimator animator;
        Qt3DAnimation::Animation::Handler handler;
        Qt3DAnimation::Animation::ClipAnimator backendAnimator;
        backendAnimator.setHandler(&handler);
        simulateInitializationSync(&animator, &backendAnimator);

        // WHEN
        animator.setEnabled(false);
        backendAnimator.syncFromFrontEnd(&animator, false);

        // THEN
        QCOMPARE(backendAnimator.isEnabled(), false);

        // WHEN
        auto newClip = new Qt3DAnimation::QAnimationClipLoader();
        animator.setClip(newClip);
        backendAnimator.syncFromFrontEnd(&animator, false);

        // THEN
        QCOMPARE(backendAnimator.clipId(), newClip->id());

        // WHEN
        auto newMapper = new Qt3DAnimation::QChannelMapper();
        animator.setChannelMapper(newMapper);
        backendAnimator.syncFromFrontEnd(&animator, false);

        // THEN
        QCOMPARE(backendAnimator.mapperId(), newMapper->id());

        // WHEN
        auto clock = new Qt3DAnimation::QClock();
        animator.setClock(clock);
        backendAnimator.syncFromFrontEnd(&animator, false);

        // THEN
        QCOMPARE(backendAnimator.clockId(), clock->id());

        // WHEN
        animator.setRunning(true);
        backendAnimator.syncFromFrontEnd(&animator, false);

        // THEN
        QCOMPARE(backendAnimator.isRunning(), true);

        // WHEN
        animator.setLoopCount(64);
        backendAnimator.syncFromFrontEnd(&animator, false);

        // THEN
        QCOMPARE(backendAnimator.loops(), 64);

        // WHEN
        animator.setNormalizedTime(0.5f);
        backendAnimator.syncFromFrontEnd(&animator, false);

        // THEN
        QVERIFY(qFuzzyCompare(backendAnimator.normalizedLocalTime(), 0.5f));
    }
};

QTEST_APPLESS_MAIN(tst_ClipAnimator)

#include "tst_clipanimator.moc"
