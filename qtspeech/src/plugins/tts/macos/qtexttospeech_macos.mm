// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only

#include <Cocoa/Cocoa.h>
#include "qtexttospeech_macos.h"

#include <QtCore/QDebug>
#include <QtCore/QCoreApplication>

@interface QT_MANGLE_NAMESPACE(StateDelegate) : NSObject <NSSpeechSynthesizerDelegate>
@end

using namespace Qt::StringLiterals;

@implementation QT_MANGLE_NAMESPACE(StateDelegate)
{
    QT_PREPEND_NAMESPACE(QTextToSpeechEngineMacOS) *speechPrivate;
}

- (instancetype)initWithSpeechPrivate:(QTextToSpeechEngineMacOS *) priv
{
    if ((self = [super init]))
        speechPrivate = priv;

    return self;
}

- (void)speechSynthesizer:(NSSpeechSynthesizer *)sender willSpeakWord:(NSRange)characterRange ofString:(NSString *)string
{
    Q_UNUSED(sender);
    const QString text = QString::fromNSString(string);
    if (characterRange.location + characterRange.length > size_t(text.length())) {
        qCritical() << characterRange.location << characterRange.length << "are out of bounds of" << text;
        return;
    }

    // the macOS engine doesn't strip all punctuation characters
    auto length = characterRange.length;
    if (text.at(characterRange.location + length - 1).isPunct())
        --length;
    if (length) {
        emit speechPrivate->sayingWord(text.sliced(characterRange.location, length),
                                       characterRange.location, length);
    }
    speechPrivate->speaking();
}

- (void)speechSynthesizer:(NSSpeechSynthesizer *)sender didFinishSpeaking:(BOOL)finishedSpeaking
{
    Q_UNUSED(sender);
    speechPrivate->speechStopped(finishedSpeaking == YES);
}

- (void)speechSynthesizer:(NSSpeechSynthesizer *)sender  didEncounterSyncMessage:(NSString *)message
{
    Q_UNUSED(sender);
    speechPrivate->setError(QTextToSpeech::ErrorReason::Input,
                            QCoreApplication::translate("QTextToSpeech",
                                "Speech synthesizing failure: %1").arg(QString::fromNSString(message)));
}
@end

QT_BEGIN_NAMESPACE

QTextToSpeechEngineMacOS::QTextToSpeechEngineMacOS(const QVariantMap &/*parameters*/, QObject *parent)
    : QTextToSpeechEngine(parent)
{
    stateDelegate = [[QT_MANGLE_NAMESPACE(StateDelegate) alloc] initWithSpeechPrivate:this];
    speechSynthesizer = [[NSSpeechSynthesizer alloc] init];
    speechSynthesizer.delegate = stateDelegate;
    updateVoices();

    if (m_voices.isEmpty()) {
        setError(QTextToSpeech::ErrorReason::Configuration,
                 QCoreApplication::translate("QTextToSpeech", "No voices available."));
    } else {
        m_state = QTextToSpeech::Ready;
        m_errorReason = QTextToSpeech::ErrorReason::NoError;
    }
}

QTextToSpeechEngineMacOS::~QTextToSpeechEngineMacOS()
{
    [speechSynthesizer setDelegate: nil];
    if ([speechSynthesizer isSpeaking])
        [speechSynthesizer stopSpeakingAtBoundary:NSSpeechImmediateBoundary];
    [speechSynthesizer release];
    [stateDelegate release];
}


QTextToSpeech::State QTextToSpeechEngineMacOS::state() const
{
    return m_state;
}

void QTextToSpeechEngineMacOS::setError(QTextToSpeech::ErrorReason reason, const QString &string)
{
    m_errorReason = reason;
    m_errorString = string;
    if (reason != QTextToSpeech::ErrorReason::NoError)
        return;
    if (m_state != QTextToSpeech::Error) {
        m_state = QTextToSpeech::Error;
        emit stateChanged(m_state);
    }
    emit errorOccurred(m_errorReason, m_errorString);
}

QTextToSpeech::ErrorReason QTextToSpeechEngineMacOS::errorReason() const
{
    return m_errorReason;
}

QString QTextToSpeechEngineMacOS::errorString() const
{
    return m_errorString;
}

void QTextToSpeechEngineMacOS::speaking()
{
    if (m_state != QTextToSpeech::Speaking) {
        m_state = QTextToSpeech::Speaking;
        emit stateChanged(m_state);
    }
}

void QTextToSpeechEngineMacOS::speechStopped(bool success)
{
    Q_UNUSED(success);
    if (m_state != QTextToSpeech::Ready) {
        if (pauseRequested)
            m_state = QTextToSpeech::Paused;
        else
            m_state = QTextToSpeech::Ready;
        emit stateChanged(m_state);
    }
    pauseRequested = false;
}

void QTextToSpeechEngineMacOS::say(const QString &text)
{
    if (text.isEmpty())
        return;

    pauseRequested = false;
    if (m_state != QTextToSpeech::Ready)
        stop(QTextToSpeech::BoundaryHint::Default);

    NSString *ntext = text.toNSString();
    [speechSynthesizer startSpeakingString:ntext];
    speaking();
}

void QTextToSpeechEngineMacOS::synthesize(const QString &)
{
    setError(QTextToSpeech::ErrorReason::Configuration, tr("Synthesize not supported"));
}

void QTextToSpeechEngineMacOS::stop(QTextToSpeech::BoundaryHint boundaryHint)
{
    if (speechSynthesizer.isSpeaking || m_state == QTextToSpeech::Paused) {
        const NSSpeechBoundary atBoundary = [boundaryHint]{
            switch (boundaryHint) {
            case QTextToSpeech::BoundaryHint::Default:
            case QTextToSpeech::BoundaryHint::Immediate:
                return NSSpeechImmediateBoundary;
            case QTextToSpeech::BoundaryHint::Word:
                return NSSpeechWordBoundary;
            case QTextToSpeech::BoundaryHint::Sentence:
                return NSSpeechSentenceBoundary;
            case QTextToSpeech::BoundaryHint::Utterance:
                // should never get here, but if we do, stop immediately
                return NSSpeechImmediateBoundary;
            }
            Q_UNREACHABLE();
        }();

        [speechSynthesizer stopSpeakingAtBoundary:atBoundary];
    }
}

void QTextToSpeechEngineMacOS::pause(QTextToSpeech::BoundaryHint boundaryHint)
{
    if (speechSynthesizer.isSpeaking) {
        pauseRequested = true;
        const NSSpeechBoundary atBoundary = [boundaryHint]{
            switch (boundaryHint) {
            case QTextToSpeech::BoundaryHint::Immediate:
                return NSSpeechImmediateBoundary;
            case QTextToSpeech::BoundaryHint::Utterance:
                // should never get here, but if we do, pause at word
            case QTextToSpeech::BoundaryHint::Default:
            case QTextToSpeech::BoundaryHint::Word:
                return NSSpeechWordBoundary;
            case QTextToSpeech::BoundaryHint::Sentence:
                return NSSpeechSentenceBoundary;
            }
            Q_UNREACHABLE();
        }();
        [speechSynthesizer pauseSpeakingAtBoundary:atBoundary];
    }
}

void QTextToSpeechEngineMacOS::resume()
{
    pauseRequested = false;
    [speechSynthesizer continueSpeaking];
}

double QTextToSpeechEngineMacOS::rate() const
{
    return (speechSynthesizer.rate - 200) / 200.0;
}

bool QTextToSpeechEngineMacOS::setPitch(double pitch)
{
    // 30 to 65
    double p = 30.0 + ((pitch + 1.0) / 2.0) * 35.0;
    [speechSynthesizer setObject:[NSNumber numberWithFloat:p] forProperty:NSSpeechPitchBaseProperty error:nil];
    return true;
}

double QTextToSpeechEngineMacOS::pitch() const
{
    double pitch = [[speechSynthesizer objectForProperty:NSSpeechPitchBaseProperty error:nil] floatValue];
    return (pitch - 30.0) / 35.0 * 2.0 - 1.0;
}

double QTextToSpeechEngineMacOS::volume() const
{
    return speechSynthesizer.volume;
}

bool QTextToSpeechEngineMacOS::setRate(double rate)
{
    // NSSpeechSynthesizer supports words per minute,
    // human speech is 180 to 220 - use 0 to 400 as range here
    speechSynthesizer.rate = 200 + (rate * 200);
    return true;
}

bool QTextToSpeechEngineMacOS::setVolume(double volume)
{
    speechSynthesizer.volume = volume;
    return true;
}

static QLocale localeForVoice(NSString *voice)
{
    NSDictionary *attrs = [NSSpeechSynthesizer attributesForVoice:voice];
    return QLocale(QString::fromNSString(attrs[NSVoiceLocaleIdentifier]));
}

QVoice QTextToSpeechEngineMacOS::voiceForNSVoice(NSString *voiceString) const
{
    NSDictionary *attrs = [NSSpeechSynthesizer attributesForVoice:voiceString];
    QString voiceName = QString::fromNSString(attrs[NSVoiceName]);

    NSString *genderString = attrs[NSVoiceGender];
    QVoice::Gender gender = [genderString isEqualToString:NSVoiceGenderMale] ? QVoice::Male
                          : [genderString isEqualToString:NSVoiceGenderFemale] ? QVoice::Female
                          : QVoice::Unknown;

    NSNumber *ageNSNumber = attrs[NSVoiceAge];
    int ageInt = ageNSNumber.intValue;
    QVoice::Age age = (ageInt < 13 ? QVoice::Child :
                       ageInt < 20 ? QVoice::Teenager :
                       ageInt < 45 ? QVoice::Adult :
                       ageInt < 90 ? QVoice::Senior : QVoice::Other);
    QVariant data = QString::fromNSString(attrs[NSVoiceIdentifier]);
    return createVoice(voiceName, localeForVoice(voiceString), gender, age, data);
}

QList<QLocale> QTextToSpeechEngineMacOS::availableLocales() const
{
    return m_voices.uniqueKeys();
}

bool QTextToSpeechEngineMacOS::setLocale(const QLocale &locale)
{
    NSArray *voices = NSSpeechSynthesizer.availableVoices;
    NSString *voice = NSSpeechSynthesizer.defaultVoice;
    // always prefer default
    if (locale == localeForVoice(voice)) {
        speechSynthesizer.voice = voice;
        return true;
    }

    for (voice in voices) {
        if (locale == localeForVoice(voice)) {
            speechSynthesizer.voice = voice;
            return true;
        }
    }

    setError(QTextToSpeech::ErrorReason::Configuration,
             QCoreApplication::translate("QTextToSpeech", "No voice available for locale %1.")
                .arg(locale.bcp47Name()));
    return false;
}

QLocale QTextToSpeechEngineMacOS::locale() const
{
    NSString *voice = speechSynthesizer.voice;
    return localeForVoice(voice);
}

void QTextToSpeechEngineMacOS::updateVoices()
{
    NSArray *voices = NSSpeechSynthesizer.availableVoices;
    for (NSString *voice in voices) {
        const QVoice data = voiceForNSVoice(voice);
        m_voices.insert(data.locale(), data);
    }
}

QList<QVoice> QTextToSpeechEngineMacOS::availableVoices() const
{
    return m_voices.values(locale());
}

bool QTextToSpeechEngineMacOS::setVoice(const QVoice &voice)
{
    NSString *identifier = voiceData(voice).toString().toNSString();
    speechSynthesizer.voice = identifier;
    return true;
}

QVoice QTextToSpeechEngineMacOS::voice() const
{
    return voiceForNSVoice(speechSynthesizer.voice);
}

QT_END_NAMESPACE
