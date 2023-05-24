// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef WINRTAMBIENTLIGHTSENSOR_H
#define WINRTAMBIENTLIGHTSENSOR_H

#include <QtSensors/QSensorBackend>
#include <QtCore/QScopedPointer>

QT_USE_NAMESPACE

class WinRtAmbientLightSensorPrivate;
class WinRtAmbientLightSensor : public QSensorBackend
{
    Q_OBJECT
public:
    WinRtAmbientLightSensor(QSensor *sensor);
    ~WinRtAmbientLightSensor();

    void start() override;
    void stop() override;

private:
    QScopedPointer<WinRtAmbientLightSensorPrivate> d_ptr;
    Q_DECLARE_PRIVATE(WinRtAmbientLightSensor)
};

#endif // WINRTAMBIENTLIGHTSENSOR_H
