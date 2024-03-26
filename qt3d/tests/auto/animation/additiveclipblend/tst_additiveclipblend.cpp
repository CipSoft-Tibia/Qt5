// Copyright (C) 2017 Paul Lemire <paul.lemire350@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QtTest/QTest>
#include <Qt3DAnimation/qadditiveclipblend.h>
#include <Qt3DAnimation/qanimationcliploader.h>
#include <Qt3DAnimation/private/qadditiveclipblend_p.h>
#include <Qt3DAnimation/private/additiveclipblend_p.h>
#include "qbackendnodetester.h"

using namespace Qt3DAnimation::Animation;

Q_DECLARE_METATYPE(Handler *)
Q_DECLARE_METATYPE(AdditiveClipBlend *)

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

class tst_AdditiveClipBlend : public Qt3DCore::QBackendNodeTester
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

    AdditiveClipBlend *createAdditiveClipBlendNode(Handler *handler, const float &blendFactor)
    {
        auto id = Qt3DCore::QNodeId::createId();
        AdditiveClipBlend *node = new AdditiveClipBlend();
        node->setAdditiveFactor(blendFactor);
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
        AdditiveClipBlend backendAdditiveBlend;

        // THEN
        QCOMPARE(backendAdditiveBlend.isEnabled(), false);
        QVERIFY(backendAdditiveBlend.peerId().isNull());
        QCOMPARE(backendAdditiveBlend.baseClipId(), Qt3DCore::QNodeId());
        QCOMPARE(backendAdditiveBlend.additiveClipId(), Qt3DCore::QNodeId());
        QCOMPARE(backendAdditiveBlend.additiveFactor(), 0.0f);
        QCOMPARE(backendAdditiveBlend.blendType(), ClipBlendNode::AdditiveBlendType);
    }

    void checkInitializeFromPeer()
    {
        // GIVEN
        Qt3DAnimation::QAdditiveClipBlend additiveBlend;
        Qt3DAnimation::QAdditiveClipBlend baseClip;
        Qt3DAnimation::QAdditiveClipBlend additiveClip;
        Qt3DAnimation::QAnimationClipLoader clip;
        additiveBlend.setBaseClip(&baseClip);
        additiveBlend.setAdditiveClip(&additiveClip);
        additiveBlend.setAdditiveFactor(0.8f);

        {
            // WHEN
            AdditiveClipBlend backendAdditiveBlend;
            simulateInitializationSync(&additiveBlend, &backendAdditiveBlend);

            // THEN
            QCOMPARE(backendAdditiveBlend.isEnabled(), true);
            QCOMPARE(backendAdditiveBlend.peerId(), additiveBlend.id());
            QCOMPARE(backendAdditiveBlend.baseClipId(), baseClip.id());
            QCOMPARE(backendAdditiveBlend.additiveClipId(), additiveClip.id());
            QCOMPARE(backendAdditiveBlend.additiveFactor(), 0.8f);
        }
        {
            // WHEN
            AdditiveClipBlend backendAdditiveBlend;
            additiveBlend.setEnabled(false);
            simulateInitializationSync(&additiveBlend, &backendAdditiveBlend);

            // THEN
            QCOMPARE(backendAdditiveBlend.peerId(), additiveBlend.id());
            QCOMPARE(backendAdditiveBlend.isEnabled(), false);
        }
    }

    void checkSceneChangeEvents()
    {
        // GIVEN
        Qt3DAnimation::QAdditiveClipBlend additiveBlend;
        AdditiveClipBlend backendAdditiveBlend;
        simulateInitializationSync(&additiveBlend, &backendAdditiveBlend);
        {
            // WHEN
            const bool newValue = false;
            additiveBlend.setEnabled(newValue);
            backendAdditiveBlend.syncFromFrontEnd(&additiveBlend, false);

            // THEN
            QCOMPARE(backendAdditiveBlend.isEnabled(), newValue);
        }
        {
            // WHEN
            const float newValue = 0.883f;
            additiveBlend.setAdditiveFactor(newValue);
            backendAdditiveBlend.syncFromFrontEnd(&additiveBlend, false);

            // THEN
            QCOMPARE(backendAdditiveBlend.additiveFactor(), newValue);
        }
        {
            // WHEN
            Qt3DAnimation::QAdditiveClipBlend newValue;
            additiveBlend.setBaseClip(&newValue);
            backendAdditiveBlend.syncFromFrontEnd(&additiveBlend, false);

            // THEN
            QCOMPARE(backendAdditiveBlend.baseClipId(), newValue.id());
        }
        {
            // WHEN
            Qt3DAnimation::QAdditiveClipBlend newValue;
            additiveBlend.setAdditiveClip(&newValue);
            backendAdditiveBlend.syncFromFrontEnd(&additiveBlend, false);

            // THEN
            QCOMPARE(backendAdditiveBlend.additiveClipId(), newValue.id());
        }
    }

    void checkDependencyIds()
    {
        // GIVEN
        AdditiveClipBlend addBlend;
        auto baseClipId = Qt3DCore::QNodeId::createId();
        auto additiveClipId = Qt3DCore::QNodeId::createId();

        // WHEN
        addBlend.setBaseClipId(baseClipId);
        addBlend.setAdditiveClipId(additiveClipId);
        QList<Qt3DCore::QNodeId> actualIds = addBlend.currentDependencyIds();

        // THEN
        QCOMPARE(actualIds.size(), 2);
        QCOMPARE(actualIds[0], baseClipId);
        QCOMPARE(actualIds[1], additiveClipId);

        // WHEN
        auto anotherAdditiveClipId = Qt3DCore::QNodeId::createId();
        addBlend.setAdditiveClipId(anotherAdditiveClipId);
        actualIds = addBlend.currentDependencyIds();

        // THEN
        QCOMPARE(actualIds.size(), 2);
        QCOMPARE(actualIds[0], baseClipId);
        QCOMPARE(actualIds[1], anotherAdditiveClipId);
    }

    void checkDuration()
    {
        // GIVEN
        auto handler = new Handler();
        const double expectedDuration = 123.5;
        const double baseNodeDuration = expectedDuration;
        const double additiveNodeDuration = 5.0;

        auto baseNode = createTestBlendNode(handler, baseNodeDuration);
        auto additiveNode = createTestBlendNode(handler, additiveNodeDuration);

        AdditiveClipBlend blendNode;
        blendNode.setHandler(handler);
        blendNode.setClipBlendNodeManager(handler->clipBlendNodeManager());
        blendNode.setBaseClipId(baseNode->peerId());
        blendNode.setAdditiveClipId(additiveNode->peerId());

        // WHEN
        double actualDuration = blendNode.duration();

        // THEN
        QCOMPARE(actualDuration, expectedDuration);
    }

    void checkDoBlend_data()
    {
        QTest::addColumn<Handler *>("handler");
        QTest::addColumn<AdditiveClipBlend *>("blendNode");
        QTest::addColumn<Qt3DCore::QNodeId>("animatorId");
        QTest::addColumn<ClipResults>("expectedResults");

        {
            auto handler = new Handler();

            const qint64 globalStartTimeNS = 0;
            const int loopCount = 1;
            auto animator = createBlendedClipAnimator(handler, globalStartTimeNS, loopCount);

            const double duration = 1.0;
            auto baseNode = createTestBlendNode(handler, duration);
            baseNode->setClipResults(animator->peerId(), { 0.0f, 0.0f, 0.0f });
            auto additiveNode = createTestBlendNode(handler, duration);
            additiveNode->setClipResults(animator->peerId(), { 1.0f, 1.0f, 1.0f });

            const float additiveFactor = 0.0f;
            auto blendNode = createAdditiveClipBlendNode(handler, additiveFactor);
            blendNode->setBaseClipId(baseNode->peerId());
            blendNode->setAdditiveClipId(additiveNode->peerId());
            blendNode->setAdditiveFactor(additiveFactor);

            ClipResults expectedResults = { 0.0f, 0.0f, 0.0f };

            QTest::addRow("unit additive, beta = 0.0")
                    << handler << blendNode << animator->peerId() << expectedResults;
        }

        {
            auto handler = new Handler();

            const qint64 globalStartTimeNS = 0;
            const int loopCount = 1;
            auto animator = createBlendedClipAnimator(handler, globalStartTimeNS, loopCount);

            const double duration = 1.0;
            auto baseNode = createTestBlendNode(handler, duration);
            baseNode->setClipResults(animator->peerId(), { 0.0f, 0.0f, 0.0f });
            auto additiveNode = createTestBlendNode(handler, duration);
            additiveNode->setClipResults(animator->peerId(), { 1.0f, 1.0f, 1.0f });

            const float additiveFactor = 0.5f;
            auto blendNode = createAdditiveClipBlendNode(handler, additiveFactor);
            blendNode->setBaseClipId(baseNode->peerId());
            blendNode->setAdditiveClipId(additiveNode->peerId());
            blendNode->setAdditiveFactor(additiveFactor);

            ClipResults expectedResults = { 0.5f, 0.5f, 0.5f };

            QTest::addRow("unit additive, beta = 0.5")
                    << handler << blendNode << animator->peerId() << expectedResults;
        }

        {
            auto handler = new Handler();

            const qint64 globalStartTimeNS = 0;
            const int loopCount = 1;
            auto animator = createBlendedClipAnimator(handler, globalStartTimeNS, loopCount);

            const double duration = 1.0;
            auto baseNode = createTestBlendNode(handler, duration);
            baseNode->setClipResults(animator->peerId(), { 0.0f, 0.0f, 0.0f });
            auto additiveNode = createTestBlendNode(handler, duration);
            additiveNode->setClipResults(animator->peerId(), { 1.0f, 1.0f, 1.0f });

            const float additiveFactor = 1.0f;
            auto blendNode = createAdditiveClipBlendNode(handler, additiveFactor);
            blendNode->setBaseClipId(baseNode->peerId());
            blendNode->setAdditiveClipId(additiveNode->peerId());
            blendNode->setAdditiveFactor(additiveFactor);

            ClipResults expectedResults = { 1.0f, 1.0f, 1.0f };

            QTest::addRow("unit additive, beta = 1.0")
                    << handler << blendNode << animator->peerId() << expectedResults;
        }

        {
            auto handler = new Handler();

            const qint64 globalStartTimeNS = 0;
            const int loopCount = 1;
            auto animator = createBlendedClipAnimator(handler, globalStartTimeNS, loopCount);

            const double duration = 1.0;
            auto baseNode = createTestBlendNode(handler, duration);
            baseNode->setClipResults(animator->peerId(), { 0.0f, 1.0f, 2.0f });
            auto additiveNode = createTestBlendNode(handler, duration);
            additiveNode->setClipResults(animator->peerId(), { 1.0f, 2.0f, 3.0f });

            const float blendFactor = 0.5f;
            auto blendNode = createAdditiveClipBlendNode(handler, blendFactor);
            blendNode->setBaseClipId(baseNode->peerId());
            blendNode->setAdditiveClipId(additiveNode->peerId());
            blendNode->setAdditiveFactor(blendFactor);

            ClipResults expectedResults = { 0.5f, 2.0f, 3.5f };

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
            ClipResults baseData(dataCount);
            ClipResults additiveData(dataCount);
            ClipResults expectedResults(dataCount);
            for (int i = 0; i < dataCount; ++i) {
                baseData[i] = float(i);
                additiveData[i] = 2.0f * float(i);
                expectedResults[i] = 2.0f * float(i);
            }
            auto baseNode = createTestBlendNode(handler, duration);
            baseNode->setClipResults(animator->peerId(), baseData);
            auto additiveNode = createTestBlendNode(handler, duration);
            additiveNode->setClipResults(animator->peerId(), additiveData);

            const float blendFactor = 0.5f;
            auto blendNode = createAdditiveClipBlendNode(handler, blendFactor);
            blendNode->setBaseClipId(baseNode->peerId());
            blendNode->setAdditiveClipId(additiveNode->peerId());
            blendNode->setAdditiveFactor(blendFactor);

            QTest::addRow("lerp lots of data, beta = 0.5")
                    << handler << blendNode << animator->peerId() << expectedResults;
        }
    }

    void checkDoBlend()
    {
        // GIVEN
        QFETCH(Handler *, handler);
        QFETCH(AdditiveClipBlend *, blendNode);
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

QTEST_MAIN(tst_AdditiveClipBlend)

#include "tst_additiveclipblend.moc"
