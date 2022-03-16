/****************************************************************************
**
** Copyright (C) 2016 Klaralvdalens Datakonsult AB (KDAB).
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt3D module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QTest>
#include <Qt3DCore/qentity.h>
#include <Qt3DCore/private/qnode_p.h>
#include <Qt3DCore/private/qscene_p.h>
#include <Qt3DCore/private/qnodecreatedchangegenerator_p.h>
#include <Qt3DCore/QPropertyUpdatedChange>
#include <Qt3DCore/qtransform.h>

#include <Qt3DRender/qsceneloader.h>
#include <Qt3DRender/qcameralens.h>
#include <Qt3DRender/qmaterial.h>
#include <Qt3DRender/qgeometryrenderer.h>
#include <Qt3DRender/qspotlight.h>
#include <Qt3DRender/private/qsceneloader_p.h>
#include <QSignalSpy>

#include "testpostmanarbiter.h"

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

    void checkCreationData()
    {
        // GIVEN
        Qt3DRender::QSceneLoader sceneLoader;
        const QUrl sceneUrl = QUrl(QStringLiteral("LA ConventionCenter"));
        sceneLoader.setSource(sceneUrl);

        // WHEN
        QVector<Qt3DCore::QNodeCreatedChangeBasePtr> creationChanges;

        {
            Qt3DCore::QNodeCreatedChangeGenerator creationChangeGenerator(&sceneLoader);
            creationChanges = creationChangeGenerator.creationChanges();
        }

        // THEN
        {
            QCOMPARE(creationChanges.size(), 1);

            const auto creationChangeData =
                    qSharedPointerCast<Qt3DCore::QNodeCreatedChange<Qt3DRender::QSceneLoaderData>>(creationChanges.first());
            const Qt3DRender::QSceneLoaderData cloneData = creationChangeData->data;

            QCOMPARE(sceneLoader.id(), creationChangeData->subjectId());
            QCOMPARE(sceneLoader.isEnabled(), true);
            QCOMPARE(sceneLoader.isEnabled(), creationChangeData->isNodeEnabled());
            QCOMPARE(sceneLoader.metaObject(), creationChangeData->metaObject());
            QCOMPARE(sceneLoader.source(), sceneUrl);
            QCOMPARE(sceneLoader.source(), cloneData.source);
        }

        // WHEN
        sceneLoader.setEnabled(false);

        {
            Qt3DCore::QNodeCreatedChangeGenerator creationChangeGenerator(&sceneLoader);
            creationChanges = creationChangeGenerator.creationChanges();
        }

        // THEN
        {
            QCOMPARE(creationChanges.size(), 1);

            const auto creationChangeData =
                    qSharedPointerCast<Qt3DCore::QNodeCreatedChange<Qt3DRender::QSceneLoaderData>>(creationChanges.first());
            const Qt3DRender::QSceneLoaderData cloneData = creationChangeData->data;

            QCOMPARE(sceneLoader.id(), creationChangeData->subjectId());
            QCOMPARE(sceneLoader.isEnabled(), false);
            QCOMPARE(sceneLoader.isEnabled(), creationChangeData->isNodeEnabled());
            QCOMPARE(sceneLoader.metaObject(), creationChangeData->metaObject());
            QCOMPARE(sceneLoader.source(), sceneUrl);
            QCOMPARE(sceneLoader.source(), cloneData.source);
        }
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
        QCOMPARE(arbiter.events.size(), 1);
        Qt3DCore::QPropertyUpdatedChangePtr change = arbiter.events.first().staticCast<Qt3DCore::QPropertyUpdatedChange>();
        QCOMPARE(change->propertyName(), "source");
        QCOMPARE(change->value().value<QUrl>(), sourceUrl);

        arbiter.events.clear();
    }

    void checkStatusPropertyUpdate()
    {
        // GIVEN
        qRegisterMetaType<Qt3DRender::QSceneLoader::Status>("Status");
        TestArbiter arbiter;
        QScopedPointer<Qt3DRender::QSceneLoader> sceneLoader(new Qt3DRender::QSceneLoader());
        arbiter.setArbiterOnNode(sceneLoader.data());
        QSignalSpy spy(sceneLoader.data(), SIGNAL(statusChanged(Status)));


        // WHEN
        const Qt3DRender::QSceneLoader::Status newStatus = Qt3DRender::QSceneLoader::Ready;
        sceneLoader->setStatus(newStatus);

        // THEN
        QVERIFY(arbiter.events.empty());
        QCOMPARE(spy.count(), 1);

        spy.clear();
    }

    void checkPropertyChanges()
    {
        // GIVEN
        Qt3DCore::QScene scene;
        Qt3DCore::QEntity rootEntity;
        QScopedPointer<Qt3DRender::QSceneLoader> sceneLoader(new Qt3DRender::QSceneLoader());
        Qt3DCore::QNodePrivate::get(&rootEntity)->setScene(&scene);
        Qt3DCore::QNodePrivate::get(sceneLoader.data())->setScene(&scene);
        rootEntity.addComponent(sceneLoader.data());

        // WHEN
        Qt3DCore::QEntity backendCreatedSubtree;
        Qt3DCore::QPropertyUpdatedChangePtr valueChange(new Qt3DCore::QPropertyUpdatedChange(Qt3DCore::QNodeId()));
        valueChange->setPropertyName("scene");
        valueChange->setValue(QVariant::fromValue(&backendCreatedSubtree));
        sceneLoader->sceneChangeEvent(valueChange);

        // THEN
        QCOMPARE(static_cast<Qt3DRender::QSceneLoaderPrivate *>(Qt3DCore::QNodePrivate::get(sceneLoader.data()))->m_subTreeRoot, &backendCreatedSubtree);

        // WHEN
        const Qt3DRender::QSceneLoader::Status newStatus = Qt3DRender::QSceneLoader::Ready;
        valueChange = QSharedPointer<Qt3DCore::QPropertyUpdatedChange>::create(Qt3DCore::QNodeId());
        valueChange->setPropertyName("status");
        valueChange->setValue(QVariant::fromValue(newStatus));
        sceneLoader->sceneChangeEvent(valueChange);

        // THEN
        QCOMPARE(sceneLoader->status(), newStatus);
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

