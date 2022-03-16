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

#include "qthreadpooler_p.h"
#include <QtCore/QDebug>

#if QT_CONFIG(qt3d_profile_jobs)

#ifdef Q_OS_ANDROID
#include <QtCore/QStandardPaths>
#endif

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QThreadStorage>
#include <QtCore/QDateTime>
#include <QtCore/QCoreApplication>
#endif

QT_BEGIN_NAMESPACE

namespace Qt3DCore {

#if QT_CONFIG(qt3d_profile_jobs)
QElapsedTimer QThreadPooler::m_jobsStatTimer;
#endif

QThreadPooler::QThreadPooler(QObject *parent)
    : QObject(parent)
    , m_futureInterface(nullptr)
    , m_mutex()
    , m_taskCount(0)
{
    const QByteArray maxThreadCount = qgetenv("QT3D_MAX_THREAD_COUNT");
    if (!maxThreadCount.isEmpty()) {
        bool conversionOK = false;
        const int maxThreadCountValue = maxThreadCount.toInt(&conversionOK);
        if (conversionOK)
            m_threadPool.setMaxThreadCount(maxThreadCountValue);
    }
    // Ensures that threads will never be recycled
    m_threadPool.setExpiryTimeout(-1);
#if QT_CONFIG(qt3d_profile_jobs)
    QThreadPooler::m_jobsStatTimer.start();
#endif
}

QThreadPooler::~QThreadPooler()
{
    // Wait till all tasks are finished before deleting mutex
    QMutexLocker locker(&m_mutex);
    locker.unlock();
}

void QThreadPooler::enqueueTasks(const QVector<RunnableInterface *> &tasks)
{
    // The caller have to set the mutex
    const QVector<RunnableInterface *>::const_iterator end = tasks.cend();

    for (QVector<RunnableInterface *>::const_iterator it = tasks.cbegin();
         it != end; ++it) {

        // Only AspectTaskRunnables are checked for dependencies.
        static const auto hasDependencies = [](RunnableInterface *task) -> bool {
            return (task->type() == RunnableInterface::RunnableType::AspectTask)
                    && (static_cast<AspectTaskRunnable *>(task)->m_dependerCount > 0);
        };

        if (!hasDependencies(*it) && !(*it)->reserved()) {
            (*it)->setReserved(true);
            (*it)->setPooler(this);
            m_threadPool.start((*it));
        }
    }
}

void QThreadPooler::taskFinished(RunnableInterface *task)
{
    const QMutexLocker locker(&m_mutex);

    release();

    if (task->type() == RunnableInterface::RunnableType::AspectTask) {
        AspectTaskRunnable *aspectTask = static_cast<AspectTaskRunnable *>(task);
        const auto &dependers = aspectTask->m_dependers;
        for (auto it = dependers.begin(); it != dependers.end(); ++it) {
            aspectTask = static_cast<AspectTaskRunnable *>(*it);
            if (--aspectTask->m_dependerCount == 0) {
                if (!aspectTask->reserved()) {
                    aspectTask->setReserved(true);
                    aspectTask->setPooler(this);
                    m_threadPool.start(aspectTask);
                }
            }
        }
    }

    if (currentCount() == 0) {
        if (m_futureInterface) {
            m_futureInterface->reportFinished();
            delete m_futureInterface;
        }
        m_futureInterface = nullptr;
    }
}

QFuture<void> QThreadPooler::mapDependables(QVector<RunnableInterface *> &taskQueue)
{
    const QMutexLocker locker(&m_mutex);

    if (!m_futureInterface)
        m_futureInterface = new QFutureInterface<void>();
    if (!taskQueue.empty())
        m_futureInterface->reportStarted();

    acquire(taskQueue.size());
    enqueueTasks(taskQueue);

    return QFuture<void>(m_futureInterface);
}

QFuture<void> QThreadPooler::future()
{
    const QMutexLocker locker(&m_mutex);

    if (!m_futureInterface)
        return QFuture<void>();
    else
        return QFuture<void>(m_futureInterface);
}

void QThreadPooler::acquire(int add)
{
    // The caller have to set the mutex

    m_taskCount.fetchAndAddOrdered(add);
}

void QThreadPooler::release()
{
    // The caller have to set the mutex

    m_taskCount.fetchAndAddOrdered(-1);
}

int QThreadPooler::currentCount() const
{
    // The caller have to set the mutex

    return m_taskCount.load();
}

int QThreadPooler::maxThreadCount() const
{
    return m_threadPool.maxThreadCount();
}

#if QT_CONFIG(qt3d_profile_jobs)

QThreadStorage<QVector<JobRunStats> *> jobStatsCached;

QVector<QVector<JobRunStats> *> localStorages;
QVector<JobRunStats> *submissionStorage = nullptr;

QMutex localStoragesMutex;

// Called by the jobs
void QThreadPooler::addJobLogStatsEntry(JobRunStats &stats)
{
    if (!jobStatsCached.hasLocalData()) {
        auto jobVector = new QVector<JobRunStats>;
        jobStatsCached.setLocalData(jobVector);
        QMutexLocker lock(&localStoragesMutex);
        localStorages.push_back(jobVector);
    }
    jobStatsCached.localData()->push_back(stats);
}

// Called after jobs have been executed (AspectThread QAspectJobManager::enqueueJobs)
void QThreadPooler::writeFrameJobLogStats()
{
    static QScopedPointer<QFile> traceFile;
    static quint32 frameId = 0;
    if (!traceFile) {
        const QString fileName = QStringLiteral("trace_") + QCoreApplication::applicationName() + QDateTime::currentDateTime().toString(QStringLiteral("_ddd_dd_MM_yy-hh_mm_ss_"))+ QSysInfo::productType() + QStringLiteral("_") + QSysInfo::buildAbi() + QStringLiteral(".qt3d");
#ifdef Q_OS_ANDROID
        traceFile.reset(new QFile(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) + QStringLiteral("/") + fileName));
#else
        traceFile.reset(new QFile(fileName));
#endif
        if (!traceFile->open(QFile::WriteOnly|QFile::Truncate))
            qCritical("Failed to open trace file");
    }

    // Write Aspect + Job threads
    {
        FrameHeader header;
        header.frameId = frameId;
        header.jobCount = 0;

        for (const QVector<JobRunStats> *storage : qAsConst(localStorages))
            header.jobCount += storage->size();

        traceFile->write(reinterpret_cast<char *>(&header), sizeof(FrameHeader));

        for (QVector<JobRunStats> *storage : qAsConst(localStorages)) {
            for (const JobRunStats &stat : *storage)
                traceFile->write(reinterpret_cast<const char *>(&stat), sizeof(JobRunStats));
            storage->clear();
        }
    }

    // Write submission thread
    {
        QMutexLocker lock(&localStoragesMutex);
        const int submissionJobSize = submissionStorage != nullptr ? submissionStorage->size() : 0;
        if (submissionJobSize > 0) {
            FrameHeader header;
            header.frameId = frameId;
            header.jobCount = submissionJobSize;
            header.frameType = FrameHeader::Submission;

            traceFile->write(reinterpret_cast<char *>(&header), sizeof(FrameHeader));

            for (const JobRunStats &stat : *submissionStorage)
                traceFile->write(reinterpret_cast<const char *>(&stat), sizeof(JobRunStats));
            submissionStorage->clear();
        }
    }

    traceFile->flush();
    ++frameId;
}

// Called from Submission thread
void QThreadPooler::addSubmissionLogStatsEntry(JobRunStats &stats)
{
    QMutexLocker lock(&localStoragesMutex);
    if (!jobStatsCached.hasLocalData()) {
        submissionStorage = new QVector<JobRunStats>;
        jobStatsCached.setLocalData(submissionStorage);
    }
    submissionStorage->push_back(stats);
}

#endif

} // namespace Qt3DCore

QT_END_NAMESPACE
