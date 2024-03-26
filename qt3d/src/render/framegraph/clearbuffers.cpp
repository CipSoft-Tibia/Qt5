// Copyright (C) 2014 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "clearbuffers_p.h"
#include <Qt3DRender/private/qclearbuffers_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt3DCore;

namespace Qt3DRender {
namespace Render {

static QVector4D vec4dFromColor(const QColor &color)
{
    if (!color.isValid())
        return QVector4D(0.0f, 0.0f, 0.0f, 1.0f);
    return QVector4D(float(color.redF()), float(color.greenF()), float(color.blueF()), float(color.alphaF()));
}

ClearBuffers::ClearBuffers()
    : FrameGraphNode(FrameGraphNode::ClearBuffers)
    , m_type(QClearBuffers::None)
    , m_clearColorAsColor(Qt::black)
    , m_clearColor(vec4dFromColor(m_clearColorAsColor))
    , m_clearDepthValue(1.f)
    , m_clearStencilValue(0)
{
}

void ClearBuffers::syncFromFrontEnd(const QNode *frontEnd, bool firstTime)
{
    const QClearBuffers *node = qobject_cast<const QClearBuffers *>(frontEnd);
    if (!node)
        return;

    FrameGraphNode::syncFromFrontEnd(frontEnd, firstTime);

    if (m_clearColorAsColor != node->clearColor()) {
        m_clearColorAsColor = node->clearColor();
        m_clearColor = vec4dFromColor(node->clearColor());
        markDirty(AbstractRenderer::FrameGraphDirty);
    }

    if (!qFuzzyCompare(m_clearDepthValue, node->clearDepthValue())) {
        m_clearDepthValue = node->clearDepthValue();
        markDirty(AbstractRenderer::FrameGraphDirty);
    }

    if (m_clearStencilValue != node->clearStencilValue()) {
        m_clearStencilValue = node->clearStencilValue();
        markDirty(AbstractRenderer::FrameGraphDirty);
    }

    const QNodeId colorBufferId = qIdForNode(node->colorBuffer());
    if (m_colorBufferId != colorBufferId) {
        m_colorBufferId = colorBufferId;
        markDirty(AbstractRenderer::FrameGraphDirty);
    }

    if (m_type != node->buffers()) {
        m_type = node->buffers();
        markDirty(AbstractRenderer::FrameGraphDirty);
    }
}

QClearBuffers::BufferType ClearBuffers::type() const
{
    return m_type;
}

QColor ClearBuffers::clearColorAsColor() const
{
    return m_clearColorAsColor;
}

QVector4D ClearBuffers::clearColor() const
{
    return m_clearColor;
}

float ClearBuffers::clearDepthValue() const
{
    return m_clearDepthValue;
}

int ClearBuffers::clearStencilValue() const
{
    return m_clearStencilValue;
}

Qt3DCore::QNodeId ClearBuffers::bufferId() const
{
    return m_colorBufferId;
}

bool ClearBuffers::clearsAllColorBuffers() const
{
    return m_colorBufferId.isNull();
}

} // namespace Render
} // namespace Qt3DRender

QT_END_NAMESPACE
