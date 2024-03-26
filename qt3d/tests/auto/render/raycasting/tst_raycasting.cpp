// Copyright (C) 2015 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QTest>
#include <Qt3DRender/private/entity_p.h>
#include <Qt3DRender/private/qraycastingservice_p.h>
#include <Qt3DRender/private/sphere_p.h>
#include <Qt3DRender/private/entity_p.h>
#include <Qt3DRender/private/abstractpickingjob_p.h>
#include <Qt3DRender/private/qboundingvolumeprovider_p.h>
#include <Qt3DRender/private/qray3d_p.h>
#include <Qt3DRender/qcamera.h>

using namespace Qt3DCore;
using namespace Qt3DRender;
using namespace Qt3DRender::Render;
using namespace Qt3DRender::RayCasting;

class tst_RayCasting : public QObject
{
    Q_OBJECT
public:
    tst_RayCasting() {}
    ~tst_RayCasting() {}

private Q_SLOTS:
    void initTestCase();
    void shouldReturnValidHandle();
    void shouldReturnResultForEachHandle();
    void shouldReturnAllResults();
    void shouldReturnHits();
    void shouldReturnHits_data();
    void shouldIntersect_data();
    void shouldIntersect();
    void shouldUseProvidedBoudingVolumes();
    void mousePicking();

    void cleanupTestCase();

private:
    Sphere *volumeAt(int index);
    QList<Sphere> boundingVolumes;
};

void tst_RayCasting::initTestCase()
{
#if defined Q_OS_QNX
    QSKIP("This test times out on QNX (QTBUG-107694)");
#endif
}

void tst_RayCasting::shouldIntersect_data()
{
    QTest::addColumn<QRay3D>("ray");
    QTest::addColumn<Sphere>("sphere");
    QTest::addColumn<bool>("shouldIntersect");

    QRay3D ray(Vector3D(1, 1, 1), Vector3D(0, 0, 1));

    Sphere sphere1(Vector3D(1, 1, 1), 2);
    Sphere sphere2(Vector3D(0, 0, 0), 3);
    Sphere sphere3(Vector3D(0, 1, 3), 1);
    Sphere sphere4(Vector3D(4, 4, 5), 1);
    Sphere sphere5(Vector3D(2, 2, 11), 5);
    Sphere sphere6(Vector3D(2, 2, 13), 1);
    Sphere sphere7(Vector3D(2, 2, 15), 5);

    QTest::newRow("Ray starts inside sphere") << ray << sphere1 << true;
    QTest::newRow("Ray starts inside sphere") << ray << sphere2 << true;
    QTest::newRow("Ray intersects sphere tangentially") << ray << sphere3 << true;
    QTest::newRow("No intersection") << ray << sphere4 << false;
    QTest::newRow("Ray intersect sphere") << ray << sphere5 << true;
    QTest::newRow("No intersection") << ray << sphere6 << false;
    QTest::newRow("Ray intersect sphere") << ray << sphere7 << true;
}

void tst_RayCasting::shouldIntersect()
{
    QFETCH(QRay3D, ray);
    QFETCH(Sphere, sphere);
    QFETCH(bool, shouldIntersect);

    Vector3D intersectionPoint;

    QCOMPARE(sphere.intersects(ray, &intersectionPoint), shouldIntersect);
}

class MyBoudingVolumesProvider : public QBoundingVolumeProvider
{
public:
    MyBoudingVolumesProvider(QList<QBoundingVolume *> volumes)
        : m_volumes(volumes)
    {}

    QList<QBoundingVolume *> boundingVolumes() const override
    {
        return m_volumes;
    }

private:
    QList<QBoundingVolume *> m_volumes;
};

void tst_RayCasting::shouldReturnValidHandle()
{
    // GIVEN
    QRay3D ray;
    Sphere v1;
    MyBoudingVolumesProvider provider = QList<QBoundingVolume *> { &v1 };

    QRayCastingService service;

    // WHEN
    QQueryHandle handle = service.query(ray,
                                        QAbstractCollisionQueryService::AllHits,
                                        &provider);

    // THEN
    QVERIFY(handle >= 0);

    // Wait the query to finish
    service.fetchResult(handle);
}

void tst_RayCasting::shouldReturnResultForEachHandle()
{
    // GIVEN
    QRay3D ray;
    QList<QBoundingVolume *> volumes;
    MyBoudingVolumesProvider provider(volumes);

    QRayCastingService service;

    QQueryHandle handle1 = service.query(ray,
                                         QAbstractCollisionQueryService::AllHits,
                                         &provider);
    QQueryHandle handle2 = service.query(ray,
                                         QAbstractCollisionQueryService::FirstHit,
                                         &provider);

    // WHEN
    QCollisionQueryResult result2 = service.fetchResult(handle2);
    QCollisionQueryResult result1 = service.fetchResult(handle1);

    // THEN
    QCOMPARE(result1.handle(), handle1);
    QCOMPARE(result2.handle(), handle2);
}

void tst_RayCasting::shouldReturnAllResults()
{
    // GIVEN
    QRay3D ray;
    QList<QBoundingVolume *> volumes;
    MyBoudingVolumesProvider provider(volumes);

    QRayCastingService service;

    QList<QQueryHandle> handles;
    handles.append(service.query(ray,
                                 QAbstractCollisionQueryService::AllHits,
                                 &provider));
    handles.append(service.query(ray,
                                 QAbstractCollisionQueryService::FirstHit,
                                 &provider));

    // WHEN
    const QList<QCollisionQueryResult> results = service.fetchAllResults();

    // THEN
    bool expectedHandlesFound = true;
    for (QQueryHandle expected : std::as_const(handles)) {
        bool found = false;
        for (QCollisionQueryResult result : results) {
            if (result.handle() == expected)
                found = true;
        }

        expectedHandlesFound &= found;
    }

    QVERIFY(expectedHandlesFound);
}

void tst_RayCasting::shouldReturnHits_data()
{
    QTest::addColumn<QRay3D>("ray");
    QTest::addColumn<QList<QBoundingVolume *>>("volumes");
    QTest::addColumn<std::vector<QNodeId> >("hits");
    QTest::addColumn<QAbstractCollisionQueryService::QueryMode>("queryMode");

    QRay3D ray(Vector3D(1, 1, 1), Vector3D(0, 0, 1));

    this->boundingVolumes.clear();
    this->boundingVolumes.append(QList<Sphere>
                                 { Sphere(Vector3D(1, 1, 1), 3, QNodeId::createId()),
                                   Sphere(Vector3D(0, 0, 0), 3, QNodeId::createId()),
                                   Sphere(Vector3D(0, 1, 3), 1, QNodeId::createId()),
                                   Sphere(Vector3D(4, 4, 5), 1, QNodeId::createId()),
                                   Sphere(Vector3D(2, 2, 11), 5, QNodeId::createId()),
                                   Sphere(Vector3D(2, 2, 13), 1, QNodeId::createId()),
                                   Sphere(Vector3D(2, 2, 15), 5, QNodeId::createId()) });

    QTest::newRow("All hits, One sphere intersect") << ray
                                                    << (QList<QBoundingVolume *> { volumeAt(0), volumeAt(3) })
                                                    << (std::vector<QNodeId>{ volumeAt(0)->id() })
                                                    << QAbstractCollisionQueryService::AllHits;

    QTest::newRow("All hits, Three sphere intersect") << ray
                                                      << (QList<QBoundingVolume *> { volumeAt(0), volumeAt(3), volumeAt(6), volumeAt(2) })
                                                      << (std::vector<QNodeId>{ volumeAt(0)->id(), volumeAt(2)->id(), volumeAt(6)->id() })
                                                      << QAbstractCollisionQueryService::AllHits;

    QTest::newRow("All hits, No sphere intersect") << ray
                                                   << (QList<QBoundingVolume *> { volumeAt(3), volumeAt(5) })
                                                   << (std::vector<QNodeId>{})
                                                   << QAbstractCollisionQueryService::AllHits;

    QTest::newRow("Sphere 1 intersect, returns First Hit") << ray
                                                           << (QList<QBoundingVolume *> { volumeAt(0), volumeAt(3), volumeAt(6) })
                                                           << (std::vector<QNodeId>{ volumeAt(0)->id() })
                                                           << QAbstractCollisionQueryService::FirstHit;

    QTest::newRow("Sphere 3 and 5 intersects, returns First Hit") << ray
                                                                  << (QList<QBoundingVolume *> { volumeAt(3), volumeAt(6), volumeAt(4) })
                                                                  << (std::vector<QNodeId>{ volumeAt(4)->id() })
                                                                  << QAbstractCollisionQueryService::FirstHit;

    QTest::newRow("Sphere 5 and 3 intersects, unordered list, returns First Hit") << ray
                                                                                  << (QList<QBoundingVolume *> { volumeAt(4), volumeAt(3), volumeAt(6) })
                                                                                  << (std::vector<QNodeId>{volumeAt(4)->id() })
                                                                                  << QAbstractCollisionQueryService::FirstHit;

    QTest::newRow("No sphere intersect, returns First Hit") << ray
                                                            << (QList<QBoundingVolume *> { volumeAt(3), volumeAt(5) })
                                                            << (std::vector<QNodeId>{})
                                                            << QAbstractCollisionQueryService::FirstHit;
}

void tst_RayCasting::shouldReturnHits()
{
    // GIVEN
    QFETCH(QRay3D, ray);
    QFETCH(QList<QBoundingVolume *>, volumes);
    QFETCH(std::vector<QNodeId>, hits);
    QFETCH(QAbstractCollisionQueryService::QueryMode, queryMode);

    MyBoudingVolumesProvider provider(volumes);
    QRayCastingService service;

    // WHEN
    QQueryHandle handle = service.query(ray, queryMode, &provider);
    QCollisionQueryResult result = service.fetchResult(handle);

    // THEN
    QCOMPARE(result.entitiesHit().size(), hits.size());
    QCOMPARE(result.entitiesHit(), hits);
}

void tst_RayCasting::shouldUseProvidedBoudingVolumes()
{
    // GIVEN
    QRay3D ray(Vector3D(1, 1, 1), Vector3D(0, 0, 1));

    Sphere sphere1(Vector3D(1, 1, 1), 3);
    Sphere sphere3(Vector3D(0, 1, 3), 1);
    Sphere sphere4(Vector3D(4, 4, 5), 1);

    MyBoudingVolumesProvider provider(QList<QBoundingVolume *> { &sphere1, &sphere4, &sphere3 });
    std::vector<QNodeId> hits{ sphere1.id(), sphere3.id()};

    QRayCastingService service;

    // WHEN
    QQueryHandle handle = service.query(ray,
                                        QAbstractCollisionQueryService::AllHits,
                                        &provider);
    QCollisionQueryResult result = service.fetchResult(handle);

    // THEN
    QCOMPARE(result.entitiesHit().size(), hits.size());
    QCOMPARE(result.entitiesHit(), hits);
}

void tst_RayCasting::cleanupTestCase()
{
    this->boundingVolumes.clear();
}

Sphere *tst_RayCasting::volumeAt(int index)
{
    return &*(boundingVolumes.begin() + index);
}

void tst_RayCasting::mousePicking()
{
    // GIVEN
    Qt3DRender::QCamera camera;
    camera.setProjectionType(QCameraLens::PerspectiveProjection);
    camera.setFieldOfView(45.0f);
    camera.setAspectRatio(800.0f/600.0f);
    camera.setNearPlane(0.1f);
    camera.setFarPlane(1000.0f);
    camera.setPosition(QVector3D(0.0f, 0.0f, -40.0f));
    camera.setUpVector(QVector3D(0.0f, 1.0f, 0.0f));
    camera.setViewCenter(QVector3D(0.0f, 0.0f, 0.0f));

    const QRectF viewport(0., 0., 800., 600.);

    // Window center on near plane
    QRay3D ray = Qt3DRender::Render::AbstractPickingJob::intersectionRay(viewport.center().toPoint(),
                                                                         Matrix4x4(camera.viewMatrix()),
                                                                         Matrix4x4(camera.projectionMatrix()),
                                                                         viewport.toRect());
    Qt3DRender::Render::Sphere s(Vector3D(0.0f, 0.5f, 0.0f), 1.0f);

    // WHEN
    bool intersects = s.intersects(ray, nullptr);

    // THEN
    QVERIFY(intersects);

    // WHEN
    ray = Qt3DRender::Render::AbstractPickingJob::intersectionRay(viewport.topLeft().toPoint(),
                                                                  Matrix4x4(camera.viewMatrix()),
                                                                  Matrix4x4(camera.projectionMatrix()),
                                                                  viewport.toRect());
    intersects = s.intersects(ray, nullptr);

    // THEN
    QVERIFY(!intersects);

    // WHEN
    ray = Qt3DRender::Render::AbstractPickingJob::intersectionRay(viewport.topRight().toPoint(),
                                                                  Matrix4x4(camera.viewMatrix()),
                                                                  Matrix4x4(camera.projectionMatrix()),
                                                                  viewport.toRect());
    intersects = s.intersects(ray, nullptr);

    // THEN
    QVERIFY(!intersects);

    // WHEN
    ray = Qt3DRender::Render::AbstractPickingJob::intersectionRay(viewport.bottomLeft().toPoint(),
                                                                  Matrix4x4(camera.viewMatrix()),
                                                                  Matrix4x4(camera.projectionMatrix()),
                                                                  viewport.toRect());
    intersects = s.intersects(ray, nullptr);

    // THEN
    QVERIFY(!intersects);

    // WHEN
    ray = Qt3DRender::Render::AbstractPickingJob::intersectionRay(viewport.bottomRight().toPoint(),
                                                                  Matrix4x4(camera.viewMatrix()),
                                                                  Matrix4x4(camera.projectionMatrix()),
                                                                  viewport.toRect());
    intersects = s.intersects(ray, nullptr);

    // THEN
    QVERIFY(!intersects);
}

QTEST_APPLESS_MAIN(tst_RayCasting)

#include "tst_raycasting.moc"
