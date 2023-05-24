// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only

#ifndef QTEXTTOSPEECHENGINE_MACOS_H
#define QTEXTTOSPEECHENGINE_MACOS_H

#include <QtCore/qhash.h>
#include <QtCore/qlist.h>
#include <QtCore/qlocale.h>
#include <QtCore/qobject.h>
#include <QtCore/qstring.h>
#include <QtTextToSpeech/qtexttospeechengine.h>
#include <QtTextToSpeech/qvoice.h>

Q_FORWARD_DECLARE_OBJC_CLASS(QT_MANGLE_NAMESPACE(StateDelegate));
Q_FORWARD_DECLARE_OBJC_CLASS(NSSpeechSynthesizer);
Q_FORWARD_DECLARE_OBJC_CLASS(NSString);

QT_BEGIN_NAMESPACE

class QTextToSpeechEngineMacOS : public QTextToSpeechEngine
{
    Q_OBJECT

public:
    QTextToSpeechEngineMacOS(const QVariantMap &parameters, QObject *parent);
    ~QTextToSpeechEngineMacOS();

    // Plug-in API:
    QList<QLocale> availableLocales() const override;
    QList<QVoice> availableVoices() const override;
    void say(const QString &text) override;
    void synthesize(const QString &text) override;
    void stop(QTextToSpeech::BoundaryHint boundaryHint) override;
    void pause(QTextToSpeech::BoundaryHint boundaryHint) override;
    void resume() override;
    double rate() const override;
    bool setRate(double rate) override;
    double pitch() const override;
    bool setPitch(double pitch) override;
    QLocale locale() const override;
    bool setLocale(const QLocale &locale) override;
    double volume() const override;
    bool setVolume(double volume) override;
    QVoice voice() const override;
    bool setVoice(const QVoice &voice) override;
    QTextToSpeech::State state() const override;
    QTextToSpeech::ErrorReason errorReason() const override;
    QString errorString() const override;

    void setError(QTextToSpeech::ErrorReason reason, const QString &string);
    void speechStopped(bool);
    void speaking();

private:
    void updateVoices();

    QTextToSpeech::State m_state = QTextToSpeech::Error;
    QTextToSpeech::ErrorReason m_errorReason = QTextToSpeech::ErrorReason::Initialization;
    QString m_errorString;
    bool pauseRequested = false;

    QVoice voiceForNSVoice(NSString *voiceString) const;
    NSSpeechSynthesizer *speechSynthesizer;
    QT_MANGLE_NAMESPACE(StateDelegate) *stateDelegate;
    QMultiHash<QLocale, QVoice> m_voices;
};

QT_END_NAMESPACE

#endif
