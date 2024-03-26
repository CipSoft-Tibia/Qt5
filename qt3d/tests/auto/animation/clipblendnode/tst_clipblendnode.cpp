// Copyright (C) 2017 Paul Lemire <paul.lemire350@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QtTest/QTest>
#include <Qt3DAnimation/qlerpclipblend.h>
#include <Qt3DAnimation/qanimationcliploader.h>
#include <Qt3DAnimation/private/qabstractclipblendnode_p.h>
#include <Qt3DAnimation/private/clipblendnode_p.h>
#include <Qt3DAnimation/private/managers_p.h>
#include "qbackendnodetester.h"

#include <random>
#include <algorithm>

using namespace Qt3DAnimation::Animation;

namespace {

class TestClipBlendNode : public ClipBlendNode
{
public:
    TestClipBlendNode(const ClipResults &clipResults = ClipResults())
        : ClipBlendNode(ClipBlendNode::LerpBlendType)
        , m_clipResults(clipResults)
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

    double duration() const final { return 0.0f; }

protected:
    ClipResults doBlend(const QList<ClipResults> &) const final
    {
        return m_clipResults;
    }

private:
    ClipResults m_clipResults;
};

} // anonymous

Q_DECLARE_METATYPE(TestClipBlendNode *)

class tst_ClipBlendNode : public Qt3DCore::QBackendNodeTester
{
    Q_OBJECT
public:
    TestClipBlendNode *createTestClipBlendNode(Handler *handler, const ClipResults &clipResults)
    {
        auto id = Qt3DCore::QNodeId::createId();
        TestClipBlendNode *node = new TestClipBlendNode(clipResults);
        setPeerId(node, id);
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
        TestClipBlendNode backendClipBlendNode;

        // THEN
        QCOMPARE(backendClipBlendNode.isEnabled(), false);
        QVERIFY(backendClipBlendNode.peerId().isNull());
        QVERIFY(backendClipBlendNode.clipBlendNodeManager() == nullptr);
        QCOMPARE(backendClipBlendNode.blendType(), ClipBlendNode::LerpBlendType);
        QCOMPARE(backendClipBlendNode.clipResults(Qt3DCore::QNodeId()), ClipResults());
    }

    void checkInitializeFromPeer()
    {
        // GIVEN
        Qt3DAnimation::QLerpClipBlend clipBlendNode;
        Qt3DAnimation::QAnimationClipLoader clip;

        QCoreApplication::processEvents();

        {
            // WHEN
            ClipBlendNodeManager manager;
            TestClipBlendNode backendClipBlendNode;
            backendClipBlendNode.setClipBlendNodeManager(&manager);
            simulateInitializationSync(&clipBlendNode, &backendClipBlendNode);

            // THEN
            QCOMPARE(backendClipBlendNode.isEnabled(), true);
            QCOMPARE(backendClipBlendNode.peerId(), clipBlendNode.id());
            QCOMPARE(backendClipBlendNode.clipBlendNodeManager(), &manager);
            QCOMPARE(backendClipBlendNode.blendType(), ClipBlendNode::LerpBlendType);
            QCOMPARE(backendClipBlendNode.clipResults(Qt3DCore::QNodeId()), ClipResults());
        }
        {
            // WHEN
            ClipBlendNodeManager manager;
            TestClipBlendNode backendClipBlendNode;
            clipBlendNode.setEnabled(false);
            backendClipBlendNode.setClipBlendNodeManager(&manager);
            simulateInitializationSync(&clipBlendNode, &backendClipBlendNode);

            // THEN
            QCOMPARE(backendClipBlendNode.peerId(), clipBlendNode.id());
            QCOMPARE(backendClipBlendNode.isEnabled(), false);
        }
    }

    void checkClipResults_data()
    {
        QTest::addColumn<TestClipBlendNode *>("blendNode");
        QTest::addColumn<QList<int>>("indexes");
        QTest::addColumn<QList<Qt3DCore::QNodeId>>("animatorIds");
        QTest::addColumn<QList<ClipResults>>("expectedClipResults");

        // Single entry
        {
            auto blendNode = new TestClipBlendNode;
            QList<Qt3DCore::QNodeId> animatorIds;
            QList<ClipResults> expectedClipResults;

            const auto animatorId = Qt3DCore::QNodeId::createId();
            animatorIds.push_back(animatorId);

            ClipResults clipResults = { 0.0f, 1.0f, 2.0f };
            for (int i = 0; i < 3; ++i)
                clipResults.push_back(float(i));
            expectedClipResults.push_back(clipResults);

            // Set data and indexes
            blendNode->setClipResults(animatorId, clipResults);
            const QList<int> indexes = { 0 };

            QTest::newRow("single entry")
                    << blendNode << indexes << animatorIds << expectedClipResults;
        }

        // No data
        {
            auto blendNode = new TestClipBlendNode;
            QList<Qt3DCore::QNodeId> animatorIds;
            QList<ClipResults> expectedClipResults;

            auto animatorId = Qt3DCore::QNodeId::createId();
            animatorIds.push_back(animatorId);

            ClipResults clipResults;
            expectedClipResults.push_back(clipResults);

            // Don't set any data
            const QList<int> indexes = { 0 };

            QTest::newRow("no entries")
                    << blendNode << indexes << animatorIds << expectedClipResults;
        }

        // Multiple entries, ordered
        {
            auto blendNode = new TestClipBlendNode;
            QList<Qt3DCore::QNodeId> animatorIds;
            QList<ClipResults> expectedClipResults;

            const int animatorCount = 10;
            for (int j = 0; j < animatorCount; ++j) {
                auto animatorId = Qt3DCore::QNodeId::createId();
                animatorIds.push_back(animatorId);

                ClipResults clipResults;
                for (int i = 0; i < j + 5; ++i)
                    clipResults.push_back(float(i + j));
                expectedClipResults.push_back(clipResults);

                blendNode->setClipResults(animatorId, clipResults);
            }

            QVector<int> indexes(animatorCount);
            std::iota(indexes.begin(), indexes.end(), 0);

            QTest::newRow("multiple entries, ordered")
                    << blendNode << indexes << animatorIds << expectedClipResults;
        }

        // Multiple entries, unordered
        {
            auto blendNode = new TestClipBlendNode;
            QList<Qt3DCore::QNodeId> animatorIds;
            QList<ClipResults> expectedClipResults;

            const int animatorCount = 10;
            for (int j = 0; j < animatorCount; ++j) {
                auto animatorId = Qt3DCore::QNodeId::createId();
                animatorIds.push_back(animatorId);

                ClipResults clipResults;
                for (int i = 0; i < j + 5; ++i)
                    clipResults.push_back(float(i + j));
                expectedClipResults.push_back(clipResults);

                blendNode->setClipResults(animatorId, clipResults);
            }

            // Shuffle the animatorIds to randomise the lookups
            QVector<int> indexes(animatorCount);
            std::iota(indexes.begin(), indexes.end(), 0);
            std::random_device rd;
            std::mt19937 generator(rd());
            std::shuffle(indexes.begin(), indexes.end(), generator);

            QTest::newRow("multiple entries, unordered")
                    << blendNode << indexes << animatorIds << expectedClipResults;
        }
    }

    void checkClipResults()
    {
        // GIVEN
        QFETCH(TestClipBlendNode *, blendNode);
        QFETCH(QVector<int>, indexes);
        QFETCH(QList<Qt3DCore::QNodeId>, animatorIds);
        QFETCH(QList<ClipResults>, expectedClipResults);

        for (int i = 0; i < indexes.size(); ++i) {
            // WHEN
            const int index = indexes[i];
            const ClipResults actualClipResults = blendNode->clipResults(animatorIds[index]);

            // THEN
            QCOMPARE(actualClipResults.size(), expectedClipResults[index].size());
            for (int j = 0; j < actualClipResults.size(); ++j)
                QCOMPARE(actualClipResults[j], expectedClipResults[index][j]);
        }

        delete blendNode;
    }

    void checkPerformBlend()
    {
        // GIVEN
        auto handler = new Handler();
        ClipResults expectedResults = { 1.0f, 2.0f, 3.0f };
        auto blendNode = createTestClipBlendNode(handler, expectedResults);
        const qint64 globalStartTimeNS = 0;
        const int loopCount = 1;
        auto animator = createBlendedClipAnimator(handler, globalStartTimeNS, loopCount);

        // WHEN
        blendNode->blend(animator->peerId());

        // THEN
        const ClipResults actualResults = blendNode->clipResults(animator->peerId());
        QCOMPARE(actualResults.size(), expectedResults.size());
        for (int i = 0; i < actualResults.size(); ++i)
            QCOMPARE(actualResults[i], expectedResults[i]);
    }
};

QTEST_MAIN(tst_ClipBlendNode)

#include "tst_clipblendnode.moc"
