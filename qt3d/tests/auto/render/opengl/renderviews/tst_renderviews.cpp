// Copyright (C) 2015 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QTest>
#include <qbackendnodetester.h>
#include <private/memorybarrier_p.h>
#include <testarbiter.h>
#include <testrenderer.h>
#include <private/shader_p.h>
#include <Qt3DRender/qshaderprogram.h>
#include <renderview_p.h>
#include <Qt3DRender/private/renderviewjobutils_p.h>
#include <rendercommand_p.h>
#include <renderer_p.h>
#include <glresourcemanagers_p.h>
#include <private/shader_p.h>

QT_BEGIN_NAMESPACE

namespace Qt3DRender {

namespace Render {

namespace OpenGL {

namespace {

void compareShaderParameterPacks(const ShaderParameterPack &t1,
                                 const ShaderParameterPack &t2)
{
    const PackUniformHash hash1 = t1.uniforms();
    const PackUniformHash hash2 = t2.uniforms();

    QCOMPARE(hash1.keys.size(), hash2.keys.size());

    for (int i = 0, m = hash1.keys.size(); i < m; ++i) {
        const int key = hash1.keys.at(i);
        QCOMPARE(hash1.value(key), hash2.value(key));
    }
}

} // anonymous

class tst_RenderViews : public Qt3DCore::QBackendNodeTester
{
    Q_OBJECT

private Q_SLOTS:

    void checkRenderViewSizeFitsWithAllocator()
    {
        QSKIP("Allocated Disabled");
        QVERIFY(sizeof(RenderView) <= 192);
    }

    void checkRenderViewInitialState()
    {
        // GIVEN
        RenderView renderView;

        // THEN
        QCOMPARE(renderView.memoryBarrier(), QMemoryBarrier::None);
    }

    void checkMemoryBarrierInitialization()
    {
        // GIVEN
        RenderView renderView;

        // THEN
        QCOMPARE(renderView.memoryBarrier(), QMemoryBarrier::None);

        // WHEN
        const QMemoryBarrier::Operations barriers(QMemoryBarrier::BufferUpdate|QMemoryBarrier::ShaderImageAccess);
        renderView.setMemoryBarrier(barriers);

        // THEN
        QCOMPARE(renderView.memoryBarrier(), barriers);
    }

    void checkSetRenderViewConfig()
    {
        TestRenderer renderer;
        {
            // GIVEN
            const QMemoryBarrier::Operations barriers(QMemoryBarrier::AtomicCounter|QMemoryBarrier::ShaderStorage);
            Qt3DRender::QMemoryBarrier frontendBarrier;
            FrameGraphManager frameGraphManager;
            MemoryBarrier backendBarrier;
            RenderView renderView;
            // setRenderViewConfigFromFrameGraphLeafNode assumes node has a manager
            backendBarrier.setFrameGraphManager(&frameGraphManager);
            backendBarrier.setRenderer(&renderer);

            // WHEN
            frontendBarrier.setWaitOperations(barriers);
            simulateInitializationSync(&frontendBarrier, &backendBarrier);

            // THEN
            QCOMPARE(renderView.memoryBarrier(), QMemoryBarrier::None);
            QCOMPARE(backendBarrier.waitOperations(), barriers);

            // WHEN
            Qt3DRender::Render::OpenGL::RenderView::setRenderViewConfigFromFrameGraphLeafNode(&renderView, &backendBarrier);

            // THEN
            QCOMPARE(backendBarrier.waitOperations(), renderView.memoryBarrier());
        }
        // TO DO: Complete tests for other framegraph node types
    }

    void checkDoesntCrashWhenNoCommandsToSort()
    {
        // GIVEN
        Qt3DRender::Render::NodeManagers nodeManagers;
        Renderer renderer;
        RenderView renderView;

        renderer.setNodeManagers(&nodeManagers);
        renderView.setRenderer(&renderer);

        // WHEN
        renderView.addSortType(QList<QSortPolicy::SortType> { QSortPolicy::BackToFront });

        EntityRenderCommandDataViewPtr view = EntityRenderCommandDataViewPtr::create();
        renderView.setRenderCommandDataView(view);

        // THEN -> shouldn't crash
        renderView.sort();

        // RenderCommands are deleted by RenderView dtor
        renderer.shutdown();
    }

    void checkRenderCommandBackToFrontSorting()
    {
        // GIVEN
        Qt3DRender::Render::NodeManagers nodeManagers;
        Renderer renderer;
        RenderView renderView;
        std::vector<RenderCommand> rawCommands;

        renderer.setNodeManagers(&nodeManagers);
        renderView.setRenderer(&renderer);

        for (int i = 0; i < 200; ++i) {
            RenderCommand c;
            c.m_depth = float(i);
            rawCommands.push_back(c);
        }

        // WHEN
        renderView.addSortType(QList<QSortPolicy::SortType> { QSortPolicy::BackToFront });

        EntityRenderCommandDataViewPtr view = EntityRenderCommandDataViewPtr::create();
        view->data.commands = rawCommands;
        view->indices.resize(rawCommands.size());
        std::iota(view->indices.begin(), view->indices.end(), 0);

        renderView.setRenderCommandDataView(view);

        renderView.sort();

        // THEN
        int j = 0;
        RenderCommand previousRC;
        renderView.forEachCommand([&] (const RenderCommand &command) {
            if (j > 0)
                QVERIFY(previousRC.m_depth > command.m_depth);
            previousRC = command;
            ++j;
        });

        // RenderCommands are deleted by RenderView dtor
        renderer.shutdown();
    }

    void checkRenderCommandMaterialSorting()
    {
        // GIVEN
        Qt3DRender::Render::NodeManagers nodeManagers;
        Renderer renderer;
        RenderView renderView;
        std::vector<RenderCommand> rawCommands;

        renderer.setNodeManagers(&nodeManagers);
        renderView.setRenderer(&renderer);

        GLShader *dnas[5] = {
            reinterpret_cast<GLShader *>(0x250),
            reinterpret_cast<GLShader *>(0x500),
            reinterpret_cast<GLShader *>(0x1000),
            reinterpret_cast<GLShader *>(0x1500),
            reinterpret_cast<GLShader *>(0x2000)
        };

        for (int i = 0; i < 20; ++i) {
            RenderCommand c;
            c.m_glShader = dnas[i % 5];
            rawCommands.push_back(c);
        }

        // WHEN
        renderView.addSortType(QList<QSortPolicy::SortType> { QSortPolicy::Material });

        EntityRenderCommandDataViewPtr view = EntityRenderCommandDataViewPtr::create();
        view->data.commands = rawCommands;
        view->indices.resize(rawCommands.size());
        std::iota(view->indices.begin(), view->indices.end(), 0);

        renderView.setRenderCommandDataView(view);

        renderView.sort();

        // THEN
        int j = 0;
        GLShader *targetShader;
        RenderCommand previousRC;

        renderView.forEachCommand([&] (const RenderCommand &command) {
            if (j % 4 == 0) {
                targetShader = command.m_glShader;
                if (j > 0)
                    QVERIFY(targetShader != previousRC.m_glShader);
            }
            QCOMPARE(targetShader, command.m_glShader);
            previousRC = command;
            ++j;
        });

        // RenderCommands are deleted by RenderView dtor
        renderer.shutdown();
    }

    void checkRenderViewUniformMinification_data()
    {
        QTest::addColumn<QList<QShaderProgram*>>("shaders");
        QTest::addColumn<QList<ShaderParameterPack>>("rawParameters");
        QTest::addColumn<QList<ShaderParameterPack>>("expectedMinimizedParameters");

        Qt3DCore::QNodeId fakeTextureNodeId = Qt3DCore::QNodeId::createId();

        ShaderParameterPack pack1;
        pack1.setUniform(1, UniformValue(883));
        pack1.setUniform(2, UniformValue(1584.0f));
        pack1.setTexture(3, 0, fakeTextureNodeId);

        QShaderProgram *shader1 = new QShaderProgram();
        QShaderProgram *shader2 = new QShaderProgram();

        shader1->setShaderCode(QShaderProgram::Vertex, QByteArrayLiteral("1"));
        shader2->setShaderCode(QShaderProgram::Vertex, QByteArrayLiteral("2"));

        ShaderParameterPack minifiedPack1;

        QTest::newRow("NoMinification")
                << QList<QShaderProgram *> { shader1, shader2 }
                << QList<ShaderParameterPack> { pack1, pack1 }
                << QList<ShaderParameterPack> { pack1, pack1 };

        QTest::newRow("SingleShaderMinified")
                << QList<QShaderProgram *> { shader1, shader1, shader1 }
                << QList<ShaderParameterPack> { pack1, pack1, pack1 }
                << QList<ShaderParameterPack> { pack1, minifiedPack1, minifiedPack1 };

        QTest::newRow("MultipleShadersMinified")
                << QList<QShaderProgram *> { shader1, shader1, shader1, shader2, shader2, shader2 }
                << QList<ShaderParameterPack> { pack1, pack1, pack1, pack1, pack1, pack1 }
                << QList<ShaderParameterPack> { pack1, minifiedPack1, minifiedPack1, pack1, minifiedPack1, minifiedPack1 };
    }

    void checkShaderParameterPackStoresSingleUBOPerBlockIndex()
    {
        // GIVEN
        ShaderParameterPack pack;
        auto nodeId1 = Qt3DCore::QNodeId::createId();
        auto nodeId2 = Qt3DCore::QNodeId::createId();

        BlockToUBO ubo1 { 1, nodeId1, false, {}};
        BlockToUBO ubo2 { 1, nodeId2, false, {}};

        // WHEN
        pack.setUniformBuffer(ubo1);
        pack.setUniformBuffer(ubo2);

        // THEN
        QCOMPARE(pack.uniformBuffers().size(), 1U);
        QCOMPARE(pack.uniformBuffers().front().m_blockIndex, 1);
        QCOMPARE(pack.uniformBuffers().front().m_bufferID, nodeId2);

        // WHEN
        BlockToUBO ubo3 { 2, nodeId2, false, {}};
        pack.setUniformBuffer(ubo3);

        // THEN
        QCOMPARE(pack.uniformBuffers().size(), 2U);
        QCOMPARE(pack.uniformBuffers().front().m_blockIndex, 1);
        QCOMPARE(pack.uniformBuffers().front().m_bufferID, nodeId2);
        QCOMPARE(pack.uniformBuffers().back().m_blockIndex, 2);
        QCOMPARE(pack.uniformBuffers().back().m_bufferID, nodeId2);
    }

    void checkShaderParameterPackStoresSingleSSBOPerBlockIndex()
    {
        // GIVEN
        ShaderParameterPack pack;
        auto nodeId1 = Qt3DCore::QNodeId::createId();
        auto nodeId2 = Qt3DCore::QNodeId::createId();

        BlockToSSBO ssbo1 { 1, 0, nodeId1};
        BlockToSSBO ssbo2 { 1, 0, nodeId2};

        // WHEN
        pack.setShaderStorageBuffer(ssbo1);
        pack.setShaderStorageBuffer(ssbo2);

        // THEN
        QCOMPARE(pack.shaderStorageBuffers().size(), 1U);
        QCOMPARE(pack.shaderStorageBuffers().front().m_blockIndex, 1);
        QCOMPARE(pack.shaderStorageBuffers().front().m_bufferID, nodeId2);

        // WHEN
        BlockToSSBO ssbo3 { 2, 1, nodeId2};
        pack.setShaderStorageBuffer(ssbo3);

        // THEN
        QCOMPARE(pack.shaderStorageBuffers().size(), 2U);
        QCOMPARE(pack.shaderStorageBuffers().front().m_blockIndex, 1);
        QCOMPARE(pack.shaderStorageBuffers().front().m_bufferID, nodeId2);
        QCOMPARE(pack.shaderStorageBuffers().back().m_blockIndex, 2);
        QCOMPARE(pack.shaderStorageBuffers().back().m_bufferID, nodeId2);
    }

    void checkRenderViewUniformMinification()
    {
        QFETCH(QList<QShaderProgram*>, shaders);
        QFETCH(QList<ShaderParameterPack>, rawParameters);
        QFETCH(QList<ShaderParameterPack>, expectedMinimizedParameters);

        Qt3DRender::Render::NodeManagers nodeManagers;
        Renderer renderer;
        renderer.setNodeManagers(&nodeManagers);

        GLShaderManager *shaderManager = renderer.glResourceManagers()->glShaderManager();
        for (int i = 0, m = shaders.size(); i < m; ++i) {
            Shader* backend = new Shader();
            backend->setRenderer(&renderer);
            simulateInitializationSync(shaders.at(i), backend);
            shaderManager->createOrAdoptExisting(backend);
        }

        RenderView renderView;
        std::vector<RenderCommand> rawCommands;

        renderView.setRenderer(&renderer);

        for (int i = 0, m = shaders.size(); i < m; ++i) {
            RenderCommand c;
            c.m_shaderId = shaders.at(i)->id();
            c.m_glShader = shaderManager->lookupResource(c.m_shaderId);
            c.m_parameterPack = rawParameters.at(i);
            rawCommands.push_back(c);
        }

        // WHEN
        renderView.addSortType(QList<QSortPolicy::SortType> { QSortPolicy::Uniform });

        EntityRenderCommandDataViewPtr view = EntityRenderCommandDataViewPtr::create();
        view->data.commands = rawCommands;
        view->indices.resize(rawCommands.size());
        std::iota(view->indices.begin(), view->indices.end(), 0);

        renderView.setRenderCommandDataView(view);

        renderView.sort();

        // THEN
        int j = 0;
        renderView.forEachCommand([&] (const RenderCommand &command) {
            QCOMPARE(command.m_shaderId, shaders.at(j)->id());
            compareShaderParameterPacks(command.m_parameterPack, expectedMinimizedParameters.at(j));
            ++j;
        });

        renderer.shutdown();
    }


    void checkRenderCommandFrontToBackSorting()
    {
        // GIVEN
        Qt3DRender::Render::NodeManagers nodeManagers;
        Renderer renderer;
        RenderView renderView;
        std::vector<RenderCommand> rawCommands;

        renderer.setNodeManagers(&nodeManagers);
        renderView.setRenderer(&renderer);

        for (int i = 0; i < 200; ++i) {
            RenderCommand c;
            c.m_depth = float(i);
            rawCommands.push_back(c);
        }

        // WHEN
        renderView.addSortType(QList<QSortPolicy::SortType> { QSortPolicy::FrontToBack });

        EntityRenderCommandDataViewPtr view = EntityRenderCommandDataViewPtr::create();
        view->data.commands = rawCommands;
        view->indices.resize(rawCommands.size());
        std::iota(view->indices.begin(), view->indices.end(), 0);

        renderView.setRenderCommandDataView(view);

        renderView.sort();

        // THEN
        int j = 0;
        RenderCommand previousRC;
        renderView.forEachCommand([&] (const RenderCommand &command) {
            if (j > 0)
                QVERIFY(previousRC.m_depth < command.m_depth);
            previousRC = command;
            ++j;
        });

        // RenderCommands are deleted by RenderView dtor
        renderer.shutdown();
    }

    void checkRenderCommandStateCostSorting()
    {
        // GIVEN
        Qt3DRender::Render::NodeManagers nodeManagers;
        Renderer renderer;
        RenderView renderView;
        std::vector<RenderCommand> rawCommands;

        renderer.setNodeManagers(&nodeManagers);
        renderView.setRenderer(&renderer);

        for (int i = 0; i < 200; ++i) {
            RenderCommand c;
            c.m_changeCost = i;
            rawCommands.push_back(c);
        }

        // WHEN
        renderView.addSortType(QList<QSortPolicy::SortType> { QSortPolicy::StateChangeCost });

        EntityRenderCommandDataViewPtr view = EntityRenderCommandDataViewPtr::create();
        view->data.commands = rawCommands;
        view->indices.resize(rawCommands.size());
        std::iota(view->indices.begin(), view->indices.end(), 0);

        renderView.setRenderCommandDataView(view);

        renderView.sort();

        // THEN
        int j = 0;
        RenderCommand previousRC;
        renderView.forEachCommand([&] (const RenderCommand &command) {
            if (j > 0)
                QVERIFY(previousRC.m_changeCost > command.m_changeCost);
            previousRC = command;
            ++j;
        });


        // RenderCommands are deleted by RenderView dtor
        renderer.shutdown();
    }

    void checkRenderCommandCombinedStateMaterialDepthSorting()
    {
        // GIVEN
        Qt3DRender::Render::NodeManagers nodeManagers;
        Renderer renderer;
        RenderView renderView;
        std::vector<RenderCommand> rawCommands;

        renderer.setNodeManagers(&nodeManagers);
        renderView.setRenderer(&renderer);

        GLShader *dna[5] = {
            reinterpret_cast<GLShader *>(0x250),
            reinterpret_cast<GLShader *>(0x500),
            reinterpret_cast<GLShader *>(0x1000),
            reinterpret_cast<GLShader *>(0x1500),
            reinterpret_cast<GLShader *>(0x2000)
        };

        float depth[3] = {
            10.0f,
            25.0f,
            30.0f
        };

        int stateChangeCost[2] = {
            100,
            200
        };

        auto buildRC = [] (GLShader *dna, float depth, int changeCost) {
            RenderCommand c;
            c.m_glShader = dna;
            c.m_depth = depth;
            c.m_changeCost = changeCost;
            return c;
        };

        RenderCommand c5 = buildRC(dna[3], depth[1], stateChangeCost[1]);
        RenderCommand c3 = buildRC(dna[3], depth[0], stateChangeCost[1]);
        RenderCommand c4 = buildRC(dna[2], depth[1], stateChangeCost[1]);
        RenderCommand c8 = buildRC(dna[1], depth[1], stateChangeCost[1]);
        RenderCommand c0 = buildRC(dna[0], depth[2], stateChangeCost[1]);

        RenderCommand c2 = buildRC(dna[2], depth[2], stateChangeCost[0]);
        RenderCommand c9 = buildRC(dna[2], depth[0], stateChangeCost[0]);
        RenderCommand c1 = buildRC(dna[1], depth[0], stateChangeCost[0]);
        RenderCommand c7 = buildRC(dna[0], depth[2], stateChangeCost[0]);
        RenderCommand c6 = buildRC(dna[0], depth[1], stateChangeCost[0]);

        rawCommands = { c0, c1, c2, c3, c4, c5, c6, c7, c8, c9 };

        // WHEN
        renderView.addSortType(QList<QSortPolicy::SortType>
                               { QSortPolicy::StateChangeCost,
                                 QSortPolicy::Material,
                                 QSortPolicy::BackToFront });

        EntityRenderCommandDataViewPtr view = EntityRenderCommandDataViewPtr::create();
        view->data.commands = rawCommands;
        view->indices.resize(rawCommands.size());
        std::iota(view->indices.begin(), view->indices.end(), 0);

        renderView.setRenderCommandDataView(view);


        renderView.sort();

        // THEN
        const std::vector<RenderCommand> &sortedCommands = view->data.commands;
        const std::vector<size_t> &sortedCommandIndices = view->indices;
        QCOMPARE(rawCommands.size(), sortedCommands.size());

        // Ordered by higher state, higher shaderDNA and higher depth
        QCOMPARE(c0, sortedCommands.at(sortedCommandIndices[4]));
        QCOMPARE(c3, sortedCommands.at(sortedCommandIndices[1]));
        QCOMPARE(c4, sortedCommands.at(sortedCommandIndices[2]));
        QCOMPARE(c5, sortedCommands.at(sortedCommandIndices[0]));
        QCOMPARE(c8, sortedCommands.at(sortedCommandIndices[3]));

        QCOMPARE(c1, sortedCommands.at(sortedCommandIndices[7]));
        QCOMPARE(c2, sortedCommands.at(sortedCommandIndices[5]));
        QCOMPARE(c6, sortedCommands.at(sortedCommandIndices[9]));
        QCOMPARE(c7, sortedCommands.at(sortedCommandIndices[8]));
        QCOMPARE(c9, sortedCommands.at(sortedCommandIndices[6]));

        // RenderCommands are deleted by RenderView dtor
        renderer.shutdown();
    }

    void checkRenderCommandTextureSorting()
    {
        // GIVEN
        RenderView renderView;

        Qt3DCore::QNodeId tex1 = Qt3DCore::QNodeId::createId();
        Qt3DCore::QNodeId tex2 = Qt3DCore::QNodeId::createId();
        Qt3DCore::QNodeId tex3 = Qt3DCore::QNodeId::createId();
        Qt3DCore::QNodeId tex4 = Qt3DCore::QNodeId::createId();

        RenderCommand a;
        {
            ShaderParameterPack pack;
            pack.setTexture(0, 0, tex1);
            pack.setTexture(1, 0, tex3);
            pack.setTexture(2, 0, tex4);
            pack.setTexture(3, 0, tex2);
            a.m_parameterPack = pack;
        }
        RenderCommand b;
        RenderCommand c;
        {
            ShaderParameterPack pack;
            pack.setTexture(0, 0, tex1);
            pack.setTexture(3, 0, tex2);
            c.m_parameterPack = pack;
        }
        RenderCommand d;
        {
            ShaderParameterPack pack;
            pack.setTexture(1, 0, tex3);
            pack.setTexture(2, 0, tex4);
            d.m_parameterPack = pack;
        }
        RenderCommand e;
        {
            ShaderParameterPack pack;
            pack.setTexture(3, 0, tex2);
            e.m_parameterPack = pack;
        }
        RenderCommand f;
        {
            ShaderParameterPack pack;
            pack.setTexture(3, 0, tex2);
            f.m_parameterPack = pack;
        }
        RenderCommand g;
        {
            ShaderParameterPack pack;
            pack.setTexture(0, 0, tex1);
            pack.setTexture(1, 0, tex3);
            pack.setTexture(2, 0, tex4);
            pack.setTexture(3, 0, tex2);
            g.m_parameterPack = pack;
        }

        // WHEN
        std::vector<RenderCommand> rawCommands = {a, b, c, d, e, f, g};
        renderView.addSortType(QList<QSortPolicy::SortType> { QSortPolicy::Texture });

        EntityRenderCommandDataViewPtr view = EntityRenderCommandDataViewPtr::create();
        view->data.commands = rawCommands;
        view->indices.resize(rawCommands.size());
        std::iota(view->indices.begin(), view->indices.end(), 0);

        renderView.setRenderCommandDataView(view);

        renderView.sort();

        // THEN
        const std::vector<RenderCommand> &sortedCommands = view->data.commands;
        const std::vector<size_t> &sortedCommandIndices = view->indices;
        QCOMPARE(rawCommands.size(), sortedCommands.size());
        QCOMPARE(sortedCommands.at(sortedCommandIndices[0]), a);
        QCOMPARE(sortedCommands.at(sortedCommandIndices[1]), g);
        QCOMPARE(sortedCommands.at(sortedCommandIndices[2]), d);
        QCOMPARE(sortedCommands.at(sortedCommandIndices[3]), c);
        QCOMPARE(sortedCommands.at(sortedCommandIndices[4]), e);
        QCOMPARE(sortedCommands.at(sortedCommandIndices[5]), f);
        QCOMPARE(sortedCommands.at(sortedCommandIndices[6]), b);
        // RenderCommands are deleted by RenderView dtor
    }
private:
};

} // OpenGL

} // Render

} // Qt3DRender

QT_END_NAMESPACE

//APPLESS_
QTEST_MAIN(Qt3DRender::Render::OpenGL::tst_RenderViews)

#include "tst_renderviews.moc"
