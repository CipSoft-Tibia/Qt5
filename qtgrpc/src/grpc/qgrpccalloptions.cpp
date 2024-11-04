// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qtgrpcglobal_p.h>

#include "qgrpccalloptions.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*!
    \class QGrpcCallOptions
    \inmodule QtGrpc
    \brief The QGrpcCallOptions is an storage class used to set additional call options.
    \since 6.6

    QGrpcCallOptions provides a set of functions to access the call options
    that are used by gRPC channels to communicate with the services.
*/

struct QGrpcCallOptionsPrivate
{
public:
    std::optional<QUrl> host;
    std::optional<std::chrono::milliseconds> deadline;
    QGrpcMetadata metadata;
};

/*!
    Constructs an empty QGrpcCallOptions object.
*/
QGrpcCallOptions::QGrpcCallOptions() : dPtr(std::make_unique<QGrpcCallOptionsPrivate>())
{
}

/*!
    Destroys the QGrpcCallOptions object.
*/
QGrpcCallOptions::~QGrpcCallOptions() = default;

/*!
    Construct a copy of QGrpcCallOptions with \a other object.
*/
QGrpcCallOptions::QGrpcCallOptions(const QGrpcCallOptions &other)
    : dPtr(std::make_unique<QGrpcCallOptionsPrivate>(*other.dPtr))
{
}

/*!
    Assigns \a other to this QGrpcCallOptions and returns a reference to this
    QGrpcCallOptions.
*/
QGrpcCallOptions &QGrpcCallOptions::operator=(const QGrpcCallOptions &other)
{
    if (this != &other)
        *dPtr = *other.dPtr;
    return *this;
}

/*!
    Sets deadline value with \a deadline and returns updated QGrpcCallOptions object.
*/
QGrpcCallOptions &QGrpcCallOptions::withDeadline(std::chrono::milliseconds deadline)
{
    dPtr->deadline = deadline;
    return *this;
}

/*!
    Sets \a metadata for a call and returns updated QGrpcCallOptions object.

    For HTTP2-based channels, \a metadata is converted into HTTP/2 headers, that
    added to the corresponding HTTP/2 request.
*/
QGrpcCallOptions &QGrpcCallOptions::withMetadata(const QGrpcMetadata &metadata)
{
    dPtr->metadata = metadata;
    return *this;
}

/*!
    Returns deadline value for a call.

    Deadline value controls the maximum execution time of an call or a stream.
    This value overrides value set by QGrpcChannelOptions::deadline()
    for a specific call or stream.

    If value was not set returns empty std::optional.
*/
std::optional<std::chrono::milliseconds> QGrpcCallOptions::deadline() const noexcept
{
    return dPtr->deadline;
}

/*!
    Returns metadata used for a call.

    If value was not set returns empty QGrpcMetadata.
*/
QGrpcMetadata QGrpcCallOptions::metadata() const
{
    return dPtr->metadata;
}

QT_END_NAMESPACE
