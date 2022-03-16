/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef LEVELMETER_H
#define LEVELMETER_H

#include <QTime>
#include <QWidget>

/**
 * Widget which displays a vertical audio level meter, indicating the
 * RMS and peak levels of the window of audio samples most recently analyzed
 * by the Engine.
 */
class LevelMeter : public QWidget
{
    Q_OBJECT

public:
    explicit LevelMeter(QWidget *parent = 0);
    ~LevelMeter();

    void paintEvent(QPaintEvent *event) override;

public slots:
    void reset();
    void levelChanged(qreal rmsLevel, qreal peakLevel, int numSamples);

private slots:
    void redrawTimerExpired();

private:
    /**
     * Height of RMS level bar.
     * Range 0.0 - 1.0.
     */
    qreal m_rmsLevel;

    /**
     * Most recent peak level.
     * Range 0.0 - 1.0.
     */
    qreal m_peakLevel;

    /**
     * Height of peak level bar.
     * This is calculated by decaying m_peakLevel depending on the
     * elapsed time since m_peakLevelChanged, and the value of m_decayRate.
     */
    qreal m_decayedPeakLevel;

    /**
     * Time at which m_peakLevel was last changed.
     */
    QTime m_peakLevelChanged;

    /**
     * Rate at which peak level bar decays.
     * Expressed in level units / millisecond.
     */
    qreal m_peakDecayRate;

    /**
     * High watermark of peak level.
     * Range 0.0 - 1.0.
     */
    qreal m_peakHoldLevel;

    /**
     * Time at which m_peakHoldLevel was last changed.
     */
    QTime m_peakHoldLevelChanged;

    QTimer *m_redrawTimer;

    QColor m_rmsColor;
    QColor m_peakColor;

};

#endif // LEVELMETER_H
