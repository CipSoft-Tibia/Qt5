/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Speech module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QTEXTTOSPEECHENGINE_SAPI_H
#define QTEXTTOSPEECHENGINE_SAPI_H

#include <QtCore/qt_windows.h>
#include <sapi.h>

#include <QtCore/qobject.h>
#include <QtCore/qvector.h>
#include <QtCore/qstring.h>
#include <QtCore/qlocale.h>
#include <QtTextToSpeech/qtexttospeechengine.h>
#include <QtTextToSpeech/qvoice.h>

QT_BEGIN_NAMESPACE

class QTextToSpeechEngineSapi : public QTextToSpeechEngine, public ISpNotifyCallback
{
    Q_OBJECT

public:
    QTextToSpeechEngineSapi(const QVariantMap &parameters, QObject *parent);
    ~QTextToSpeechEngineSapi();

    // Plug-in API:
    QVector<QLocale> availableLocales() const override;
    QVector<QVoice> availableVoices() const override;
    void say(const QString &text) override;
    void stop() override;
    void pause() override;
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

    HRESULT STDMETHODCALLTYPE NotifyCallback(WPARAM /*wParam*/, LPARAM /*lParam*/) override;
private:

    void init();
    bool isSpeaking() const;
    bool isPaused() const { return m_pauseCount; }
    QMap<QString, QString> voiceAttributes(ISpObjectToken *speechToken) const;
    QString voiceId(ISpObjectToken *speechToken) const;
    QLocale lcidToLocale(const QString &lcid) const;
    void updateVoices();

    QTextToSpeech::State m_state;
    QVector<QLocale> m_locales;
    QVoice m_currentVoice;
    // Voices mapped by their locale name.
    QMultiMap<QString, QVoice> m_voices;

    ISpVoice *m_voice;
    double m_pitch;
    int m_pauseCount;
};
QT_END_NAMESPACE

#endif
