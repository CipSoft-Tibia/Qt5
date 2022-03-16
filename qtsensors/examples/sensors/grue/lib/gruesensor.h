/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtSensors module of the Qt Toolkit.
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

#ifndef GRUESENSOR_H
#define GRUESENSOR_H

#include <qsensor.h>

class GrueSensorReadingPrivate;

#if defined(QT_BUILD_GRUE_LIB)
#    define Q_GRUE_EXPORT Q_DECL_EXPORT
#else
#    define Q_GRUE_EXPORT Q_DECL_IMPORT
#endif

class Q_GRUE_EXPORT GrueSensorReading : public QSensorReading
{
    Q_OBJECT
    Q_PROPERTY(int chanceOfBeingEaten READ chanceOfBeingEaten WRITE setChanceOfBeingEaten)
    DECLARE_READING(GrueSensorReading)
public:
    int chanceOfBeingEaten() const;
    void setChanceOfBeingEaten(int chanceOfBeingEaten);
};

// begin generated code

class Q_GRUE_EXPORT GrueFilter : public QSensorFilter
{
public:
    virtual bool filter(GrueSensorReading *reading) = 0;
private:
    bool filter(QSensorReading *reading) override { return filter(static_cast<GrueSensorReading*>(reading)); }
};

class Q_GRUE_EXPORT GrueSensor : public QSensor
{
    Q_OBJECT
    Q_PROPERTY(GrueSensorReading* reading READ reading)
public:
    explicit GrueSensor(QObject *parent = 0) : QSensor(GrueSensor::type, parent) {}
    virtual ~GrueSensor() {}
    GrueSensorReading *reading() const { return static_cast<GrueSensorReading*>(QSensor::reading()); }
    static char const * const type;
};
// end generated code

#endif
