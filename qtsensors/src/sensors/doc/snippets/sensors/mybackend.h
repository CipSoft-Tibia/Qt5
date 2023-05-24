// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef MYBACKEND_H
#define MYBACKEND_H

#include <qaccelerometer.h>
#include <qsensorbackend.h>

class MyBackend : public QSensorBackend
{
public:
    MyBackend(QSensor *sensor) : QSensorBackend(sensor) {}
    void stop() override {}
    void start() override {}
    void poll() {}

    static const char *id;
};

#endif
