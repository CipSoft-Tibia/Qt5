// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QTEXTTOSPEECHPROCESSOR_FLITE_H
#define QTEXTTOSPEECHPROCESSOR_FLITE_H

#include "qtexttospeechengine.h"
#include "qvoice.h"

#include <QtCore/qabstracteventdispatcher.h>
#include <QtCore/qbasictimer.h>
#include <QtCore/qcoreevent.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qlibrary.h>
#include <QtCore/qlist.h>
#include <QtCore/qmutex.h>
#include <QtCore/qprocess.h>
#include <QtCore/qstring.h>
#include <QtCore/qthread.h>
#include <QtMultimedia/qaudiosink.h>
#include <QtMultimedia/qmediadevices.h>

#include <flite/flite.h>

QT_BEGIN_NAMESPACE

class QTextToSpeechProcessorFlite : public QObject
{
    Q_OBJECT

public:
    QTextToSpeechProcessorFlite(const QAudioDevice &audioDevice);
    ~QTextToSpeechProcessorFlite();

    struct VoiceInfo
    {
        int id;
        cst_voice *vox;
        void (*unregister_func)(cst_voice *vox);
        QString name;
        QString locale;
        QVoice::Gender gender;
        QVoice::Age age;
    };

    Q_INVOKABLE void say(const QString &text, int voiceId, double pitch, double rate, double volume);
    Q_INVOKABLE void synthesize(const QString &text, int voiceId, double pitch, double rate, double volume);
    Q_INVOKABLE void pause();
    Q_INVOKABLE void resume();
    Q_INVOKABLE void stop();

    const QList<QTextToSpeechProcessorFlite::VoiceInfo> &voices() const;
    static constexpr QTextToSpeech::State audioStateToTts(QAudio::State audioState);

private:
    // Flite callbacks
    static int audioOutputCb(const cst_wave *w, int start, int size,
                             int last, cst_audio_streaming_info *asi);
    static int dataOutputCb(const cst_wave *w, int start, int size,
                            int last, cst_audio_streaming_info *asi);

    using OutputHandler = decltype(QTextToSpeechProcessorFlite::audioOutputCb);
    // Process a single text
    void processText(const QString &text, int voiceId, float pitch, float rate,
                     OutputHandler outputHandler);
    int audioOutput(const cst_wave *w, int start, int size, int last, cst_audio_streaming_info *asi);
    void audioHandleNewToken(std::chrono::milliseconds tokenStartTime,
                             cst_audio_streaming_info *asi);
    int dataOutput(const cst_wave *w, int start, int size, int last, cst_audio_streaming_info *asi);

    bool init();
    bool initAudio(const cst_wave *w);
    void deinitAudio();
    bool checkFormat(const QAudioFormat &format);
    bool checkVoice(int voiceId);
    void deleteSink();
    void createSink();
    QAudio::State audioSinkState() const;
    void setError(QTextToSpeech::ErrorReason err, const QString &errorString = QString());

private slots:
    void changeState(QAudio::State newState);

Q_SIGNALS:
    void errorOccurred(QTextToSpeech::ErrorReason error, const QString &errorString);
    void stateChanged(QTextToSpeech::State);
    void sayingWord(const QString &word, qsizetype begin, qsizetype length);
    void synthesized(const QAudioFormat &format, const QByteArray &array);

private:
    QString m_text;
    qsizetype m_index = -1;

    QAudioSink *m_audioSink = nullptr;
    QAudio::State m_state = QAudio::IdleState;
    QIODevice *m_audioIODevice = nullptr;

    QAudioDevice m_audioDevice;
    QAudioFormat m_format;
    double m_volume = 1;
    std::optional<QAudioFormat> m_synthesisFormat;

    QList<VoiceInfo> m_voices;

    // Statistics for debugging
    qint64 numberChunks = 0;
    qint64 totalBytes = 0;
};

QT_END_NAMESPACE

#endif
