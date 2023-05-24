// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only

#include "qtexttospeech_flite_processor.h"
#include "qtexttospeech_flite_plugin.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QString>
#include <QtCore/QLocale>
#include <QtCore/QMap>

#include <flite/flite.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

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

void QTextToSpeechProcessorFlite::startTokenTimer()
{
    qCDebug(lcSpeechTtsFlite) << "Starting token timer with" << m_tokens.count() - m_currentToken << "left";

    const TokenData &token = m_tokens.at(m_currentToken);
    const qint64 playedTime = m_audioSink->processedUSecs() / 1000;
    m_tokenTimer.start(qMax(token.startTime - playedTime, 0), Qt::PreciseTimer, this);
}

int QTextToSpeechProcessorFlite::audioOutputCb(const cst_wave *w, int start, int size,
                                               int last, cst_audio_streaming_info *asi)
{
    QTextToSpeechProcessorFlite *processor = static_cast<QTextToSpeechProcessorFlite *>(asi->userdata);
    if (processor) {
        if (asi->item == NULL)
            asi->item = relation_head(utt_relation(asi->utt,"Token"));

        const float startTime = flite_ffeature_float(asi->item, "R:Token.daughter1.R:SylStructure.daughter1.daughter1.R:Segment.p.end");
        const int startSample = int(startTime * float(w->sample_rate));
        if ((startSample >= start) && (startSample < start + size)) {
            const char *ws = flite_ffeature_string(asi->item, "whitespace");
            const char *prepunc = flite_ffeature_string(asi->item, "prepunctuation");
            if (cst_streq("0",prepunc))
                prepunc = "";
            const char *token = flite_ffeature_string(asi->item, "name");
            const char *postpunc = flite_ffeature_string(asi->item, "punc");
            if (cst_streq("0",postpunc))
                postpunc = "";
            if (token) {
                qCDebug(lcSpeechTtsFlite).nospace() << "Processing token start_time: " << startTime
                                                    << " content: \"" << ws << prepunc << "'" << token << "'" << postpunc << "\"";
                processor->m_tokens.append(TokenData{
                    qRound(startTime * 1000),
                    QString::fromUtf8(token)
                });
                if (!processor->m_tokenTimer.isActive())
                    processor->startTokenTimer();
            }
            asi->item = item_next(asi->item);
        }
        return processor->audioOutput(w, start, size, last, asi);
    }
    return CST_AUDIO_STREAM_STOP;
}

int QTextToSpeechProcessorFlite::audioOutput(const cst_wave *w, int start, int size,
                                             int last, cst_audio_streaming_info *asi)
{
    Q_UNUSED(asi);
    Q_ASSERT(QThread::currentThread() == thread());
    if (size == 0)
        return CST_AUDIO_STREAM_CONT;
    if (start == 0 && !initAudio(w->sample_rate, w->num_channels))
        return CST_AUDIO_STREAM_STOP;

    const qsizetype bytesToWrite = size * sizeof(short);

    if (!m_audioBuffer->write(reinterpret_cast<const char *>(&w->samples[start]), bytesToWrite)) {
        setError(QTextToSpeech::ErrorReason::Playback,
                 QCoreApplication::translate("QTextToSpeech", "Audio streaming error."));
        stop();
        return CST_AUDIO_STREAM_STOP;
    }

    // Stats for debugging
    ++numberChunks;
    totalBytes += bytesToWrite;

    if (last == 1) {
        qCDebug(lcSpeechTtsFlite) << "last data chunk written";
        m_audioBuffer->close();
    }
    return CST_AUDIO_STREAM_CONT;
}

int QTextToSpeechProcessorFlite::dataOutputCb(const cst_wave *w, int start, int size,
                                              int last, cst_audio_streaming_info *asi)
{
    QTextToSpeechProcessorFlite *processor = static_cast<QTextToSpeechProcessorFlite *>(asi->userdata);
    if (processor)
        return processor->dataOutput(w, start, size, last, asi);
    return CST_AUDIO_STREAM_STOP;
}

int QTextToSpeechProcessorFlite::dataOutput(const cst_wave *w, int start, int size,
                                            int last, cst_audio_streaming_info *)
{
    if (start == 0)
        emit stateChanged(QTextToSpeech::Synthesizing);

    QAudioFormat format;
    if (w->num_channels == 1)
        format.setChannelConfig(QAudioFormat::ChannelConfigMono);
    else
        format.setChannelCount(w->num_channels);
    format.setSampleRate(w->sample_rate);
    format.setSampleFormat(QAudioFormat::Int16);

    if (!format.isValid())
        return CST_AUDIO_STREAM_STOP;

    const qsizetype bytesToWrite = size * format.bytesPerSample();
    emit synthesized(format, QByteArray(reinterpret_cast<const char *>(&w->samples[start]), bytesToWrite));

    if (last == 1)
        emit stateChanged(QTextToSpeech::Ready);

    return CST_AUDIO_STREAM_CONT;
}

void QTextToSpeechProcessorFlite::timerEvent(QTimerEvent *event)
{
    if (event->timerId() != m_tokenTimer.timerId()) {
        QObject::timerEvent(event);
        return;
    }

    qCDebug(lcSpeechTtsFlite) << "Moving current token" << m_currentToken << m_tokens.size();
    auto currentToken = m_tokens.at(m_currentToken);
    m_index = m_text.indexOf(currentToken.text, m_index);
    emit sayingWord(currentToken.text, m_index, currentToken.text.length());
    m_index += currentToken.text.length();
    ++m_currentToken;
    if (m_currentToken == m_tokens.size())
        m_tokenTimer.stop();
    else
        startTokenTimer();
}

void QTextToSpeechProcessorFlite::processText(const QString &text, int voiceId, double pitch, double rate, OutputHandler outputHandler)
{
    qCDebug(lcSpeechTtsFlite) << "processText() begin";
    if (!checkVoice(voiceId))
        return;

    m_text = text;
    m_tokens.clear();
    m_currentToken = 0;
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

void QTextToSpeechProcessorFlite::setRateForVoice(cst_voice *voice, float rate)
{
    float stretch = 1.0;
    Q_ASSERT(rate >= -1.0 && rate <= 1.0);
    // Stretch multipliers taken from Speech Dispatcher
    if (rate < 0)
        stretch -= rate * 2;
    if (rate > 0)
        stretch -= rate * (100.0 / 175.0);
    feat_set_float(voice->features, "duration_stretch", stretch);
}

void QTextToSpeechProcessorFlite::setPitchForVoice(cst_voice *voice, float pitch)
{
    float f0;
    Q_ASSERT(pitch >= -1.0 && pitch <= 1.0);
    // Conversion taken from Speech Dispatcher
    f0 = (pitch * 80) + 100;
    feat_set_float(voice->features, "int_f0_target_mean", f0);
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
    const QLatin1StringView langCode("us");
    const QLatin1StringView libPrefix("flite_cmu_%1_%2.so.1");
    const QLatin1StringView registerPrefix("register_cmu_%1_%2");
    const QLatin1StringView unregisterPrefix("unregister_cmu_%1_%2");

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
            const int id = m_voices.count();
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

QStringList QTextToSpeechProcessorFlite::fliteAvailableVoices(const QString &libPrefix,
                                                              const QString &langCode) const
{
    // Read statically linked voices
    QStringList voices;
    for (const cst_val *v = flite_voice_list; v; v = val_cdr(v)) {
        cst_voice *voice = val_voice(val_car(v));
        voices.append(voice->name);
    }

    // Read available libraries
    // TODO: make default library paths OS dependent
    const QProcessEnvironment pe;
    QStringList ldPaths = pe.value("LD_LIBRARY_PATH"_L1).split(":", Qt::SkipEmptyParts);
    if (ldPaths.isEmpty()) {
        ldPaths = QStringList{"/usr/lib64"_L1, "/usr/local/lib64"_L1, "/lib64"_L1,
                              "/usr/lib/x86_64-linux-gnu"_L1, "/usr/lib"_L1};
    } else {
        ldPaths.removeDuplicates();
    }

    const QString libPattern = ("lib"_L1 + libPrefix).arg(langCode).arg("*"_L1);
    for (const auto &path : ldPaths) {
        QDir dir(path);
        if (!dir.isReadable() || dir.isEmpty())
            continue;
        dir.setNameFilters({libPattern});
        dir.setFilter(QDir::Files);
        const QFileInfoList fileList = dir.entryInfoList();
        for (const auto &file : fileList) {
            const QString vox = file.fileName().mid(16, file.fileName().indexOf(u'.') - 16);
            voices.append(vox);
        }
    }

    voices.removeDuplicates();
    return voices;
}

bool QTextToSpeechProcessorFlite::initAudio(double rate, int channelCount)
{
    m_format.setSampleFormat(QAudioFormat::Int16);
    m_format.setSampleRate(rate);
    m_format.setChannelCount(channelCount);
    switch (channelCount) {
    case 1:
        m_format.setChannelConfig(QAudioFormat::ChannelConfigMono);
        break;
    case 2:
        m_format.setChannelConfig(QAudioFormat::ChannelConfigStereo);
        break;
    case 3:
        m_format.setChannelConfig(QAudioFormat::ChannelConfig2Dot1);
        break;
    case 5:
        m_format.setChannelConfig(QAudioFormat::ChannelConfigSurround5Dot0);
        break;
    case 6:
        m_format.setChannelConfig(QAudioFormat::ChannelConfigSurround5Dot1);
        break;
    case 7:
        m_format.setChannelConfig(QAudioFormat::ChannelConfigSurround7Dot0);
        break;
    case 8:
        m_format.setChannelConfig(QAudioFormat::ChannelConfigSurround7Dot1);
        break;
    default:
        m_format.setChannelConfig(QAudioFormat::ChannelConfigUnknown);
        break;
    }
    if (!checkFormat(m_format))
       return false;

    createSink();

    m_audioSink->setVolume(m_volume);

    return true;
}

void QTextToSpeechProcessorFlite::deleteSink()
{
    if (m_audioSink) {
        m_audioSink->disconnect();
        delete m_audioSink;
        m_audioSink = nullptr;
        m_audioBuffer = nullptr;
    }
}

void QTextToSpeechProcessorFlite::createSink()
{
    // Create new sink if none exists or the format has changed
    if (!m_audioSink || (m_audioSink->format() != m_format)) {
        // No signals while we create new sink with QIODevice
        const bool sigs = signalsBlocked();
        auto resetSignals = qScopeGuard([this, sigs](){ blockSignals(sigs); });
        blockSignals(true);
        deleteSink();
        m_audioSink = new QAudioSink(m_audioDevice, m_format, this);
        connect(m_audioSink, &QAudioSink::stateChanged, this, &QTextToSpeechProcessorFlite::changeState);
        connect(QThread::currentThread(), &QThread::finished, m_audioSink, &QObject::deleteLater);
    }
    m_audioBuffer = m_audioSink->start();
    if (!m_audioBuffer) {
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

    switch (newState) {
    case QAudio::ActiveState:
        // Once the sink starts playing, start a timer to keep track of the tokens.
        if (!m_tokenTimer.isActive() && m_currentToken < m_tokens.count())
            startTokenTimer();
        break;
    case QAudio::SuspendedState:
    case QAudio::IdleState:
    case QAudio::StoppedState:
        m_tokenTimer.stop();
        break;
    }

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
    m_tokenTimer.stop();
    m_index = -1;
    m_currentToken = -1;
    deleteSink();
}

// Check format/device and set corresponding error messages
bool QTextToSpeechProcessorFlite::checkFormat(const QAudioFormat &format)
{
    QString formatString;
    QDebug(&formatString) << format;
    bool formatOK = true;

    // Format must be valid
    if (!format.isValid()) {
        formatOK = false;
        setError(QTextToSpeech::ErrorReason::Playback,
                 QCoreApplication::translate("QTextToSpeech", "Invalid audio format: %1")
                    .arg(formatString));
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
                 QCoreApplication::translate("QTextToSpeech", "Audio device does not support format: %1")
                    .arg(formatString));
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
    return false;;
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
    processText(text, voiceId, pitch, rate, QTextToSpeechProcessorFlite::audioOutputCb);
}

void QTextToSpeechProcessorFlite::synthesize(const QString &text, int voiceId, double pitch, double rate, double volume)
{
    if (text.isEmpty())
        return;

    if (!checkVoice(voiceId))
        return;

    m_volume = volume;
    processText(text, voiceId, pitch, rate, QTextToSpeechProcessorFlite::dataOutputCb);
}

QT_END_NAMESPACE
