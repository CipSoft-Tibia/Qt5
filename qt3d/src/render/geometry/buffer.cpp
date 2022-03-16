/****************************************************************************
**
** Copyright (C) 2015 Klaralvdalens Datakonsult AB (KDAB).
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

#include "buffer_p.h"
#include <Qt3DCore/qpropertyupdatedchange.h>
#include <Qt3DRender/private/buffermanager_p.h>
#include <Qt3DRender/private/qbuffer_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt3DCore;

namespace Qt3DRender {
namespace Render {

Buffer::Buffer()
    : BackendNode(QBackendNode::ReadWrite)
    , m_usage(QBuffer::StaticDraw)
    , m_bufferDirty(false)
    , m_syncData(false)
    , m_access(QBuffer::Write)
    , m_manager(nullptr)
{
    // Maybe it could become read write if we want to inform
    // the frontend QBuffer node of any backend issue
}

Buffer::~Buffer()
{
}

void Buffer::cleanup()
{
    m_usage = QBuffer::StaticDraw;
    m_data.clear();
    m_bufferUpdates.clear();
    m_functor.reset();
    m_bufferDirty = false;
    m_syncData = false;
    m_access = QBuffer::Write;
}


void Buffer::setManager(BufferManager *manager)
{
    m_manager = manager;
}

void Buffer::executeFunctor()
{
    Q_ASSERT(m_functor);
    m_data = (*m_functor)();
    // Request data to be loaded
    forceDataUpload();

    if (m_syncData) {
        // Send data back to the frontend
        auto e = Qt3DCore::QPropertyUpdatedChangePtr::create(peerId());
        e->setDeliveryFlags(Qt3DCore::QSceneChange::DeliverToAll);
        e->setPropertyName("data");
        e->setValue(QVariant::fromValue(m_data));
        notifyObservers(e);
    }
}

//Called from th sendBufferJob
void Buffer::updateDataFromGPUToCPU(QByteArray data)
{
    // Note: when this is called, data is what's currently in GPU memory
    // so m_data shouldn't be reuploaded
    m_data = data;
    // Send data back to the frontend
    auto e = Qt3DCore::QPropertyUpdatedChangePtr::create(peerId());
    e->setDeliveryFlags(Qt3DCore::QSceneChange::DeliverToAll);
    e->setPropertyName("downloadedData");
    e->setValue(QVariant::fromValue(m_data));
    notifyObservers(e);
}

void Buffer::initializeFromPeer(const Qt3DCore::QNodeCreatedChangeBasePtr &change)
{
    const auto typedChange = qSharedPointerCast<Qt3DCore::QNodeCreatedChange<QBufferData>>(change);
    const auto &data = typedChange->data;
    m_data = data.data;
    m_usage = data.usage;
    m_syncData = data.syncData;
    m_access = data.access;
    m_bufferDirty = true;

    if (!m_data.isEmpty())
        forceDataUpload();

    m_functor = data.functor;
    Q_ASSERT(m_manager);
    if (m_functor)
        m_manager->addDirtyBuffer(peerId());

    m_manager->addBufferReference(peerId());
    markDirty(AbstractRenderer::BuffersDirty);
}

void Buffer::forceDataUpload()
{
    // We push back an update with offset = -1
    // As this is the way to force data to be loaded
    QBufferUpdate updateNewData;
    updateNewData.offset = -1;
    m_bufferUpdates.clear(); //previous updates are pointless
    m_bufferUpdates.push_back(updateNewData);
}

void Buffer::sceneChangeEvent(const Qt3DCore::QSceneChangePtr &e)
{
    if (e->type() == PropertyUpdated) {
        QPropertyUpdatedChangePtr propertyChange = qSharedPointerCast<QPropertyUpdatedChange>(e);
        QByteArray propertyName = propertyChange->propertyName();
        if (propertyName == QByteArrayLiteral("data")) {
            QByteArray newData = propertyChange->value().toByteArray();
            bool dirty = m_data != newData;
            m_bufferDirty |= dirty;
            m_data = newData;
            if (dirty)
                forceDataUpload();
        } else if (propertyName == QByteArrayLiteral("updateData")) {
            Qt3DRender::QBufferUpdate updateData = propertyChange->value().value<Qt3DRender::QBufferUpdate>();
            m_data.replace(updateData.offset, updateData.data.size(), updateData.data);
            m_bufferUpdates.push_back(updateData);
            m_bufferDirty = true;
        } else if (propertyName == QByteArrayLiteral("usage")) {
            m_usage = static_cast<QBuffer::UsageType>(propertyChange->value().value<int>());
            m_bufferDirty = true;
        } else if (propertyName == QByteArrayLiteral("accessType")) {
            m_access = static_cast<QBuffer::AccessType>(propertyChange->value().value<int>());
        } else if (propertyName == QByteArrayLiteral("dataGenerator")) {
            QBufferDataGeneratorPtr newGenerator = propertyChange->value().value<QBufferDataGeneratorPtr>();
            m_bufferDirty |= !(newGenerator && m_functor && *newGenerator == *m_functor);
            m_functor = newGenerator;
            if (m_functor && m_manager != nullptr)
                m_manager->addDirtyBuffer(peerId());
        } else if (propertyName == QByteArrayLiteral("syncData")) {
            m_syncData = propertyChange->value().toBool();
        }
        markDirty(AbstractRenderer::BuffersDirty);
    }
    BackendNode::sceneChangeEvent(e);
}

// Called by Renderer once the buffer has been uploaded to OpenGL
void Buffer::unsetDirty()
{
    m_bufferDirty = false;
}

BufferFunctor::BufferFunctor(AbstractRenderer *renderer, BufferManager *manager)
    : m_manager(manager)
    , m_renderer(renderer)
{
}

Qt3DCore::QBackendNode *BufferFunctor::create(const Qt3DCore::QNodeCreatedChangeBasePtr &change) const
{
    Buffer *buffer = m_manager->getOrCreateResource(change->subjectId());
    buffer->setManager(m_manager);
    buffer->setRenderer(m_renderer);
    return buffer;
}

Qt3DCore::QBackendNode *BufferFunctor::get(Qt3DCore::QNodeId id) const
{
    return m_manager->lookupResource(id);
}

void BufferFunctor::destroy(Qt3DCore::QNodeId id) const
{
    m_manager->removeBufferReference(id);
    return m_manager->releaseResource(id);
}

} // namespace Render
} // namespace Qt3DRender

QT_END_NAMESPACE
