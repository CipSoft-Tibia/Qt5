// Copyright (C) 2016 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QTest>
#include <Qt3DCore/qentity.h>
#include <Qt3DCore/private/qnode_p.h>
#include <Qt3DCore/private/qscene_p.h>
#include <Qt3DCore/qtransform.h>

#include <Qt3DRender/qsceneloader.h>
#include <Qt3DRender/qcameralens.h>
#include <Qt3DRender/qmaterial.h>
#include <Qt3DRender/qgeometryrenderer.h>
#include <Qt3DRender/qspotlight.h>
#include <Qt3DRender/private/qsceneloader_p.h>
#include <QSignalSpy>

#include "testarbiter.h"

class tst_QSceneLoader: public QObject
{
    Q_OBJECT

private Q_SLOTS:

    void checkInitialState()
    {
        // GIVEN
        Qt3DRender::QSceneLoader sceneLoader;

        // THEN
        QCOMPARE(sceneLoader.status(), Qt3DRender::QSceneLoader::None);
        QVERIFY(sceneLoader.source().isEmpty());
        QVERIFY(static_cast<Qt3DRender::QSceneLoaderPrivate *>(Qt3DCore::QNodePrivate::get(&sceneLoader))->m_subTreeRoot == nullptr);
    }

    void checkSourcePropertyUpdate()
    {
        // GIVEN
        TestArbiter arbiter;
        QScopedPointer<Qt3DRender::QSceneLoader> sceneLoader(new Qt3DRender::QSceneLoader());
        arbiter.setArbiterOnNode(sceneLoader.data());

        // WHEN
        const QUrl sourceUrl = QUrl(QStringLiteral("Milwaukee"));
        sceneLoader->setSource(sourceUrl);
        QCoreApplication::processEvents();

        // THEN
        QCOMPARE(arbiter.dirtyNodes().size(), 1);
        QCOMPARE(arbiter.dirtyNodes().front(), sceneLoader.data());

        arbiter.clear();
    }

    void checkEntities()
    {
        // GIVEN
        const QString e1Name = QStringLiteral("e1");
        const QString e2Name = QStringLiteral("e2");
        const QString e3Name = QStringLiteral("e3");
        const QString e4Name = QStringLiteral("e4");
        Qt3DRender::QSceneLoader loader;
        Qt3DCore::QEntity e1; // Scene container entity, will not be listed among scene entities
        Qt3DCore::QEntity e2(&e1);
        Qt3DCore::QEntity e3(&e1);
        Qt3DCore::QEntity e4(&e3);
        e1.setObjectName(e1Name);
        e2.setObjectName(e2Name);
        e3.setObjectName(e3Name);
        e4.setObjectName(e4Name);
        Qt3DCore::QTransform trans;
        Qt3DRender::QMaterial mat;
        Qt3DRender::QCameraLens cam;
        Qt3DRender::QSpotLight light;
        Qt3DRender::QGeometryRenderer mesh;
        e2.addComponent(&trans);
        e2.addComponent(&cam);
        e3.addComponent(&mat);
        e3.addComponent(&mesh);
        e4.addComponent(&light);

        // WHEN
        static_cast<Qt3DRender::QSceneLoaderPrivate *>(
                    Qt3DCore::QNodePrivate::get(&loader))->populateEntityMap(&e1);

        // THEN
        QStringList entityNames = loader.entityNames();
        entityNames.sort();
        QVERIFY(entityNames.size() == 3);
        QCOMPARE(entityNames.at(0), e2Name);
        QCOMPARE(entityNames.at(1), e3Name);
        QCOMPARE(entityNames.at(2), e4Name);

        QCOMPARE(loader.entity(e1Name), nullptr);
        QCOMPARE(loader.entity(e2Name), &e2);
        QCOMPARE(loader.entity(e3Name), &e3);
        QCOMPARE(loader.entity(e4Name), &e4);

        QCOMPARE(loader.component(e1Name, Qt3DRender::QSceneLoader::UnknownComponent), nullptr);
        QCOMPARE(loader.component(e1Name, Qt3DRender::QSceneLoader::TransformComponent), nullptr);
        QCOMPARE(loader.component(e1Name, Qt3DRender::QSceneLoader::GeometryRendererComponent), nullptr);
        QCOMPARE(loader.component(e1Name, Qt3DRender::QSceneLoader::MaterialComponent), nullptr);
        QCOMPARE(loader.component(e1Name, Qt3DRender::QSceneLoader::LightComponent), nullptr);
        QCOMPARE(loader.component(e1Name, Qt3DRender::QSceneLoader::CameraLensComponent), nullptr);

        QCOMPARE(loader.component(e2Name, Qt3DRender::QSceneLoader::UnknownComponent), nullptr);
        QCOMPARE(loader.component(e2Name, Qt3DRender::QSceneLoader::TransformComponent), &trans);
        QCOMPARE(loader.component(e2Name, Qt3DRender::QSceneLoader::GeometryRendererComponent), nullptr);
        QCOMPARE(loader.component(e2Name, Qt3DRender::QSceneLoader::MaterialComponent), nullptr);
        QCOMPARE(loader.component(e2Name, Qt3DRender::QSceneLoader::LightComponent), nullptr);
        QCOMPARE(loader.component(e2Name, Qt3DRender::QSceneLoader::CameraLensComponent), &cam);

        QCOMPARE(loader.component(e3Name, Qt3DRender::QSceneLoader::UnknownComponent), nullptr);
        QCOMPARE(loader.component(e3Name, Qt3DRender::QSceneLoader::TransformComponent), nullptr);
        QCOMPARE(loader.component(e3Name, Qt3DRender::QSceneLoader::GeometryRendererComponent), &mesh);
        QCOMPARE(loader.component(e3Name, Qt3DRender::QSceneLoader::MaterialComponent), &mat);
        QCOMPARE(loader.component(e3Name, Qt3DRender::QSceneLoader::LightComponent), nullptr);
        QCOMPARE(loader.component(e3Name, Qt3DRender::QSceneLoader::CameraLensComponent), nullptr);

        QCOMPARE(loader.component(e4Name, Qt3DRender::QSceneLoader::UnknownComponent), nullptr);
        QCOMPARE(loader.component(e4Name, Qt3DRender::QSceneLoader::TransformComponent), nullptr);
        QCOMPARE(loader.component(e4Name, Qt3DRender::QSceneLoader::GeometryRendererComponent), nullptr);
        QCOMPARE(loader.component(e4Name, Qt3DRender::QSceneLoader::MaterialComponent), nullptr);
        QCOMPARE(loader.component(e4Name, Qt3DRender::QSceneLoader::LightComponent), &light);
        QCOMPARE(loader.component(e4Name, Qt3DRender::QSceneLoader::CameraLensComponent), nullptr);
    }
};

QTEST_MAIN(tst_QSceneLoader)

#include "tst_qsceneloader.moc"

