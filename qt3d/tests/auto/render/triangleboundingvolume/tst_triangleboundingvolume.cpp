// Copyright (C) 2015 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QTest>
#include <Qt3DRender/private/entity_p.h>
#include <Qt3DRender/private/triangleboundingvolume_p.h>
#include <Qt3DRender/private/qraycastingservice_p.h>
#include <Qt3DRender/private/qray3d_p.h>
#include <Qt3DRender/qcameralens.h>
#include <Qt3DRender/qcamera.h>
#include <Qt3DRender/private/qboundingvolume_p.h>

class tst_TriangleBoundingVolume : public QObject
{
    Q_OBJECT
public:
    tst_TriangleBoundingVolume() {}
    ~tst_TriangleBoundingVolume() {}

private Q_SLOTS:
    void checkInitialState()
    {
        // GIVEN
        Qt3DRender::Render::TriangleBoundingVolume volume = Qt3DRender::Render::TriangleBoundingVolume(Qt3DCore::QNodeId(),
                                                                                                       Vector3D(),
                                                                                                       Vector3D(),
                                                                                                       Vector3D());

        // THEN
        QCOMPARE(volume.id(), Qt3DCore::QNodeId());
        QCOMPARE(volume.a(), Vector3D());
        QCOMPARE(volume.b(), Vector3D());
        QCOMPARE(volume.c(), Vector3D());
        QCOMPARE(volume.type(), Qt3DRender::RayCasting::QBoundingVolume::Triangle);
    }

    void transformed_data()
    {
        QTest::addColumn<Vector3D>("a");
        QTest::addColumn<Vector3D>("b");
        QTest::addColumn<Vector3D>("c");

        QTest::addColumn<Vector3D>("transformedA");
        QTest::addColumn<Vector3D>("transformedB");
        QTest::addColumn<Vector3D>("transformedC");

        QTest::newRow("onFarPlane")
                << Vector3D(-1.0, 1.0, 0.0)
                << Vector3D(0.0, -1.0, 0.0)
                << Vector3D(1.0, 1.0, 0.0)
                << Vector3D(-1.0, 1.0, -40.0)
                << Vector3D(0.0, -1.0, -40.0)
                << Vector3D(1.0, 1.0, -40.0);

        QTest::newRow("onNearPlane")
                << Vector3D(-1.0, 1.0, 40.0)
                << Vector3D(0.0, -1.0, 40.0)
                << Vector3D(1.0, 1.0, 40.0)
                << Vector3D(-1.0, 1.0, 0.0)
                << Vector3D(0.0, -1.0, 0.0)
                << Vector3D(1.0, 1.0, 0.0);


    }

    void transformed()
    {
        // GIVEN
        QFETCH(Vector3D, a);
        QFETCH(Vector3D, b);
        QFETCH(Vector3D, c);
        QFETCH(Vector3D, transformedA);
        QFETCH(Vector3D, transformedB);
        QFETCH(Vector3D, transformedC);
        Qt3DRender::Render::TriangleBoundingVolume volume(Qt3DCore::QNodeId(),
                                                          a,
                                                          b,
                                                          c);
        Qt3DRender::QCamera camera;
        camera.setProjectionType(Qt3DRender::QCameraLens::PerspectiveProjection);
        camera.setFieldOfView(45.0f);
        camera.setAspectRatio(800.0f/600.0f);
        camera.setNearPlane(0.1f);
        camera.setFarPlane(1000.0f);
        camera.setPosition(QVector3D(0.0f, 0.0f, 40.0f));
        camera.setUpVector(QVector3D(0.0f, 1.0f, 0.0f));
        camera.setViewCenter(QVector3D(0.0f, 0.0f, 0.0f));

        const Matrix4x4 viewMatrix(camera.viewMatrix());

        // WHEN
        volume.transform(viewMatrix);

        // THEN
        QCOMPARE(transformedA, volume.a());
        QCOMPARE(transformedB, volume.b());
        QCOMPARE(transformedC, volume.c());
    }

    void intersects_data()
    {
        QTest::addColumn<Qt3DRender::RayCasting::QRay3D>("ray");
        QTest::addColumn<Vector3D>("a");
        QTest::addColumn<Vector3D>("b");
        QTest::addColumn<Vector3D>("c");
        QTest::addColumn<Vector3D>("uvw");
        QTest::addColumn<float>("t");
        QTest::addColumn<bool>("isIntersecting");

        const float farPlaneDistance = 40.0;

        QTest::newRow("halfway_center")
                << Qt3DRender::RayCasting::QRay3D(Vector3D(), Vector3D(0.0, 0.0, 1.0), farPlaneDistance)
                << Vector3D(3.0, 1.5, 20.0)
                << Vector3D(0.0, -1.5, 20.0)
                << Vector3D(-3, 1.5, 20.0)
                << Vector3D(0.25, 0.5, 0.25)
                << 0.5f
                << true;
        QTest::newRow("miss_halfway_center_too_short")
                << Qt3DRender::RayCasting::QRay3D(Vector3D(), Vector3D(0.0, 0.0, 1.0), farPlaneDistance * 0.25f)
                << Vector3D(3.0, 1.5, 20.0)
                << Vector3D(0.0, -1.5, 20.0)
                << Vector3D(-3, 1.5, 20.0)
                << Vector3D()
                << 0.0f
                << false;
        QTest::newRow("far_center")
                << Qt3DRender::RayCasting::QRay3D(Vector3D(), Vector3D(0.0, 0.0, 1.0), farPlaneDistance)
                << Vector3D(3.0, 1.5, 40.0)
                << Vector3D(0.0, -1.5, 40.0)
                << Vector3D(-3, 1.5, 40.0)
                << Vector3D(0.25, 0.5, 0.25)
                << 1.0f
                << true;
        QTest::newRow("near_center")
                << Qt3DRender::RayCasting::QRay3D(Vector3D(), Vector3D(0.0, 0.0, 1.0), 1.0f)
                << Vector3D(3.0, 1.5, 0.0)
                << Vector3D(0.0, -1.5, 0.0)
                << Vector3D(-3, 1.5, 0.0)
                << Vector3D(0.25, 0.5, 0.25)
                << 0.0f
                << true;
        QTest::newRow("above_miss_center")
                << Qt3DRender::RayCasting::QRay3D(Vector3D(0.0, 2.0, 0.0), Vector3D(0.0, 2.0, 1.0), 1.0f)
                << Vector3D(3.0, 1.5, 0.0)
                << Vector3D(0.0, -1.5, 0.0)
                << Vector3D(-3, 1.5, 0.0)
                << Vector3D()
                << 0.0f
                << false;
        QTest::newRow("below_miss_center")
                << Qt3DRender::RayCasting::QRay3D(Vector3D(0.0, -2.0, 0.0), Vector3D(0.0, -2.0, 1.0), 1.0f)
                << Vector3D(3.0, 1.5, 0.0)
                << Vector3D(0.0, -1.5, 0.0)
                << Vector3D(-3, 1.5, 0.0)
                << Vector3D()
                << 0.0f
                << false;
    }

    void intersects()
    {
        // GIVEN
        QFETCH(Qt3DRender::RayCasting::QRay3D, ray);
        QFETCH(Vector3D, a);
        QFETCH(Vector3D, b);
        QFETCH(Vector3D, c);
        QFETCH(Vector3D, uvw);
        QFETCH(float, t);
        QFETCH(bool, isIntersecting);

        // WHEN
        Vector3D tmp_uvw;
        float tmp_t;
        const bool shouldBeIntersecting = Qt3DRender::Render::intersectsSegmentTriangle(ray,
                                                                                        a, b, c,
                                                                                        tmp_uvw,
                                                                                        tmp_t);

        // THEN
        QCOMPARE(shouldBeIntersecting, isIntersecting);
        if (isIntersecting) {
            QVERIFY(qFuzzyCompare(uvw, tmp_uvw));
            QVERIFY(qFuzzyCompare(t, tmp_t));
        }
    }
};

QTEST_APPLESS_MAIN(tst_TriangleBoundingVolume)

#include "tst_triangleboundingvolume.moc"
