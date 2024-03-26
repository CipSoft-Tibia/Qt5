// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "winrtaccelerometer.h"
#include "winrtcommon.h"

#include <QtSensors/QAccelerometerReading>

#include <functional>
#include <wrl.h>
#include <windows.devices.sensors.h>

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Devices::Sensors;

typedef ITypedEventHandler<Accelerometer *, AccelerometerReadingChangedEventArgs *> AccelerometerReadingHandler;

QT_USE_NAMESPACE

#define GRAVITY_EARTH_MS2 9.80665

class WinRtAccelerometerPrivate
{
public:
    WinRtAccelerometerPrivate(WinRtAccelerometer *p)
        : minimumReportInterval(0), q(p)
    {
        token.value = 0;
    }

    QAccelerometerReading reading;

    ComPtr<IAccelerometer> sensor;
    EventRegistrationToken token;
    quint32 minimumReportInterval;

    HRESULT readingChanged(IAccelerometer *, IAccelerometerReadingChangedEventArgs *args)
    {
        ComPtr<IAccelerometerReading> value;
        HRESULT hr = args->get_Reading(&value);
        if (FAILED(hr)) {
            qCWarning(lcWinRtSensors) << "Failed to get accelerometer reading" << qt_error_string(hr);
            return hr;
        }

        DOUBLE x;
        hr = value->get_AccelerationX(&x);
        if (FAILED(hr))
            qCWarning(lcWinRtSensors) << "Failed to get acceleration X" << qt_error_string(hr);

        DOUBLE y;
        hr = value->get_AccelerationY(&y);
        if (FAILED(hr))
            qCWarning(lcWinRtSensors) << "Failed to get acceleration Y" << qt_error_string(hr);

        DOUBLE z;
        hr = value->get_AccelerationZ(&z);
        if (FAILED(hr))
            qCWarning(lcWinRtSensors) << "Failed to get acceleration Z" << qt_error_string(hr);

        DateTime dateTime;
        hr = value->get_Timestamp(&dateTime);
        if (FAILED(hr))
            qCWarning(lcWinRtSensors) << "Failed to get acceleration timestamp" << qt_error_string(hr);

        // The reading is in G force, so convert to m/s/s
        reading.setX(x * GRAVITY_EARTH_MS2);
        reading.setY(y * GRAVITY_EARTH_MS2);
        reading.setZ(z * GRAVITY_EARTH_MS2);
        reading.setTimestamp(dateTimeToMsSinceEpoch(dateTime));
        q->newReadingAvailable();

        return S_OK;
    }

private:
    WinRtAccelerometer *q;
};

WinRtAccelerometer::WinRtAccelerometer(QSensor *sensor)
    : QSensorBackend(sensor), d_ptr(new WinRtAccelerometerPrivate(this))
{
    Q_D(WinRtAccelerometer);

    HStringReference classId(RuntimeClass_Windows_Devices_Sensors_Accelerometer);
    ComPtr<IAccelerometerStatics> factory;
    HRESULT hr = RoGetActivationFactory(classId.Get(), IID_PPV_ARGS(&factory));
    if (FAILED(hr)) {
        qCWarning(lcWinRtSensors) << "Unable to initialize accelerometer factory."
                                      << qt_error_string(hr);
        sensorError(hr);
        return;
    }
    hr = factory->GetDefault(&d->sensor);

    if (FAILED(hr)) {
        qCWarning(lcWinRtSensors) << "Unable to get default accelerometer."
                                      << qt_error_string(hr);
        sensorError(hr);
        return;
    }

    if (!d->sensor) {
        qCWarning(lcWinRtSensors) << "Default accelerometer was not found on the system.";
        return;
    }

    hr = d->sensor->get_MinimumReportInterval(&d->minimumReportInterval);
    if (FAILED(hr)) {
        qCWarning(lcWinRtSensors) << "Unable to get the minimum report interval."
                                  << qt_error_string(hr);
        sensorError(hr);
        return;
    }

    addDataRate(1, 1000 / d->minimumReportInterval); // dataRate in Hz
    sensor->setDataRate(1);

    setReading<QAccelerometerReading>(&d->reading);
}

WinRtAccelerometer::~WinRtAccelerometer()
{
}

void WinRtAccelerometer::start()
{
    Q_D(WinRtAccelerometer);
    if (!d->sensor)
        return;
    if (d->token.value)
        return;

    ComPtr<AccelerometerReadingHandler> callback =
        Callback<AccelerometerReadingHandler>(d, &WinRtAccelerometerPrivate::readingChanged);
    HRESULT hr = d->sensor->add_ReadingChanged(callback.Get(), &d->token);

    if (FAILED(hr)) {
        qCWarning(lcWinRtSensors) << "Unable to attach to reading changed event."
                                  << qt_error_string(hr);
        sensorError(hr);
        return;
    }

    int dataRate = sensor()->dataRate();
    if (!dataRate)
        return;

    quint32 reportInterval = qMax(d->minimumReportInterval, quint32(1000/dataRate));
    hr = d->sensor->put_ReportInterval(reportInterval);
    if (FAILED(hr)) {
        qCWarning(lcWinRtSensors) << "Unable to attach to set report interval."
                                  << qt_error_string(hr);
        sensorError(hr);
    }
}

void WinRtAccelerometer::stop()
{
    Q_D(WinRtAccelerometer);
    if (!d->sensor)
        return;
    if (!d->token.value)
        return;

    HRESULT hr = d->sensor->remove_ReadingChanged(d->token);
    if (FAILED(hr)) {
        qCWarning(lcWinRtSensors) << "Unable to detach from reading changed event."
                                  << qt_error_string(hr);
        sensorError(hr);
        return;
    }
    hr = d->sensor->put_ReportInterval(0);
    if (FAILED(hr)) {
        qCWarning(lcWinRtSensors) << "Unable to reset report interval."
                                  << qt_error_string(hr);
        sensorError(hr);
        return;
    }
    d->token.value = 0;
}
