// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGrpc/qgrpcchanneloptions.h>
#include <QtGrpc/qgrpcserializationformat.h>
#include <QtGrpc/qtgrpcnamespace.h>

#include <QtCore/qbytearray.h>
#include <QtCore/qdebug.h>
#include <QtCore/qvariant.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;
using namespace QtGrpc;

/*!
    \class QGrpcChannelOptions
    \inmodule QtGrpc
    \since 6.6
    \brief The QGrpcChannelOptions class offers various options for fine-tuning
    a gRPC channel.

    QGrpcChannelOptions lets you customize a \gRPC channel. Some options apply
    to all remote procedure calls (RPCs) that operate on the associated
    channel, which is used to communicate with services.

    Override options for specific RPCs with QGrcCallOptions.

    \note It is up to the channel's implementation to determine the specifics
    of these options.
*/

class QGrpcChannelOptionsPrivate : public QSharedData
{
public:
    std::optional<std::chrono::milliseconds> timeout;
    QHash<QByteArray, QByteArray> metadata;
    QGrpcSerializationFormat serializationFormat;
#if QT_CONFIG(ssl)
    std::optional<QSslConfiguration> sslConfiguration;
#endif
};

QT_DEFINE_QESDP_SPECIALIZATION_DTOR(QGrpcChannelOptionsPrivate)

/*!
    Default-constructs an empty QGrpcChannelOptions.
*/
QGrpcChannelOptions::QGrpcChannelOptions() : d_ptr(new QGrpcChannelOptionsPrivate())
{
}

/*!
    Copy-constructs a QGrpcChannelOptions from \a other.
*/
QGrpcChannelOptions::QGrpcChannelOptions(const QGrpcChannelOptions &other) = default;

/*!
    Assigns \a other to this QGrpcChannelOptions and returns a reference to the
    updated object.
*/
QGrpcChannelOptions &QGrpcChannelOptions::operator=(const QGrpcChannelOptions &other) = default;

/*!
    \fn QGrpcChannelOptions::QGrpcChannelOptions(QGrpcChannelOptions &&other)

    Move-constructs a new QGrpcChannelOptions from \a other.

    \include qtgrpc-shared.qdocinc move-note-desc
*/

/*!
    \fn QGrpcChannelOptions &QGrpcChannelOptions::operator=(QGrpcChannelOptions &&other)

    Move-assigns \a other to this QGrpcChannelOptions and returns a reference
    to the updated object.

    \include qtgrpc-shared.qdocinc move-note-desc
*/

/*!
    \since 6.8
    \fn void QGrpcChannelOptions::swap(QGrpcChannelOptions &other)

    \include qtgrpc-shared.qdocinc swap-desc
*/

/*!
    Destroys the QGrpcChannelOptions.
*/
QGrpcChannelOptions::~QGrpcChannelOptions() = default;

/*!
    \since 6.8
    \include qtgrpc-shared.qdocinc qvariant-desc
*/
QGrpcChannelOptions::operator QVariant() const
{
    return QVariant::fromValue(*this);
}

/*!
    Sets the \a timeout for the channel and returns a reference to the updated
    object.

    \include qgrpccalloptions.cpp set-deadline-desc

    \note The deadline set via the channel options applies to all RPCs that
    operate on the channel, except those overridden by
    QGrpcCallOptions::setDeadline().
*/
QGrpcChannelOptions &QGrpcChannelOptions::setDeadlineTimeout(std::chrono::milliseconds timeout)
{
    if (d_ptr->timeout == timeout)
        return *this;
    d_ptr.detach();
    Q_D(QGrpcChannelOptions);
    d->timeout = timeout;
    return *this;
}

/*!
    \fn QGrpcChannelOptions &QGrpcChannelOptions::setMetadata(const QHash<QByteArray, QByteArray> &metadata)
    \fn QGrpcChannelOptions &QGrpcChannelOptions::setMetadata(QHash<QByteArray, QByteArray> &&metadata)

    Sets the client \a metadata for the channel and returns a reference to the
    updated object.

    \include qgrpccalloptions.cpp set-metadata-desc

    \note The metadata set via the channel options applies to all RPCs that
    operate on the channel, except those overridden by
    QGrpcCallOptions::setMetadata().
*/
QGrpcChannelOptions &QGrpcChannelOptions::setMetadata(const QHash<QByteArray, QByteArray> &metadata)
{
    if (d_ptr->metadata == metadata)
        return *this;
    d_ptr.detach();
    Q_D(QGrpcChannelOptions);
    d->metadata = metadata;
    return *this;
}

QGrpcChannelOptions &QGrpcChannelOptions::setMetadata(QHash<QByteArray, QByteArray> &&metadata)
{
    if (d_ptr->metadata == metadata)
        return *this;
    d_ptr.detach();
    Q_D(QGrpcChannelOptions);
    d->metadata = std::move(metadata);
    return *this;
}

/*!
    \since 6.8

    Sets the serialization \a format for the channel and returns a reference to
    the updated object.
*/
QGrpcChannelOptions &
QGrpcChannelOptions::setSerializationFormat(const QGrpcSerializationFormat &format)
{
    if (d_ptr->serializationFormat == format)
        return *this;
    d_ptr.detach();
    Q_D(QGrpcChannelOptions);
    d->serializationFormat = format;
    return *this;
}

/*!
    Returns the timeout duration that is used to calculate the deadline for the
    channel.

    If this field is unset, returns an empty \c {std::optional}.
*/
std::optional<std::chrono::milliseconds> QGrpcChannelOptions::deadlineTimeout() const noexcept
{
    Q_D(const QGrpcChannelOptions);
    return d->timeout;
}

/*!
    \fn const QHash<QByteArray, QByteArray> &QGrpcChannelOptions::metadata() const &
    \fn QHash<QByteArray, QByteArray> QGrpcChannelOptions::metadata() &&

    Returns the client metadata for the channel.

    If this field is unset, returns empty metadata.
*/
const QHash<QByteArray, QByteArray> &QGrpcChannelOptions::metadata() const & noexcept
{
    Q_D(const QGrpcChannelOptions);
    return d->metadata;
}

QHash<QByteArray, QByteArray> QGrpcChannelOptions::metadata() &&
{
    Q_D(QGrpcChannelOptions);
    if (d->ref.loadRelaxed() != 1) // return copy if shared
        return { d->metadata };
    return std::move(d->metadata);
}

/*!
    \since 6.8

    Returns the serialization format used by the channel.

    If this field is unset, returns a \l {QtGrpc::SerializationFormat::}
    {Default} constructed serialization format.
 */
QGrpcSerializationFormat QGrpcChannelOptions::serializationFormat() const
{
    Q_D(const QGrpcChannelOptions);
    return d->serializationFormat;
}

#if QT_CONFIG(ssl)
/*!
    Sets the \a sslConfiguration for the channel and returns a reference to the
    updated object.
*/
QGrpcChannelOptions &
QGrpcChannelOptions::setSslConfiguration(const QSslConfiguration &sslConfiguration)
{
    if (d_ptr->sslConfiguration == sslConfiguration)
        return *this;
    d_ptr.detach();
    Q_D(QGrpcChannelOptions);
    d->sslConfiguration = sslConfiguration;
    return *this;
}

/*!
    Returns the SSL configuration for the channel.

    If this field is unset, returns an empty \c {std::optional}.
*/
std::optional<QSslConfiguration> QGrpcChannelOptions::sslConfiguration() const
{
    Q_D(const QGrpcChannelOptions);
    return d->sslConfiguration;
}
#endif

#ifndef QT_NO_DEBUG_STREAM
/*!
    \since 6.8
    \fn QDebug QGrpcChannelOptions::operator<<(QDebug debug, const QGrpcChannelOptions &chOpts)

    Writes \a chOpts to the specified stream \a debug.
*/
QDebug operator<<(QDebug debug, const QGrpcChannelOptions &chOpts)
{
    const QDebugStateSaver save(debug);
    debug.nospace().noquote();
    debug << "QGrpcChannelOptions(deadline: " << chOpts.deadlineTimeout()
          << ", metadata: " << chOpts.metadata()
          << ", serializationFormat: " << chOpts.serializationFormat().suffix()
          << ", sslConfiguration: ";
#  if QT_CONFIG(ssl)
    if (chOpts.sslConfiguration())
        debug << "available";
    else
        debug << std::nullopt;
#  else
    debug << "unsupported";
#  endif
    debug << ')';
    return debug;
}
#endif

QT_END_NAMESPACE
