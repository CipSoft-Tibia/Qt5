// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qhttpserverconfiguration.h"

QT_BEGIN_NAMESPACE

/*!
    \class QHttpServerConfiguration
    \since 6.9
    \inmodule QtHttpServer
    \brief The QHttpServerConfiguration class controls server parameters.
*/
class QHttpServerConfigurationPrivate : public QSharedData
{
public:
    quint32 rateLimit = 0;
};

QT_DEFINE_QESDP_SPECIALIZATION_DTOR(QHttpServerConfigurationPrivate)

/*!
    Default constructs a QHttpServerConfiguration object.

    Such a configuration has the following values:
     \list
         \li Rate limit is disabled
     \endlist
*/
QHttpServerConfiguration::QHttpServerConfiguration()
    : d(new QHttpServerConfigurationPrivate)
{
}

/*!
    Copy-constructs this QHttpServerConfiguration.
*/
QHttpServerConfiguration::QHttpServerConfiguration(const QHttpServerConfiguration &) = default;

/*!
    \fn QHttpServerConfiguration::QHttpServerConfiguration(QHttpServerConfiguration &&other) noexcept

    Move-constructs this QHttpServerConfiguration from \a other
*/

/*!
    Copy-assigns \a other to this QHttpServerConfiguration.
*/
QHttpServerConfiguration &QHttpServerConfiguration::operator=(const QHttpServerConfiguration &) = default;

/*!
    \fn QHttpServerConfiguration &QHttpServerConfiguration::operator=(QHttpServerConfiguration &&other) noexcept

    Move-assigns \a other to this QHttpServerConfiguration.
*/

/*!
    Destructor.
*/
QHttpServerConfiguration::~QHttpServerConfiguration()
    = default;

/*!
    Sets \a maxRequests as the maximum number of incoming requests
    per second per IP that will be accepted by QHttpServer.
    If the limit is exceeded, QHttpServer will respond with
    QHttpServerResponder::StatusCode::TooManyRequests.

    \sa rateLimitPerSecond(), QHttpServerResponder::StatusCode
*/
void QHttpServerConfiguration::setRateLimitPerSecond(quint32 maxRequests)
{
    d.detach();
    d->rateLimit = maxRequests;
}

/*!
    Returns maximum number of incoming requests per second per IP
    accepted by the server.

    \sa setRateLimitPerSecond()
*/
quint32 QHttpServerConfiguration::rateLimitPerSecond() const
{
    return d->rateLimit;
}

/*!
    \fn void QHttpServerConfiguration::swap(QHttpServerConfiguration &other)
    \memberswap{configuration}
*/

/*!
    \fn bool QHttpServerConfiguration::operator==(const QHttpServerConfiguration &lhs, const QHttpServerConfiguration &rhs) noexcept
    Returns \c true if \a lhs and \a rhs have the same set of configuration
    parameters.
*/

/*!
    \fn bool QHttpServerConfiguration::operator!=(const QHttpServerConfiguration &lhs, const QHttpServerConfiguration &rhs) noexcept
    Returns \c true if \a lhs and \a rhs do not have the same set of configuration
    parameters.
*/

/*!
    \internal
*/
bool comparesEqual(const QHttpServerConfiguration &lhs, const QHttpServerConfiguration &rhs) noexcept
{
    if (lhs.d == rhs.d)
        return true;

    return lhs.d->rateLimit == rhs.d->rateLimit;
}

QT_END_NAMESPACE
