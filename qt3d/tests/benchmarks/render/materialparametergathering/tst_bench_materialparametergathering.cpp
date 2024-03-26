// Copyright (C) 2016 Paul Lemire
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QTest>
#include <Qt3DCore/qentity.h>
#include <Qt3DCore/private/qaspectjobmanager_p.h>
#include <Qt3DCore/private/qnodevisitor_p.h>
#include <Qt3DCore/private/qnode_p.h>

#include <Qt3DRender/private/nodemanagers_p.h>
#include <Qt3DRender/private/managers_p.h>
#include <Qt3DRender/private/entity_p.h>
#include <Qt3DRender/qrenderaspect.h>
#include <Qt3DRender/private/qrenderaspect_p.h>
#include <Qt3DRender/private/materialparametergathererjob_p.h>
#include <Qt3DRender/private/technique_p.h>
#include <Qt3DRender/private/techniquemanager_p.h>
#include <Qt3DExtras/qphongmaterial.h>

QT_BEGIN_NAMESPACE

namespace Qt3DRender {

class TestAspect : public Qt3DRender::QRenderAspect
{
public:
    TestAspect(Qt3DCore::QNode *root)
        : Qt3DRender::QRenderAspect(Qt3DRender::QRenderAspect::Manual)
        , m_jobManager(new Qt3DCore::QAspectJobManager())
    {
        Qt3DCore::QAbstractAspectPrivate::get(this)->m_jobManager = m_jobManager.data();
        QRenderAspect::onRegistered();

        QList<Qt3DCore::NodeTreeChange> nodes;
        Qt3DCore::QNodeVisitor v;
        v.traverse(root, [&nodes](Qt3DCore::QNode *node) {
            Qt3DCore::QNodePrivate *d = Qt3DCore::QNodePrivate::get(node);
            d->m_typeInfo = const_cast<QMetaObject*>(Qt3DCore::QNodePrivate::findStaticMetaObject(node->metaObject()));
            d->m_hasBackendNode = true;
            nodes.push_back({
                node->id(),
                Qt3DCore::QNodePrivate::get(node)->m_typeInfo,
                Qt3DCore::NodeTreeChange::Added,
                node
            });
        });

        for (const auto &node: nodes)
            d_func()->createBackendNode(node);

        const auto handles = nodeManagers()->techniqueManager()->activeHandles();
        for (const auto handle: handles) {
            Render::Technique *technique = nodeManagers()->techniqueManager()->data(handle);
            technique->setCompatibleWithRenderer(true);
        }
    }

    ~TestAspect()
    {
        QRenderAspect::onUnregistered();
    }

    Qt3DRender::Render::NodeManagers *nodeManagers() const
    {
        return d_func()->m_renderer->nodeManagers();
    }

    Render::MaterialParameterGathererJobPtr materialGathererJob() const
    {
        Render::MaterialParameterGathererJobPtr job = Render::MaterialParameterGathererJobPtr::create();
        job->setNodeManagers(nodeManagers());
        return job;
    }

    void onRegistered() { QRenderAspect::onRegistered(); }
    void onUnregistered() { QRenderAspect::onUnregistered(); }

private:
    QScopedPointer<Qt3DCore::QAspectJobManager> m_jobManager;
};

} // namespace Qt3DRender

QT_END_NAMESPACE

namespace {

Qt3DCore::QEntity *buildTestScene(int entityCount)
{
    Qt3DCore::QEntity *root = new Qt3DCore::QEntity();

    for (int i = 0; i < entityCount; ++i) {
        Qt3DCore::QEntity *entity = new Qt3DCore::QEntity(root);
        Qt3DExtras::QPhongMaterial *material = new Qt3DExtras::QPhongMaterial();
            entity->addComponent(material);
    }

    return root;
}

} // anonymous

class tst_BenchMaterialParameterGathering : public QObject
{
    Q_OBJECT
private Q_SLOTS:

    void parameterGathering()
    {
        // GIVEN
        QScopedPointer<Qt3DRender::TestAspect> aspect(new Qt3DRender::TestAspect(buildTestScene(2000)));

        // WHEN
        Qt3DRender::Render::MaterialParameterGathererJobPtr gatheringJob = aspect->materialGathererJob();
        gatheringJob->setHandles(aspect->nodeManagers()->materialManager()->activeHandles());

        QBENCHMARK {
            gatheringJob->run();
        }

        QVERIFY(!gatheringJob->materialToPassAndParameter().empty());
    }
};

QTEST_MAIN(tst_BenchMaterialParameterGathering)

#include "tst_bench_materialparametergathering.moc"
