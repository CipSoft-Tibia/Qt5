/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtSensors module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qtest2recognizerdup.h"

#include "qtestsensorgestureplugindup.h"

QTest2RecognizerDup::QTest2RecognizerDup(QObject *parent)
    : QSensorGestureRecognizer(parent),
    active(0)
{
}

QTest2RecognizerDup::~QTest2RecognizerDup()
{
}

bool QTest2RecognizerDup::start()
{
    Q_EMIT test2_dup();

    active = true;

    return true;
}

bool QTest2RecognizerDup::stop()
{
    active = false;
    return true;
}

bool QTest2RecognizerDup::isActive()
{
    return active;
}


void QTest2RecognizerDup::create()
{
    active = false;
}

QString QTest2RecognizerDup::id() const
{
    return QString("QtSensors.test.dup");
}

int QTest2RecognizerDup::thresholdTime() const
{
    return timerTimeout;
}

void QTest2RecognizerDup::setThresholdTime(int msec)
{
    timer->setInterval(msec);
}

