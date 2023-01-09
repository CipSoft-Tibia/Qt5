/****************************************************************************
**
** Copyright (C) 2016 Research In Motion
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

#ifndef QNXAUDIODEVICEINFO_H
#define QNXAUDIODEVICEINFO_H

#include "qaudiosystem.h"

QT_BEGIN_NAMESPACE

class QnxAudioDeviceInfo : public QAbstractAudioDeviceInfo
{
    Q_OBJECT

public:
    QnxAudioDeviceInfo(const QString &deviceName, QAudio::Mode mode);
    ~QnxAudioDeviceInfo();

    QAudioFormat preferredFormat() const override;
    bool isFormatSupported(const QAudioFormat &format) const override;
    QString deviceName() const override;
    QStringList supportedCodecs() override;
    QList<int> supportedSampleRates() override;
    QList<int> supportedChannelCounts() override;
    QList<int> supportedSampleSizes() override;
    QList<QAudioFormat::Endian> supportedByteOrders() override;
    QList<QAudioFormat::SampleType> supportedSampleTypes() override;

private:
    const QString m_name;
    const QAudio::Mode m_mode;
};

QT_END_NAMESPACE

#endif
