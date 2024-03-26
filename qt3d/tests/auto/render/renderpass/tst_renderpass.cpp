// Copyright (C) 2015 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QTest>
#include <qbackendnodetester.h>
#include <Qt3DRender/private/renderpass_p.h>

#include <Qt3DRender/QFilterKey>
#include <Qt3DRender/QRenderPass>
#include <Qt3DRender/QShaderProgram>
#include <Qt3DRender/QParameter>

#include <Qt3DRender/QAlphaCoverage>
#include <Qt3DRender/QAlphaTest>
#include <Qt3DRender/QBlendEquation>
#include <Qt3DRender/QBlendEquationArguments>
#include <Qt3DRender/QColorMask>
#include <Qt3DRender/QCullFace>
#include <Qt3DRender/QNoDepthMask>
#include <Qt3DRender/QDepthTest>
#include <Qt3DRender/QDithering>
#include <Qt3DRender/QFrontFace>
#include <Qt3DRender/QPolygonOffset>
#include <Qt3DRender/QScissorTest>
#include <Qt3DRender/QStencilTest>
#include <Qt3DRender/QStencilTestArguments>
#include <Qt3DRender/QStencilMask>
#include <Qt3DRender/QStencilOperation>
#include <Qt3DRender/QStencilOperationArguments>
#include <Qt3DRender/QClipPlane>

#include <Qt3DRender/private/renderstates_p.h>
#include <Qt3DRender/private/managers_p.h>

#include "testrenderer.h"

using namespace Qt3DCore;
using namespace Qt3DRender;
using namespace Qt3DRender::Render;

class tst_RenderRenderPass : public Qt3DCore::QBackendNodeTester
{
    Q_OBJECT

public:
    tst_RenderRenderPass()
        : m_renderStateManager(new RenderStateManager())
    {
        qRegisterMetaType<Qt3DCore::QNode *>();
    }
    ~tst_RenderRenderPass() {}

private slots:
    void shouldHaveInitialState()
    {
        // GIVEN
        RenderPass backend;

        // THEN
        QVERIFY(!backend.isEnabled());
        QVERIFY(backend.shaderProgram().isNull());
        QVERIFY(backend.filterKeys().isEmpty());
        QVERIFY(backend.renderStates().isEmpty());
        QVERIFY(backend.parameters().isEmpty());
    }

    void checkCleanupState()
    {
        // GIVEN
        RenderPass backend;
        TestRenderer renderer;
        backend.setRenderer(&renderer);

        // WHEN
        backend.setEnabled(true);

        {
            QRenderPass frontend;
            QShaderProgram program;
            QBlendEquationArguments state;
            QParameter parameter;
            QFilterKey filterKey;

            frontend.addFilterKey(&filterKey);
            frontend.addParameter(&parameter);
            frontend.addRenderState(&state);
            frontend.setShaderProgram(&program);

            simulateInitializationSync(&frontend, &backend);
        }

        backend.cleanup();

        // THEN
        QVERIFY(!backend.isEnabled());
        QVERIFY(backend.shaderProgram().isNull());
        QVERIFY(backend.filterKeys().isEmpty());
        QVERIFY(backend.renderStates().isEmpty());
        QVERIFY(backend.parameters().isEmpty());
        QVERIFY(!backend.hasRenderStates());
    }

    void shouldHavePropertiesMirroringItsPeer()
    {
        // GIVEN
        QRenderPass frontend;
        frontend.setShaderProgram(new QShaderProgram(&frontend));

        frontend.addFilterKey(new QFilterKey(&frontend));

        frontend.addParameter(new QParameter(&frontend));

        QRenderState *frontendState = new QBlendEquationArguments();
        frontendState->setParent(&frontend);
        frontend.addRenderState(frontendState);

        RenderPass backend;
        TestRenderer renderer;
        backend.setRenderer(&renderer);

        RenderStateNode *backendState = m_renderStateManager->getOrCreateResource(frontendState->id());
        backendState->setRenderer(&renderer);
        simulateInitializationSync(frontendState, backendState);

        // WHEN
        simulateInitializationSync(&frontend, &backend);

        // THEN
        QCOMPARE(backend.shaderProgram(), frontend.shaderProgram()->id());

        QCOMPARE(backend.filterKeys().size(), 1);
        QCOMPARE(backend.filterKeys().first(), frontend.filterKeys().first()->id());

        QCOMPARE(backend.parameters().size(), 1);
        QCOMPARE(backend.parameters().first(), frontend.parameters().first()->id());

        QCOMPARE(backend.renderStates().size(), 1);
        QCOMPARE(backend.renderStates().first(), backendState->peerId());
        QVERIFY(backend.hasRenderStates());
    }

    void shouldHandleShaderPropertyChangeEvents()
    {
        // GIVEN
        QShaderProgram *shader = new QShaderProgram;

        RenderPass backend;
        TestRenderer renderer;
        backend.setRenderer(&renderer);

        QRenderPass frontend;
        simulateInitializationSync(&frontend, &backend);

        // WHEN
        frontend.setShaderProgram(shader);
        backend.syncFromFrontEnd(&frontend, false);

        // THEN
        QCOMPARE(backend.shaderProgram(), shader->id());
        QVERIFY(renderer.dirtyBits() != 0);

        // WHEN
        frontend.setShaderProgram(nullptr);
        backend.syncFromFrontEnd(&frontend, false);

        // THEN
        QVERIFY(backend.shaderProgram().isNull());
    }

    void shouldHandleAnnotationsPropertyChangeEvents()
    {
        // GIVEN
        QFilterKey *annotation = new QFilterKey;

        RenderPass backend;
        TestRenderer renderer;
        backend.setRenderer(&renderer);

        QRenderPass frontend;
        simulateInitializationSync(&frontend, &backend);

        // WHEN
        frontend.addFilterKey(annotation);
        backend.syncFromFrontEnd(&frontend, false);

        // THEN
        QCOMPARE(backend.filterKeys().size(), 1);
        QCOMPARE(backend.filterKeys().first(), annotation->id());
        QVERIFY(renderer.dirtyBits() != 0);

        // WHEN
        frontend.removeFilterKey(annotation);
        backend.syncFromFrontEnd(&frontend, false);

        // THEN
        QVERIFY(backend.filterKeys().isEmpty());
    }

    void shouldHandleParametersPropertyChangeEvents()
    {
        // GIVEN
        QParameter *parameter = new QParameter;

        RenderPass backend;
        TestRenderer renderer;
        backend.setRenderer(&renderer);

        QRenderPass frontend;
        simulateInitializationSync(&frontend, &backend);

        // WHEN
        frontend.addParameter(parameter);
        backend.syncFromFrontEnd(&frontend, false);

        // THEN
        QCOMPARE(backend.parameters().size(), 1);
        QCOMPARE(backend.parameters().first(), parameter->id());
        QVERIFY(renderer.dirtyBits() != 0);

        // WHEN
        frontend.removeParameter(parameter);
        backend.syncFromFrontEnd(&frontend, false);

        // THEN
        QVERIFY(backend.parameters().isEmpty());
    }

    void shouldHandleRenderStatePropertyChangeEvents()
    {
        QRenderState *frontendState = new QBlendEquationArguments();

        RenderPass backend;
        TestRenderer renderer;
        backend.setRenderer(&renderer);

        RenderStateNode *backendState = m_renderStateManager->getOrCreateResource(frontendState->id());
        backendState->setRenderer(&renderer);
        simulateInitializationSync(frontendState, backendState);

        QRenderPass frontend;
        simulateInitializationSync(&frontend, &backend);

        // WHEN
        frontend.addRenderState(frontendState);
        backend.syncFromFrontEnd(&frontend, false);

        // THEN
        QCOMPARE(backend.renderStates().size(), 1);
        QCOMPARE(backend.renderStates().first(), backendState->peerId());
        QVERIFY(renderer.dirtyBits() != 0);

        // WHEN
        frontend.removeRenderState(frontendState);
        backend.syncFromFrontEnd(&frontend, false);

        // THEN
        QVERIFY(backend.renderStates().isEmpty());
    }

private:
    RenderStateManager *m_renderStateManager;
};

QTEST_APPLESS_MAIN(tst_RenderRenderPass)

#include "tst_renderpass.moc"
