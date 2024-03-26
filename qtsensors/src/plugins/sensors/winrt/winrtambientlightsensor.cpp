// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "winrtambientlightsensor.h"
#include "winrtcommon.h"

#include <QtSensors/QAmbientLightSensor>

#include <functional>
#include <wrl.h>
#include <windows.devices.sensors.h>
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Devices::Sensors;

typedef ITypedEventHandler<LightSensor *, LightSensorReadingChangedEventArgs *> LightSensorReadingHandler;

QT_USE_NAMESPACE

class WinRtAmbientLightSensorPrivate
{
public:
    WinRtAmbientLightSensorPrivate(WinRtAmbientLightSensor *p)
        : minimumReportInterval(0), q(p)
    {
        token.value = 0;
    }

    QAmbientLightReading reading;

    ComPtr<ILightSensor> sensor;
    EventRegistrationToken token;
    quint32 minimumReportInterval;

    HRESULT readingChanged(ILightSensor *, ILightSensorReadingChangedEventArgs *args)
    {
        ComPtr<ILightSensorReading> value;
        HRESULT hr = args->get_Reading(&value);
        if (FAILED(hr)) {
            qCWarning(lcWinRtSensors) << "Failed to get light sensor reading" << qt_error_string(hr);
            return hr;
        }

        FLOAT lux;
        hr = value->get_IlluminanceInLux(&lux);
        if (FAILED(hr)) {
            qCWarning(lcWinRtSensors) << "Failed to get illuminance value" << qt_error_string(hr);
            return hr;
        }

        DateTime dateTime;
        hr = value->get_Timestamp(&dateTime);
        if (FAILED(hr)) {
            qCWarning(lcWinRtSensors) << "Failed to get light sensor reading timestamp" << qt_error_string(hr);
            return hr;
        }

        // Using same values as BB light sensor
        if (lux < 10)
            reading.setLightLevel(QAmbientLightReading::Dark);
        else if (lux < 80)
            reading.setLightLevel(QAmbientLightReading::Twilight);
        else if (lux < 400)
            reading.setLightLevel(QAmbientLightReading::Light);
        else if (lux < 2500)
            reading.setLightLevel(QAmbientLightReading::Bright);
        else
            reading.setLightLevel(QAmbientLightReading::Sunny);

        reading.setTimestamp(dateTimeToMsSinceEpoch(dateTime));
        q->newReadingAvailable();
        return S_OK;
    }

private:
    WinRtAmbientLightSensor *q;
};

WinRtAmbientLightSensor::WinRtAmbientLightSensor(QSensor *sensor)
    : QSensorBackend(sensor), d_ptr(new WinRtAmbientLightSensorPrivate(this))
{
    Q_D(WinRtAmbientLightSensor);

    HStringReference classId(RuntimeClass_Windows_Devices_Sensors_LightSensor);
    ComPtr<ILightSensorStatics> factory;
    HRESULT hr = RoGetActivationFactory(classId.Get(), IID_PPV_ARGS(&factory));
    if (FAILED(hr)) {
        qCWarning(lcWinRtSensors) << "Unable to initialize light sensor factory."
                                      << qt_error_string(hr);
        sensorError(hr);
        return;
    }
    hr = factory->GetDefault(&d->sensor);
    if (FAILED(hr)) {
        qCWarning(lcWinRtSensors) << "Unable to get default light sensor."
                                      << qt_error_string(hr);
        sensorError(hr);
        return;
    }
    if (!d->sensor) {
        qCWarning(lcWinRtSensors) << "Default lightsensor was not found on the system.";
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

    setReading<QAmbientLightReading>(&d->reading);
}

WinRtAmbientLightSensor::~WinRtAmbientLightSensor()
{
}

void WinRtAmbientLightSensor::start()
{
    Q_D(WinRtAmbientLightSensor);
    if (!d->sensor)
        return;
    if (d->token.value)
        return;

    ComPtr<LightSensorReadingHandler> callback =
            Callback<LightSensorReadingHandler>(d, &WinRtAmbientLightSensorPrivate::readingChanged);
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

void WinRtAmbientLightSensor::stop()
{
    Q_D(WinRtAmbientLightSensor);
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
