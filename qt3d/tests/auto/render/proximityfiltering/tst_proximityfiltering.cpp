// Copyright (C) 2016 Paul Lemire
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QTest>
#include <Qt3DCore/qentity.h>
#include <Qt3DCore/qtransform.h>
#include <Qt3DCore/qgeometry.h>
#include <Qt3DRender/qgeometryrenderer.h>
#include <Qt3DCore/qattribute.h>
#include <Qt3DCore/qbuffer.h>
#include <Qt3DRender/private/nodemanagers_p.h>
#include <Qt3DRender/private/managers_p.h>
#include <Qt3DRender/private/entity_p.h>
#include <Qt3DRender/private/filterproximitydistancejob_p.h>
#include <Qt3DRender/private/updatetreeenabledjob_p.h>
#include <Qt3DRender/private/updateworldtransformjob_p.h>
#include <Qt3DRender/private/updateworldboundingvolumejob_p.h>
#include <Qt3DRender/private/calcboundingvolumejob_p.h>
#include <Qt3DRender/private/expandboundingvolumejob_p.h>
#include <Qt3DRender/qproximityfilter.h>

#include "testaspect.h"

namespace {

Qt3DCore::QEntity *buildEntityAtDistance(float distance, Qt3DCore::QEntity *parent)
{
    Qt3DCore::QEntity *entity = new Qt3DCore::QEntity(parent);

    // create geometry with a valid bounding volume - a single point is sufficient
    auto geometry = new Qt3DCore::QGeometry;
    auto vertexBuffer = new Qt3DCore::QBuffer(geometry);

    auto positionAttribute = new Qt3DCore::QAttribute;
    positionAttribute->setName(Qt3DCore::QAttribute::defaultPositionAttributeName());
    positionAttribute->setAttributeType(Qt3DCore::QAttribute::VertexAttribute);
    positionAttribute->setVertexBaseType(Qt3DCore::QAttribute::Float);
    positionAttribute->setVertexSize(3);
    positionAttribute->setByteStride(3 * sizeof(float));
    positionAttribute->setBuffer(vertexBuffer);

    QByteArray vertexBufferData;
    vertexBufferData.resize(static_cast<int>(3 * sizeof(float)));

    auto vertexArray = reinterpret_cast<float*>(vertexBufferData.data());

    int i = 0;
    vertexArray[i++] = 0.0f;
    vertexArray[i++] = 0.0f;
    vertexArray[i++] = 0.0f;

    vertexBuffer->setData(vertexBufferData);
    positionAttribute->setCount(1);

    geometry->addAttribute(positionAttribute);

    auto geometryRenderer = new Qt3DRender::QGeometryRenderer;
    geometryRenderer->setPrimitiveType(Qt3DRender::QGeometryRenderer::Points);
    geometryRenderer->setGeometry(geometry);

    entity->addComponent(geometryRenderer);

    Qt3DCore::QTransform *transform = new Qt3DCore::QTransform(parent);
    const QVector3D t = QVector3D(1.0f, 0.0f, 0.0f) * distance;

    transform->setTranslation(t);
    entity->addComponent(transform);

    return entity;
}

} // anonymous

class tst_ProximityFiltering : public QObject
{
    Q_OBJECT
private Q_SLOTS:

    void checkInitialState()
    {
        // GIVEN
        Qt3DRender::Render::FilterProximityDistanceJob filterJob;

        // THEN
        QCOMPARE(filterJob.hasProximityFilter(), false);
        QCOMPARE(filterJob.filteredEntities().size(), 0U);
        QCOMPARE(filterJob.proximityFilterIds().size(), 0U);
        QVERIFY(filterJob.manager() == nullptr);
    }

    void filterEntities_data()
    {
        QTest::addColumn<Qt3DCore::QEntity *>("entitySubtree");
        QTest::addColumn<Qt3DCore::QNodeIdVector>("proximityFilterIds");
        QTest::addColumn<Qt3DCore::QNodeIdVector>("expectedSelectedEntities");

        {
            Qt3DCore::QEntity *rootEntity = new Qt3DCore::QEntity();
            Qt3DCore::QEntity *targetEntity = new Qt3DCore::QEntity(rootEntity);
            Qt3DCore::QEntity *childEntity1 = buildEntityAtDistance(50.0f, rootEntity);
            Qt3DCore::QEntity *childEntity2 = buildEntityAtDistance(25.0f, rootEntity);
            Qt3DCore::QEntity *childEntity3 = buildEntityAtDistance(75.0f, rootEntity);

            Qt3DRender::QProximityFilter *proximityFilter = new Qt3DRender::QProximityFilter(rootEntity);
            proximityFilter->setDistanceThreshold(200.0f);
            proximityFilter->setEntity(targetEntity);

            QTest::newRow("ShouldSelectAll") << rootEntity
                                             << (Qt3DCore::QNodeIdVector() << proximityFilter->id())
                                             << (Qt3DCore::QNodeIdVector()
                                                 << rootEntity->id()
                                                 << targetEntity->id()
                                                 << childEntity1->id()
                                                 << childEntity2->id()
                                                 << childEntity3->id()
                                                 );
        }

        {
            Qt3DCore::QEntity *rootEntity = new Qt3DCore::QEntity();
            Qt3DCore::QEntity *childEntity1 = new Qt3DCore::QEntity(rootEntity);
            Qt3DCore::QEntity *childEntity2 = new Qt3DCore::QEntity(rootEntity);
            Qt3DCore::QEntity *childEntity3 = new Qt3DCore::QEntity(rootEntity);
            Q_UNUSED(childEntity1);
            Q_UNUSED(childEntity2);
            Q_UNUSED(childEntity3);

            Qt3DRender::QProximityFilter *proximityFilter = new Qt3DRender::QProximityFilter(rootEntity);

            QTest::newRow("ShouldSelectNone") << rootEntity
                                              << (Qt3DCore::QNodeIdVector() << proximityFilter->id())
                                              << (Qt3DCore::QNodeIdVector());
        }

        {
            Qt3DCore::QEntity *rootEntity = new Qt3DCore::QEntity();
            Qt3DCore::QEntity *targetEntity = new Qt3DCore::QEntity(rootEntity);
            Qt3DCore::QEntity *childEntity1 = buildEntityAtDistance(50.0f, rootEntity);
            Qt3DCore::QEntity *childEntity2 = buildEntityAtDistance(25.0f, rootEntity);
            Qt3DCore::QEntity *childEntity3 = buildEntityAtDistance(75.0f, rootEntity);
            Qt3DRender::QProximityFilter *proximityFilter = new Qt3DRender::QProximityFilter(rootEntity);
            proximityFilter->setDistanceThreshold(30.0f);

            // Note: rootEntity BoundingSphere will be centered in vec3(75.0f / 2.0, 0.0f 0.0f);

            // Note: we cannot set rootEntity here as that would mean
            // that the parent of the root would then be the proximity filter
            // (since rootEntity has no parent) but this isn't valid in the way
            // we have build the test
            // Also rootEntity is centered based on the size of the child it contains
            proximityFilter->setEntity(targetEntity);

            Q_UNUSED(childEntity1);
            Q_UNUSED(childEntity3);

            QTest::newRow("ShouldSelectChild2") << rootEntity
                                                << (Qt3DCore::QNodeIdVector() << proximityFilter->id())
                                                << (Qt3DCore::QNodeIdVector()
                                                    << targetEntity->id()
                                                    << childEntity2->id()
                                                    );
        }

        {
            Qt3DCore::QEntity *rootEntity = new Qt3DCore::QEntity();
            Qt3DCore::QEntity *targetEntity = new Qt3DCore::QEntity(rootEntity);
            Qt3DCore::QEntity *childEntity1 = buildEntityAtDistance(50.0f, rootEntity);
            Qt3DCore::QEntity *childEntity2 = buildEntityAtDistance(25.0f, rootEntity);
            Qt3DCore::QEntity *childEntity3 = buildEntityAtDistance(49.9f, rootEntity);
            Qt3DRender::QProximityFilter *proximityFilter = new Qt3DRender::QProximityFilter(rootEntity);
            proximityFilter->setDistanceThreshold(50.0f);

            // Note: rootEntity BoundingSphere will be centered in vec3(50.0f / 2.0, 0.0f 0.0f);

            // Note: we cannot set rootEntity here as that would mean
            // that the parent of the root would then be the proximity filter
            // (since rootEntity has no parent) but this isn't valid in the way
            // we have build the test
            // Also rootEntity is centered based on the size of the child it contains
            proximityFilter->setEntity(targetEntity);

            QTest::newRow("ShouldSelectRootChild123") << rootEntity
                                                      << (Qt3DCore::QNodeIdVector() << proximityFilter->id())
                                                      << (Qt3DCore::QNodeIdVector()
                                                          << rootEntity->id()
                                                          << targetEntity->id()
                                                          << childEntity1->id()
                                                          << childEntity2->id()
                                                          << childEntity3->id()
                                                          );
        }

        {
            Qt3DCore::QEntity *rootEntity = new Qt3DCore::QEntity();
            Qt3DCore::QEntity *targetEntity = new Qt3DCore::QEntity(rootEntity);
            Qt3DCore::QEntity *childEntity1 = buildEntityAtDistance(51.0f, rootEntity);
            Qt3DCore::QEntity *childEntity2 = buildEntityAtDistance(75.0f, rootEntity);
            Qt3DCore::QEntity *childEntity3 = buildEntityAtDistance(883.0f, rootEntity);
            Qt3DRender::QProximityFilter *proximityFilter = new Qt3DRender::QProximityFilter(rootEntity);
            proximityFilter->setDistanceThreshold(50.0f);

            // Note: rootEntity BoundingSphere will be centered in vec3(883.0f / 2.0, 0.0f 0.0f);

            // Note: we cannot set rootEntity here as that would mean
            // that the parent of the root would then be the proximity filter
            // (since rootEntity has no parent) but this isn't valid in the way
            // we have build the test
            // Also rootEntity is centered based on the size of the child it contains
            proximityFilter->setEntity(targetEntity);

            Q_UNUSED(childEntity1);
            Q_UNUSED(childEntity2);
            Q_UNUSED(childEntity3);

            QTest::newRow("ShouldSelectNoneButTarget") << rootEntity
                                                       << (Qt3DCore::QNodeIdVector() << proximityFilter->id())
                                                       << (Qt3DCore::QNodeIdVector() << targetEntity->id());
        }

        {
            Qt3DCore::QEntity *rootEntity = new Qt3DCore::QEntity();
            Qt3DCore::QEntity *targetEntity = new Qt3DCore::QEntity(rootEntity);
            Qt3DCore::QEntity *childEntity1 = buildEntityAtDistance(50.0f, rootEntity);
            Qt3DCore::QEntity *childEntity2 = buildEntityAtDistance(150.0f, rootEntity);
            Qt3DCore::QEntity *childEntity3 = buildEntityAtDistance(25.0f, rootEntity);

            Qt3DRender::QProximityFilter *proximityFilter = new Qt3DRender::QProximityFilter(rootEntity);
            proximityFilter->setDistanceThreshold(50.0f);

            Qt3DRender::QProximityFilter *proximityFilter2 = new Qt3DRender::QProximityFilter(rootEntity);
            proximityFilter2->setDistanceThreshold(30.0f);

            // Note: rootEntity BoundingSphere will be centered in vec3(150.0f / 2.0, 0.0f 0.0f);

            // Note: we cannot set rootEntity here as that would mean
            // that the parent of the root would then be the proximity filter
            // (since rootEntity has no parent) but this isn't valid in the way
            // we have build the test
            // Also rootEntity is centered based on the size of the child it contains
            proximityFilter->setEntity(targetEntity);
            proximityFilter2->setEntity(targetEntity);

            Q_UNUSED(childEntity2);

            QTest::newRow("Nested-Step1") << rootEntity
                                          << (Qt3DCore::QNodeIdVector() << proximityFilter->id())
                                          << (Qt3DCore::QNodeIdVector()
                                              << targetEntity->id()
                                              << childEntity1->id()
                                              << childEntity3->id()
                                              );
            QTest::newRow("Nested-Step2") << rootEntity
                                          << (Qt3DCore::QNodeIdVector() << proximityFilter->id() << proximityFilter2->id())
                                          << (Qt3DCore::QNodeIdVector()
                                              << targetEntity->id()
                                              << childEntity3->id()
                                              );
        }
    }

    void filterEntities()
    {
        QFETCH(Qt3DCore::QEntity *, entitySubtree);
        QFETCH(Qt3DCore::QNodeIdVector, proximityFilterIds);
        QFETCH(Qt3DCore::QNodeIdVector, expectedSelectedEntities);

        // GIVEN
        QScopedPointer<Qt3DRender::TestAspect> aspect(new Qt3DRender::TestAspect(entitySubtree));
        aspect->registerTree(entitySubtree);

        // WHEN
        Qt3DRender::Render::Entity *backendRoot = aspect->nodeManagers()->renderNodesManager()->getOrCreateResource(entitySubtree->id());

        Qt3DRender::Render::UpdateTreeEnabledJob updateTreeEnabledJob;
        updateTreeEnabledJob.setRoot(backendRoot);
        updateTreeEnabledJob.setManagers(aspect->nodeManagers());
        updateTreeEnabledJob.run();

        Qt3DRender::Render::UpdateWorldTransformJob updateWorldTransform;
        updateWorldTransform.setRoot(backendRoot);
        updateWorldTransform.setManagers(aspect->nodeManagers());
        updateWorldTransform.run();

        Qt3DRender::Render::CalculateBoundingVolumeJob calcBVolume;
        calcBVolume.setManagers(aspect->nodeManagers());
        calcBVolume.setRoot(backendRoot);
        calcBVolume.setFrontEndNodeManager(aspect.data());
        calcBVolume.run();

        Qt3DRender::Render::UpdateWorldBoundingVolumeJob updateWorldBVolume;
        updateWorldBVolume.setManager(aspect->nodeManagers()->renderNodesManager());
        updateWorldBVolume.run();

        Qt3DRender::Render::ExpandBoundingVolumeJob expandBVolume;
        expandBVolume.setRoot(backendRoot);
        expandBVolume.setManagers(aspect->nodeManagers());
        expandBVolume.run();

        // WHEN
        Qt3DRender::Render::FilterProximityDistanceJob filterJob;
        filterJob.setProximityFilterIds(proximityFilterIds);
        filterJob.setManager(aspect->nodeManagers());
        filterJob.run();

        // THEN
        const std::vector<Qt3DRender::Render::Entity *> &filterEntities = filterJob.filteredEntities();
        QCOMPARE(filterEntities.size(), size_t(expectedSelectedEntities.size()));

        for (int i = 0, m = expectedSelectedEntities.size(); i < m; ++i)
            QCOMPARE(filterEntities.at(i)->peerId(), expectedSelectedEntities.at(i));
    }
};

QTEST_MAIN(tst_ProximityFiltering)

#include "tst_proximityfiltering.moc"
