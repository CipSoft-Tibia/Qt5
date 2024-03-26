// Copyright (C) 2016 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QtTest/QTest>
#include <Qt3DCore/private/qaspectjobmanager_p.h>
#include <Qt3DCore/private/qnodevisitor_p.h>
#include <Qt3DCore/private/qnode_p.h>
#include <Qt3DCore/qentity.h>
#include <Qt3DRender/qtechnique.h>
#include <Qt3DRender/qviewport.h>
#include <Qt3DRender/private/technique_p.h>
#include <Qt3DRender/private/nodemanagers_p.h>
#include <Qt3DRender/private/qrenderaspect_p.h>
#include <Qt3DRender/private/techniquemanager_p.h>
#include <Qt3DRender/private/filtercompatibletechniquejob_p.h>
#include <renderer_p.h>
#include <submissioncontext_p.h>

#include "qbackendnodetester.h"

QT_BEGIN_NAMESPACE

namespace Qt3DRender {

class TestAspect : public Qt3DRender::QRenderAspect
{
public:
    TestAspect(Qt3DCore::QNode *root)
        : Qt3DRender::QRenderAspect()
        , m_jobManager(new Qt3DCore::QAspectJobManager())
        , m_window(new QWindow())
        , m_contextCreationSuccessful(false)
    {
        m_window->setSurfaceType(QWindow::OpenGLSurface);
        m_window->setGeometry(0, 0, 10, 10);
        m_window->create();

        if (!m_glContext.create()) {
            qWarning() << "Failed to create OpenGL context";
            return;
        }

        if (!m_glContext.makeCurrent(m_window.data())) {
            qWarning() << "Failed to make OpenGL context current";
            return;
        }

        m_contextCreationSuccessful = true;

        Qt3DCore::QAbstractAspectPrivate::get(this)->m_jobManager = m_jobManager.data();
        QRenderAspect::onRegistered();

        QList<Qt3DCore::QNode *> nodes;
        Qt3DCore::QNodeVisitor v;
        v.traverse(root, [&nodes](Qt3DCore::QNode *node) {
            Qt3DCore::QNodePrivate *d = Qt3DCore::QNodePrivate::get(node);
            d->m_typeInfo = const_cast<QMetaObject*>(Qt3DCore::QNodePrivate::findStaticMetaObject(node->metaObject()));
            d->m_hasBackendNode = true;
            nodes << node;
        });

        for (const auto node: nodes)
            d_func()->createBackendNode({
                                            node->id(),
                                            Qt3DCore::QNodePrivate::get(node)->m_typeInfo,
                                            Qt3DCore::NodeTreeChange::Added,
                                            node
                                        });
    }

    ~TestAspect()
    {
        if (m_contextCreationSuccessful)
            QRenderAspect::onUnregistered();
    }

    Qt3DRender::Render::NodeManagers *nodeManagers() const
    {
        return d_func()->m_renderer
            ? d_func()->m_renderer->nodeManagers() : nullptr;
    }

    bool contextCreationSuccessful() const
    {
        return m_contextCreationSuccessful;
    }

    void initializeRenderer()
    {
        renderer()->setOpenGLContext(&m_glContext);
        d_func()->m_renderer->initialize();
        renderer()->submissionContext()->beginDrawing(m_window.data());
    }

    Render::OpenGL::Renderer *renderer() const
    {
        return static_cast<Render::OpenGL::Renderer *>(d_func()->m_renderer);
    }

    void onRegistered() override { QRenderAspect::onRegistered(); }
    void onUnregistered() override { QRenderAspect::onUnregistered(); }

private:
    QScopedPointer<Qt3DCore::QAspectJobManager> m_jobManager;
    QScopedPointer<QWindow> m_window;
    QOpenGLContext m_glContext;
    bool m_contextCreationSuccessful;
};

} // namespace Qt3DRender

QT_END_NAMESPACE

namespace {

Qt3DCore::QEntity *buildTestScene()
{
    Qt3DCore::QEntity *root = new Qt3DCore::QEntity();

    // FrameGraph
    Qt3DRender::QRenderSettings* renderSettings = new Qt3DRender::QRenderSettings();
    renderSettings->setActiveFrameGraph(new Qt3DRender::QViewport());
    root->addComponent(renderSettings);

    // Scene
    Qt3DRender::QTechnique *gl2Technique = new Qt3DRender::QTechnique(root);
    gl2Technique->graphicsApiFilter()->setApi(Qt3DRender::QGraphicsApiFilter::OpenGL);
    gl2Technique->graphicsApiFilter()->setMajorVersion(2);
    gl2Technique->graphicsApiFilter()->setMinorVersion(0);
    gl2Technique->graphicsApiFilter()->setProfile(Qt3DRender::QGraphicsApiFilter::NoProfile);

    Qt3DRender::QTechnique *gl3Technique = new Qt3DRender::QTechnique(root);
    gl3Technique->graphicsApiFilter()->setApi(Qt3DRender::QGraphicsApiFilter::OpenGL);
    gl3Technique->graphicsApiFilter()->setMajorVersion(3);
    gl3Technique->graphicsApiFilter()->setMinorVersion(2);
    gl3Technique->graphicsApiFilter()->setProfile(Qt3DRender::QGraphicsApiFilter::CoreProfile);

    Qt3DRender::QTechnique *es2Technique = new Qt3DRender::QTechnique(root);
    es2Technique->graphicsApiFilter()->setApi(Qt3DRender::QGraphicsApiFilter::OpenGLES);
    es2Technique->graphicsApiFilter()->setMajorVersion(2);
    es2Technique->graphicsApiFilter()->setMinorVersion(0);
    es2Technique->graphicsApiFilter()->setProfile(Qt3DRender::QGraphicsApiFilter::NoProfile);

    return root;
}

} // anonymous

class tst_FilterCompatibleTechniqueJob : public QObject
{
    Q_OBJECT

private Q_SLOTS:

    void initTestCase()
    {
        QSurfaceFormat format;
#if QT_CONFIG(opengles2)
        format.setRenderableType(QSurfaceFormat::OpenGLES);
#else
        if (QOpenGLContext::openGLModuleType() == QOpenGLContext::LibGL) {
            format.setVersion(4, 3);
            format.setProfile(QSurfaceFormat::CoreProfile);
        }
#endif
        format.setDepthBufferSize(24);
        format.setSamples(4);
        format.setStencilBufferSize(8);
        QSurfaceFormat::setDefaultFormat(format);
    }

    void checkInitialState()
    {
        // GIVEN
        Qt3DRender::Render::FilterCompatibleTechniqueJob backendFilterCompatibleTechniqueJob;

        // THEN
        QVERIFY(backendFilterCompatibleTechniqueJob.manager() == nullptr);
        QVERIFY(backendFilterCompatibleTechniqueJob.renderer() == nullptr);

        // WHEN
        Qt3DRender::Render::TechniqueManager techniqueManager;
        Qt3DRender::Render::OpenGL::Renderer renderer;
        backendFilterCompatibleTechniqueJob.setManager(&techniqueManager);
        backendFilterCompatibleTechniqueJob.setRenderer(&renderer);

        // THEN
        QCOMPARE(backendFilterCompatibleTechniqueJob.manager(), &techniqueManager);
        QCOMPARE(backendFilterCompatibleTechniqueJob.renderer(), &renderer);

        renderer.shutdown();
    }

    void checkRunRendererRunning()
    {
        // GIVEN
        Qt3DRender::Render::FilterCompatibleTechniqueJob backendFilterCompatibleTechniqueJob;
        Qt3DRender::TestAspect testAspect(buildTestScene());

        const bool unableToCreateContext = !testAspect.contextCreationSuccessful();

        if (unableToCreateContext)
            QSKIP("Initialization failed, unable to create GL context");

        // WHEN
        Qt3DRender::Render::NodeManagers *nodeManagers = testAspect.nodeManagers();
        QVERIFY(nodeManagers);
        Qt3DRender::Render::TechniqueManager *techniqueManager = nodeManagers->techniqueManager();
        QVERIFY(techniqueManager);
        backendFilterCompatibleTechniqueJob.setManager(techniqueManager);
        backendFilterCompatibleTechniqueJob.setRenderer(testAspect.renderer());
        testAspect.initializeRenderer();

        // THEN
        QCOMPARE(testAspect.renderer()->isRunning(), true);
        QCOMPARE(testAspect.renderer()->submissionContext()->isInitialized(), true);
        const std::vector<Qt3DRender::Render::HTechnique> &handles = testAspect.nodeManagers()->techniqueManager()->activeHandles();
        QCOMPARE(handles.size(), size_t(3));

        // WHEN
        backendFilterCompatibleTechniqueJob.run();

        // THEN -> empty if job ran properly
        const std::vector<Qt3DCore::QNodeId> dirtyTechniquesId = testAspect.nodeManagers()->techniqueManager()->takeDirtyTechniques();
        QCOMPARE(dirtyTechniquesId.size(), 0U);

        // Check at least one technique is valid
        bool foundValid = false;
        for (const auto &handle: handles) {
            Qt3DRender::Render::Technique *technique = testAspect.nodeManagers()->techniqueManager()->data(handle);
            foundValid |= technique->isCompatibleWithRenderer();
        }
        QCOMPARE(foundValid, true);
    }
};

QTEST_MAIN(tst_FilterCompatibleTechniqueJob)

#include "tst_filtercompatibletechniquejob.moc"
