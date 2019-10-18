/****************************************************************************
**
** Copyright (C) 2019 BogDan Vatra <bogdan@kde.org>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtSensors module of the Qt Toolkit.
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

#include "androidrotation.h"

#include <QtCore/qmath.h>

AndroidRotation::AndroidRotation(int type, QSensor *sensor, QObject *parent)
    : SensorEventQueue<QRotationReading>(type, sensor, parent)
{}


void AndroidRotation::dataReceived(const ASensorEvent &event)
{
    // From android documentation, the rotation sensor values are:
    // values[0]: x*sin(θ/2)
    // values[1]: y*sin(θ/2)
    // values[2]: z*sin(θ/2)
    // values[3]: cos(θ/2)

    // The mathematics below is adapted from
    // https://android.googlesource.com/platform/frameworks/base/+/master/core/java/android/hardware/SensorManager.java#1644
    // and
    // https://android.googlesource.com/platform/frameworks/base/+/master/core/java/android/hardware/SensorManager.java#1469
    // (getRotationMatrixFromVector() followed by getOrientation())
    qreal angles[3];
    qreal q1 = qreal(event.data[0]);
    qreal q2 = qreal(event.data[1]);
    qreal q3 = qreal(event.data[2]);
    qreal q0 = qreal(event.data[3]);

    qreal sq_q1 = 2 * q1 * q1;
    qreal sq_q2 = 2 * q2 * q2;
    qreal sq_q3 = 2 * q3 * q3;
    qreal q1_q2 = 2 * q1 * q2;
    qreal q3_q0 = 2 * q3 * q0;
    qreal q1_q3 = 2 * q1 * q3;
    qreal q2_q0 = 2 * q2 * q0;
    qreal q2_q3 = 2 * q2 * q3;
    qreal q1_q0 = 2 * q1 * q0;

    angles[0] = std::atan2((q1_q2 - q3_q0), (1 - sq_q1 - sq_q3));
    angles[1] = std::asin(-(q2_q3 + q1_q0));
    angles[2] = std::atan2(-(q1_q3 - q2_q0), (1 - sq_q1 - sq_q2));

    qreal rz = -qRadiansToDegrees(angles[0]);
    qreal rx = -qRadiansToDegrees(angles[1]);
    qreal ry =  qRadiansToDegrees(angles[2]);

    if (sensor()->skipDuplicates() && qFuzzyCompare(m_reader.x(), rx) &&
            qFuzzyCompare(m_reader.y(), ry) &&
            qFuzzyCompare(m_reader.z(), rz)) {
        return;
    }
    m_reader.setTimestamp(uint64_t(event.timestamp / 1000));
    m_reader.setFromEuler(rx, ry, rz);
    newReadingAvailable();
}
