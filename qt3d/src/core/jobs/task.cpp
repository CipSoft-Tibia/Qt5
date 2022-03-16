/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "task_p.h"

#include <QtCore/QDebug>
#include <QtCore/QElapsedTimer>
#include <QtCore/QMutexLocker>

#include <Qt3DCore/private/qthreadpooler_p.h>

QT_BEGIN_NAMESPACE

namespace Qt3DCore {

RunnableInterface::~RunnableInterface()
{
}

// Aspect task

AspectTaskRunnable::AspectTaskRunnable()
    : m_pooler(nullptr)
    , m_reserved(false)
{
}

AspectTaskRunnable::~AspectTaskRunnable()
{
}

void AspectTaskRunnable::run()
{
    if (m_job) {
#if QT_CONFIG(qt3d_profile_jobs)
        QAspectJobPrivate *jobD = QAspectJobPrivate::get(m_job.data());
        if (m_pooler) {
            jobD->m_stats.startTime = QThreadPooler::m_jobsStatTimer.nsecsElapsed();
            jobD->m_stats.threadId = reinterpret_cast<quint64>(QThread::currentThreadId());
        }
#endif
        m_job->run();
#if QT_CONFIG(qt3d_profile_jobs)
        if (m_pooler) {
            jobD->m_stats.endTime = QThreadPooler::m_jobsStatTimer.nsecsElapsed();
            // Add current job's stats to log output
            QThreadPooler::addJobLogStatsEntry(jobD->m_stats);
        }
#endif
    }

    // We could have an append sub task or something in here
    // So that a job can post sub jobs ?

    if (m_pooler)
        m_pooler->taskFinished(this);
}

// Synchronized task

SyncTaskRunnable::SyncTaskRunnable(QAbstractAspectJobManager::JobFunction func,
                                   void *arg, QAtomicInt *atomicCount)
    : m_func(func)
    , m_arg(arg)
    , m_atomicCount(atomicCount)
    , m_pooler(nullptr)
    , m_reserved(false)
    , m_id(0)
{
}

SyncTaskRunnable::~SyncTaskRunnable()
{
}

void SyncTaskRunnable::run()
{
    // Call the function
    m_func(m_arg);

    // Decrement the atomic counter to let others know we've done our bit
    m_atomicCount->deref();

    // Wait for the other worker threads to be done
    while (m_atomicCount->load() > 0)
        QThread::currentThread()->yieldCurrentThread();

    if (m_pooler)
        m_pooler->taskFinished(this);
}

} // namespace Qt3DCore {

QT_END_NAMESPACE
