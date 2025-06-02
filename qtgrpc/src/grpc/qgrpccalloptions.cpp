// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGrpc/qgrpccalloptions.h>

#include <QtCore/qbytearray.h>
#include <QtCore/qdebug.h>
#include <QtCore/qvariant.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*!
    \class QGrpcCallOptions
    \inmodule QtGrpc
    \brief The QGrpcCallOptions class offers various options for fine-tuning
    individual RPCs.
    \since 6.6

    QGrpcCallOptions lets you customize individual remote procedure calls (RPCs).
    The generated client interface provides access points to pass the QGrpcCallOptions.
    These options supersede the ones set via QGrpcChannelOptions.

    To configure the default options shared by RPCs, use QGrpcChannelOptions.
*/

class QGrpcCallOptionsPrivate : public QSharedData
{
public:
    std::optional<std::chrono::milliseconds> timeout;
    QHash<QByteArray, QByteArray> metadata;
};

QT_DEFINE_QESDP_SPECIALIZATION_DTOR(QGrpcCallOptionsPrivate)

/*!
    Default-constructs an empty QGrpcCallOptions.
*/
QGrpcCallOptions::QGrpcCallOptions() : d_ptr(new QGrpcCallOptionsPrivate())
{
}

/*!
    Destroys the QGrpcCallOptions.
*/
QGrpcCallOptions::~QGrpcCallOptions() = default;

/*!
    Copy-constructs a QGrpcCallOptions from \a other.
*/
QGrpcCallOptions::QGrpcCallOptions(const QGrpcCallOptions &other) = default;

/*!
    Assigns \a other to this QGrpcCallOptions and returns a reference to the
    updated object.
*/
QGrpcCallOptions &QGrpcCallOptions::operator=(const QGrpcCallOptions &other) = default;

/*!
    \fn QGrpcCallOptions::QGrpcCallOptions(QGrpcCallOptions &&other)

    Move-constructs a new QGrpcCallOptions from \a other.

    \include qtgrpc-shared.qdocinc move-note-desc
*/

/*!
    \fn QGrpcCallOptions &QGrpcCallOptions::operator=(QGrpcCallOptions &&other)

    Move-assigns \a other to this QGrpcCallOptions and returns a reference to
    the updated object.

    \include qtgrpc-shared.qdocinc move-note-desc
*/

/*!
    \since 6.8

    \include qtgrpc-shared.qdocinc qvariant-desc
*/
QGrpcCallOptions::operator QVariant() const
{
    return QVariant::fromValue(*this);
}

/*!
    \since 6.8
    \fn void QGrpcCallOptions::swap(QGrpcCallOptions &other)

    \include qtgrpc-shared.qdocinc swap-desc
*/

/*!
    Sets the \a timeout for a specific RPC and returns a reference to the
    updated object.

//! [set-deadline-desc]
    A deadline sets the limit for how long a client is willing to wait for a
    response from a server. The actual deadline is computed by adding the \a
    timeout to the start time of the RPC.

    The deadline applies to the entire lifetime of an RPC, which includes
    receiving the final QGrpcStatus for a previously started call and can thus
    be unwanted for (long-lived) streams.
//! [set-deadline-desc]

    \note Setting this field overrides the value set by
    QGrpcChannelOptions::setDeadline() for a specific RPC.
*/
QGrpcCallOptions &QGrpcCallOptions::setDeadlineTimeout(std::chrono::milliseconds timeout)
{
    if (d_ptr->timeout == timeout)
        return *this;
    d_ptr.detach();
    Q_D(QGrpcCallOptions);
    d->timeout = timeout;
    return *this;
}

/*!
    \fn QGrpcCallOptions &QGrpcCallOptions::setMetadata(const QHash<QByteArray, QByteArray> &metadata)
    \fn QGrpcCallOptions &QGrpcCallOptions::setMetadata(QHash<QByteArray, QByteArray> &&metadata)

    Sets the client \a metadata for a specific RPC and returns a reference to the
    updated object.

//! [set-metadata-desc]
    QGrpcHttp2Channel converts the metadata into appropriate HTTP/2 headers
    which will be added to the HTTP/2 request.
//! [set-metadata-desc]

    \note Setting this field overrides the value set by
    QGrpcChannelOptions::setMetadata() for a specific RPC.
*/
QGrpcCallOptions &QGrpcCallOptions::setMetadata(const QHash<QByteArray, QByteArray> &metadata)
{
    if (d_ptr->metadata == metadata)
        return *this;
    d_ptr.detach();
    Q_D(QGrpcCallOptions);
    d->metadata = metadata;
    return *this;
}

QGrpcCallOptions &QGrpcCallOptions::setMetadata(QHash<QByteArray, QByteArray> &&metadata)
{
    if (d_ptr->metadata == metadata)
        return *this;
    d_ptr.detach();
    Q_D(QGrpcCallOptions);
    d->metadata = std::move(metadata);
    return *this;
}

/*!
    Returns the timeout duration that is used to calculate the deadline for a
    specific RPC.

    If this field is unset, returns an empty \c {std::optional}.
*/
std::optional<std::chrono::milliseconds> QGrpcCallOptions::deadlineTimeout() const noexcept
{
    Q_D(const QGrpcCallOptions);
    return d->timeout;
}

/*!
    \fn const QHash<QByteArray, QByteArray> &QGrpcCallOptions::metadata() const &
    \fn QHash<QByteArray, QByteArray> QGrpcCallOptions::metadata() &&

    Returns the client metadata for a specific RPC.
    If this field is unset, returns empty metadata.
*/
const QHash<QByteArray, QByteArray> &QGrpcCallOptions::metadata() const & noexcept
{
    Q_D(const QGrpcCallOptions);
    return d->metadata;
}

QHash<QByteArray, QByteArray> QGrpcCallOptions::metadata() &&
{
    Q_D(QGrpcCallOptions);
    if (d->ref.loadRelaxed() != 1) // return copy if shared
        return { d->metadata };
    return std::move(d->metadata);
}

#ifndef QT_NO_DEBUG_STREAM
/*!
    \since 6.8
    \fn QDebug QGrpcCallOptions::operator<<(QDebug debug, const QGrpcCallOptions &callOpts)

    Writes \a callOpts to the specified stream \a debug.
*/
QDebug operator<<(QDebug debug, const QGrpcCallOptions &callOpts)
{
    const QDebugStateSaver save(debug);
    debug.nospace().noquote();
    debug << "QGrpcCallOptions(deadline: " << callOpts.deadlineTimeout()
          << ", metadata: " << callOpts.metadata() << ')';
    return debug;
}
#endif

QT_END_NAMESPACE
