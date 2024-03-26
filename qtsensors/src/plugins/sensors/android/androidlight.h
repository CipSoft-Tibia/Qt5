// Copyright (C) 2019 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef ANDROIDLIGHT_H
#define ANDROIDLIGHT_H

#include <qlightsensor.h>

#include "sensoreventqueue.h"

class AndroidLight : public SensorEventQueue<QLightReading>
{
public:
    AndroidLight(int type, QSensor *sensor, QObject *parent = nullptr);

protected:
    // SensorEventQueue interface
    void dataReceived(const ASensorEvent &event) override;
};

#endif // ANDROIDLIGHT_H
