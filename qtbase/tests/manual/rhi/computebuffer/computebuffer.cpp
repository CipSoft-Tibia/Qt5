// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "../shared/examplefw.h"
#include <QRandomGenerator>

// Compute shader example. Writes to a storage buffer from a compute shader,
// then uses the same buffer as vertex buffer in the vertex stage. This would
// be typical when implementing particles for example. Here we just simply move
// the positions back and forth along the X axis.

// Note that the example relies on gl_PointSize which is not supported
// everywhere. So in some cases the points will be of size 1.

struct {
    QList<QRhiResource *> releasePool;
    QRhiBuffer *sbuf = nullptr;
    QRhiBuffer *computeUniBuf = nullptr;
    QRhiShaderResourceBindings *computeBindings = nullptr;
    QRhiComputePipeline *computePipeline = nullptr;
    QRhiShaderResourceBindings *graphicsBindings = nullptr;
    QRhiGraphicsPipeline *graphicsPipeline = nullptr;
    QRhiResourceUpdateBatch *initialUpdates = nullptr;
    float step = 0.2f;
} d;

// these struct must match the std140 packing rules
struct Data {
    float pos[2];
    float dir;
    quint32 pad[1];
};
struct ComputeUBuf {
    float step;
    quint32 count;
};

const int DATA_COUNT = 256 * 128;

const int COMPUTE_UBUF_SIZE = 8;

void Window::customInit()
{
    if (!m_r->isFeatureSupported(QRhi::Compute))
        qFatal("Compute is not supported");

    if (!m_r->isFeatureSupported(QRhi::VertexShaderPointSize))
        qWarning("Point sizes other than 1 not supported");

    // compute pass

    d.sbuf = m_r->newBuffer(QRhiBuffer::Immutable,
                            QRhiBuffer::StorageBuffer | QRhiBuffer::VertexBuffer,
                            sizeof(Data) * DATA_COUNT);
    d.sbuf->create();
    d.releasePool << d.sbuf;

    d.computeUniBuf = m_r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, COMPUTE_UBUF_SIZE);
    d.computeUniBuf->create();
    d.releasePool << d.computeUniBuf;

    d.initialUpdates = m_r->nextResourceUpdateBatch();

    QByteArray data;
    data.resize(sizeof(Data) * DATA_COUNT);
    Data *p = reinterpret_cast<Data *>(data.data());
    QRandomGenerator *rgen = QRandomGenerator::global();
    for (int i = 0; i < DATA_COUNT; ++i) {
        p->pos[0] = rgen->bounded(1000) / 500.0f - 1.0f;
        p->pos[1] = rgen->bounded(1000) / 500.0f - 1.0f;
        p->dir = rgen->bounded(2) ? 1 : -1;
        ++p;
    }
    d.initialUpdates->uploadStaticBuffer(d.sbuf, data.constData());

    ComputeUBuf ud;
    ud.step = d.step;
    ud.count = DATA_COUNT;
    d.initialUpdates->updateDynamicBuffer(d.computeUniBuf, 0, COMPUTE_UBUF_SIZE, &ud);

    d.computeBindings = m_r->newShaderResourceBindings();
    d.computeBindings->setBindings({
                                       QRhiShaderResourceBinding::bufferLoadStore(0, QRhiShaderResourceBinding::ComputeStage, d.sbuf),
                                       QRhiShaderResourceBinding::uniformBuffer(1, QRhiShaderResourceBinding::ComputeStage, d.computeUniBuf)
                                   });
    d.computeBindings->create();
    d.releasePool << d.computeBindings;

    d.computePipeline = m_r->newComputePipeline();
    d.computePipeline->setShaderResourceBindings(d.computeBindings);
    d.computePipeline->setShaderStage({ QRhiShaderStage::Compute, getShader(QLatin1String(":/buffer.comp.qsb")) });
    d.computePipeline->create();
    d.releasePool << d.computePipeline;

    // graphics pass

    d.graphicsBindings = m_r->newShaderResourceBindings();
    d.graphicsBindings->create();
    d.releasePool << d.graphicsBindings;

    d.graphicsPipeline = m_r->newGraphicsPipeline();
    d.graphicsPipeline->setTopology(QRhiGraphicsPipeline::Points);
    d.graphicsPipeline->setShaderStages({
        { QRhiShaderStage::Vertex, getShader(QLatin1String(":/main.vert.qsb")) },
        { QRhiShaderStage::Fragment, getShader(QLatin1String(":/main.frag.qsb")) }
    });
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({
        { 2 * sizeof(float) }
    });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float2, 0 },
    });
    d.graphicsPipeline->setVertexInputLayout(inputLayout);
    d.graphicsPipeline->setShaderResourceBindings(d.graphicsBindings);
    d.graphicsPipeline->setRenderPassDescriptor(m_rp);
    d.graphicsPipeline->create();
    d.releasePool << d.graphicsPipeline;
}

void Window::customRelease()
{
    qDeleteAll(d.releasePool);
    d.releasePool.clear();
}

void Window::customRender()
{
    const QSize outputSizeInPixels = m_sc->currentPixelSize();
    QRhiCommandBuffer *cb = m_sc->currentFrameCommandBuffer();
    QRhiResourceUpdateBatch *u = m_r->nextResourceUpdateBatch();
    if (d.initialUpdates) {
        u->merge(d.initialUpdates);
        d.initialUpdates->release();
        d.initialUpdates = nullptr;
    }

#if 0
    u->updateDynamicBuffer(d.computeUniBuf, 0, sizeof(float), &d.step);
    d.step += 0.01f;
#endif

    // compute pass
    cb->beginComputePass(u);
    cb->setComputePipeline(d.computePipeline);
    cb->setShaderResources();
    cb->dispatch(DATA_COUNT / 256, 1, 1);
    cb->endComputePass();

    // graphics pass
    cb->beginPass(m_sc->currentFrameRenderTarget(), m_clearColor, { 1.0f, 0 });
    cb->setGraphicsPipeline(d.graphicsPipeline);
    cb->setViewport({ 0, 0, float(outputSizeInPixels.width()), float(outputSizeInPixels.height()) });
    QRhiCommandBuffer::VertexInput vbufBinding(d.sbuf, 0);
    cb->setVertexInput(0, 1, &vbufBinding);
    cb->draw(DATA_COUNT);
    cb->endPass();
}
