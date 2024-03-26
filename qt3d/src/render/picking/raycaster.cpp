// Copyright (C) 2018 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "raycaster_p.h"
#include "qpickevent.h"
#include <Qt3DRender/private/abstractrenderer_p.h>
#include <Qt3DRender/qabstractraycaster.h>
#include <Qt3DRender/qlayer.h>
#include <Qt3DRender/private/qabstractraycaster_p.h>
#include <Qt3DRender/private/raycastingjob_p.h>
#include <Qt3DRender/private/qrenderaspect_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt3DCore;

namespace Qt3DRender {

namespace Render {

RayCaster::RayCaster()
    : BackendNode(QBackendNode::ReadWrite)
{
}

RayCaster::~RayCaster()
{
    notifyJob();
}

QAbstractRayCasterPrivate::RayCasterType RayCaster::type() const
{
    return m_type;
}

QAbstractRayCaster::RunMode RayCaster::runMode() const
{
    return m_runMode;
}

QVector3D RayCaster::origin() const
{
    return m_origin;
}

QVector3D RayCaster::direction() const
{
    return m_direction;
}

float RayCaster::length() const
{
    return m_length;
}

QPoint RayCaster::position() const
{
    return m_position;
}

Qt3DCore::QNodeIdVector RayCaster::layerIds() const
{
    return m_layerIds;
}

QAbstractRayCaster::FilterMode RayCaster::filterMode() const
{
    return m_filterMode;
}

void RayCaster::cleanup()
{
    BackendNode::setEnabled(false);
    m_type = QAbstractRayCasterPrivate::WorldSpaceRayCaster;
    m_runMode = QAbstractRayCaster::SingleShot;
    m_direction = QVector3D(0.f, 0.f, 1.f);
    m_origin = {};
    m_length = 0.f;
    m_position = {};
    m_filterMode = QAbstractRayCaster::AcceptAllMatchingLayers;
    m_layerIds.clear();
    notifyJob();
}

void RayCaster::syncFromFrontEnd(const Qt3DCore::QNode *frontEnd, bool firstTime)
{
    const QAbstractRayCaster *node = qobject_cast<const QAbstractRayCaster *>(frontEnd);
    if (!node)
        return;

    BackendNode::syncFromFrontEnd(frontEnd, firstTime);

    if (node->runMode() != m_runMode) {
        m_runMode = node->runMode();
        notifyJob();
        markDirty(AbstractRenderer::AllDirty);
    }

    if (node->filterMode() != m_filterMode) {
        m_filterMode = node->filterMode();
        notifyJob();
        markDirty(AbstractRenderer::AllDirty);
    }

    const Qt3DCore::QNodeIdVector layerIds = Qt3DCore::qIdsForNodes(node->layers());
    if (layerIds != m_layerIds) {
        m_layerIds = layerIds;
        markDirty(AbstractRenderer::LayersDirty);
        notifyJob();
    }

    const QAbstractRayCasterPrivate *d = static_cast<const QAbstractRayCasterPrivate *>(QNodePrivate::get(node));
    if (d->m_direction != m_direction) {
        m_direction = d->m_direction;
        notifyJob();
        markDirty(AbstractRenderer::AllDirty);
    }

    if (!qFuzzyCompare(d->m_length, m_length)) {
        m_length = d->m_length;
        notifyJob();
        markDirty(AbstractRenderer::AllDirty);
    }

    if (d->m_origin != m_origin) {
        m_origin = d->m_origin;
        notifyJob();
        markDirty(AbstractRenderer::AllDirty);
    }

    if (d->m_position != m_position) {
        m_position = d->m_position;
        notifyJob();
        markDirty(AbstractRenderer::AllDirty);
    }

    if (d->m_rayCasterType != m_type) {
        m_type = d->m_rayCasterType;
        notifyJob();
        markDirty(AbstractRenderer::AllDirty);
    }
}

void RayCaster::notifyJob()
{
    if (m_renderer && m_renderer->aspect())
        QRenderAspectPrivate::get(m_renderer->aspect())->m_rayCastingJob->markCastersDirty();
}

} // Render

} // Qt3DRender

QT_END_NAMESPACE
