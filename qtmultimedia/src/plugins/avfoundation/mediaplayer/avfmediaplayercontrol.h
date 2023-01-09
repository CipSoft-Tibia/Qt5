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

#ifndef AVFMEDIAPLAYERCONTROL_H
#define AVFMEDIAPLAYERCONTROL_H

#include <QtMultimedia/QMediaPlayerControl>
#include <QtCore/QObject>

QT_BEGIN_NAMESPACE

class AVFMediaPlayerSession;

class AVFMediaPlayerControl : public QMediaPlayerControl
{
    Q_OBJECT
public:
    explicit AVFMediaPlayerControl(QObject *parent = nullptr);
    ~AVFMediaPlayerControl();

    void setSession(AVFMediaPlayerSession *session);

    QMediaPlayer::State state() const override;
    QMediaPlayer::MediaStatus mediaStatus() const override;

    QMediaContent media() const override;
    const QIODevice *mediaStream() const override;
    void setMedia(const QMediaContent &content, QIODevice *stream) override;

    qint64 position() const override;
    qint64 duration() const override;

    int bufferStatus() const override;

    int volume() const override;
    bool isMuted() const override;

    bool isAudioAvailable() const override;
    bool isVideoAvailable() const override;

    bool isSeekable() const override;
    QMediaTimeRange availablePlaybackRanges() const override;

    qreal playbackRate() const override;
    void setPlaybackRate(qreal rate) override;

public Q_SLOTS:
    void setPosition(qint64 pos) override;

    void play() override;
    void pause() override;
    void stop() override;

    void setVolume(int volume) override;
    void setMuted(bool muted) override;

private:
    AVFMediaPlayerSession *m_session;
};

QT_END_NAMESPACE

#endif // AVFMEDIAPLAYERCONTROL_H
