// Copyright (C) 2015 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QTest>
#include <Qt3DCore/QEntity>
#include <Qt3DCore/private/qnode_p.h>
#include <Qt3DCore/private/qscene_p.h>
#include <Qt3DCore/qbackendnodepropertychange.h>
#include <Qt3DRender/private/qboundingvolumedebug_p.h>

#include "testpostmanarbiter.h"

#if 0

class MyBoundingVolumeDebug : public Qt3DRender::QBoundingVolumeDebug
{
    Q_OBJECT
public:
    MyBoundingVolumeDebug(Qt3DCore::QNode *parent = nullptr)
        : Qt3DRender::QBoundingVolumeDebug(parent)
    {}

    void sceneChangeEvent(const Qt3DCore::QSceneChangePtr &change) final
    {
        Qt3DRender::QBoundingVolumeDebug::sceneChangeEvent(change);
    }

private:
    friend class tst_ObjectPicker;

};

// We need to call QNode::clone which is protected
// So we sublcass QNode instead of QObject
class tst_QBoundingVolumeDebug : public Qt3DCore::QNode
{
    Q_OBJECT
public:
    tst_QBoundingVolumeDebug()
    {
    }

    ~tst_QBoundingVolumeDebug()
    {
        QNode::cleanup();
    }

private Q_SLOTS:

    void checkCloning_data()
    {
        QTest::addColumn<Qt3DRender::QBoundingVolumeDebug *>("bvD");

        Qt3DRender::QBoundingVolumeDebug *bvD = new Qt3DRender::QBoundingVolumeDebug();
        QTest::newRow("empty bvd") << bvD;
        bvD = new Qt3DRender::QBoundingVolumeDebug();
        bvD->setRecursive(true);
        QTest::newRow("recursive_bvd") << bvD;
    }

    // TODO: Avoid cloning here
//    void checkCloning()
//    {
//        // GIVEN
//        QFETCH(Qt3DRender::QBoundingVolumeDebug *, bvD);

//        // WHEN
//        Qt3DRender::QBoundingVolumeDebug *clone = static_cast<Qt3DRender::QBoundingVolumeDebug *>(QNode::clone(bvD));
//        QCoreApplication::processEvents();

//        // THEN
//        QVERIFY(clone != nullptr);
//        QCOMPARE(bvD->id(), clone->id());
//        QCOMPARE(bvD->recursive(), clone->recursive());
//    }

    void checkPropertyUpdates()
    {
        // GIVEN
        QScopedPointer<Qt3DRender::QBoundingVolumeDebug> bvD(new Qt3DRender::QBoundingVolumeDebug());
        TestArbiter arbiter(bvD.data());

        // WHEN
        bvD->setRecursive(true);
        QCoreApplication::processEvents();

        // THEN
        QCOMPARE(arbiter.events.size(), 1);
        Qt3DCore::QNodePropertyChangePtr change = arbiter.events.last().staticCast<Qt3DCore::QNodePropertyChange>();
        QCOMPARE(change->propertyName(), "recursive");
        QCOMPARE(change->value().toBool(), true);
        QCOMPARE(change->type(), Qt3DCore::NodeUpdated);

        arbiter.events.clear();

        // WHEN
        bvD->setRecursive(true);
        QCoreApplication::processEvents();

        // THEN
        QCOMPARE(arbiter.events.size(), 0);

        // WHEN
        bvD->setRecursive(false);
        QCoreApplication::processEvents();

        // THEN
        QCOMPARE(arbiter.events.size(), 1);
        change = arbiter.events.last().staticCast<Qt3DCore::QNodePropertyChange>();
        QCOMPARE(change->propertyName(), "recursive");
        QCOMPARE(change->value().toBool(), false);
        QCOMPARE(change->type(), Qt3DCore::NodeUpdated);

        arbiter.events.clear();
    }

    void checkBackendUpdates()
    {
        // GIVEN
        QScopedPointer<Qt3DCore::QEntity> entity(new Qt3DCore::QEntity());
        MyBoundingVolumeDebug *bvD(new MyBoundingVolumeDebug(entity.data()));
        entity->addComponent(bvD);

        // THEN
        QCoreApplication::processEvents();
        QCOMPARE(entity->children().count(), 1);
        QCOMPARE(bvD->children().count(), 0);

        // WHEN
        // Create Backend Change and distribute it to frontend node
        Qt3DCore::QBackendNodePropertyChangePtr e(new Qt3DCore::QBackendNodePropertyChange(bvD->id()));
        e->setPropertyName("center");
        bvD->sceneChangeEvent(e);
        QCoreApplication::processEvents();

        // THEN
        QCOMPARE(entity->children().count(), 2);
        QCOMPARE(bvD->children().count(), 0);
    }
};

QTEST_MAIN(tst_QBoundingVolumeDebug)

#include "tst_qboundingvolumedebug.moc"
