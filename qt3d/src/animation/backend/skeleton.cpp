/****************************************************************************
**
** Copyright (C) 2017 Klaralvdalens Datakonsult AB (KDAB).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt3D module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "skeleton_p.h"
#include <Qt3DCore/private/qabstractskeleton_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt3DCore;

namespace Qt3DAnimation {
namespace Animation {

// Rather than store backend nodes for the individual joints, the
// animation aspect operates on the vector of local poses as aggregated
// by the skeleton. This allows us to animate a skeleton even when the
// frontend QSkeletonLoader does not instantiate the frontend QJoint nodes.
// It also means we don't need a QChannelMapping for each property of each
// joint.

Skeleton::Skeleton()
    : BackendNode(Qt3DCore::QBackendNode::ReadWrite)
{
}

void Skeleton::cleanup()
{
    m_jointNames.clear();
    m_jointLocalPoses.clear();
}

void Skeleton::syncFromFrontEnd(const Qt3DCore::QNode *frontEnd, bool firstTime)
{
    BackendNode::syncFromFrontEnd(frontEnd, firstTime);

    const Qt3DCore::QAbstractSkeleton *node = qobject_cast<const Qt3DCore::QAbstractSkeleton *>(frontEnd);
    if (!node)
        return;

    auto dnode = Qt3DCore::QAbstractSkeletonPrivate::get(node);

    // TODO: Mark joint info as dirty so we can rebuild any indexes used
    // by the animators and channel mappings.
    m_jointNames = dnode->m_jointNames;
    m_jointLocalPoses = dnode->m_localPoses;
}

} // namespace Animation
} // namespace Qt3DAnimation

QT_END_NAMESPACE
