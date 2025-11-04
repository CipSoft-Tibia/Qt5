// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:data-parser, execute-external-code

#include "qtexttospeech_flite_processor.h"
#include "qtexttospeech_flite_plugin.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qlocale.h>
#include <QtCore/qmap.h>
#include <QtCore/qprocessordetection.h>
#include <QtCore/qspan.h>
#include <QtCore/qstring.h>

#include <thread>

#include <flite/flite.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace {

void setRateForVoice(cst_voice *voice, float rate)
{
    float stretch = 1.0;
    Q_ASSERT(rate >= -1.0 && rate <= 1.0);
    // Stretch multipliers taken from Speech Dispatcher
    if (rate < 0)
        stretch -= rate * 2;
    if (rate > 0)
        stretch -= rate * (100.0f / 175.0f);
    feat_set_float(voice->features, "duration_stretch", stretch);
}

void setPitchForVoice(cst_voice *voice, float pitch)
{
    float f0;
    Q_ASSERT(pitch >= -1.0 && pitch <= 1.0);
    // Conversion taken from Speech Dispatcher
    f0 = (pitch * 80) + 100;
    feat_set_float(voice->features, "int_f0_target_mean", f0);
}

// Read available flite voices
QStringList fliteAvailableVoices(const QString &libPrefix, const QString &langCode)
{
    // Read statically linked voices
    QStringList voices;
    for (const cst_val *v = flite_voice_list; v; v = val_cdr(v)) {
        cst_voice *voice = val_voice(val_car(v));
        voices.append(voice->name);
    }

    // Read available libraries
    static const QStringList ldPaths = [] {
        const QProcessEnvironment pe;
        QStringList ldPaths = pe.value(u"LD_LIBRARY_PATH"_s).split(u":"_s, Qt::SkipEmptyParts);
        if (ldPaths.isEmpty()) {
            ldPaths = QStringList{
                // Fedora-style lib64 library paths
                u"/usr/lib64"_s,
                u"/usr/local/lib64"_s,
                u"/lib64"_s,

                // Debian-style multi-arch library paths
#if defined(Q_PROCESSOR_ARM_V8)
#  if defined(__MUSL__)
                u"/usr/lib/aarch64-linux-musl"_s,
#  else
                u"/usr/lib/aarch64-linux-gnu"_s,
#  endif
#elif defined(Q_PROCESSOR_ARM_V7)
#  if defined(__MUSL__)
                u"/usr/lib/arm-linux-musleabihf"_s,
#  else
#    if defined(__ARM_PCS_VFP)
                u"/usr/lib/arm-linux-gnueabihf"_s,
#    else
                u"/usr/lib/arm-linux-gnueabi"_s,
#    endif
#  endif
#elif defined(Q_PROCESSOR_X86_64)
                u"/usr/lib/x86_64-linux-gnu"_s,
#elif defined(Q_PROCESSOR_X86)
                u"/usr/lib/i686-linux-gnu"_s,
                u"/usr/lib/i386-linux-gnu"_s,
#endif

                // generic paths
                u"/usr/lib"_s,
                u"/usr/local/lib"_s,
                u"/lib"_s,
            };
        } else {
            ldPaths.removeDuplicates();
        }

        ldPaths.removeIf([](const QString &path) {
            QDir dir(path);
            return !dir.isReadable() || dir.isEmpty();
        });

        qCDebug(lcSpeechTtsFlite) << "QTextToSpeechProcessorFlite: initialized voice paths to"
                                  << ldPaths;

        return ldPaths;
    }();

    const QString libPattern = QString(u"lib"_s + libPrefix).arg(langCode).arg("*"_L1);
    for (const auto &path : ldPaths) {
        QDir dir(path);
        dir.setNameFilters({ libPattern });
        dir.setFilter(QDir::Files);
        const QFileInfoList fileList = dir.entryInfoList();
        for (const auto &file : fileList) {
            QString vox = file.fileName().mid(16, file.fileName().indexOf(u'.') - 16);
            voices.append(std::move(vox));
        }
    }

    voices.removeDuplicates();
    return voices;
}

QAudioFormat getAudioFormat(const cst_wave &w)
{
    QAudioFormat fmt;
    fmt.setSampleFormat(QAudioFormat::Int16);
    fmt.setSampleRate(w.sample_rate);
    fmt.setChannelCount(w.num_channels);
    fmt.setChannelConfig(QAudioFormat::defaultChannelConfigForChannelCount(w.num_channels));
    return fmt;
}

} // namespace

QTextToSpeechProcessorFlite::QTextToSpeechProcessorFlite(const QAudioDevice &audioDevice)
    : m_audioDevice(audioDevice)
{
    init();
}

QTextToSpeechProcessorFlite::~QTextToSpeechProcessorFlite()
{
    for (const VoiceInfo &voice : std::as_const(m_voices))
        voice.unregister_func(voice.vox);
}

const QList<QTextToSpeechProcessorFlite::VoiceInfo> &QTextToSpeechProcessorFlite::voices() const
{
    return m_voices;
}

int QTextToSpeechProcessorFlite::audioOutputCb(const cst_wave *w, int start, int size,
                                               int last, cst_audio_streaming_info *asi)
{
    auto *processor = static_cast<QTextToSpeechProcessorFlite *>(asi->userdata);
    Q_ASSERT(processor);

    if (!asi->item)
        asi->item = relation_head(utt_relation(asi->utt, "Token"));

    const float tokenStartTime = flite_ffeature_float(
            asi->item, "R:Token.daughter1.R:SylStructure.daughter1.daughter1.R:Segment.p.end");
    const int tokenStartSample = int(tokenStartTime * float(w->sample_rate));
    if ((tokenStartSample >= start) && (tokenStartSample < start + size)) {
        // a new token starts in this chunk
        processor->audioHandleNewToken(
                std::chrono::milliseconds(std::lround(tokenStartTime * 1000)), asi);
        asi->item = item_next(asi->item);
    }
    return processor->audioOutput(w, start, size, last, asi);
}

int QTextToSpeechProcessorFlite::audioOutput(const cst_wave *w, int start, int size, int last,
                                             cst_audio_streaming_info *)
{
    Q_ASSERT(QThread::currentThread() == thread());
    if (size == 0)
        return CST_AUDIO_STREAM_CONT;
    if (start == 0 && !initAudio(w))
        return CST_AUDIO_STREAM_STOP;

    QSpan fliteStream{ w->samples + start, size };
    QSpan fliteBytes = as_bytes(fliteStream);

    using namespace std::chrono_literals;

    std::optional<std::chrono::steady_clock::time_point> startTime;
    qsizetype totalBytesWritten = 0;

    auto handleStreamingError = [&] {
        setError(QTextToSpeech::ErrorReason::Playback,
                 QCoreApplication::translate("QTextToSpeech", "Audio streaming error."));
        stop();
        return CST_AUDIO_STREAM_STOP;
    };

    while (!fliteBytes.isEmpty()) {
        qsizetype bytesWritten = m_audioIODevice->write(
                reinterpret_cast<const char *>(fliteBytes.data()), fliteBytes.size());

        if (bytesWritten < 0) // something really went wrong
            return handleStreamingError();

        totalBytesWritten += bytesWritten;
        if (bytesWritten == fliteBytes.size())
            break;

        if (bytesWritten)
            fliteBytes = fliteBytes.subspan(bytesWritten); // ranges::drop

        // we could not write (all) data to the QIODevice. Back off and retry for 5 seconds before
        // we give up. We cannot query the state of the QAudioSink here, as that would require event
        // loop interaction.
        constexpr auto timeout = 5s;

        if (!startTime)
            startTime = std::chrono::steady_clock::now();
        else if (std::chrono::steady_clock::now() - *startTime > timeout)
            return handleStreamingError();

        std::this_thread::sleep_for(5ms);
    }

    // Stats for debugging
    ++numberChunks;
    totalBytes += totalBytesWritten;

    if (last == 1) {
        qCDebug(lcSpeechTtsFlite) << "last data chunk written";
        m_audioIODevice->close();
    }
    return CST_AUDIO_STREAM_CONT;
}

void QTextToSpeechProcessorFlite::audioHandleNewToken(std::chrono::milliseconds tokenStartTime,
                                                      cst_audio_streaming_info *asi)
{
    auto normalizeFeatureString = [&](const char *feature) -> const char * {
        const char *featureString = flite_ffeature_string(asi->item, feature);
        if (cst_streq("0", featureString))
            return "";
        return featureString;
    };

    const char *token = flite_ffeature_string(asi->item, "name");
    if (!token) {
        Q_UNLIKELY_BRANCH;
        qCWarning(lcSpeechTtsFlite) << "No token found, skipping";
        return;
    }

    qCDebug(lcSpeechTtsFlite).nospace()
            << "Processing token start_time: " << tokenStartTime << " content: \""
            << flite_ffeature_string(asi->item, "whitespace")
            << normalizeFeatureString("prepunctuation") << "'" << token << "'"
            << normalizeFeatureString("punc") << "\"";

    QString currentToken = QString::fromUtf8(token);
    m_index = m_text.indexOf(currentToken, m_index);
    emit sayingWord(currentToken, m_index, currentToken.length());
}

int QTextToSpeechProcessorFlite::dataOutputCb(const cst_wave *w, int start, int size,
                                              int last, cst_audio_streaming_info *asi)
{
    auto *processor = static_cast<QTextToSpeechProcessorFlite *>(asi->userdata);
    Q_ASSERT(processor);
    return processor->dataOutput(w, start, size, last, asi);
}

int QTextToSpeechProcessorFlite::dataOutput(const cst_wave *w, int start, int size,
                                            int last, cst_audio_streaming_info *)
{
    if (start == 0)
        emit stateChanged(QTextToSpeech::Synthesizing);

    if (!m_synthesisFormat) {
        QAudioFormat format = getAudioFormat(*w);
        if (!format.isValid())
            return CST_AUDIO_STREAM_STOP;
        m_synthesisFormat = format;
    }

    const qsizetype bytesToWrite = size * m_synthesisFormat->bytesPerSample();
    emit synthesized(*m_synthesisFormat,
                     QByteArray(reinterpret_cast<const char *>(&w->samples[start]), bytesToWrite));

    if (last == 1)
        emit stateChanged(QTextToSpeech::Ready);

    return CST_AUDIO_STREAM_CONT;
}

void QTextToSpeechProcessorFlite::processText(const QString &text, int voiceId, float pitch,
                                              float rate, OutputHandler outputHandler)
{
    qCDebug(lcSpeechTtsFlite) << "processText() begin";
    if (!checkVoice(voiceId))
        return;

    m_text = text;
    m_index = 0;
    float secsToSpeak = -1;
    const VoiceInfo &voiceInfo = m_voices.at(voiceId);
    cst_voice *voice = voiceInfo.vox;
    cst_audio_streaming_info *asi = new_audio_streaming_info();
    asi->asc = outputHandler;
    asi->userdata = (void *)this;
    feat_set(voice->features, "streaming_info", audio_streaming_info_val(asi));
    setRateForVoice(voice, rate);
    setPitchForVoice(voice, pitch);
    secsToSpeak = flite_text_to_speech(text.toUtf8().constData(), voice, "none");

    if (secsToSpeak <= 0) {
        setError(QTextToSpeech::ErrorReason::Input,
                 QCoreApplication::translate("QTextToSpeech", "Speech synthesizing failure."));
        return;
    }

    qCDebug(lcSpeechTtsFlite) << "processText() end" << secsToSpeak << "Seconds";
}

typedef cst_voice*(*registerFnType)();
typedef void(*unregisterFnType)(cst_voice *);

bool QTextToSpeechProcessorFlite::init()
{
    flite_init();

    const QLocale locale(QLocale::English, QLocale::UnitedStates);
    // ### FIXME: hardcode for now, the only voice files we know about are for en_US
    // We could source the language and perhaps the list of voices we want to load
    // (hardcoded below) from an environment variable.
    const QString langCode(u"us"_s);
    const QString libPrefix(u"flite_cmu_%1_%2.so.1"_s);
    const QString registerPrefix(u"register_cmu_%1_%2"_s);
    const QString unregisterPrefix(u"unregister_cmu_%1_%2"_s);

    for (const auto &voice : fliteAvailableVoices(libPrefix, langCode)) {
        QLibrary library(libPrefix.arg(langCode, voice));
        if (!library.load()) {
            qWarning("Voice library could not be loaded: %s", qPrintable(library.fileName()));
            continue;
        }
        auto registerFn = reinterpret_cast<registerFnType>(library.resolve(
            registerPrefix.arg(langCode, voice).toLatin1().constData()));
        auto unregisterFn = reinterpret_cast<unregisterFnType>(library.resolve(
            unregisterPrefix.arg(langCode, voice).toLatin1().constData()));
        if (registerFn && unregisterFn) {
            const int id = int(m_voices.count());
            m_voices.append(VoiceInfo{
                id,
                registerFn(),
                unregisterFn,
                voice,
                locale.name(),
                QVoice::Male,
                QVoice::Adult
            });
        } else {
            library.unload();
        }
    }

    return !m_voices.isEmpty();
}

bool QTextToSpeechProcessorFlite::initAudio(const cst_wave *w)
{
    m_format = getAudioFormat(*w);
    if (!checkFormat(m_format))
       return false;

    createSink();

    return bool(m_audioSink);
}

void QTextToSpeechProcessorFlite::deleteSink()
{
    if (m_audioSink) {
        m_audioSink->disconnect();
        delete m_audioSink;
        m_audioSink = nullptr;
        m_audioIODevice = nullptr;
    }
}

void QTextToSpeechProcessorFlite::createSink()
{
    using namespace std::chrono;
    // Create new sink if none exists or the format has changed
    if (!m_audioSink || (m_audioSink->format() != m_format)) {
        // No signals while we create new sink with QIODevice
        const bool sigs = signalsBlocked();
        auto resetSignals = qScopeGuard([this, sigs](){ blockSignals(sigs); });
        blockSignals(true);
        deleteSink();
        m_audioSink = new QAudioSink(m_audioDevice, m_format, this);
        m_audioSink->setVolume(m_volume);
        constexpr auto bufferDuration = milliseconds(100);
        m_audioSink->setBufferSize(m_format.bytesForDuration(microseconds(bufferDuration).count()));
        connect(m_audioSink, &QAudioSink::stateChanged, this,
                &QTextToSpeechProcessorFlite::changeState);
        connect(QThread::currentThread(), &QThread::finished, m_audioSink, &QObject::deleteLater);
    } else {
        // stop before we can restart with a new QIODevice
        m_audioSink->reset();
    }

    m_audioIODevice = m_audioSink->start();
    if (!m_audioIODevice) {
        deleteSink();
        setError(QTextToSpeech::ErrorReason::Playback,
                 QCoreApplication::translate("QTextToSpeech", "Audio Open error: No I/O device available."));
    }

    numberChunks = 0;
    totalBytes = 0;
}

// Wrapper for QAudioSink::stateChanged, bypassing early idle bug
void QTextToSpeechProcessorFlite::changeState(QAudio::State newState)
{
    if (m_state == newState)
        return;

    qCDebug(lcSpeechTtsFlite) << "Audio sink state transition" << m_state << newState;

    m_state = newState;
    const QTextToSpeech::State ttsState = audioStateToTts(newState);
    emit stateChanged(ttsState);
}

void QTextToSpeechProcessorFlite::setError(QTextToSpeech::ErrorReason err, const QString &errorString)
{
     if (err == QTextToSpeech::ErrorReason::NoError) {
        changeState(QAudio::IdleState);
        return;
     }

     qCDebug(lcSpeechTtsFlite) << "Error" << err << errorString;
     emit stateChanged(QTextToSpeech::Error);
     emit errorOccurred(err, errorString);
}

constexpr QTextToSpeech::State QTextToSpeechProcessorFlite::audioStateToTts(QAudio::State AudioState)
{
    switch (AudioState) {
    case QAudio::ActiveState:
        return QTextToSpeech::Speaking;
    case QAudio::IdleState:
        return QTextToSpeech::Ready;
    case QAudio::SuspendedState:
        return QTextToSpeech::Paused;
    case QAudio::StoppedState:
        return QTextToSpeech::Ready;
    }
    Q_UNREACHABLE();
}

void QTextToSpeechProcessorFlite::deinitAudio()
{
    m_index = -1;
    deleteSink();
}

// Check format/device and set corresponding error messages
bool QTextToSpeechProcessorFlite::checkFormat(const QAudioFormat &format)
{
    auto streamToString = [](auto &&arg) {
        QString string;
        QDebug(&string) << arg;
        return string;
    };

    bool formatOK = true;

    // Format must be valid
    if (!format.isValid()) {
        formatOK = false;
        setError(QTextToSpeech::ErrorReason::Playback,
                 QCoreApplication::translate("QTextToSpeech", "Invalid audio format: %1")
                         .arg(streamToString(format)));
    }

    // Device must exist
    if (m_audioDevice.isNull()) {
        formatOK = false;
        setError(QTextToSpeech::ErrorReason::Playback,
                 QCoreApplication::translate("QTextToSpeech", "No audio device specified."));
    }

    // Device must support requested format
    if (!m_audioDevice.isFormatSupported(format)) {
        formatOK = false;
        setError(QTextToSpeech::ErrorReason::Playback,
                 QCoreApplication::translate("QTextToSpeech",
                                             "Audio device does not support format: %1")
                         .arg(streamToString(format)));
    }

    return formatOK;
}

// Check voice validity
bool QTextToSpeechProcessorFlite::checkVoice(int voiceId)
{
    if (voiceId >= 0 && voiceId < m_voices.size())
        return true;

    setError(QTextToSpeech::ErrorReason::Configuration,
             QCoreApplication::translate("QTextToSpeech", "Invalid voiceId %1.").arg(voiceId));
    return false;
}

// Wrap QAudioSink::state and compensate early idle bug
QAudio::State QTextToSpeechProcessorFlite::audioSinkState() const
{
    return (m_audioSink) ? m_state : QAudio::StoppedState;
}

// Stop current and cancel subsequent utterances
void QTextToSpeechProcessorFlite::stop()
{
    if (audioSinkState() == QAudio::ActiveState || audioSinkState() == QAudio::SuspendedState) {
        deinitAudio();
        // Call manual state change as audio sink has been deleted
        changeState(QAudio::StoppedState);
    }
}

void QTextToSpeechProcessorFlite::pause()
{
    if (audioSinkState() == QAudio::ActiveState)
        m_audioSink->suspend();
}

void QTextToSpeechProcessorFlite::resume()
{
    if (audioSinkState() == QAudio::SuspendedState) {
        m_audioSink->resume();
        // QAudioSink in push mode transitions to Idle when resumed, even if
        // there is still data to play. Workaround this weird behavior if we
        // know we are not done yet.
        changeState(QAudio::ActiveState);
    }
}

void QTextToSpeechProcessorFlite::say(const QString &text, int voiceId, double pitch, double rate, double volume)
{
    if (text.isEmpty())
        return;

    if (!checkVoice(voiceId))
        return;

    m_volume = volume;
    processText(text, voiceId, float(pitch), float(rate),
                QTextToSpeechProcessorFlite::audioOutputCb);
}

void QTextToSpeechProcessorFlite::synthesize(const QString &text, int voiceId, double pitch, double rate, double volume)
{
    if (text.isEmpty())
        return;

    if (!checkVoice(voiceId))
        return;

    m_synthesisFormat = std::nullopt;
    m_volume = volume;
    processText(text, voiceId, float(pitch), float(rate),
                QTextToSpeechProcessorFlite::dataOutputCb);
    m_synthesisFormat = std::nullopt;
}

QT_END_NAMESPACE
