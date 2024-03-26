// Copyright (C) 2016 Jolla Ltd.
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qnmeapositioninfosource_p.h"
#include "qgeopositioninfo_p.h"
#include "qlocationutils_p.h"

#include <QIODevice>
#include <QBasicTimer>
#include <QTimerEvent>
#include <QTimer>
#include <array>
#include <QDebug>
#include <QtCore/QtNumeric>
#include <QtCore/QDateTime>
#include <QtCore/QTimeZone>

#include <algorithm>

QT_BEGIN_NAMESPACE

#define USE_POSITION_NMEA_PIMPL 0

#if USE_POSITION_NMEA_PIMPL
class QGeoPositionInfoPrivateNmea : public QGeoPositionInfoPrivate
{
public:
    virtual ~QGeoPositionInfoPrivateNmea();

    QList<QByteArray> nmeaSentences;
};


QGeoPositionInfoPrivateNmea::~QGeoPositionInfoPrivateNmea()
{

}
#else
typedef QGeoPositionInfoPrivate QGeoPositionInfoPrivateNmea;
#endif

static bool propagateCoordinate(QGeoPositionInfo &dst, const QGeoPositionInfo &src, bool force = true)
{
    bool updated = false;
    QGeoCoordinate c = dst.coordinate();
    const QGeoCoordinate & srcCoordinate = src.coordinate();
    if (qIsFinite(src.coordinate().latitude())
            && (!qIsFinite(dst.coordinate().latitude()) || force)) {
        updated |= (c.latitude() != srcCoordinate.latitude());
        c.setLatitude(src.coordinate().latitude());
    }
    if (qIsFinite(src.coordinate().longitude())
            && (!qIsFinite(dst.coordinate().longitude()) || force)) {
        updated |= (c.longitude() != srcCoordinate.longitude());
        c.setLongitude(src.coordinate().longitude());
    }
    if (qIsFinite(src.coordinate().altitude())
            && (!qIsFinite(dst.coordinate().altitude()) || force)) {
        updated |= (c.altitude() != srcCoordinate.altitude());
        c.setAltitude(src.coordinate().altitude());
    }
    dst.setCoordinate(c);
    return updated;
}

static bool propagateDate(QGeoPositionInfo &dst, const QGeoPositionInfo &src)
{
    if (!dst.timestamp().date().isValid() && src.timestamp().isValid()) { // time was supposed to be set/the same already. Date can be overwritten.
        dst.setTimestamp(src.timestamp());
        return true;
    }
    return false;
}

static bool propagateAttributes(QGeoPositionInfo &dst, const QGeoPositionInfo &src, bool force = true)
{
    bool updated = false;
    static Q_DECL_CONSTEXPR std::array<QGeoPositionInfo::Attribute, 6> attrs {
                                                { QGeoPositionInfo::GroundSpeed
                                                 ,QGeoPositionInfo::HorizontalAccuracy
                                                 ,QGeoPositionInfo::VerticalAccuracy
                                                 ,QGeoPositionInfo::Direction
                                                 ,QGeoPositionInfo::VerticalSpeed
                                                 ,QGeoPositionInfo::MagneticVariation} };
    for (const auto a: attrs) {
        if (src.hasAttribute(a) && (!dst.hasAttribute(a) || force)) {
            updated |= (dst.attribute(a) != src.attribute(a));
            dst.setAttribute(a, src.attribute(a));
        }
    }

    return updated;
}

// returns false if src does not contain any additional or different data than dst,
// true otherwise.
static bool mergePositions(QGeoPositionInfo &dst, const QGeoPositionInfo &src, QByteArray nmeaSentence)
{
    bool updated = false;

    updated |= propagateCoordinate(dst, src);
    updated |= propagateDate(dst, src);
    updated |= propagateAttributes(dst, src);

#if USE_POSITION_NMEA_PIMPL
    QGeoPositionInfoPrivateNmea *dstPimpl = static_cast<QGeoPositionInfoPrivateNmea *>(QGeoPositionInfoPrivate::get(dst));
    dstPimpl->nmeaSentences.append(nmeaSentence);
#else
    Q_UNUSED(nmeaSentence);
#endif
    return updated;
}

static qint64 msecsTo(const QDateTime &from, const QDateTime &to)
{
    if (!from.time().isValid() || !to.time().isValid())
        return 0;

    if (!from.date().isValid() || !to.date().isValid()) // use only time
        return from.time().msecsTo(to.time());

    return from.msecsTo(to);
}

QNmeaReader::~QNmeaReader()
    = default;

QNmeaRealTimeReader::QNmeaRealTimeReader(QNmeaPositionInfoSourcePrivate *sourcePrivate)
        : QNmeaReader(sourcePrivate), m_update(*new QGeoPositionInfoPrivateNmea)
{
    // An env var controlling the number of milliseconds to use to withold
    // an update and wait for additional data to combine.
    // The update will be pushed earlier than this if a newer update will be received.
    // The update will be withold longer than this amount of time if additional
    // valid data will keep arriving within this time frame.
    bool ok = false;
    int pushDelay = qEnvironmentVariableIntValue("QT_NMEA_PUSH_DELAY", &ok);
    if (ok)
        pushDelay = std::clamp(pushDelay, -1, 1000);
    else
        pushDelay = 20;

    if (pushDelay >= 0) {
        m_timer.setSingleShot(true);
        m_timer.setInterval(pushDelay);
        m_timer.connect(&m_timer, &QTimer::timeout, [this]() {
           this->notifyNewUpdate();
        });
    }
    m_pushDelay = pushDelay;
}

QNmeaRealTimeReader::~QNmeaRealTimeReader()
    = default;

void QNmeaRealTimeReader::readAvailableData()
{
    while (m_proxy->m_device->canReadLine()) {
        const QTime infoTime = m_update.timestamp().time(); // if update has been set, time must be valid.
        const QDate infoDate = m_update.timestamp().date(); // this one might not be valid, as some sentences do not contain it

        QGeoPositionInfoPrivateNmea *pimpl = new QGeoPositionInfoPrivateNmea;
        QGeoPositionInfo pos(*pimpl);

        char buf[1024];
        qint64 size = m_proxy->m_device->readLine(buf, sizeof(buf));
        if (size <= 0)
            continue;

        const bool oldFix = m_hasFix;
        bool hasFix;
        const bool parsed = m_proxy->parsePosInfoFromNmeaData(
            QByteArrayView{buf, static_cast<qsizetype>(size)}, &pos, &hasFix);

        if (!parsed) {
            // got garbage, don't stop the timer
            continue;
        }

        m_hasFix |= hasFix;
        m_updateParsed = true;

        // Date may or may not be valid, as some packets do not have date.
        // If date isn't valid, match is performed on time only.
        // Hence, make sure that packet blocks are generated with
        // the sentences containing the full timestamp (e.g., GPRMC) *first* !
        if (infoTime.isValid()) {
            if (pos.timestamp().time().isValid()) {
                const bool newerTime = infoTime < pos.timestamp().time();
                const bool newerDate = (infoDate.isValid() // if time is valid but one date or both are not,
                                        && pos.timestamp().date().isValid()
                                        && infoDate < pos.timestamp().date());
                if (newerTime || newerDate) {
                    // Effectively read data for different update, that is also newer,
                    // so flush retained update, and copy the new pos into m_update
                    const QDate updateDate = m_update.timestamp().date();
                    const QDate lastPushedDate = m_lastPushedTS.date();
                    const bool newerTimestampSinceLastPushed = m_update.timestamp() > m_lastPushedTS;
                    const bool invalidDate = !(updateDate.isValid() && lastPushedDate.isValid());
                    const bool newerTimeSinceLastPushed = m_update.timestamp().time() > m_lastPushedTS.time();
                    if ( newerTimestampSinceLastPushed || (invalidDate && newerTimeSinceLastPushed)) {
                        m_proxy->notifyNewUpdate(&m_update, oldFix);
                        m_lastPushedTS = m_update.timestamp();
                    }
                    m_timer.stop();
                    // next update data
                    propagateAttributes(pos, m_update, false);
                    m_update = pos;
                    m_hasFix = hasFix;
                } else {
                    if (infoTime == pos.timestamp().time())
                        // timestamps match -- merge into m_update
                        if (mergePositions(m_update, pos, QByteArray(buf, size))) {
                            // Reset the timer only if new info has been received.
                            // Else the source might be keep repeating outdated info until
                            // new info become available.
                            m_timer.stop();
                        }
                    // else discard out of order outdated info.
                }
            } else {
                // no timestamp available in parsed update-- merge into m_update
                if (mergePositions(m_update, pos, QByteArray(buf, size)))
                    m_timer.stop();
            }
        } else {
            // there was no info with valid TS. Overwrite with whatever is parsed.
#if USE_POSITION_NMEA_PIMPL
            pimpl->nmeaSentences.append(QByteArray(buf, size));
#endif
            propagateAttributes(pos, m_update);
            m_update = pos;
            m_timer.stop();
        }
    }

    if (m_updateParsed) {
        if (m_pushDelay < 0)
            notifyNewUpdate();
        else
            m_timer.start();
    }
}

void QNmeaRealTimeReader::notifyNewUpdate()
{
    const bool newerTime = m_update.timestamp().time() > m_lastPushedTS.time();
    const bool newerDate = (m_update.timestamp().date().isValid()
                            && m_lastPushedTS.date().isValid()
                            && m_update.timestamp().date() > m_lastPushedTS.date());
    if (newerTime || newerDate) {
        m_proxy->notifyNewUpdate(&m_update, m_hasFix);
        m_lastPushedTS = m_update.timestamp();
    }
    m_timer.stop();
}


//============================================================

QNmeaSimulatedReader::QNmeaSimulatedReader(QNmeaPositionInfoSourcePrivate *sourcePrivate)
        : QNmeaReader(sourcePrivate),
        m_currTimerId(-1),
        m_hasValidDateTime(false)
{
}

QNmeaSimulatedReader::~QNmeaSimulatedReader()
{
    if (m_currTimerId > 0)
        killTimer(m_currTimerId);
}

void QNmeaSimulatedReader::readAvailableData()
{
    if (m_currTimerId > 0)     // we are already reading
        return;

    if (!m_hasValidDateTime) {      // first update
        Q_ASSERT(m_proxy->m_device && (m_proxy->m_device->openMode() & QIODevice::ReadOnly));

        if (!setFirstDateTime()) {
            //m_proxy->notifyReachedEndOfFile();
            qWarning("QNmeaPositionInfoSource: cannot find NMEA sentence with valid date & time");
            return;
        }

        m_hasValidDateTime = true;
        simulatePendingUpdate();

    } else {
        // previously read to EOF, but now new data has arrived
        processNextSentence();
    }
}

static int processSentence(QGeoPositionInfo &info,
                           QByteArray &m_nextLine,
                           QNmeaPositionInfoSourcePrivate *m_proxy,
                           QQueue<QPendingGeoPositionInfo> &m_pendingUpdates,
                           bool &hasFix)
{
    int timeToNextUpdate = -1;
    QDateTime prevTs;
    if (!m_pendingUpdates.isEmpty())
        prevTs = m_pendingUpdates.head().info.timestamp();

    // find the next update with a valid time (as long as the time is valid,
    // we can calculate when the update should be emitted)
    while (!m_nextLine.isEmpty() || (m_proxy->m_device && m_proxy->m_device->bytesAvailable() > 0)) {
        char static_buf[1024];
        char *buf = static_buf;
        QByteArray nextLine;
        qint64 size = 0;
        if (!m_nextLine.isEmpty()) {
            // Read something in the previous call, but TS was later.
            size = m_nextLine.size();
            nextLine = m_nextLine;
            m_nextLine.clear();
            buf = nextLine.data();
        } else {
            size = m_proxy->m_device->readLine(buf, sizeof(static_buf));
        }

        if (size <= 0)
            continue;

        const QTime infoTime = info.timestamp().time(); // if info has been set, time must be valid.
        const QDate infoDate = info.timestamp().date(); // this one might not be valid, as some sentences do not contain it

        /*
             Packets containing time information are GGA, RMC, ZDA, GLL:

             GGA : GPS fix data                           - only time
             GLL : geographic latitude and longitude      - only time
             RMC : recommended minimum FPOS/transit data  - date and time
             ZDA : only timestamp                         - date and time

             QLocationUtils is currently also capable of parsing VTG and GSA sentences:

             VTG: containing Track made good and ground speed
             GSA: overall satellite data, w. accuracies (ends up into PositionInfo)

             Since these sentences contain no timestamp, their content will be merged with the content
             from any prior sentence that had timestamp info, if any is available.
         */

        QGeoPositionInfoPrivateNmea *pimpl = new QGeoPositionInfoPrivateNmea;
        QGeoPositionInfo pos(*pimpl);
        if (m_proxy->parsePosInfoFromNmeaData(
                QByteArrayView{buf, static_cast<qsizetype>(size)}, &pos, &hasFix)) {
            // Date may or may not be valid, as some packets do not have date.
            // If date isn't valid, match is performed on time only.
            // Hence, make sure that packet blocks are generated with
            // the sentences containing the full timestamp (e.g., GPRMC) *first* !
            if (infoTime.isValid()) {
                if (pos.timestamp().time().isValid()) {
                    const bool newerTime = infoTime < pos.timestamp().time();
                    const bool newerDate = (infoDate.isValid() // if time is valid but one date or both are not,
                                            && pos.timestamp().date().isValid()
                                            && infoDate < pos.timestamp().date());
                    if (newerTime || newerDate) {
                        // Effectively read data for different update, that is also newer, so copy buf into m_nextLine
                        m_nextLine = QByteArray(buf, size);
                        break;
                    } else {
                        if (infoTime == pos.timestamp().time())
                            // timestamps match -- merge into info
                            mergePositions(info, pos, QByteArray(buf, size));
                        // else discard out of order outdated info.
                    }
                } else {
                    // no timestamp available -- merge into info
                    mergePositions(info, pos, QByteArray(buf, size));
                }
            } else {
                // there was no info with valid TS. Overwrite with whatever is parsed.
#if USE_POSITION_NMEA_PIMPL
                pimpl->nmeaSentences.append(QByteArray(buf, size));
#endif
                info = pos;
            }

            if (prevTs.time().isValid()) {
                timeToNextUpdate = msecsTo(prevTs, info.timestamp());
                if (timeToNextUpdate < 0) // Somehow parsing expired packets, reset info
                    info = QGeoPositionInfo(*new QGeoPositionInfoPrivateNmea);
            }
        }
    }

    return timeToNextUpdate;
}

bool QNmeaSimulatedReader::setFirstDateTime()
{
    // find the first update with valid date and time
    QGeoPositionInfo info(*new QGeoPositionInfoPrivateNmea);
    bool hasFix = false;
    processSentence(info, m_nextLine, m_proxy, m_pendingUpdates, hasFix);

    if (info.timestamp().time().isValid()) { // NMEA may have sentences with only time and no date. These would generate invalid positions
        QPendingGeoPositionInfo pending;
        pending.info = info;
        pending.hasFix = hasFix;
        m_pendingUpdates.enqueue(pending);
        return true;
    }
    return false;
}

void QNmeaSimulatedReader::simulatePendingUpdate()
{
    if (!m_pendingUpdates.isEmpty()) {
        // will be dequeued in processNextSentence()
        QPendingGeoPositionInfo &pending = m_pendingUpdates.head();
        m_proxy->notifyNewUpdate(&pending.info, pending.hasFix);
    }

    processNextSentence();
}

void QNmeaSimulatedReader::timerEvent(QTimerEvent *event)
{
    killTimer(event->timerId());
    m_currTimerId = -1;
    simulatePendingUpdate();
}

void QNmeaSimulatedReader::processNextSentence()
{
    QGeoPositionInfo info(*new QGeoPositionInfoPrivateNmea);
    bool hasFix = false;

    int timeToNextUpdate = processSentence(info, m_nextLine, m_proxy, m_pendingUpdates, hasFix);
    if (timeToNextUpdate < 0)
        return;

    m_pendingUpdates.dequeue();

    QPendingGeoPositionInfo pending;
    pending.info = info;
    pending.hasFix = hasFix;
    m_pendingUpdates.enqueue(pending);
    m_currTimerId = startTimer(timeToNextUpdate);
}


//============================================================


QNmeaPositionInfoSourcePrivate::QNmeaPositionInfoSourcePrivate(QNmeaPositionInfoSource *parent, QNmeaPositionInfoSource::UpdateMode updateMode)
        : QObject(parent),
        m_updateMode(updateMode),
        m_device(0),
        m_invokedStart(false),
        m_positionError(QGeoPositionInfoSource::UnknownSourceError),
        m_userEquivalentRangeError(qQNaN()),
        m_source(parent),
        m_nmeaReader(0),
        m_updateTimer(0),
        m_requestTimer(0),
        m_horizontalAccuracy(qQNaN()),
        m_verticalAccuracy(qQNaN()),
        m_noUpdateLastInterval(false),
        m_updateTimeoutSent(false),
        m_connectedReadyRead(false)
{
}

QNmeaPositionInfoSourcePrivate::~QNmeaPositionInfoSourcePrivate()
{
    delete m_nmeaReader;
    delete m_updateTimer;
}

bool QNmeaPositionInfoSourcePrivate::openSourceDevice()
{
    if (!m_device) {
        qWarning("QNmeaPositionInfoSource: no QIODevice data source, call setDevice() first");
        return false;
    }

    if (!m_device->isOpen() && !m_device->open(QIODevice::ReadOnly)) {
        qWarning("QNmeaPositionInfoSource: cannot open QIODevice data source");
        return false;
    }

    connect(m_device, SIGNAL(aboutToClose()), SLOT(sourceDataClosed()));
    connect(m_device, SIGNAL(readChannelFinished()), SLOT(sourceDataClosed()));
    connect(m_device, SIGNAL(destroyed()), SLOT(sourceDataClosed()));

    return true;
}

void QNmeaPositionInfoSourcePrivate::sourceDataClosed()
{
    if (m_nmeaReader && m_device && m_device->bytesAvailable())
        m_nmeaReader->readAvailableData();
}

void QNmeaPositionInfoSourcePrivate::readyRead()
{
    if (m_nmeaReader)
        m_nmeaReader->readAvailableData();
}

bool QNmeaPositionInfoSourcePrivate::initialize()
{
    if (m_nmeaReader)
        return true;

    if (!openSourceDevice())
        return false;

    if (m_updateMode == QNmeaPositionInfoSource::RealTimeMode)
        m_nmeaReader = new QNmeaRealTimeReader(this);
    else
        m_nmeaReader = new QNmeaSimulatedReader(this);

    return true;
}

void QNmeaPositionInfoSourcePrivate::prepareSourceDevice()
{
    // some data may already be available
    if (m_updateMode == QNmeaPositionInfoSource::SimulationMode) {
        if (m_nmeaReader && m_device->bytesAvailable())
            m_nmeaReader->readAvailableData();
    }

    if (!m_connectedReadyRead) {
        connect(m_device, SIGNAL(readyRead()), SLOT(readyRead()));
        m_connectedReadyRead = true;
    }
}

bool QNmeaPositionInfoSourcePrivate::parsePosInfoFromNmeaData(QByteArrayView data,
        QGeoPositionInfo *posInfo, bool *hasFix)
{
    return m_source->parsePosInfoFromNmeaData(data, posInfo, hasFix);
}

void QNmeaPositionInfoSourcePrivate::startUpdates()
{
    if (m_invokedStart)
        return;

    m_positionError = QGeoPositionInfoSource::NoError;

    m_invokedStart = true;
    m_pendingUpdate = QGeoPositionInfo();
    m_noUpdateLastInterval = false;

    bool initialized = initialize();
    if (!initialized) {
        m_source->setError(QGeoPositionInfoSource::AccessError);
        return;
    }

    if (m_updateMode == QNmeaPositionInfoSource::RealTimeMode) {
        // skip over any buffered data - we only want the newest data.
        // Don't do this in requestUpdate. In that case bufferedData is good to have/use.
        if (m_device->bytesAvailable()) {
            if (m_device->isSequential())
                m_device->readAll();
            else
                m_device->seek(m_device->bytesAvailable());
        }
    }

    if (m_updateTimer)
        m_updateTimer->stop();

    if (m_source->updateInterval() > 0) {
        if (!m_updateTimer)
            m_updateTimer = new QBasicTimer;
        m_updateTimer->start(m_source->updateInterval(), this);
    }

    if (initialized)
        prepareSourceDevice();
}

void QNmeaPositionInfoSourcePrivate::stopUpdates()
{
    m_invokedStart = false;
    if (m_updateTimer)
        m_updateTimer->stop();
    m_pendingUpdate = QGeoPositionInfo();
    m_noUpdateLastInterval = false;
}

void QNmeaPositionInfoSourcePrivate::requestUpdate(int msec)
{
    if (m_requestTimer && m_requestTimer->isActive())
        return;

    m_positionError = QGeoPositionInfoSource::NoError;

    if (msec <= 0 || msec < m_source->minimumUpdateInterval()) {
        m_source->setError(QGeoPositionInfoSource::UpdateTimeoutError);
        return;
    }

    if (!m_requestTimer) {
        m_requestTimer = new QTimer(this);
        connect(m_requestTimer, SIGNAL(timeout()), SLOT(updateRequestTimeout()));
    }

    bool initialized = initialize();
    if (!initialized) {
        m_source->setError(QGeoPositionInfoSource::UpdateTimeoutError);
        return;
    }

    m_requestTimer->start(msec);
    prepareSourceDevice();
}

void QNmeaPositionInfoSourcePrivate::updateRequestTimeout()
{
    m_requestTimer->stop();
    m_source->setError(QGeoPositionInfoSource::UpdateTimeoutError);
}

void QNmeaPositionInfoSourcePrivate::notifyNewUpdate(QGeoPositionInfo *update, bool hasFix)
{
    // include <QDebug> before uncommenting
    //qDebug() << "QNmeaPositionInfoSourcePrivate::notifyNewUpdate()" << update->timestamp() << hasFix << m_invokedStart << (m_requestTimer && m_requestTimer->isActive());

    QDate date = update->timestamp().date();
    if (date.isValid()) {
        m_currentDate = date;
    } else {
        // some sentence have time but no date
        QTime time = update->timestamp().time();
        if (time.isValid() && m_currentDate.isValid())
            update->setTimestamp(QDateTime(m_currentDate, time, QTimeZone::UTC));
    }

    // Some attributes are sent in separate NMEA sentences. Save and restore the accuracy
    // measurements.
    if (update->hasAttribute(QGeoPositionInfo::HorizontalAccuracy))
        m_horizontalAccuracy = update->attribute(QGeoPositionInfo::HorizontalAccuracy);
    else if (!qIsNaN(m_horizontalAccuracy))
        update->setAttribute(QGeoPositionInfo::HorizontalAccuracy, m_horizontalAccuracy);

    if (update->hasAttribute(QGeoPositionInfo::VerticalAccuracy))
        m_verticalAccuracy = update->attribute(QGeoPositionInfo::VerticalAccuracy);
    else if (!qIsNaN(m_verticalAccuracy))
        update->setAttribute(QGeoPositionInfo::VerticalAccuracy, m_verticalAccuracy);

    if (hasFix && update->isValid()) {
        if (m_requestTimer && m_requestTimer->isActive()) { // User called requestUpdate()
            m_requestTimer->stop();
            emitUpdated(*update);
        } else if (m_invokedStart) { // user called startUpdates()
            if (m_updateTimer && m_updateTimer->isActive()) { // update interval > 0
                // for periodic updates, only want the most recent update
                m_pendingUpdate = *update; // Set what to send in timerEvent()
                if (m_noUpdateLastInterval) {
                    // if the update was invalid when timerEvent was last called, a valid update
                    // should be sent ASAP
                    emitPendingUpdate();
                    m_noUpdateLastInterval = false;
                }
            } else { // update interval <= 0
                emitUpdated(*update);
            }
        }
        m_lastUpdate = *update; // Set in any case, if update is valid. Used in lastKnownPosition().
    }
}

void QNmeaPositionInfoSourcePrivate::timerEvent(QTimerEvent *)
{
    emitPendingUpdate();
}

void QNmeaPositionInfoSourcePrivate::emitPendingUpdate()
{
    if (m_pendingUpdate.isValid()) {
        m_updateTimeoutSent = false;
        m_noUpdateLastInterval = false;
        emitUpdated(m_pendingUpdate);
        m_pendingUpdate = QGeoPositionInfo();
    } else { // invalid update
        if (m_noUpdateLastInterval && !m_updateTimeoutSent) {
            m_updateTimeoutSent = true;
            m_pendingUpdate = QGeoPositionInfo(); // Invalid already, but clear just in case.
            m_source->setError(QGeoPositionInfoSource::UpdateTimeoutError);
        }
        m_noUpdateLastInterval = true;
    }
}

void QNmeaPositionInfoSourcePrivate::emitUpdated(const QGeoPositionInfo &update)
{
    // check for duplication already done in QNmeaRealTimeReader::notifyNewUpdate
    // and QNmeaRealTimeReader::readAvailableData
    m_lastUpdate = update;
    emit m_source->positionUpdated(update);
}

//=========================================================

/*!
    \class QNmeaPositionInfoSource
    \inmodule QtPositioning
    \ingroup QtPositioning-positioning
    \since 5.2

    \brief The QNmeaPositionInfoSource class provides positional information using a NMEA data source.

    NMEA is a commonly used protocol for the specification of one's global
    position at a certain point in time. The QNmeaPositionInfoSource class reads NMEA
    data and uses it to provide positional data in the form of
    QGeoPositionInfo objects.

    A QNmeaPositionInfoSource instance operates in either \l {RealTimeMode} or
    \l {SimulationMode}. These modes allow NMEA data to be read from either a
    live source of positional data, or replayed for simulation purposes from
    previously recorded NMEA data.

    The source of NMEA data is set with setDevice().

    Use startUpdates() to start receiving regular position updates and stopUpdates() to stop these
    updates.  If you only require updates occasionally, you can call requestUpdate() to request a
    single update.

    In both cases the position information is received via the positionUpdated() signal and the
    last known position can be accessed with lastKnownPosition().

    QNmeaPositionInfoSource supports reporting the accuracy of the horizontal and vertical position.
    To enable position accuracy reporting an estimate of the User Equivalent Range Error associated
    with the NMEA source must be set with setUserEquivalentRangeError().
*/


/*!
    \enum QNmeaPositionInfoSource::UpdateMode
    Defines the available update modes.

    \value RealTimeMode Positional data is read and distributed from the data source as it becomes available. Use this mode if you are using a live source of positional data (for example, a GPS hardware device).
    \value SimulationMode The data and time information in the NMEA source data is used to provide positional updates at the rate at which the data was originally recorded. Use this mode if the data source contains previously recorded NMEA data and you want to replay the data for simulation purposes.
*/


/*!
    Constructs a QNmeaPositionInfoSource instance with the given \a parent
    and \a updateMode.
*/
QNmeaPositionInfoSource::QNmeaPositionInfoSource(UpdateMode updateMode, QObject *parent)
        : QGeoPositionInfoSource(parent),
        d(new QNmeaPositionInfoSourcePrivate(this, updateMode))
{
}

/*!
    Destroys the position source.
*/
QNmeaPositionInfoSource::~QNmeaPositionInfoSource()
{
    delete d;
}

/*!
    Sets the User Equivalent Range Error (UERE) to \a uere. The UERE is used in calculating an
    estimate of the accuracy of the position information reported by the position info source. The
    UERE should be set to a value appropriate for the GPS device which generated the NMEA stream.

    The true UERE value is calculated from multiple error sources including errors introduced by
    the satellites and signal propogation delays through the atmosphere as well as errors
    introduced by the receiving GPS equipment. For details on GPS accuracy see
    \l {https://web.archive.org/web/20161212144906if_/http://edu-observatory.org/gps/gps_accuracy.html} {Sam J. Wormley, GPS Errors & Estimating Your Reveiver's Accuracy}.

    A typical value for UERE is approximately 5.1.

    \since 5.3

    \sa userEquivalentRangeError()
*/
void QNmeaPositionInfoSource::setUserEquivalentRangeError(double uere)
{
    d->m_userEquivalentRangeError = uere;
}

/*!
    Returns the current User Equivalent Range Error (UERE). The UERE is used in calculating an
    estimate of the accuracy of the position information reported by the position info source. The
    default value is NaN which means no accuracy information will be provided.

    \since 5.3

    \sa setUserEquivalentRangeError()
*/
double QNmeaPositionInfoSource::userEquivalentRangeError() const
{
    return d->m_userEquivalentRangeError;
}

/*!
    Parses an NMEA sentence string into a QGeoPositionInfo.

    The default implementation will parse standard NMEA sentences.
    This method should be reimplemented in a subclass whenever the need to deal with non-standard
    NMEA sentences arises.

    The parser reads \a size bytes from \a data and uses that information to setup \a posInfo and
    \a hasFix.  If \a hasFix is set to false then \a posInfo may contain only the time or the date
    and the time.

    Returns true if the sentence was successfully parsed, otherwise returns false and should not
    modifiy \a posInfo or \a hasFix.
*/

#if QT_DEPRECATED_SINCE(7, 0)
bool QNmeaPositionInfoSource::parsePosInfoFromNmeaData(const char *data, int size,
                                                       QGeoPositionInfo *posInfo, bool *hasFix)
{
#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
    return QLocationUtils::getPosInfoFromNmea(QByteArrayView{data, size}, posInfo,
                                              d->m_userEquivalentRangeError, hasFix);
#else
    return parsePosInfoFromNmeaData(QByteArrayView{data, size}, posInfo, hasFix);
#endif
}
#endif // QT_DEPRECATED_SINCE(7, 0)

bool QNmeaPositionInfoSource::parsePosInfoFromNmeaData(QByteArrayView data,
                                                       QGeoPositionInfo *posInfo, bool *hasFix)
{
#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
    return parsePosInfoFromNmeaData(data.data(), static_cast<int>(data.size()),
                                                             posInfo, hasFix);
#else
    return QLocationUtils::getPosInfoFromNmea(data, posInfo,
                                              d->m_userEquivalentRangeError, hasFix);
#endif
}


/*!
    Returns the update mode.
*/
QNmeaPositionInfoSource::UpdateMode QNmeaPositionInfoSource::updateMode() const
{
    return d->m_updateMode;
}

/*!
    Sets the NMEA data source to \a device. If the device is not open, it
    will be opened in QIODevice::ReadOnly mode.

    The source device can only be set once and must be set before calling
    startUpdates() or requestUpdate().

    \b {Note:} The \a device must emit QIODevice::readyRead() for the
    source to be notified when data is available for reading.
    QNmeaPositionInfoSource does not assume the ownership of the device,
    and hence does not deallocate it upon destruction.
*/
void QNmeaPositionInfoSource::setDevice(QIODevice *device)
{
    if (device != d->m_device) {
        if (!d->m_device)
            d->m_device = device;
        else
            qWarning("QNmeaPositionInfoSource: source device has already been set");
    }
}

/*!
    Returns the NMEA data source.
*/
QIODevice *QNmeaPositionInfoSource::device() const
{
    return d->m_device;
}

/*!
    \reimp
*/
void QNmeaPositionInfoSource::setUpdateInterval(int msec)
{
    int interval = msec;
    if (interval != 0)
        interval = qMax(msec, minimumUpdateInterval());
    QGeoPositionInfoSource::setUpdateInterval(interval);
    if (d->m_invokedStart) {
        d->stopUpdates();
        d->startUpdates();
    }
}

/*!
    \reimp
*/
void QNmeaPositionInfoSource::startUpdates()
{
    d->startUpdates();
}

/*!
    \reimp
*/
void QNmeaPositionInfoSource::stopUpdates()
{
    d->stopUpdates();
}

/*!
    \reimp
*/
void QNmeaPositionInfoSource::requestUpdate(int msec)
{
    d->requestUpdate(msec == 0 ? 60000 * 5 : msec); // 5min default timeout
}

/*!
    \reimp
*/
QGeoPositionInfo QNmeaPositionInfoSource::lastKnownPosition(bool) const
{
    // the bool value does not matter since we only use satellite positioning
    return d->m_lastUpdate;
}

/*!
    \reimp
*/
QGeoPositionInfoSource::PositioningMethods QNmeaPositionInfoSource::supportedPositioningMethods() const
{
    return SatellitePositioningMethods;
}

/*!
    \reimp
*/
int QNmeaPositionInfoSource::minimumUpdateInterval() const
{
    return 2; // Some chips are capable of over 100 updates per seconds.
}

/*!
    \reimp
*/
QGeoPositionInfoSource::Error QNmeaPositionInfoSource::error() const
{
    return d->m_positionError;
}

void QNmeaPositionInfoSource::setError(QGeoPositionInfoSource::Error positionError)
{
    d->m_positionError = positionError;
    if (d->m_positionError != QGeoPositionInfoSource::NoError)
        emit QGeoPositionInfoSource::errorOccurred(positionError);
}

QT_END_NAMESPACE

#include "moc_qnmeapositioninfosource_p.cpp"
#include "moc_qnmeapositioninfosource.cpp"
