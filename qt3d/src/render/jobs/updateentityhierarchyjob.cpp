/****************************************************************************
**
** Copyright (C) 2019 Klaralvdalens Datakonsult AB (KDAB).
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

#include "updateentityhierarchyjob_p.h"
#include <Qt3DRender/private/managers_p.h>
#include <Qt3DRender/private/nodemanagers_p.h>
#include <Qt3DRender/private/entity_p.h>
#include <Qt3DRender/private/job_common_p.h>

QT_BEGIN_NAMESPACE

namespace Qt3DRender {

namespace Render {

UpdateEntityHierarchyJob::UpdateEntityHierarchyJob()
    : m_manager(nullptr)
{
    SET_JOB_RUN_STAT_TYPE(this, JobTypes::UpdateEntityHierarchy, 0);
}

void UpdateEntityHierarchyJob::run()
{
    Q_ASSERT(m_manager);
    EntityManager *entityManager = m_manager->renderNodesManager();

    const QVector<HEntity> handles = entityManager->activeHandles();

    // Clear the parents and children
    for (const HEntity &handle : handles) {
        Entity *entity = entityManager->data(handle);
        entity->clearEntityHierarchy();
    }
    for (const HEntity &handle : handles) {
        Entity *entity = entityManager->data(handle);
        entity->rebuildEntityHierarchy();
    }
}

} // Render

} // Qt3DRender

QT_END_NAMESPACE
