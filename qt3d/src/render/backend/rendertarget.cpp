/****************************************************************************
**
** Copyright (C) 2014 Klaralvdalens Datakonsult AB (KDAB).
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt3D module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <Qt3DRender/private/rendertarget_p.h>
#include <Qt3DRender/qrendertarget.h>
#include <Qt3DRender/private/qrendertarget_p.h>
#include <Qt3DRender/qrendertargetoutput.h>
#include <Qt3DRender/private/managers_p.h>
#include <QVariant>

QT_BEGIN_NAMESPACE

using namespace Qt3DCore;

namespace Qt3DRender {
namespace Render {

RenderTarget::RenderTarget()
    : BackendNode()
{
}

void RenderTarget::syncFromFrontEnd(const Qt3DCore::QNode *frontEnd, bool firstTime)
{
    const QRenderTarget *node = qobject_cast<const QRenderTarget *>(frontEnd);
    if (!node)
        return;

    BackendNode::syncFromFrontEnd(frontEnd, firstTime);

    auto outputIds = qIdsForNodes(node->outputs());
    std::sort(std::begin(outputIds), std::end(outputIds));

    if (m_renderOutputs != outputIds) {
        m_renderOutputs = outputIds;
        markDirty(AbstractRenderer::AllDirty);
    }
}

void RenderTarget::cleanup()
{
    m_renderOutputs.clear();
    QBackendNode::setEnabled(false);
}

void RenderTarget::appendRenderOutput(QNodeId outputId)
{
    if (!m_renderOutputs.contains(outputId))
        m_renderOutputs.append(outputId);
}

void RenderTarget::removeRenderOutput(QNodeId outputId)
{
    m_renderOutputs.removeOne(outputId);
}

QVector<Qt3DCore::QNodeId> RenderTarget::renderOutputs() const
{
    return m_renderOutputs;
}

RenderTargetFunctor::RenderTargetFunctor(AbstractRenderer *renderer, RenderTargetManager *manager)
    : m_renderer(renderer)
    , m_renderTargetManager(manager)
{
}

QBackendNode *RenderTargetFunctor::create(const QNodeCreatedChangeBasePtr &change) const
{
    RenderTarget *backend = m_renderTargetManager->getOrCreateResource(change->subjectId());
    // Remove from the list of ids to destroy in case we were added to it
    m_renderTargetManager->removeRenderTargetToCleanup(change->subjectId());
    backend->setRenderer(m_renderer);
    return backend;
}

QBackendNode *RenderTargetFunctor::get(QNodeId id) const
{
    return m_renderTargetManager->lookupResource(id);
}

void RenderTargetFunctor::destroy(QNodeId id) const
{
    // We add ourselves to the dirty list to tell the renderer that this rendertarget has been destroyed
    m_renderTargetManager->addRenderTargetIdToCleanup(id);
    m_renderTargetManager->releaseResource(id);
}

} // namespace Render
} // namespace Qt3DRender

QT_END_NAMESPACE
