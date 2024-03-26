// Copyright (C) 2017 Paul Lemire <paul.lemire350@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QtTest/QTest>
#include <Qt3DAnimation/qlerpclipblend.h>
#include <Qt3DAnimation/qanimationcliploader.h>
#include <Qt3DAnimation/private/qlerpclipblend_p.h>
#include <Qt3DAnimation/private/lerpclipblend_p.h>
#include "qbackendnodetester.h"

using namespace Qt3DAnimation::Animation;

Q_DECLARE_METATYPE(Handler *)
Q_DECLARE_METATYPE(LerpClipBlend *)

namespace {

class TestClipBlendNode : public ClipBlendNode
{
public:
    TestClipBlendNode(double duration)
        : ClipBlendNode(ClipBlendNode::LerpBlendType)
        , m_duration(duration)
    {}

    inline QList<Qt3DCore::QNodeId> allDependencyIds() const override
    {
        return currentDependencyIds();
    }

    QList<Qt3DCore::QNodeId> currentDependencyIds() const final
    {
        return {};
    }

    using ClipBlendNode::setClipResults;

    double duration() const final { return m_duration; }

protected:
    ClipResults doBlend(const QList<ClipResults> &) const final { return ClipResults(); }

private:
    double m_duration;
};

} // anonymous

class tst_LerpClipBlend : public Qt3DCore::QBackendNodeTester
{
    Q_OBJECT
public:
    TestClipBlendNode *createTestBlendNode(Handler *handler,
                                           double duration)
    {
        auto id = Qt3DCore::QNodeId::createId();
        TestClipBlendNode *node = new TestClipBlendNode(duration);
        setPeerId(node, id);
        node->setHandler(handler);
        node->setClipBlendNodeManager(handler->clipBlendNodeManager());
        handler->clipBlendNodeManager()->appendNode(id, node);
        return node;
    }

    LerpClipBlend *createLerpClipBlendNode(Handler *handler, const float &blendFactor)
    {
        auto id = Qt3DCore::QNodeId::createId();
        LerpClipBlend *node = new LerpClipBlend();
        node->setBlendFactor(blendFactor);
        setPeerId(node, id);
        node->setHandler(handler);
        node->setClipBlendNodeManager(handler->clipBlendNodeManager());
        handler->clipBlendNodeManager()->appendNode(id, node);
        return node;
    }

    BlendedClipAnimator *createBlendedClipAnimator(Handler *handler,
                                                   qint64 globalStartTimeNS,
                                                   int loops)
    {
        auto animatorId = Qt3DCore::QNodeId::createId();
        BlendedClipAnimator *animator = handler->blendedClipAnimatorManager()->getOrCreateResource(animatorId);
        setPeerId(animator, animatorId);
        animator->setStartTime(globalStartTimeNS);
        animator->setLoops(loops);
        return animator;
    }

private Q_SLOTS:

    void checkInitialState()
    {
        // GIVEN
        LerpClipBlend backendLerpBlend;

        // THEN
        QCOMPARE(backendLerpBlend.isEnabled(), false);
        QVERIFY(backendLerpBlend.peerId().isNull());
        QCOMPARE(backendLerpBlend.blendFactor(), 0.0f);
        QCOMPARE(backendLerpBlend.blendType(), ClipBlendNode::LerpBlendType);
    }

    void checkInitializeFromPeer()
    {
        // GIVEN
        Qt3DAnimation::QLerpClipBlend lerpBlend;
        Qt3DAnimation::QAnimationClipLoader clip;
        lerpBlend.setBlendFactor(0.8f);

        {
            // WHEN
            LerpClipBlend backendLerpBlend;
            simulateInitializationSync(&lerpBlend, &backendLerpBlend);

            // THEN
            QCOMPARE(backendLerpBlend.isEnabled(), true);
            QCOMPARE(backendLerpBlend.peerId(), lerpBlend.id());
            QCOMPARE(backendLerpBlend.blendFactor(), 0.8f);
        }
        {
            // WHEN
            LerpClipBlend backendLerpBlend;
            lerpBlend.setEnabled(false);
            simulateInitializationSync(&lerpBlend, &backendLerpBlend);

            // THEN
            QCOMPARE(backendLerpBlend.peerId(), lerpBlend.id());
            QCOMPARE(backendLerpBlend.isEnabled(), false);
        }
    }

    void checkSceneChangeEvents()
    {
        // GIVEN
        Qt3DAnimation::QLerpClipBlend lerpBlend;
        LerpClipBlend backendLerpBlend;
        simulateInitializationSync(&lerpBlend, &backendLerpBlend);

        {
            // WHEN
            const bool newValue = false;
            lerpBlend.setEnabled(newValue);
            backendLerpBlend.syncFromFrontEnd(&lerpBlend, false);

            // THEN
            QCOMPARE(backendLerpBlend.isEnabled(), newValue);
        }
        {
            // WHEN
            const float newValue = 0.883f;
            lerpBlend.setBlendFactor(newValue);
            backendLerpBlend.syncFromFrontEnd(&lerpBlend, false);

            // THEN
            QCOMPARE(backendLerpBlend.blendFactor(), newValue);
        }
    }

    void checkDependencyIds()
    {
        // GIVEN
        LerpClipBlend lerpBlend;
        auto startClipId = Qt3DCore::QNodeId::createId();
        auto endClipId = Qt3DCore::QNodeId::createId();

        // WHEN
        lerpBlend.setStartClipId(startClipId);
        lerpBlend.setEndClipId(endClipId);
        QList<Qt3DCore::QNodeId> actualIds = lerpBlend.currentDependencyIds();

        // THEN
        QCOMPARE(actualIds.size(), 2);
        QCOMPARE(actualIds[0], startClipId);
        QCOMPARE(actualIds[1], endClipId);

        // WHEN
        auto anotherEndClipId = Qt3DCore::QNodeId::createId();
        lerpBlend.setEndClipId(anotherEndClipId);
        actualIds = lerpBlend.currentDependencyIds();

        // THEN
        QCOMPARE(actualIds.size(), 2);
        QCOMPARE(actualIds[0], startClipId);
        QCOMPARE(actualIds[1], anotherEndClipId);
    }

    void checkDuration()
    {
        // GIVEN
        auto handler = new Handler();
        const double startNodeDuration = 10.0;
        const double endNodeDuration = 20.0;
        const float blendFactor = 0.25f;
        const double expectedDuration = 12.5;

        auto startNode = createTestBlendNode(handler, startNodeDuration);
        auto endNode = createTestBlendNode(handler, endNodeDuration);

        LerpClipBlend blendNode;
        blendNode.setHandler(handler);
        blendNode.setClipBlendNodeManager(handler->clipBlendNodeManager());
        blendNode.setStartClipId(startNode->peerId());
        blendNode.setEndClipId(endNode->peerId());
        blendNode.setBlendFactor(blendFactor);

        // WHEN
        double actualDuration = blendNode.duration();

        // THEN
        QCOMPARE(actualDuration, expectedDuration);
    }

    void checkDoBlend_data()
    {
        QTest::addColumn<Handler *>("handler");
        QTest::addColumn<LerpClipBlend *>("blendNode");
        QTest::addColumn<Qt3DCore::QNodeId>("animatorId");
        QTest::addColumn<ClipResults>("expectedResults");

        {
            auto handler = new Handler();

            const qint64 globalStartTimeNS = 0;
            const int loopCount = 1;
            auto animator = createBlendedClipAnimator(handler, globalStartTimeNS, loopCount);

            const double duration = 1.0;
            auto startNode = createTestBlendNode(handler, duration);
            startNode->setClipResults(animator->peerId(), { 0.0f, 0.0f, 0.0f });
            auto endNode = createTestBlendNode(handler, duration);
            endNode->setClipResults(animator->peerId(), { 1.0f, 1.0f, 1.0f });

            const float blendFactor = 0.0f;
            auto blendNode = createLerpClipBlendNode(handler, blendFactor);
            blendNode->setStartClipId(startNode->peerId());
            blendNode->setEndClipId(endNode->peerId());
            blendNode->setBlendFactor(blendFactor);

            ClipResults expectedResults = { 0.0f, 0.0f, 0.0f };

            QTest::addRow("unit lerp, beta = 0.0")
                    << handler << blendNode << animator->peerId() << expectedResults;
        }

        {
            auto handler = new Handler();

            const qint64 globalStartTimeNS = 0;
            const int loopCount = 1;
            auto animator = createBlendedClipAnimator(handler, globalStartTimeNS, loopCount);

            const double duration = 1.0;
            auto startNode = createTestBlendNode(handler, duration);
            startNode->setClipResults(animator->peerId(), { 0.0f, 0.0f, 0.0f });
            auto endNode = createTestBlendNode(handler, duration);
            endNode->setClipResults(animator->peerId(), { 1.0f, 1.0f, 1.0f });

            const float blendFactor = 0.5f;
            auto blendNode = createLerpClipBlendNode(handler, blendFactor);
            blendNode->setStartClipId(startNode->peerId());
            blendNode->setEndClipId(endNode->peerId());
            blendNode->setBlendFactor(blendFactor);

            ClipResults expectedResults = { 0.5f, 0.5f, 0.5f };

            QTest::addRow("unit lerp, beta = 0.5")
                    << handler << blendNode << animator->peerId() << expectedResults;
        }

        {
            auto handler = new Handler();

            const qint64 globalStartTimeNS = 0;
            const int loopCount = 1;
            auto animator = createBlendedClipAnimator(handler, globalStartTimeNS, loopCount);

            const double duration = 1.0;
            auto startNode = createTestBlendNode(handler, duration);
            startNode->setClipResults(animator->peerId(), { 0.0f, 0.0f, 0.0f });
            auto endNode = createTestBlendNode(handler, duration);
            endNode->setClipResults(animator->peerId(), { 1.0f, 1.0f, 1.0f });

            const float blendFactor = 1.0f;
            auto blendNode = createLerpClipBlendNode(handler, blendFactor);
            blendNode->setStartClipId(startNode->peerId());
            blendNode->setEndClipId(endNode->peerId());
            blendNode->setBlendFactor(blendFactor);

            ClipResults expectedResults = { 1.0f, 1.0f, 1.0f };

            QTest::addRow("unit lerp, beta = 1.0")
                    << handler << blendNode << animator->peerId() << expectedResults;
        }

        {
            auto handler = new Handler();

            const qint64 globalStartTimeNS = 0;
            const int loopCount = 1;
            auto animator = createBlendedClipAnimator(handler, globalStartTimeNS, loopCount);

            const double duration = 1.0;
            auto startNode = createTestBlendNode(handler, duration);
            startNode->setClipResults(animator->peerId(), { 0.0f, 1.0f, 2.0f });
            auto endNode = createTestBlendNode(handler, duration);
            endNode->setClipResults(animator->peerId(), { 1.0f, 2.0f, 3.0f });

            const float blendFactor = 0.5f;
            auto blendNode = createLerpClipBlendNode(handler, blendFactor);
            blendNode->setStartClipId(startNode->peerId());
            blendNode->setEndClipId(endNode->peerId());
            blendNode->setBlendFactor(blendFactor);

            ClipResults expectedResults = { 0.5f, 1.5f, 2.5f };

            QTest::addRow("lerp varying data, beta = 0.5")
                    << handler << blendNode << animator->peerId() << expectedResults;
        }

        {
            auto handler = new Handler();

            const qint64 globalStartTimeNS = 0;
            const int loopCount = 1;
            auto animator = createBlendedClipAnimator(handler, globalStartTimeNS, loopCount);

            const double duration = 1.0;
            const int dataCount = 1000;
            ClipResults startData(dataCount);
            ClipResults endData(dataCount);
            ClipResults expectedResults(dataCount);
            for (int i = 0; i < dataCount; ++i) {
                startData[i] = float(i);
                endData[i] = 2.0f * float(i);
                expectedResults[i] = 1.5f * float(i);
            }
            auto startNode = createTestBlendNode(handler, duration);
            startNode->setClipResults(animator->peerId(), startData);
            auto endNode = createTestBlendNode(handler, duration);
            endNode->setClipResults(animator->peerId(), endData);

            const float blendFactor = 0.5f;
            auto blendNode = createLerpClipBlendNode(handler, blendFactor);
            blendNode->setStartClipId(startNode->peerId());
            blendNode->setEndClipId(endNode->peerId());
            blendNode->setBlendFactor(blendFactor);

            QTest::addRow("lerp lots of data, beta = 0.5")
                    << handler << blendNode << animator->peerId() << expectedResults;
        }
    }

    void checkDoBlend()
    {
        // GIVEN
        QFETCH(Handler *, handler);
        QFETCH(LerpClipBlend *, blendNode);
        QFETCH(Qt3DCore::QNodeId, animatorId);
        QFETCH(ClipResults, expectedResults);

        // WHEN
        blendNode->blend(animatorId);

        // THEN
        const ClipResults actualResults = blendNode->clipResults(animatorId);
        QCOMPARE(actualResults.size(), expectedResults.size());
        for (int i = 0; i < actualResults.size(); ++i)
            QCOMPARE(actualResults[i], expectedResults[i]);

        // Cleanup
        delete handler;
    }
};

QTEST_MAIN(tst_LerpClipBlend)

#include "tst_lerpclipblend.moc"
