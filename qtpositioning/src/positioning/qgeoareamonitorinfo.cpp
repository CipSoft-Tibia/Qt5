// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QGeoAreaMonitorInfo>
#include <QDateTime>
#include <QSharedData>
#include <QUuid>
#include <QDataStream>

#ifndef QT_NO_DEBUG_STREAM
#include <QDebug>
#endif

QT_BEGIN_NAMESPACE

QT_IMPL_METATYPE_EXTERN(QGeoAreaMonitorInfo)

/*!
    \class QGeoAreaMonitorInfo
    \inmodule QtPositioning
    \since 5.2
    \ingroup QtPositioning-positioning
    \ingroup shared

    \brief The QGeoAreaMonitorInfo class describes the parameters of an area or region
    to be monitored for proximity.

    The purpose of area monitoring is to inform a user when he/she comes close to an area of
    interest. In general such an area is described by a \l QGeoCircle. The circle's center
    represents the place of interest and the area around it identifies the geographical region
    within which notifications are sent.

    A QGeoAreaMonitorInfo object is valid if it has a non-empty name and a valid \l area().
    Such objects must be registered with a \l QGeoAreaMonitorSource to start and stop the
    monitoring process. Note that extensive monitoring can be very resource consuming
    because the positioning engine must remain active and has to match the current position
    with each QGeoAreaMonitorInfo instance.

    To further reduce the burden on the system there are optional attributes which can
    set. Each monitored area can have an expiry date which automatically removes the
    to-be-monitored area from the monitoring source once the expiry date has been reached.
    Another option is to adjust the persistence of a monitored area. A QGeoAreaMonitorInfo
    that \l isPersistent() will remain active beyond
    the current applications lifetime. If an area is entered while the monitoring
    application is not running the application will be started. Note that this feature is
    not available on all platforms. Its availability can be checked via
    \l QGeoAreaMonitorSource::supportedAreaMonitorFeatures().

    \sa QGeoAreaMonitorSource

 */

class QGeoAreaMonitorInfoPrivate : public QSharedData
{
public:
    QGeoAreaMonitorInfoPrivate() : QSharedData(), persistent(false) {}
    QGeoAreaMonitorInfoPrivate(const QGeoAreaMonitorInfoPrivate &other)
            : QSharedData(other)
    {
        uid = other.uid;
        name = other.name;
        shape = other.shape;
        persistent = other.persistent;
        notificationParameters = other.notificationParameters;
        expiry = other.expiry;
    }
    ~QGeoAreaMonitorInfoPrivate() {}

    QUuid uid;
    QString name;
    QGeoShape shape;
    bool persistent;
    QVariantMap notificationParameters;
    QDateTime expiry;
};

/*!
    Constructs a QGeoAreaMonitorInfo object with the specified \a name.

    \sa name()
 */
QGeoAreaMonitorInfo::QGeoAreaMonitorInfo(const QString &name)
{
    d = new QGeoAreaMonitorInfoPrivate;
    d->name = name;
    d->uid = QUuid::createUuid();
}

/*!
     Constructs a QGeoAreaMonitorInfo object as a copy of \a other.
 */
QGeoAreaMonitorInfo::QGeoAreaMonitorInfo(const QGeoAreaMonitorInfo &other)
    : d(other.d)
{
}

/*!
    \fn QGeoAreaMonitorInfo::QGeoAreaMonitorInfo(QGeoAreaMonitorInfo &&other) noexcept
    \since 6.2

    Constructs a QGeoAreaMonitorInfo object by moving from \a other.

    Note that a moved-from QGeoAreaMonitorInfo can only be destroyed or
    assigned to. The effect of calling other functions than the destructor
    or one of the assignment operators is undefined.
*/

/*!
    Destructor
 */
QGeoAreaMonitorInfo::~QGeoAreaMonitorInfo()
{
}

QT_DEFINE_QESDP_SPECIALIZATION_DTOR(QGeoAreaMonitorInfoPrivate)

/*!
    Assigns \a other to this QGeoAreaMonitorInfo object and returns a reference
    to this QGeoAreaMonitorInfo object.
 */
QGeoAreaMonitorInfo &QGeoAreaMonitorInfo::operator=(const QGeoAreaMonitorInfo &other)
{
    d = other.d;
    return *this;
}

/*!
    \fn QGeoAreaMonitorInfo &QGeoAreaMonitorInfo::operator=(QGeoAreaMonitorInfo &&other) noexcept
    \since 6.2

    Move-assigns \a other to this QGeoAreaMonitorInfo object and returns a
    reference to this QGeoAreaMonitorInfo object.

    Note that a moved-from QGeoAreaMonitorInfo can only be destroyed or
    assigned to. The effect of calling other functions than the destructor
    or one of the assignment operators is undefined.
*/

/*!
    \fn bool QGeoAreaMonitorInfo::operator==(const QGeoAreaMonitorInfo &lhs, const QGeoAreaMonitorInfo &rhs)

    Returns \c true if all of the \a lhs object's values are the same as those
    of \a rhs object. Otherwise returns \c false.
*/

/*!
    \fn bool QGeoAreaMonitorInfo::operator!=(const QGeoAreaMonitorInfo &lhs, const QGeoAreaMonitorInfo &rhs)

    Returns \c true if any of the \a lhs object's values are not the same as
    those of \a rhs object. Otherwise returns \c false.
*/

/*!
    Returns the name of the QGeoAreaMonitorInfo object. The name should be used
    for user-visibility purposes.
 */
QString QGeoAreaMonitorInfo::name() const
{
    return d->name;
}

/*!
    Sets the user visibile \a name.
 */
void QGeoAreaMonitorInfo::setName(const QString &name)
{
    if (d->name != name) {
        d.detach();
        d->name = name;
    }
}

/*!
    Returns the identifier of the QGeoAreaMonitorInfo object.
    The identifier is automatically generated upon construction of a new
    QGeoAreaMonitorInfo object.
*/

QString QGeoAreaMonitorInfo::identifier() const
{
    return d->uid.toString();
}

/*!
    Returns true, if the monitor is valid. A valid QGeoAreaMonitorInfo has a non-empty name()
    and the monitored area is not \l {QGeoShape::isEmpty()}{empty()}.
    Otherwise this function returns false.
 */
bool QGeoAreaMonitorInfo::isValid() const
{
    return (!d->name.isEmpty() && !d->shape.isEmpty());
}

/*!
    Returns the boundaries of the to-be-monitored area. This area must not be empty.

    \sa setArea()
 */
QGeoShape QGeoAreaMonitorInfo::area() const
{
    return d->shape;
}

/*!
    Sets the to-be-monitored area to \a newShape.

    \sa area()
 */
void QGeoAreaMonitorInfo::setArea(const QGeoShape &newShape)
{
    d.detach();
    d->shape = newShape;
}

/*!
    Returns the expiry date.

    After an active QGeoAreaMonitorInfo has expired the region is no longer monitored
    and the QGeoAreaMonitorInfo object is removed from the list of
    \l {QGeoAreaMonitorSource::activeMonitors()}{active monitors}.

    If the expiry \l QDateTime is invalid the QGeoAreaMonitorInfo object is treated as not having
    an expiry date. This implies an indefinite monitoring period if the object is persistent or
    until the current application closes if the object is non-persistent.

    \sa QGeoAreaMonitorSource::activeMonitors()
 */
QDateTime QGeoAreaMonitorInfo::expiration() const
{
    return d->expiry;
}

/*!
     Sets the expiry date and time to \a expiry.
 */
void QGeoAreaMonitorInfo::setExpiration(const QDateTime &expiry)
{
    d.detach();
    d->expiry = expiry;
}

/*!
    Returns true if the QGeoAreaMonitorInfo is persistent.
    The default value for this property is false.

    A non-persistent QGeoAreaMonitorInfo will be removed by the system once
    the application owning the monitor object stops. Persistent objects remain
    active and can be retrieved once the application restarts.

    If the system triggers an event associated to a persistent QGeoAreaMonitorInfo
    the relevant application will be re-started and the appropriate signal emitted.

    \sa setPersistent()
 */
bool QGeoAreaMonitorInfo::isPersistent() const
{
    return d->persistent;
}

/*!
    Sets the QGeoAreaMonitorInfo object's persistence to \a isPersistent.

    Note that setting this flag does not imply that \l QGeoAreaMonitorSource
    supports persistent monitoring.
    \l QGeoAreaMonitorSource::supportedAreaMonitorFeatures() can be used to
    check for this feature's availability.

    \sa isPersistent()
 */
void QGeoAreaMonitorInfo::setPersistent(bool isPersistent)
{
    d.detach();
    d->persistent = isPersistent;
}


/*!
    Returns the set of platform specific parameters used by this
    QGeoAreaMonitorInfo.

    \sa setNotificationParameters()
 */
QVariantMap QGeoAreaMonitorInfo::notificationParameters() const
{
    return d->notificationParameters;
}

/*!
    Sets the set of platform specific \a parameters used by QGeoAreaMonitorInfo.

    \sa notificationParameters()
 */
void QGeoAreaMonitorInfo::setNotificationParameters(const QVariantMap &parameters)
{
    d.detach();
    d->notificationParameters = parameters;
}

/*!
    \internal
*/
void QGeoAreaMonitorInfo::detach()
{
    if (d)
        d.detach();
    else
        d = new QGeoAreaMonitorInfoPrivate;
}

bool QGeoAreaMonitorInfo::equals(const QGeoAreaMonitorInfo &lhs, const QGeoAreaMonitorInfo &rhs)
{
    return (lhs.d->name == rhs.d->name &&
            lhs.d->uid == rhs.d->uid &&
            lhs.d->shape == rhs.d->shape &&
            lhs.d->persistent == rhs.d->persistent &&
            lhs.d->expiry == rhs.d->expiry &&
            lhs.d->notificationParameters == rhs.d->notificationParameters);
}

#ifndef QT_NO_DATASTREAM

/*!
    \fn QDataStream &QGeoAreaMonitorInfo::operator<<(QDataStream &stream, const QGeoAreaMonitorInfo &monitor)

    Writes the given \a monitor to the specified \a stream.

    \sa {Serializing Qt Data Types}
*/
QDataStream &QGeoAreaMonitorInfo::dataStreamOut(QDataStream &ds, const QGeoAreaMonitorInfo &monitor)
{
    ds << monitor.name() << monitor.d->uid << monitor.area()
       << monitor.isPersistent() << monitor.notificationParameters() << monitor.expiration();
    return ds;
}

/*!
    \fn QDataStream &QGeoAreaMonitorInfo::operator>>(QDataStream &stream, QGeoAreaMonitorInfo &monitor)

    Reads a area monitoring data from the specified \a stream into the given
    \a monitor.

    \sa {Serializing Qt Data Types}
*/
QDataStream &QGeoAreaMonitorInfo::dataStreamIn(QDataStream &ds, QGeoAreaMonitorInfo &monitor)
{
    QString s;
    ds >> s;
    monitor = QGeoAreaMonitorInfo(s);

    QUuid id;
    ds >> id;
    monitor.d->uid = id;

    QGeoShape shape;
    ds >> shape;
    monitor.setArea(shape);

    bool persistent;
    ds >> persistent;
    monitor.setPersistent(persistent);

    QVariantMap map;
    ds >> map;
    monitor.setNotificationParameters(map);

    QDateTime dt;
    ds >> dt;
    monitor.setExpiration(dt);

    return ds;
}

#endif

#ifndef QT_NO_DEBUG_STREAM
QDebug QGeoAreaMonitorInfo::debugStreaming(QDebug dbg, const QGeoAreaMonitorInfo &monitor)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "QGeoAreaMonitorInfo(\"" << qPrintable(monitor.name())
                  << "\", " << monitor.area()
                  << ", persistent: " << monitor.isPersistent()
                  << ", expiry: " << monitor.expiration() << ")";
    return dbg;
}

#endif

size_t qHash(const QGeoAreaMonitorInfo &key, size_t seed) noexcept
{
    return qHashMulti(seed, key.d->uid);
}

namespace QTest
{

char *toString(const QGeoAreaMonitorInfo &info)
{
    QString result;
    QDebug dbg(&result);
    dbg << info;
    return qstrdup(qPrintable(result));
}

} // namespace QTest

QT_END_NAMESPACE
