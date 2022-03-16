/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#ifndef QMLACCELEROMETER_H
#define QMLACCELEROMETER_H

#include "qmlsensor.h"

QT_BEGIN_NAMESPACE

class QAccelerometer;

class QmlAccelerometer : public QmlSensor
{
    Q_OBJECT
    Q_ENUMS(AccelerationMode)
    Q_PROPERTY(AccelerationMode accelerationMode READ accelerationMode WRITE setAccelerationMode
               NOTIFY accelerationModeChanged REVISION 1)
public:
    explicit QmlAccelerometer(QObject *parent = 0);
    ~QmlAccelerometer();

    // Keep this enum in sync with QAccelerometer::AccelerationMode
    enum AccelerationMode {
        Combined,
        Gravity,
        User
    };

    AccelerationMode accelerationMode() const;
    void setAccelerationMode(AccelerationMode accelerationMode);

signals:
    Q_REVISION(1) void accelerationModeChanged(AccelerationMode accelerationMode);

private:
    QSensor *sensor() const override;
    QAccelerometer *m_sensor;
    QmlSensorReading *createReading() const override;
};

class QmlAccelerometerReading : public QmlSensorReading
{
    Q_OBJECT
    Q_PROPERTY(qreal x READ x NOTIFY xChanged)
    Q_PROPERTY(qreal y READ y NOTIFY yChanged)
    Q_PROPERTY(qreal z READ z NOTIFY zChanged)
public:
    explicit QmlAccelerometerReading(QAccelerometer *sensor);
    ~QmlAccelerometerReading();

    qreal x() const;
    qreal y() const;
    qreal z() const;

Q_SIGNALS:
    void xChanged();
    void yChanged();
    void zChanged();

private:
    QSensorReading *reading() const  override;
    void readingUpdate() override;
    QAccelerometer *m_sensor;
    qreal m_x;
    qreal m_y;
    qreal m_z;
};

QT_END_NAMESPACE
#endif
