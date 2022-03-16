/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
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

#include "qsamplecache_p.h"
#include "qwavedecoder_p.h"

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

#include <QtCore/QDebug>
//#define QT_SAMPLECACHE_DEBUG

QT_BEGIN_NAMESPACE


/*!
    \class QSampleCache
    \internal

    When you want to get a sound sample data, you need to request the QSample reference from QSampleCache.


    \code
        QSample *m_sample;     // class member.

      private Q_SLOTS:
        void decoderError();
        void sampleReady();
    \endcode

    \code
      Q_GLOBAL_STATIC(QSampleCache, sampleCache) //declare a singleton manager
    \endcode

    \code
        m_sample = sampleCache()->requestSample(url);
        switch(m_sample->state()) {
        case QSample::Ready:
            sampleReady();
            break;
        case QSample::Error:
            decoderError();
            break;
        default:
            connect(m_sample, SIGNAL(error()), this, SLOT(decoderError()));
            connect(m_sample, SIGNAL(ready()), this, SLOT(sampleReady()));
            break;
        }
    \endcode

    When you no longer need the sound sample data, you need to release it:

    \code
       if (m_sample) {
           m_sample->release();
           m_sample = 0;
       }
    \endcode
*/

QSampleCache::QSampleCache(QObject *parent)
    : QObject(parent)
    , m_networkAccessManager(0)
    , m_mutex(QMutex::Recursive)
    , m_capacity(0)
    , m_usage(0)
    , m_loadingRefCount(0)
{
    m_loadingThread.setObjectName(QLatin1String("QSampleCache::LoadingThread"));
    connect(&m_loadingThread, SIGNAL(finished()), this, SIGNAL(isLoadingChanged()));
    connect(&m_loadingThread, SIGNAL(started()), this, SIGNAL(isLoadingChanged()));
}

QNetworkAccessManager& QSampleCache::networkAccessManager()
{
    if (!m_networkAccessManager)
        m_networkAccessManager = new QNetworkAccessManager();
    return *m_networkAccessManager;
}

QSampleCache::~QSampleCache()
{
    QMutexLocker m(&m_mutex);

    m_loadingThread.quit();
    m_loadingThread.wait();

    // Killing the loading thread means that no samples can be
    // deleted using deleteLater.  And some samples that had deleteLater
    // already called won't have been processed (m_staleSamples)
    for (auto it = m_samples.cbegin(), end = m_samples.cend(); it != end; ++it)
        delete it.value();

    const auto copyStaleSamples = m_staleSamples; //deleting a sample does affect the m_staleSamples list, but we create a copy
    for (QSample* sample : copyStaleSamples)
        delete sample;

    delete m_networkAccessManager;
}

void QSampleCache::loadingRelease()
{
    QMutexLocker locker(&m_loadingMutex);
    m_loadingRefCount--;
    if (m_loadingRefCount == 0) {
        if (m_loadingThread.isRunning()) {
            if (m_networkAccessManager) {
                m_networkAccessManager->deleteLater();
                m_networkAccessManager = nullptr;
            }
            m_loadingThread.exit();
        }
    }
}

bool QSampleCache::isLoading() const
{
    return m_loadingThread.isRunning();
}

bool QSampleCache::isCached(const QUrl &url) const
{
    QMutexLocker locker(&m_mutex);
    return m_samples.contains(url);
}

QSample* QSampleCache::requestSample(const QUrl& url)
{
    //lock and add first to make sure live loadingThread will not be killed during this function call
    m_loadingMutex.lock();
    m_loadingRefCount++;
    m_loadingMutex.unlock();

    if (!m_loadingThread.isRunning())
        m_loadingThread.start();

#ifdef QT_SAMPLECACHE_DEBUG
    qDebug() << "QSampleCache: request sample [" << url << "]";
#endif
    QMutexLocker locker(&m_mutex);
    QMap<QUrl, QSample*>::iterator it = m_samples.find(url);
    QSample* sample;
    if (it == m_samples.end()) {
        sample = new QSample(url, this);
        m_samples.insert(url, sample);
        sample->moveToThread(&m_loadingThread);
    } else {
        sample = *it;
    }

    sample->addRef();
    locker.unlock();

    sample->loadIfNecessary();
    return sample;
}

void QSampleCache::setCapacity(qint64 capacity)
{
    QMutexLocker locker(&m_mutex);
    if (m_capacity == capacity)
        return;
#ifdef QT_SAMPLECACHE_DEBUG
    qDebug() << "QSampleCache: capacity changes from " << m_capacity << "to " << capacity;
#endif
    if (m_capacity > 0 && capacity <= 0) { //memory management strategy changed
        for (QMap<QUrl, QSample*>::iterator it = m_samples.begin(); it != m_samples.end();) {
            QSample* sample = *it;
            if (sample->m_ref == 0) {
                unloadSample(sample);
                it = m_samples.erase(it);
            } else {
                ++it;
            }
        }
    }

    m_capacity = capacity;
    refresh(0);
}

// Called locked
void QSampleCache::unloadSample(QSample *sample)
{
    m_usage -= sample->m_soundData.size();
    m_staleSamples.insert(sample);
    sample->deleteLater();
}

// Called in both threads
void QSampleCache::refresh(qint64 usageChange)
{
    QMutexLocker locker(&m_mutex);
    m_usage += usageChange;
    if (m_capacity <= 0 || m_usage <= m_capacity)
        return;

#ifdef QT_SAMPLECACHE_DEBUG
    qint64 recoveredSize = 0;
#endif

    //free unused samples to keep usage under capacity limit.
    for (QMap<QUrl, QSample*>::iterator it = m_samples.begin(); it != m_samples.end();) {
        QSample* sample = *it;
        if (sample->m_ref > 0) {
            ++it;
            continue;
        }
#ifdef QT_SAMPLECACHE_DEBUG
        recoveredSize += sample->m_soundData.size();
#endif
        unloadSample(sample);
        it = m_samples.erase(it);
        if (m_usage <= m_capacity)
            return;
    }

#ifdef QT_SAMPLECACHE_DEBUG
    qDebug() << "QSampleCache: refresh(" << usageChange
             << ") recovered size =" << recoveredSize
             << "new usage =" << m_usage;
#endif

    if (m_usage > m_capacity)
        qWarning() << "QSampleCache: usage[" << m_usage << " out of limit[" << m_capacity << "]";
}

// Called in both threads
void QSampleCache::removeUnreferencedSample(QSample *sample)
{
    QMutexLocker m(&m_mutex);
    m_staleSamples.remove(sample);
}

// Called in loader thread (since this lives in that thread)
// Also called from application thread after loader thread dies.
QSample::~QSample()
{
    // Remove ourselves from our parent
    m_parent->removeUnreferencedSample(this);

    QMutexLocker locker(&m_mutex);
#ifdef QT_SAMPLECACHE_DEBUG
    qDebug() << "~QSample" << this << ": deleted [" << m_url << "]" << QThread::currentThread();
#endif
    cleanup();
}

// Called in application thread
void QSample::loadIfNecessary()
{
    QMutexLocker locker(&m_mutex);
    if (m_state == QSample::Error || m_state == QSample::Creating) {
        m_state = QSample::Loading;
        QMetaObject::invokeMethod(this, "load", Qt::QueuedConnection);
    } else {
        qobject_cast<QSampleCache*>(m_parent)->loadingRelease();
    }
}

// Called in application thread
bool QSampleCache::notifyUnreferencedSample(QSample* sample)
{
    if (m_loadingThread.isRunning())
        m_loadingThread.wait();

    QMutexLocker locker(&m_mutex);
    if (m_capacity > 0)
        return false;
    m_samples.remove(sample->m_url);
    unloadSample(sample);
    return true;
}

// Called in application thread
void QSample::release()
{
    QMutexLocker locker(&m_mutex);
#ifdef QT_SAMPLECACHE_DEBUG
    qDebug() << "Sample:: release" << this << QThread::currentThread() << m_ref;
#endif
    if (--m_ref == 0) {
        locker.unlock();
        m_parent->notifyUnreferencedSample(this);
    }
}

// Called in dtor and when stream is loaded
// must be called locked.
void QSample::cleanup()
{
    if (m_waveDecoder)
        m_waveDecoder->deleteLater();
    if (m_stream)
        m_stream->deleteLater();

    m_waveDecoder = 0;
    m_stream = 0;
}

// Called in application thread
void QSample::addRef()
{
    m_ref++;
}

// Called in loading thread
void QSample::readSample()
{
    Q_ASSERT(QThread::currentThread()->objectName() == QLatin1String("QSampleCache::LoadingThread"));
    QMutexLocker m(&m_mutex);
#ifdef  QT_SAMPLECACHE_DEBUG
    qDebug() << "QSample: readSample";
#endif
    qint64 read = m_waveDecoder->read(m_soundData.data() + m_sampleReadLength,
                      qMin(m_waveDecoder->bytesAvailable(),
                           qint64(m_waveDecoder->size() - m_sampleReadLength)));
    if (read > 0)
        m_sampleReadLength += read;
    if (m_sampleReadLength < m_waveDecoder->size())
        return;
    Q_ASSERT(m_sampleReadLength == qint64(m_soundData.size()));
    onReady();
}

// Called in loading thread
void QSample::decoderReady()
{
    Q_ASSERT(QThread::currentThread()->objectName() == QLatin1String("QSampleCache::LoadingThread"));
    QMutexLocker m(&m_mutex);
#ifdef QT_SAMPLECACHE_DEBUG
    qDebug() << "QSample: decoder ready";
#endif
    m_parent->refresh(m_waveDecoder->size());

    m_soundData.resize(m_waveDecoder->size());
    m_sampleReadLength = 0;
    qint64 read = m_waveDecoder->read(m_soundData.data(), m_waveDecoder->size());
    if (read > 0)
        m_sampleReadLength += read;
    if (m_sampleReadLength >= m_waveDecoder->size())
        onReady();
}

// Called in all threads
QSample::State QSample::state() const
{
    QMutexLocker m(&m_mutex);
    return m_state;
}

// Called in loading thread
// Essentially a second ctor, doesn't need locks (?)
void QSample::load()
{
    Q_ASSERT(QThread::currentThread()->objectName() == QLatin1String("QSampleCache::LoadingThread"));
#ifdef QT_SAMPLECACHE_DEBUG
    qDebug() << "QSample: load [" << m_url << "]";
#endif
    m_stream = m_parent->networkAccessManager().get(QNetworkRequest(m_url));
    connect(m_stream, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(decoderError()));
    m_waveDecoder = new QWaveDecoder(m_stream);
    connect(m_waveDecoder, SIGNAL(formatKnown()), SLOT(decoderReady()));
    connect(m_waveDecoder, SIGNAL(parsingError()), SLOT(decoderError()));
    connect(m_waveDecoder, SIGNAL(readyRead()), SLOT(readSample()));
}

// Called in loading thread
void QSample::decoderError()
{
    Q_ASSERT(QThread::currentThread()->objectName() == QLatin1String("QSampleCache::LoadingThread"));
    QMutexLocker m(&m_mutex);
#ifdef QT_SAMPLECACHE_DEBUG
    qDebug() << "QSample: decoder error";
#endif
    cleanup();
    m_state = QSample::Error;
    qobject_cast<QSampleCache*>(m_parent)->loadingRelease();
    emit error();
}

// Called in loading thread from decoder when sample is done. Locked already.
void QSample::onReady()
{
    Q_ASSERT(QThread::currentThread()->objectName() == QLatin1String("QSampleCache::LoadingThread"));
#ifdef QT_SAMPLECACHE_DEBUG
    qDebug() << "QSample: load ready";
#endif
    m_audioFormat = m_waveDecoder->audioFormat();
    cleanup();
    m_state = QSample::Ready;
    qobject_cast<QSampleCache*>(m_parent)->loadingRelease();
    emit ready();
}

// Called in application thread, then moved to loader thread
QSample::QSample(const QUrl& url, QSampleCache *parent)
    : m_parent(parent)
    , m_stream(0)
    , m_waveDecoder(0)
    , m_url(url)
    , m_sampleReadLength(0)
    , m_state(Creating)
    , m_ref(0)
{
}

QT_END_NAMESPACE

#include "moc_qsamplecache_p.cpp"
