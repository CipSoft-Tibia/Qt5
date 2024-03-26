// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qambientlightsensor.h"
#include "qambientlightsensor_p.h"

QT_BEGIN_NAMESPACE

IMPLEMENT_READING(QAmbientLightReading)

/*!
    \class QAmbientLightReading
    \ingroup sensors_reading
    \inmodule QtSensors
    \since 5.1

    \brief The QAmbientLightReading class represents one reading from the
           ambient light sensor.

    \section2 QAmbientLightReading Units
    The ambient light sensor returns the intensity of the ambient light
    using the pre-defined values found in the QAmbientLightReading::LightLevel
    enum.
*/

/*!
    \enum QAmbientLightReading::LightLevel

    This enum represents the ambient light level.

    \value Undefined  The light level is unknown.
    \value Dark       It is dark.
    \value Twilight   It is moderately dark.
    \value Light      It is light (eg. internal lights).
    \value Bright     It is bright (eg. shade).
    \value Sunny      It is very bright (eg. direct sunlight).
*/

/*!
    \property QAmbientLightReading::lightLevel
    \brief the ambient light level.

    The value represents the ambient light and comes from QAmbientLightReading::LightLevel.
    \sa {QAmbientLightReading Units}
*/

QAmbientLightReading::LightLevel QAmbientLightReading::lightLevel() const
{
    return static_cast<LightLevel>(d->lightLevel);
}

/*!
    Sets the ambient light level to \a lightLevel.
*/
void QAmbientLightReading::setLightLevel(QAmbientLightReading::LightLevel lightLevel)
{
    switch (lightLevel) {
    case Dark:
    case Twilight:
    case Light:
    case Bright:
    case Sunny:
        d->lightLevel = lightLevel;
        break;
    default:
        d->lightLevel = Undefined;
        break;
    }
}

// =====================================================================

/*!
    \class QAmbientLightFilter
    \ingroup sensors_filter
    \inmodule QtSensors
    \since 5.1

    \brief The QAmbientLightFilter class is a convenience wrapper around QSensorFilter.

    The only difference is that the filter() method features a pointer to QAmbientLightReading
    instead of QSensorReading.
*/

/*!
    \fn QAmbientLightFilter::filter(QAmbientLightReading *reading)

    Called when \a reading changes. Returns false to prevent the reading from propagating.

    \sa QSensorFilter::filter()
*/

bool QAmbientLightFilter::filter(QSensorReading *reading)
{
    return filter(static_cast<QAmbientLightReading*>(reading));
}

char const * const QAmbientLightSensor::sensorType("QAmbientLightSensor");

/*!
    \class QAmbientLightSensor
    \ingroup sensors_type
    \inmodule QtSensors
    \since 5.1

    \brief The QAmbientLightSensor class is a convenience wrapper around QSensor.

    The only behavioural difference is that this class sets the type properly.

    This class also features a reading() function that returns a QAmbientLightReading instead of a QSensorReading.

    For details about how the sensor works, see \l QAmbientLightReading.

    \sa QAmbientLightReading
*/

/*!
    Construct the sensor as a child of \a parent.
*/
QAmbientLightSensor::QAmbientLightSensor(QObject *parent)
    : QSensor(QAmbientLightSensor::sensorType, parent)
{
}

/*!
    Destroy the sensor. Stops the sensor if it has not already been stopped.
*/
QAmbientLightSensor::~QAmbientLightSensor()
{
}

/*!
    \fn QAmbientLightSensor::reading() const

    Returns the reading class for this sensor.

    \sa QSensor::reading()
*/

QAmbientLightReading *QAmbientLightSensor::reading() const
{
    return static_cast<QAmbientLightReading*>(QSensor::reading());
}

QT_END_NAMESPACE

#include "moc_qambientlightsensor.cpp"
