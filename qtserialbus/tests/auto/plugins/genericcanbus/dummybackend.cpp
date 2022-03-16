/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtSerialBus module of the Qt Toolkit.
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

#include "dummybackend.h"

#include <QtCore/qdatetime.h>
#include <QtCore/qdebug.h>
#include <QtCore/qtimer.h>

QT_BEGIN_NAMESPACE

DummyBackend::DummyBackend() :
    simulateReceivingTimer(new QTimer(this))
{
    connect(simulateReceivingTimer, &QTimer::timeout, [this]() {
        const quint64 timeStamp = QDateTime::currentDateTime().toMSecsSinceEpoch();
        QCanBusFrame dummyFrame(12, "def");
        dummyFrame.setTimeStamp(QCanBusFrame::TimeStamp::fromMicroSeconds(timeStamp * 1000));

        enqueueReceivedFrames({dummyFrame});
    });
    simulateReceivingTimer->start(1000);
}

bool DummyBackend::open()
{
    setState(QCanBusDevice::ConnectedState);
    return true;
}

void DummyBackend::close()
{
    setState(QCanBusDevice::UnconnectedState);
}

bool DummyBackend::writeFrame(const QCanBusFrame &data)
{
    qDebug("DummyBackend::writeFrame: %ls", qUtf16Printable(data.toString()));
    return true;
}

QString DummyBackend::interpretErrorFrame(const QCanBusFrame &/*errorFrame*/)
{
    return QString();
}

QList<QCanBusDeviceInfo> DummyBackend::interfaces()
{
    return {createDeviceInfo(QStringLiteral("can0"), true, true)};
}

QT_END_NAMESPACE
