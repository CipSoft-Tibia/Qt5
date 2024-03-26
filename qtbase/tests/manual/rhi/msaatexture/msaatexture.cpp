// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "../shared/examplefw.h"

// Renders into a non-multisample and a multisample (4x) texture, and then uses
// those textures to draw two quads. Note that this uses an MSAA sampler in the
// shader, not resolves. Not supported on the GL(ES) backend atm.

static float vertexData[] =
{ // Y up, CCW
  -0.5f,   0.5f,   0.0f, 0.0f,
  -0.5f,  -0.5f,   0.0f, 1.0f,
  0.5f,   -0.5f,   1.0f, 1.0f,
  0.5f,   0.5f,    1.0f, 0.0f
};

static quint16 indexData[] =
{
    0, 1, 2, 0, 2, 3
};

static float triangleData[] =
{ // Y up, CCW
     0.0f,   0.5f,   1.0f, 0.0f, 0.0f,
    -0.5f,  -0.5f,   0.0f, 1.0f, 0.0f,
     0.5f,  -0.5f,   0.0f, 0.0f, 1.0f,
};

const int UBUFSZ = 68;

struct {
    QList<QRhiResource *> releasePool;
    QRhiBuffer *vbuf = nullptr;
    QRhiBuffer *ibuf = nullptr;
    QRhiBuffer *ubuf = nullptr;
    QRhiTexture *tex = nullptr;
    QRhiTexture *msaaTex = nullptr;
    QRhiSampler *sampler = nullptr;
    QRhiShaderResourceBindings *srbLeft = nullptr;
    QRhiShaderResourceBindings *srbRight = nullptr;
    QRhiGraphicsPipeline *psLeft = nullptr;
    QRhiGraphicsPipeline *psRight = nullptr;
    QRhiResourceUpdateBatch *initialUpdates = nullptr;
    int rightOfs;
    QMatrix4x4 winProj;
    QMatrix4x4 triBaseMvp;
    float triRot = 0;

    QRhiShaderResourceBindings *triSrb = nullptr;
    QRhiGraphicsPipeline *msaaTriPs = nullptr;
    QRhiGraphicsPipeline *triPs = nullptr;
    QRhiBuffer *triUbuf = nullptr;
    QRhiTextureRenderTarget *msaaRt = nullptr;
    QRhiRenderPassDescriptor *msaaRtRp = nullptr;
    QRhiTextureRenderTarget *rt = nullptr;
    QRhiRenderPassDescriptor *rtRp = nullptr;
} d;

//#define NO_MSAA

void Window::customInit()
{
    if (!m_r->isFeatureSupported(QRhi::MultisampleTexture))
        qFatal("Multisample textures not supported by this backend");

    d.vbuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(vertexData) + sizeof(triangleData));
    d.releasePool << d.vbuf;
    d.vbuf->create();

    d.ibuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, sizeof(indexData));
    d.releasePool << d.ibuf;
    d.ibuf->create();

    d.rightOfs = m_r->ubufAligned(UBUFSZ);
    d.ubuf = m_r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, d.rightOfs + UBUFSZ);
    d.releasePool << d.ubuf;
    d.ubuf->create();

    d.tex = m_r->newTexture(QRhiTexture::RGBA8, QSize(512, 512), 1, QRhiTexture::RenderTarget);
    d.releasePool << d.tex;
    d.tex->create();
#ifndef NO_MSAA
    d.msaaTex = m_r->newTexture(QRhiTexture::RGBA8, QSize(512, 512), 4, QRhiTexture::RenderTarget);
#else
    d.msaaTex = m_r->newTexture(QRhiTexture::RGBA8, QSize(512, 512), 1, QRhiTexture::RenderTarget);
#endif
    d.releasePool << d.msaaTex;
    d.msaaTex->create();

    d.initialUpdates = m_r->nextResourceUpdateBatch();
    d.initialUpdates->uploadStaticBuffer(d.vbuf, 0, sizeof(vertexData), vertexData);
    d.initialUpdates->uploadStaticBuffer(d.vbuf, sizeof(vertexData), sizeof(triangleData), triangleData);
    d.initialUpdates->uploadStaticBuffer(d.ibuf, indexData);

    d.sampler = m_r->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge);
    d.releasePool << d.sampler;
    d.sampler->create();

    d.srbLeft = m_r->newShaderResourceBindings();
    d.releasePool << d.srbLeft;
    d.srbLeft->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, d.ubuf, 0, UBUFSZ),
        QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, d.tex, d.sampler)
    });
    d.srbLeft->create();

    d.srbRight = m_r->newShaderResourceBindings();
    d.releasePool << d.srbRight;
    d.srbRight->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, d.ubuf, d.rightOfs, UBUFSZ),
        QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, d.msaaTex, d.sampler)
    });
    d.srbRight->create();

    d.psLeft = m_r->newGraphicsPipeline();
    d.releasePool << d.psLeft;
    d.psLeft->setShaderStages({
        { QRhiShaderStage::Vertex, getShader(QLatin1String(":/texture.vert.qsb")) },
        { QRhiShaderStage::Fragment, getShader(QLatin1String(":/texture.frag.qsb")) }
    });
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({ { 4 * sizeof(float) } });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float2, 0 },
        { 0, 1, QRhiVertexInputAttribute::Float2, quint32(2 * sizeof(float)) }
    });
    d.psLeft->setVertexInputLayout(inputLayout);
    d.psLeft->setShaderResourceBindings(d.srbLeft);
    d.psLeft->setRenderPassDescriptor(m_rp);
    d.psLeft->create();

    d.psRight = m_r->newGraphicsPipeline();
    d.releasePool << d.psRight;
    d.psRight->setShaderStages({
        { QRhiShaderStage::Vertex, getShader(QLatin1String(":/texture.vert.qsb")) },
#ifndef NO_MSAA
        { QRhiShaderStage::Fragment, getShader(QLatin1String(":/texture_ms4.frag.qsb")) }
#else
        { QRhiShaderStage::Fragment, getShader(QLatin1String(":/texture.frag.qsb")) }
#endif
    });
    d.psRight->setVertexInputLayout(d.psLeft->vertexInputLayout());
    d.psRight->setShaderResourceBindings(d.srbRight);
    d.psRight->setRenderPassDescriptor(m_rp);
    d.psRight->create();

    // set up the offscreen triangle that goes into tex and msaaTex
    d.triUbuf = m_r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 68);
    d.releasePool << d.triUbuf;
    d.triUbuf->create();
    d.triSrb = m_r->newShaderResourceBindings();
    d.releasePool << d.triSrb;
    d.triSrb->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, d.triUbuf)
    });
    d.triSrb->create();
    d.rt = m_r->newTextureRenderTarget({ d.tex });
    d.releasePool << d.rt;
    d.rtRp = d.rt->newCompatibleRenderPassDescriptor();
    d.releasePool << d.rtRp;
    d.rt->setRenderPassDescriptor(d.rtRp);
    d.rt->create();
    d.msaaRt = m_r->newTextureRenderTarget({ d.msaaTex });
    d.releasePool << d.msaaRt;
    d.msaaRtRp = d.msaaRt->newCompatibleRenderPassDescriptor();
    d.releasePool << d.msaaRtRp;
    d.msaaRt->setRenderPassDescriptor(d.msaaRtRp);
    d.msaaRt->create();
    d.triPs = m_r->newGraphicsPipeline();
    d.releasePool << d.triPs;
    d.triPs->setSampleCount(1);
    d.triPs->setShaderStages({
        { QRhiShaderStage::Vertex, getShader(QLatin1String(":/color.vert.qsb")) },
        { QRhiShaderStage::Fragment, getShader(QLatin1String(":/color.frag.qsb")) }
    });
    inputLayout.setBindings({
        { 5 * sizeof(float) }
    });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float2, 0 },
        { 0, 1, QRhiVertexInputAttribute::Float3, quint32(2 * sizeof(float)) }
    });
    d.triPs->setVertexInputLayout(inputLayout);
    d.triPs->setShaderResourceBindings(d.triSrb);
    d.triPs->setRenderPassDescriptor(d.rtRp);
    d.triPs->create();
    d.msaaTriPs = m_r->newGraphicsPipeline();
    d.releasePool << d.msaaTriPs;
#ifndef NO_MSAA
    d.msaaTriPs->setSampleCount(4);
#else
    d.msaaTriPs->setSampleCount(1);
#endif
    d.msaaTriPs->setShaderStages(d.triPs->cbeginShaderStages(), d.triPs->cendShaderStages());
    d.msaaTriPs->setVertexInputLayout(d.triPs->vertexInputLayout());
    d.msaaTriPs->setShaderResourceBindings(d.triSrb);
    d.msaaTriPs->setRenderPassDescriptor(d.msaaRtRp);
    d.msaaTriPs->create();
}

void Window::customRelease()
{
    qDeleteAll(d.releasePool);
    d.releasePool.clear();
}

void Window::customRender()
{
    QRhiCommandBuffer *cb = m_sc->currentFrameCommandBuffer();
    QRhiResourceUpdateBatch *u = m_r->nextResourceUpdateBatch();
    if (d.initialUpdates) {
        u->merge(d.initialUpdates);
        d.initialUpdates->release();
        d.initialUpdates = nullptr;

        // onscreen ubuf
        qint32 flip = m_r->isYUpInFramebuffer() ? 1 : 0;
        u->updateDynamicBuffer(d.ubuf, 64, 4, &flip);
        u->updateDynamicBuffer(d.ubuf, d.rightOfs + 64, 4, &flip);

        // offscreen ubuf
        d.triBaseMvp = m_r->clipSpaceCorrMatrix();
        d.triBaseMvp.perspective(45.0f, d.msaaTex->pixelSize().width() / float(d.msaaTex->pixelSize().height()), 0.01f, 1000.0f);
        d.triBaseMvp.translate(0, 0, -2);
        float opacity = 1.0f;
        u->updateDynamicBuffer(d.triUbuf, 64, 4, &opacity);
    }

    if (d.winProj != m_proj) {
        // onscreen buf, window size dependent
        d.winProj = m_proj;
        QMatrix4x4 mvp = m_proj;
        mvp.scale(2);
        mvp.translate(-0.8f, 0, 0);
        u->updateDynamicBuffer(d.ubuf, 0, 64, mvp.constData());
        mvp.translate(1.6f, 0, 0);
        u->updateDynamicBuffer(d.ubuf, d.rightOfs, 64, mvp.constData());
    }

    // offscreen buf, apply the rotation on every frame
    QMatrix4x4 triMvp = d.triBaseMvp;
    triMvp.rotate(d.triRot, 0, 1, 0);
    d.triRot += 1;
    u->updateDynamicBuffer(d.triUbuf, 0, 64, triMvp.constData());

    cb->resourceUpdate(u); // could have passed u to beginPass but exercise this one too

    // offscreen
    cb->beginPass(d.rt, QColor::fromRgbF(0.5f, 0.2f, 0.0f, 1.0f), { 1.0f, 0 });
    cb->setGraphicsPipeline(d.triPs);
    cb->setViewport({ 0, 0, float(d.msaaTex->pixelSize().width()), float(d.msaaTex->pixelSize().height()) });
    cb->setShaderResources();
    QRhiCommandBuffer::VertexInput vbufBinding(d.vbuf, quint32(sizeof(vertexData)));
    cb->setVertexInput(0, 1, &vbufBinding);
    cb->draw(3);
    cb->endPass();

    // offscreen msaa
    cb->beginPass(d.msaaRt, QColor::fromRgbF(0.5f, 0.2f, 0.0f, 1.0f), { 1.0f, 0 });
    cb->setGraphicsPipeline(d.msaaTriPs);
    cb->setViewport({ 0, 0, float(d.msaaTex->pixelSize().width()), float(d.msaaTex->pixelSize().height()) });
    cb->setShaderResources();
    cb->setVertexInput(0, 1, &vbufBinding);
    cb->draw(3);
    cb->endPass();

    // onscreen
    const QSize outputSizeInPixels = m_sc->currentPixelSize();
    cb->beginPass(m_sc->currentFrameRenderTarget(), m_clearColor, { 1.0f, 0 });
    cb->setGraphicsPipeline(d.psLeft); // showing the non-msaa version
    cb->setViewport({ 0, 0, float(outputSizeInPixels.width()), float(outputSizeInPixels.height()) });
    cb->setShaderResources();
    vbufBinding.second = 0;
    cb->setVertexInput(0, 1, &vbufBinding, d.ibuf, 0, QRhiCommandBuffer::IndexUInt16);
    cb->drawIndexed(6);
    cb->setGraphicsPipeline(d.psRight); // showing the msaa version, resolved in the shader
    cb->setShaderResources();
    cb->drawIndexed(6);
    cb->endPass();
}
