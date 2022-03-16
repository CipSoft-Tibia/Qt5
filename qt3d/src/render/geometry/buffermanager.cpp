/****************************************************************************
**
** Copyright (C) 2015 Paul Lemire
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

#include "buffermanager_p.h"

QT_BEGIN_NAMESPACE

namespace Qt3DRender {
namespace Render {

BufferManager::BufferManager()
{
}

BufferManager::~BufferManager()
{
}

void BufferManager::addDirtyBuffer(Qt3DCore::QNodeId bufferId)
{
    if (!m_dirtyBuffers.contains(bufferId))
        m_dirtyBuffers.push_back(bufferId);
}

QVector<Qt3DCore::QNodeId> BufferManager::takeDirtyBuffers()
{
    return qMove(m_dirtyBuffers);
}

// Called in QAspectThread::syncChanges
void BufferManager::removeBufferReference(Qt3DCore::QNodeId bufferId)
{
    QMutexLocker lock(&m_mutex);
    Q_ASSERT(m_bufferReferences.contains(bufferId) && m_bufferReferences[bufferId] > 0);
    m_bufferReferences[bufferId]--;
}

// Called in QAspectThread
void BufferManager::addBufferReference(Qt3DCore::QNodeId bufferId)
{
    QMutexLocker lock(&m_mutex);
    m_bufferReferences[bufferId]++;
}

// Called in Render thread
QVector<Qt3DCore::QNodeId> BufferManager::takeBuffersToRelease()
{
    QMutexLocker lock(&m_mutex);
    QVector<Qt3DCore::QNodeId> buffersToRelease;
    QMutableHashIterator<Qt3DCore::QNodeId, int> it(m_bufferReferences);
    while (it.hasNext()) {
        it.next();
        if (it.value() == 0) {
            buffersToRelease.append(it.key());
            it.remove();
        }
    }
    return buffersToRelease;
}

} // namespace Render
} // namespace Qt3DRender

QT_END_NAMESPACE
