// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsensorbackend.h"
#include "qsensorbackend_p.h"
#include "qsensor_p.h"
#include <QDebug>

QT_BEGIN_NAMESPACE

/*!
    \class QSensorBackend
    \ingroup sensors_backend
    \inmodule QtSensors
    \since 5.1

    \brief The QSensorBackend class is a sensor implementation.

    Sensors on a device will be represented by sub-classes of
    QSensorBackend.
*/

/*!
    \internal
*/
QSensorBackend::QSensorBackend(QSensor *sensor, QObject *parent)
    : QObject(*new QSensorBackendPrivate(sensor), parent)
{
}

/*!
    \internal
*/
QSensorBackend::~QSensorBackend()
{
}

/*!
   Checks whether a feature is supported by this sensor backend.

   This is the backend side of QSensor::isFeatureSupported(). Reimplement this function if the
   backend supports one of the additional sensor features of QSensor::Feature.

   Returns whether the feature \a feature is supported by this backend. The default implementation returns false.
   \since 5.0
 */
bool QSensorBackend::isFeatureSupported(QSensor::Feature feature) const
{
    Q_UNUSED(feature);
    return false;
}

/*!
    Notify the QSensor class that a new reading is available.
*/
void QSensorBackend::newReadingAvailable()
{
    Q_D(QSensorBackend);
    QSensorPrivate *sensorPrivate = d->m_sensor->d_func();

    // Copy the values from the device reading to the filter reading
    sensorPrivate->filter_reading->copyValuesFrom(sensorPrivate->device_reading);

    for (QFilterList::const_iterator it = sensorPrivate->filters.constBegin(); it != sensorPrivate->filters.constEnd(); ++it) {
        QSensorFilter *filter = (*it);
        if (!filter->filter(sensorPrivate->filter_reading))
            return;
    }

    // Copy the values from the filter reading to the cached reading
    sensorPrivate->cache_reading->copyValuesFrom(sensorPrivate->filter_reading);

    Q_EMIT d->m_sensor->readingChanged();
}

/*!
    \fn QSensorBackend::start()

    Start reporting values.
*/

/*!
    \fn QSensorBackend::stop()

    Stop reporting values.
*/

/*!
    If the backend has lost its reference to the reading
    it can call this method to get the address.

    Note that you will need to down-cast to the appropriate
    type.

    \sa setReading()
*/
QSensorReading *QSensorBackend::reading() const
{
    Q_D(const QSensorBackend);
    QSensorPrivate *sensorPrivate = d->m_sensor->d_func();
    return sensorPrivate->device_reading;
}

/*!
    Returns the sensor front end associated with this backend.
*/
QSensor *QSensorBackend::sensor() const
{
    Q_D(const QSensorBackend);
    return d->m_sensor;
}

/*!
    \fn template <typename T> T *QSensorBackend::setReading(T *reading)

    This function is called to initialize the \a reading
    classes used for a sensor.

    If your backend has already allocated a reading you
    should pass the address of this to the function.
    Otherwise you should pass 0 and the function will
    return the address of the reading your backend
    should use when it wants to notify the sensor API
    of new readings.

    Note that this is a template function so it should
    be called with the appropriate type.

    \code
    class MyBackend : public QSensorBackend
    {
        QAccelerometerReading m_reading;
    public:
        MyBackend(QSensor *sensor)
            : QSensorBackend(sensor)
        {
            setReading<QAccelerometerReading>(&m_reading);
        }

        ...
    \endcode

    Note that this function must be called or you will
    not be able to send readings to the front end.

    If you do not wish to store the address of the reading
    you may use the reading() method to get it again later.

    \code
    class MyBackend : public QSensorBackend
    {
    public:
        MyBackend(QSensor *sensor)
            : QSensorBackend(sensor)
        {
            setReading<QAccelerometerReading>(0);
        }

        void poll()
        {
            quint64 timestamp;
            qreal x, y, z;
            ...
            QAccelerometerReading *reading = static_cast<QAccelerometerReading*>(reading());
            reading->setTimestamp(timestamp);
            reading->setX(x);
            reading->setY(y);
            reading->setZ(z);
        }

        ...
    \endcode

    \sa reading()
*/

/*!
    \internal
*/
void QSensorBackend::setReadings(QSensorReading *device, QSensorReading *filter, QSensorReading *cache)
{
    Q_D(QSensorBackend);
    QSensorPrivate *sensorPrivate = d->m_sensor->d_func();
    sensorPrivate->device_reading = device;
    sensorPrivate->filter_reading = filter;
    sensorPrivate->cache_reading = cache;
}

/*!
    Add a data rate (consisting of \a min and \a max values) for the sensor.

    Note that this function should be called from the constructor so that the information
    is available immediately.

    \sa QSensor::availableDataRates
*/
void QSensorBackend::addDataRate(qreal min, qreal max)
{
    Q_D(QSensorBackend);
    QSensorPrivate *sensorPrivate = d->m_sensor->d_func();
    sensorPrivate->availableDataRates << qrange(min, max);
}

/*!
    Set the data rates for the sensor based on \a otherSensor.

    This is designed for sensors that are based on other sensors.

    \code
    setDataRates(otherSensor);
    \endcode

    Note that this function must be called from the constructor.

    \sa QSensor::availableDataRates, addDataRate()
*/
void QSensorBackend::setDataRates(const QSensor *otherSensor)
{
    Q_D(QSensorBackend);
    if (!otherSensor) {
        qWarning() << "ERROR: Cannot call QSensorBackend::setDataRates with 0";
        return;
    }
    if (otherSensor->identifier().isEmpty()) {
        qWarning() << "ERROR: Cannot call QSensorBackend::setDataRates with an invalid sensor";
        return;
    }
    if (d->m_sensor->isConnectedToBackend()) {
        qWarning() << "ERROR: Cannot call QSensorBackend::setDataRates outside of the constructor";
        return;
    }
    QSensorPrivate *sensorPrivate = d->m_sensor->d_func();
    sensorPrivate->availableDataRates = otherSensor->availableDataRates();
}

/*!
    Add an output range (consisting of \a min, \a max values and \a accuracy) for the sensor.

    Note that this function should be called from the constructor so that the information
    is available immediately.

    \sa QSensor::outputRange, QSensor::outputRanges
*/
void QSensorBackend::addOutputRange(qreal min, qreal max, qreal accuracy)
{
    Q_D(QSensorBackend);
    QSensorPrivate *sensorPrivate = d->m_sensor->d_func();

    qoutputrange details = {min, max, accuracy};

    sensorPrivate->outputRanges << details;
}

/*!
    Set the \a description for the sensor.

    Note that this function should be called from the constructor so that the information
    is available immediately.
*/
void QSensorBackend::setDescription(const QString &description)
{
    Q_D(QSensorBackend);
    QSensorPrivate *sensorPrivate = d->m_sensor->d_func();
    sensorPrivate->description = description;
}

/*!
    Inform the front end that the sensor has stopped.
    This can be due to start() failing or for some
    unexpected reason (eg. hardware failure).

    Note that the front end must call QSensor::isActive() to see if
    the sensor has stopped. If the sensor has stopped due to an error
    the sensorError() function should be called to notify the class
    of the error condition.
*/
void QSensorBackend::sensorStopped()
{
    Q_D(QSensorBackend);
    QSensorPrivate *sensorPrivate = d->m_sensor->d_func();
    sensorPrivate->active = false;
}

/*!
    Inform the front end of the sensor's busy state according
    to the provided \a busy parameter.

    If the sensor is set \e busy this implicitly calls sensorStopped().
    Busy indication is typically done in start().

    Note that the front end must call QSensor::isBusy() to see if
    the sensor is busy. If the sensor has stopped due to an error
    the sensorError() function should be called to notify the class
    of the error condition.
*/
void QSensorBackend::sensorBusy(bool busy)
{
    Q_D(QSensorBackend);
    QSensorPrivate *sensorPrivate = d->m_sensor->d_func();
    if (sensorPrivate->busy == busy)
        return;
    if (busy)
        sensorPrivate->active = false;
    sensorPrivate->busy = busy;
    emit d->m_sensor->busyChanged();
}

/*!
    Inform the front end that a sensor error occurred.
    Note that this only reports an \a error code. It does
    not stop the sensor.

    \sa sensorStopped()
*/
void QSensorBackend::sensorError(int error)
{
    Q_D(QSensorBackend);
    QSensorPrivate *sensorPrivate = d->m_sensor->d_func();
    sensorPrivate->error = error;
    Q_EMIT d->m_sensor->sensorError(error);
}

QT_END_NAMESPACE

#include "moc_qsensorbackend.cpp"
