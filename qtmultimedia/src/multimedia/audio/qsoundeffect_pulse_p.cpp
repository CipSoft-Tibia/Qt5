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

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// INTERNAL USE ONLY: Do NOT use for any other purpose.
//

#include <QtCore/qcoreapplication.h>
#include <qaudioformat.h>
#include <QTime>
#include <QTimer>

#include "qsoundeffect_pulse_p.h"

#include <private/qaudiohelpers_p.h>
#include <private/qmediaresourcepolicy_p.h>
#include <private/qmediaresourceset_p.h>

#include <unistd.h>

//#define QT_PA_DEBUG

QT_BEGIN_NAMESPACE

namespace
{
inline pa_sample_spec audioFormatToSampleSpec(const QAudioFormat &format)
{
    pa_sample_spec  spec;

    spec.rate = format.sampleRate();
    spec.channels = format.channelCount();
    spec.format = PA_SAMPLE_INVALID;
    const bool isBigEndian = (format.byteOrder() == QAudioFormat::BigEndian);

    if (format.sampleType() == QAudioFormat::UnSignedInt) {
        if (format.sampleSize() == 8)
            spec.format = PA_SAMPLE_U8;
    } else if (format.sampleType() == QAudioFormat::SignedInt) {
        if (format.sampleSize() == 16) {
            spec.format = isBigEndian ? PA_SAMPLE_S16BE : PA_SAMPLE_S16LE;
        } else if (format.sampleSize() == 24) {
            spec.format = isBigEndian ? PA_SAMPLE_S24BE : PA_SAMPLE_S24LE;
        } else if (format.sampleSize() == 32) {
            spec.format = isBigEndian ? PA_SAMPLE_S32BE : PA_SAMPLE_S32LE;
        }
    } else if (format.sampleType() == QAudioFormat::Float) {
        if (format.sampleSize() == 32)
            spec.format = isBigEndian ? PA_SAMPLE_FLOAT32BE : PA_SAMPLE_FLOAT32LE;
    }

    return spec;
}

class PulseDaemon : public QObject
{
    Q_OBJECT
public:
    PulseDaemon()
    {
        prepare();
    }

    ~PulseDaemon()
    {
        release();
    }

    inline void ref()
    {
        m_ref.ref();
        prepare();
    }

    inline void deref()
    {
        if (!m_ref.deref())
            release();
    }

    inline void lock()
    {
        if (m_mainLoop) {
            if (++m_lockCount == 1)
                pa_threaded_mainloop_lock(m_mainLoop);
        }
    }

    inline void unlock()
    {
        if (m_mainLoop) {
            if (--m_lockCount == 0)
                pa_threaded_mainloop_unlock(m_mainLoop);
        }
    }

    inline pa_context *context() const
    {
        return m_context;
    }

Q_SIGNALS:
    void contextReady();
    void contextFailed();

private Q_SLOTS:
    void onContextFailed()
    {
        release();

        // Try to reconnect later
        QTimer::singleShot(30000, this, &PulseDaemon::prepare);

        emit contextFailed();
    }

    void prepare()
    {
        if (m_prepared)
            return;

        m_context = nullptr;
        m_mainLoop = pa_threaded_mainloop_new();
        if (m_mainLoop == nullptr) {
            qWarning("PulseAudioService: unable to create pulseaudio mainloop");
            return;
        }

        if (pa_threaded_mainloop_start(m_mainLoop) != 0) {
            qWarning("PulseAudioService: unable to start pulseaudio mainloop");
            pa_threaded_mainloop_free(m_mainLoop);
            return;
        }

        m_mainLoopApi = pa_threaded_mainloop_get_api(m_mainLoop);

        lock();
        m_context = pa_context_new(m_mainLoopApi, QString(QLatin1String("QtPulseAudio:%1")).arg(::getpid()).toLatin1().constData());

        if (m_context == nullptr) {
            qWarning("PulseAudioService: Unable to create new pulseaudio context");
            unlock();
            pa_threaded_mainloop_free(m_mainLoop);
            m_mainLoop = nullptr;
            onContextFailed();
            return;
        }

        pa_context_set_state_callback(m_context, context_state_callback, this);

        const QByteArray srvStrEnv = qgetenv("QT_PULSE_SERVER_STRING");
        const char *srvStr = srvStrEnv.isNull() ? nullptr : srvStrEnv.constData();
        pa_context_flags_t flags = qEnvironmentVariableIsSet("QT_PULSE_NOAUTOSPAWN") ? PA_CONTEXT_NOAUTOSPAWN : (pa_context_flags_t)0;

        if (pa_context_connect(m_context, srvStr, flags, nullptr) < 0) {
            qWarning("PulseAudioService: pa_context_connect() failed");
            pa_context_unref(m_context);
            unlock();
            pa_threaded_mainloop_free(m_mainLoop);
            m_mainLoop = nullptr;
            m_context = nullptr;
            return;
        }
        unlock();

        m_prepared = true;
    }

private:
    void release()
    {
        if (!m_prepared)
            return;

        if (m_context) {
            lock();
            pa_context_disconnect(m_context);
            unlock();
        }

        if (m_mainLoop) {
            pa_threaded_mainloop_stop(m_mainLoop);
            pa_threaded_mainloop_free(m_mainLoop);
            m_mainLoop = nullptr;
        }

        if (m_context) {
            pa_context_unref(m_context);
            m_context = nullptr;
        }

        m_prepared = false;
    }

    static void context_state_callback(pa_context *c, void *userdata)
    {
        PulseDaemon *self = reinterpret_cast<PulseDaemon*>(userdata);
        switch (pa_context_get_state(c)) {
            case PA_CONTEXT_CONNECTING:
            case PA_CONTEXT_AUTHORIZING:
            case PA_CONTEXT_SETTING_NAME:
                break;
            case PA_CONTEXT_READY:
                QMetaObject::invokeMethod(self, "contextReady", Qt::QueuedConnection);
                break;
            case PA_CONTEXT_FAILED:
                QMetaObject::invokeMethod(self, "onContextFailed", Qt::QueuedConnection);
                break;
            default:
                break;
        }
    }

    bool m_prepared = false;
    pa_context *m_context = nullptr;
    pa_threaded_mainloop *m_mainLoop = nullptr;
    pa_mainloop_api *m_mainLoopApi = nullptr;
    uint m_lockCount = 0;
    QAtomicInt m_ref;
};

}

Q_GLOBAL_STATIC(PulseDaemon, pulseDaemon)
Q_GLOBAL_STATIC(QSampleCache, sampleCache)

namespace
{
class PulseDaemonLocker
{
public:
    PulseDaemonLocker()
    {
        pulseDaemon()->lock();
    }

    ~PulseDaemonLocker()
    {
        pulseDaemon()->unlock();
    }
};
}

class QSoundEffectRef
{
public:
    QSoundEffectRef(QSoundEffectPrivate *target)
        : m_target(target)
    {
#ifdef QT_PA_DEBUG
        qDebug() << "QSoundEffectRef(" << this << ") ctor";
#endif
    }

    QSoundEffectRef *getRef()
    {
#ifdef QT_PA_DEBUG
        qDebug() << "QSoundEffectRef(" << this << ") getRef";
#endif
        QMutexLocker locker(&m_mutex);
        m_ref++;
        return this;
    }

    void release()
    {
#ifdef QT_PA_DEBUG
        qDebug() << "QSoundEffectRef(" << this << ") Release";
#endif
        m_mutex.lock();
        --m_ref;
        if (m_ref == 0) {
            m_mutex.unlock();
#ifdef QT_PA_DEBUG
            qDebug() << "QSoundEffectRef(" << this << ") deleted";
#endif
            delete this;
            return;
        }
        m_mutex.unlock();
    }

    QSoundEffectPrivate* soundEffect() const
    {
        QMutexLocker locker(&m_mutex);
        return m_target;
    }

    void notifyDeleted()
    {
#ifdef QT_PA_DEBUG
        qDebug() << "QSoundEffectRef(" << this << ") notifyDeleted";
#endif
        QMutexLocker locker(&m_mutex);
        m_target = nullptr;
    }

private:
    int m_ref = 1;
    mutable QMutex m_mutex;
    QSoundEffectPrivate *m_target = nullptr;
};

QSoundEffectPrivate::QSoundEffectPrivate(QObject* parent):
    QObject(parent)
{
    pulseDaemon()->ref();

    m_ref = new QSoundEffectRef(this);
    if (pulseDaemon()->context())
        pa_sample_spec_init(&m_pulseSpec);

    m_resources = QMediaResourcePolicy::createResourceSet<QMediaPlayerResourceSetInterface>();
    Q_ASSERT(m_resources);
    m_resourcesAvailable = m_resources->isAvailable();
    connect(m_resources, &QMediaPlayerResourceSetInterface::availabilityChanged,
            this, &QSoundEffectPrivate::handleAvailabilityChanged);
}

void QSoundEffectPrivate::handleAvailabilityChanged(bool available)
{
    m_resourcesAvailable = available;
#ifdef DEBUG_RESOURCE
    qDebug() << Q_FUNC_INFO << "Resource availability changed " << m_resourcesAvailable;
#endif
    if (!m_resourcesAvailable)
        stop();
}

void QSoundEffectPrivate::release()
{
#ifdef QT_PA_DEBUG
    qDebug() << this << "release";
#endif
    m_ref->notifyDeleted();
    unloadPulseStream();
    if (m_sample) {
        m_sample->release();
        m_sample = nullptr;
    }

    this->deleteLater();
}

QString QSoundEffectPrivate::category() const
{
    return m_category;
}

void QSoundEffectPrivate::setCategory(const QString &category)
{
    if (m_category != category) {
        m_category = category;

        PulseDaemonLocker locker;

        if (m_playing || m_playQueued) {
            // Currently playing, we need to disconnect when
            // playback stops
            m_reloadCategory = true;
        } else if (m_pulseStream) {
            // We have to disconnect and reconnect
            unloadPulseStream();
            createPulseStream();
        } else {
            // Well, next time we create the pulse stream
            // it should be set
        }

        emit categoryChanged();
    }
}

QSoundEffectPrivate::~QSoundEffectPrivate()
{
    QMediaResourcePolicy::destroyResourceSet(m_resources);
    m_resources = nullptr;
    m_ref->release();

    pulseDaemon()->deref();
}

QStringList QSoundEffectPrivate::supportedMimeTypes()
{
    QStringList supportedTypes;
    supportedTypes << QLatin1String("audio/x-wav") << QLatin1String("audio/vnd.wave") ;
    return supportedTypes;
}

QUrl QSoundEffectPrivate::source() const
{
    return m_source;
}

void QSoundEffectPrivate::setSource(const QUrl &url)
{
    Q_ASSERT(m_source != url);
#ifdef QT_PA_DEBUG
    qDebug() << this << "setSource =" << url;
#endif

    PulseDaemonLocker locker;

    // Make sure the stream is empty before loading a new source (otherwise whatever is there will
    // be played before the new source)
    emptyStream();

    stop();

    if (m_sample) {
        if (!m_sampleReady) {
            disconnect(m_sample, &QSample::error, this, &QSoundEffectPrivate::decoderError);
            disconnect(m_sample, &QSample::ready, this, &QSoundEffectPrivate::sampleReady);
        }
        m_sample->release();
        m_sample = nullptr;
    }

    m_source = url;
    m_sampleReady = false;

    setLoopsRemaining(0);
    if (m_pulseStream && !pa_stream_is_corked(m_pulseStream)) {
        pa_stream_set_write_callback(m_pulseStream, nullptr, nullptr);
        pa_stream_set_underflow_callback(m_pulseStream, nullptr, nullptr);
        pa_operation *op = pa_stream_cork(m_pulseStream, 1, nullptr, nullptr);
        if (op)
            pa_operation_unref(op);
        else
            qWarning("QSoundEffect(pulseaudio): failed to cork stream");
    }
    setPlaying(false);

    if (url.isEmpty()) {
        setStatus(QSoundEffect::Null);
        return;
    }

    setStatus(QSoundEffect::Loading);
    m_sample = sampleCache()->requestSample(url);
    connect(m_sample, &QSample::error, this, &QSoundEffectPrivate::decoderError);
    connect(m_sample, &QSample::ready, this, &QSoundEffectPrivate::sampleReady);
    switch(m_sample->state()) {
    case QSample::Ready:
        sampleReady();
        break;
    case QSample::Error:
        decoderError();
        break;
    default:
        break;
    }
}

int QSoundEffectPrivate::loopCount() const
{
    return m_loopCount;
}

int QSoundEffectPrivate::loopsRemaining() const
{
    return m_runningCount;
}

void QSoundEffectPrivate::setLoopCount(int loopCount)
{
    if (loopCount == 0)
        loopCount = 1;
    m_loopCount = loopCount;
    if (m_playing) {
        PulseDaemonLocker locker;
        setLoopsRemaining(loopCount);
    }
}

qreal QSoundEffectPrivate::volume() const
{
    QMutexLocker locker(&m_volumeLock);
    return m_volume;
}

void QSoundEffectPrivate::setVolume(qreal volume)
{
    QMutexLocker locker(&m_volumeLock);

    if (qFuzzyCompare(m_volume, volume))
        return;

    m_volume = qBound(qreal(0), volume, qreal(1));
    locker.unlock();
    emit volumeChanged();
}

bool QSoundEffectPrivate::isMuted() const
{
    QMutexLocker locker(&m_volumeLock);
    return m_muted;
}

void QSoundEffectPrivate::setMuted(bool muted)
{
    m_volumeLock.lock();
    m_muted = muted;
    m_volumeLock.unlock();

    emit mutedChanged();
}

bool QSoundEffectPrivate::isLoaded() const
{
    return m_status == QSoundEffect::Ready;
}

bool QSoundEffectPrivate::isPlaying() const
{
    return m_playing;
}

QSoundEffect::Status QSoundEffectPrivate::status() const
{
    return m_status;
}

void QSoundEffectPrivate::setPlaying(bool playing)
{
#ifdef QT_PA_DEBUG
    qDebug() << this << "setPlaying(" << playing << ")";
#endif
    if (m_playing == playing)
        return;
    if (!playing)
        m_playQueued = false;
    m_playing = playing;
    emit playingChanged();
}

void QSoundEffectPrivate::setStatus(QSoundEffect::Status status)
{
#ifdef QT_PA_DEBUG
    qDebug() << this << "setStatus" << status;
#endif
    if (m_status == status)
        return;
    bool oldLoaded = isLoaded();
    m_status = status;
    emit statusChanged();
    if (oldLoaded != isLoaded())
        emit loadedChanged();
}

void QSoundEffectPrivate::setLoopsRemaining(int loopsRemaining)
{
#ifdef QT_PA_DEBUG
    qDebug() << this << "setLoopsRemaining " << loopsRemaining;
#endif
    if (m_runningCount == loopsRemaining)
        return;
    m_runningCount = loopsRemaining;
    emit loopsRemainingChanged();
}

void QSoundEffectPrivate::play()
{
    if (!m_resourcesAvailable)
        return;

    playAvailable();
}

void QSoundEffectPrivate::playAvailable()
{
#ifdef QT_PA_DEBUG
    qDebug() << this << "play";
#endif
    if (m_status == QSoundEffect::Null || m_status == QSoundEffect::Error || m_playQueued)
        return;

    PulseDaemonLocker locker;

    if (!m_pulseStream || m_status != QSoundEffect::Ready || m_stopping || m_emptying) {
#ifdef QT_PA_DEBUG
        qDebug() << this << "play deferred";
#endif
        m_playQueued = true;
    } else {
        if (m_playing) { //restart playing from the beginning
#ifdef QT_PA_DEBUG
           qDebug() << this << "restart playing";
#endif
            setLoopsRemaining(0);
            m_playQueued = true;
            Q_ASSERT(m_pulseStream);
            emptyStream(ReloadSampleWhenDone);
            return;
        }
        setLoopsRemaining(m_loopCount);
        playSample();
    }

    setPlaying(true);
}

void QSoundEffectPrivate::emptyStream(EmptyStreamOptions options)
{
#ifdef QT_PA_DEBUG
    qDebug() << this << "emptyStream";
#endif
    if (!m_pulseStream || m_emptying)
        return;

    const bool reloadSample = options.testFlag(ReloadSampleWhenDone);
    pa_stream_success_cb_t flushCompleteCb = reloadSample ? stream_flush_reload_callback
                                                          : stream_flush_callback;

    PulseDaemonLocker locker;

    m_emptying = true;
    pa_stream_set_write_callback(m_pulseStream, nullptr, nullptr);
    pa_stream_set_underflow_callback(m_pulseStream, nullptr, nullptr);
    pa_operation *op = pa_stream_flush(m_pulseStream, flushCompleteCb, m_ref->getRef());
    if (op)
        pa_operation_unref(op);
    else
        qWarning("QSoundEffect(pulseaudio): failed to flush stream");
}

void QSoundEffectPrivate::emptyComplete(void *stream, bool reload)
{
#ifdef QT_PA_DEBUG
    qDebug() << this << "emptyComplete";
#endif

    PulseDaemonLocker locker;

    m_emptying = false;

    if ((pa_stream *)stream == m_pulseStream) {
        pa_operation *op = pa_stream_cork(m_pulseStream, 1,
                                          reload ? stream_cork_callback : nullptr, m_ref->getRef());
        if (op)
            pa_operation_unref(op);
        else
            qWarning("QSoundEffect(pulseaudio): failed to cork stream");
    }
}

void QSoundEffectPrivate::sampleReady()
{
    PulseDaemonLocker locker;

    // The slot might be called right after a new call to setSource().
    // In this case, the sample has been reset and the slot is being called for the previous sample.
    // Just ignore it.
    if (Q_UNLIKELY(!m_sample || m_sample->state() != QSample::Ready))
        return;

#ifdef QT_PA_DEBUG
    qDebug() << this << "sampleReady";
#endif
    disconnect(m_sample, &QSample::error, this, &QSoundEffectPrivate::decoderError);
    disconnect(m_sample, &QSample::ready, this, &QSoundEffectPrivate::sampleReady);
    pa_sample_spec newFormatSpec = audioFormatToSampleSpec(m_sample->format());

    if (m_pulseStream && !pa_sample_spec_equal(&m_pulseSpec, &newFormatSpec)) {
        unloadPulseStream();
    }
    m_pulseSpec = newFormatSpec;

    m_sampleReady = true;
    m_position = 0;

    if (m_name.isNull())
        m_name = QString(QLatin1String("QtPulseSample-%1-%2")).arg(::getpid()).arg(quintptr(this)).toUtf8();

    if (m_pulseStream && pa_stream_get_state(m_pulseStream) == PA_STREAM_READY) {
#ifdef QT_PA_DEBUG
        qDebug() << this << "reuse existing pulsestream";
#endif
        const pa_buffer_attr *bufferAttr = pa_stream_get_buffer_attr(m_pulseStream);
        if (bufferAttr->prebuf > uint32_t(m_sample->data().size())) {
            pa_buffer_attr newBufferAttr;
            newBufferAttr = *bufferAttr;
            newBufferAttr.prebuf = m_sample->data().size();
            pa_operation *op = pa_stream_set_buffer_attr(m_pulseStream, &newBufferAttr, stream_adjust_prebuffer_callback, m_ref->getRef());
            if (op)
                pa_operation_unref(op);
            else
                qWarning("QSoundEffect(pulseaudio): failed to adjust pre-buffer attribute");
        } else {
            streamReady();
        }
    } else if (!m_pulseStream) {
        if (!pulseDaemon()->context() || pa_context_get_state(pulseDaemon()->context()) != PA_CONTEXT_READY) {
            connect(pulseDaemon(), &PulseDaemon::contextReady,
                    this, &QSoundEffectPrivate::contextReady);
            return;
        }
        createPulseStream();
    }
}

void QSoundEffectPrivate::decoderError()
{
    qWarning("QSoundEffect(pulseaudio): Error decoding source %ls", qUtf16Printable(m_source.toString()));
    disconnect(m_sample, &QSample::error, this, &QSoundEffectPrivate::decoderError);
    bool playingDirty = false;
    if (m_playing) {
        m_playing = false;
        playingDirty = true;
    }
    setStatus(QSoundEffect::Error);
    if (playingDirty)
        emit playingChanged();
}

void QSoundEffectPrivate::unloadPulseStream()
{
#ifdef QT_PA_DEBUG
    qDebug() << this << "unloadPulseStream";
#endif
    m_sinkInputId = -1;
    PulseDaemonLocker locker;
    if (m_pulseStream) {
        pa_stream_set_state_callback(m_pulseStream, nullptr, nullptr);
        pa_stream_set_write_callback(m_pulseStream, nullptr, nullptr);
        pa_stream_set_underflow_callback(m_pulseStream, nullptr, nullptr);
        pa_stream_disconnect(m_pulseStream);
        pa_stream_unref(m_pulseStream);
        disconnect(pulseDaemon(), &PulseDaemon::contextFailed,
                   this, &QSoundEffectPrivate::contextFailed);
        m_pulseStream = nullptr;
        m_reloadCategory = false; // category will be reloaded when we connect anyway
    }
}

void QSoundEffectPrivate::prepare()
{
    if (!m_pulseStream || !m_sampleReady)
        return;
    PulseDaemonLocker locker;

    if (pa_stream_get_state(m_pulseStream) != PA_STREAM_READY)
        return;

    pa_stream_set_write_callback(m_pulseStream, stream_write_callback, this);
    pa_stream_set_underflow_callback(m_pulseStream, stream_underrun_callback, this);
    m_stopping = false;
    size_t writeBytes = size_t(qMin(m_pulseBufferSize, m_sample->data().size()));
#ifdef QT_PA_DEBUG
    qDebug() << this << "prepare(): writable size =" << pa_stream_writable_size(m_pulseStream)
             << "actual writeBytes =" << writeBytes
             << "m_playQueued =" << m_playQueued;
#endif
    m_position = writeToStream(m_sample->data().data(), writeBytes);

    if (m_playQueued) {
        m_playQueued = false;
        setLoopsRemaining(m_loopCount);
        playSample();
    }
}

void QSoundEffectPrivate::uploadSample()
{
    // Always called on PulseAudio thread

    if (m_runningCount == 0) {
#ifdef QT_PA_DEBUG
    qDebug() << this << "uploadSample: return due to 0 m_runningCount";
#endif
        return;
    }

    if (Q_UNLIKELY(!m_pulseStream
                   || pa_stream_get_state(m_pulseStream) != PA_STREAM_READY
                   || !m_sampleReady)) {
        return;
    }

#ifdef QT_PA_DEBUG
    qDebug() << this << "uploadSample: m_runningCount =" << m_runningCount;
#endif
    if (m_position == m_sample->data().size()) {
        m_position = 0;
        if (m_runningCount > 0)
            setLoopsRemaining(m_runningCount - 1);
        if (m_runningCount == 0) {
            return;
        }
    }

    int writableSize = int(pa_stream_writable_size(m_pulseStream));
    int firstPartLength = qMin(m_sample->data().size() - m_position, writableSize);

    int writtenBytes = writeToStream(m_sample->data().data() + m_position,
                                     firstPartLength);

    m_position += writtenBytes;
    if (m_position == m_sample->data().size()) {
        m_position = 0;
        if (m_runningCount > 0)
            setLoopsRemaining(m_runningCount - 1);
        if (m_runningCount != 0 && firstPartLength < writableSize)
        {
            while (writtenBytes < writableSize) {
                int writeSize = qMin(writableSize - writtenBytes, m_sample->data().size());
                writtenBytes += writeToStream(m_sample->data().data(), writeSize);

                if (writeSize < m_sample->data().size()) {
                    m_position = writeSize;
                    break;
                }
                if (m_runningCount > 0)
                    setLoopsRemaining(m_runningCount - 1);
                if (m_runningCount == 0)
                    break;
            }
        }
    }
#ifdef QT_PA_DEBUG
    qDebug() << this << "uploadSample: use direct write, writeable size =" << writableSize
             << "actual writtenBytes =" << writtenBytes;
#endif
}

int QSoundEffectPrivate::writeToStream(const void *data, int size)
{
    // Always called on PulseAudio thread

    if (size < 1)
        return 0;

    m_volumeLock.lock();
    qreal volume = m_muted ? 0 : m_volume;
    m_volumeLock.unlock();
    pa_free_cb_t writeDoneCb = stream_write_done_callback;

    if (volume < 1.0f) {
        // Don't use PulseAudio volume, as it might affect all other streams of the same category
        // or even affect the system volume if flat volumes are enabled
        void *dest = nullptr;
        size_t nbytes = size;
        if (pa_stream_begin_write(m_pulseStream, &dest, &nbytes) < 0) {
            qWarning("QSoundEffect(pulseaudio): pa_stream_begin_write, error = %s",
                     pa_strerror(pa_context_errno(pulseDaemon()->context())));
            return 0;
        }

        size = int(nbytes);
        QAudioHelperInternal::qMultiplySamples(volume, m_sample->format(), data, dest, size);
        data = dest;
        writeDoneCb = nullptr;
    }

    if (pa_stream_write(m_pulseStream, data, size, writeDoneCb, 0, PA_SEEK_RELATIVE) < 0) {
        qWarning("QSoundEffect(pulseaudio): pa_stream_write, error = %s",
                 pa_strerror(pa_context_errno(pulseDaemon()->context())));
        return 0;
    }

    return size;
}

void QSoundEffectPrivate::playSample()
{
    PulseDaemonLocker locker;

#ifdef QT_PA_DEBUG
    qDebug() << this << "playSample";
#endif
    Q_ASSERT(m_pulseStream);
    Q_ASSERT(pa_stream_get_state(m_pulseStream) == PA_STREAM_READY);
    pa_operation *o = pa_stream_cork(m_pulseStream, 0, nullptr, nullptr);
    if (o)
        pa_operation_unref(o);
}

void QSoundEffectPrivate::stop()
{
#ifdef QT_PA_DEBUG
    qDebug() << this << "stop";
#endif
    if (!m_playing)
        return;

    PulseDaemonLocker locker;

    setPlaying(false);

    m_stopping = true;
    if (m_pulseStream) {
        emptyStream(ReloadSampleWhenDone);
        if (m_reloadCategory) {
            unloadPulseStream(); // upon play we reconnect anyway
        }
    }
    setLoopsRemaining(0);
    m_position = 0;
    m_playQueued = false;
    m_reloadCategory = false;
}

void QSoundEffectPrivate::underRun()
{
    stop();
}

void QSoundEffectPrivate::streamReady()
{
    PulseDaemonLocker locker;

    if (Q_UNLIKELY(!m_sample || m_sample->state() != QSample::Ready
                   || !m_pulseStream || pa_stream_get_state(m_pulseStream) != PA_STREAM_READY)) {
        return;
    }

#ifdef QT_PA_DEBUG
    qDebug() << this << "streamReady";
#endif

    m_sinkInputId =  pa_stream_get_index(m_pulseStream);
#ifdef QT_PA_DEBUG
    const pa_buffer_attr *realBufAttr = pa_stream_get_buffer_attr(m_pulseStream);
    qDebug() << this << "m_sinkInputId =" << m_sinkInputId
             << "tlength =" << realBufAttr->tlength << "maxlength =" << realBufAttr->maxlength
             << "minreq = " << realBufAttr->minreq << "prebuf =" << realBufAttr->prebuf;
#endif
    prepare();
    setStatus(QSoundEffect::Ready);
}

void QSoundEffectPrivate::createPulseStream()
{
#ifdef QT_PA_DEBUG
    qDebug() << this << "createPulseStream";
#endif

    if (!pulseDaemon()->context())
        return;

    pa_proplist *propList = pa_proplist_new();
    if (!m_category.isNull())
        pa_proplist_sets(propList, PA_PROP_MEDIA_ROLE, m_category.toLatin1().constData());
    pa_stream *stream = pa_stream_new_with_proplist(pulseDaemon()->context(), m_name.constData(),
                                                    &m_pulseSpec, nullptr, propList);
    pa_proplist_free(propList);

    connect(pulseDaemon(), &PulseDaemon::contextFailed,
            this, &QSoundEffectPrivate::contextFailed);

    if (stream == nullptr) {
        qWarning("QSoundEffect(pulseaudio): Failed to create stream");
        m_pulseStream = nullptr;
        setStatus(QSoundEffect::Error);
        setPlaying(false);
        return;
    }
    else {
        pa_stream_set_state_callback(stream, stream_state_callback, this);
        pa_stream_set_write_callback(stream, stream_write_callback, this);
        pa_stream_set_underflow_callback(stream, stream_underrun_callback, this);
    }
    m_pulseStream = stream;

    if (pa_stream_connect_playback(m_pulseStream, nullptr, nullptr,
                                   PA_STREAM_START_CORKED, nullptr, nullptr) < 0) {
        qWarning("QSoundEffect(pulseaudio): Failed to connect stream, error = %s",
                 pa_strerror(pa_context_errno(pulseDaemon()->context())));
    }
}

void QSoundEffectPrivate::contextReady()
{
    disconnect(pulseDaemon(), &PulseDaemon::contextReady,
               this, &QSoundEffectPrivate::contextReady);
    PulseDaemonLocker locker;
    createPulseStream();
}

void QSoundEffectPrivate::contextFailed()
{
    unloadPulseStream();
    connect(pulseDaemon(), &PulseDaemon::contextReady,
            this, &QSoundEffectPrivate::contextReady);
}

void QSoundEffectPrivate::stream_write_callback(pa_stream *s, size_t length, void *userdata)
{
    Q_UNUSED(length);
    Q_UNUSED(s);

    QSoundEffectPrivate *self = reinterpret_cast<QSoundEffectPrivate*>(userdata);
#ifdef QT_PA_DEBUG
    qDebug() << self << "stream_write_callback";
#endif
    self->uploadSample();
}

void QSoundEffectPrivate::stream_state_callback(pa_stream *s, void *userdata)
{
    QSoundEffectPrivate *self = reinterpret_cast<QSoundEffectPrivate*>(userdata);
    switch (pa_stream_get_state(s)) {
        case PA_STREAM_READY:
        {
#ifdef QT_PA_DEBUG
            qDebug() << self << "pulse stream ready";
#endif
            if (Q_UNLIKELY(!self->m_sample || self->m_sample->state() != QSample::Ready))
                return;

            const pa_buffer_attr *bufferAttr = pa_stream_get_buffer_attr(self->m_pulseStream);
            self->m_pulseBufferSize = bufferAttr->tlength;
            if (bufferAttr->prebuf > uint32_t(self->m_sample->data().size())) {
                pa_buffer_attr newBufferAttr;
                newBufferAttr = *bufferAttr;
                newBufferAttr.prebuf = self->m_sample->data().size();
                pa_operation *op = pa_stream_set_buffer_attr(self->m_pulseStream, &newBufferAttr, stream_adjust_prebuffer_callback, self->m_ref->getRef());
                if (op)
                    pa_operation_unref(op);
                else
                    qWarning("QSoundEffect(pulseaudio): failed to adjust pre-buffer attribute");
            } else {
                QMetaObject::invokeMethod(self, "streamReady", Qt::QueuedConnection);
            }
            break;
        }
        case PA_STREAM_CREATING:
#ifdef QT_PA_DEBUG
            qDebug() << self << "pulse stream creating";
#endif
            break;
        case PA_STREAM_TERMINATED:
#ifdef QT_PA_DEBUG
            qDebug() << self << "pulse stream terminated";
#endif
            break;

        case PA_STREAM_FAILED:
        default:
            qWarning("QSoundEffect(pulseaudio): Error in pulse audio stream");
            break;
    }
}

void QSoundEffectPrivate::stream_adjust_prebuffer_callback(pa_stream *s, int success, void *userdata)
{
#ifdef QT_PA_DEBUG
    qDebug() << "stream_adjust_prebuffer_callback";
#endif
    Q_UNUSED(s);
    QSoundEffectRef *ref = reinterpret_cast<QSoundEffectRef*>(userdata);
    QSoundEffectPrivate *self = ref->soundEffect();
    ref->release();
    if (!self)
        return;

    if (!success)
        qWarning("QSoundEffect(pulseaudio): failed to adjust pre-buffer attribute");
#ifdef QT_PA_DEBUG
    qDebug() << self << "stream_adjust_prebuffer_callback";
#endif
    QMetaObject::invokeMethod(self, "streamReady", Qt::QueuedConnection);
}

void QSoundEffectPrivate::stream_underrun_callback(pa_stream *s, void *userdata)
{
    Q_UNUSED(s);
    QSoundEffectPrivate *self = reinterpret_cast<QSoundEffectPrivate*>(userdata);
#ifdef QT_PA_DEBUG
    qDebug() << self << "stream_underrun_callback";
#endif
    if (self->m_runningCount == 0 && !self->m_playQueued)
        QMetaObject::invokeMethod(self, "underRun", Qt::QueuedConnection);
#ifdef QT_PA_DEBUG
    else
        qDebug() << "underun corked =" << pa_stream_is_corked(s);
#endif
}

void QSoundEffectPrivate::stream_cork_callback(pa_stream *s, int success, void *userdata)
{
#ifdef QT_PA_DEBUG
    qDebug() << "stream_cork_callback";
#endif
    Q_UNUSED(s);
    QSoundEffectRef *ref = reinterpret_cast<QSoundEffectRef*>(userdata);
    QSoundEffectPrivate *self = ref->soundEffect();
    ref->release();
    if (!self)
        return;

    if (!success)
        qWarning("QSoundEffect(pulseaudio): failed to stop");
#ifdef QT_PA_DEBUG
    qDebug() << self << "stream_cork_callback";
#endif
    QMetaObject::invokeMethod(self, "prepare", Qt::QueuedConnection);
}

void QSoundEffectPrivate::stream_flush_callback(pa_stream *s, int success, void *userdata)
{
#ifdef QT_PA_DEBUG
    qDebug() << "stream_flush_callback";
#endif
    Q_UNUSED(s);
    QSoundEffectRef *ref = reinterpret_cast<QSoundEffectRef*>(userdata);
    QSoundEffectPrivate *self = ref->soundEffect();
    ref->release();
    if (!self)
        return;

    if (!success)
        qWarning("QSoundEffect(pulseaudio): failed to drain");

    QMetaObject::invokeMethod(self, "emptyComplete", Qt::QueuedConnection, Q_ARG(void*, s), Q_ARG(bool, false));
}

void QSoundEffectPrivate::stream_flush_reload_callback(pa_stream *s, int success, void *userdata)
{
#ifdef QT_PA_DEBUG
    qDebug() << "stream_flush_reload_callback";
#endif
    Q_UNUSED(s);
    QSoundEffectRef *ref = reinterpret_cast<QSoundEffectRef*>(userdata);
    QSoundEffectPrivate *self = ref->soundEffect();
    ref->release();
    if (!self)
        return;

    if (!success)
        qWarning("QSoundEffect(pulseaudio): failed to drain");

    QMetaObject::invokeMethod(self, "emptyComplete", Qt::QueuedConnection, Q_ARG(void*, s), Q_ARG(bool, true));
}

void QSoundEffectPrivate::stream_write_done_callback(void *p)
{
    Q_UNUSED(p);
#ifdef QT_PA_DEBUG
    qDebug() << "stream_write_done_callback";
#endif
}

QT_END_NAMESPACE

#include "moc_qsoundeffect_pulse_p.cpp"
#include "qsoundeffect_pulse_p.moc"
