// Copyright (C) 2018 The Qt Company Ltd.
// Copyright (C) 2016 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qbluetoothsocket.h"
#if QT_CONFIG(bluez)
#include "qbluetoothsocket_bluez_p.h"
#include "qbluetoothsocket_bluezdbus_p.h"
#include "bluez/bluez5_helper_p.h"
#elif defined(QT_ANDROID_BLUETOOTH)
#include "qbluetoothsocket_android_p.h"
#elif defined(QT_WINRT_BLUETOOTH)
#include "qbluetoothsocket_winrt_p.h"
#elif defined(QT_OSX_BLUETOOTH)
#include "qbluetoothsocket_macos_p.h"
#else
#include "qbluetoothsocket_dummy_p.h"
#endif

#include "qbluetoothservicediscoveryagent.h"

#include <QtCore/QLoggingCategory>
#include <QSocketNotifier>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(QT_BT)

/*!
    \class QBluetoothSocket
    \inmodule QtBluetooth
    \brief The QBluetoothSocket class enables connection to a Bluetooth device
    running a bluetooth server.

    \since 5.2

    QBluetoothSocket supports two socket types, \l {QBluetoothServiceInfo::L2capProtocol}{L2CAP} and
    \l {QBluetoothServiceInfo::RfcommProtocol}{RFCOMM}.

    \l {QBluetoothServiceInfo::L2capProtocol}{L2CAP} is a low level datagram-oriented Bluetooth socket.
    Android does not support \l {QBluetoothServiceInfo::L2capProtocol}{L2CAP} for socket
    connections.

    \l {QBluetoothServiceInfo::RfcommProtocol}{RFCOMM} is a reliable, stream-oriented socket. RFCOMM
    sockets emulate an RS-232 serial port.

    To create a connection to a Bluetooth service, create a socket of the appropriate type and call
    connectToService() passing the Bluetooth address and port number. QBluetoothSocket will emit
    the connected() signal when the connection is established.

    If the \l {QBluetoothServiceInfo::Protocol}{Protocol} is not supported on a platform, calling
    \l connectToService() will emit a \l {QBluetoothSocket::SocketError::UnsupportedProtocolError}{UnsupportedProtocolError} error.

    \note QBluetoothSocket does not support synchronous read and write operations. Functions such
    as \l waitForReadyRead() and \l waitForBytesWritten() are not implemented. I/O operations should be
    performed using \l readyRead(), \l read() and \l write().

    On iOS, this class cannot be used because the platform does not expose
    an API which may permit access to QBluetoothSocket related features.

    \note On macOS Monterey (12) the socket data flow is paused when a
    modal dialogue is executing, or an event tracking mode is entered (for
    example by long-pressing a Window close button). This may change in the
    future releases of macOS.
*/

/*!
    \enum QBluetoothSocket::SocketState

    This enum describes the state of the Bluetooth socket.

    \value UnconnectedState     Socket is not connected.
    \value ServiceLookupState   Socket is querying connection parameters.
    \value ConnectingState      Socket is attempting to connect to a device.
    \value ConnectedState       Socket is connected to a device.
    \value BoundState           Socket is bound to a local address and port.
    \value ClosingState         Socket is connected and will be closed once all pending data is
    written to the socket.
    \value ListeningState       Socket is listening for incoming connections.
*/

/*!
    \enum QBluetoothSocket::SocketError

    This enum describes Bluetooth socket error types.

    \value UnknownSocketError       An unknown error has occurred.
    \value NoSocketError            No error. Used for testing.
    \value HostNotFoundError        Could not find the remote host.
    \value ServiceNotFoundError     Could not find the service UUID on remote host.
    \value NetworkError             Attempt to read or write from socket returned an error
    \value UnsupportedProtocolError The \l {QBluetoothServiceInfo::Protocol}{Protocol} is not
                                    supported on this platform.
    \value OperationError           An operation was attempted while the socket was in a state
                                    that did not permit it.
    \value [since 5.10] RemoteHostClosedError   The remote host closed the connection.
    \value [since 6.4] MissingPermissionsError  The operating system requests
                                                permissions which were not
                                                granted by the user.
*/

/*!
    \fn void QBluetoothSocket::connected()

    This signal is emitted when a connection is established.

    \sa QBluetoothSocket::SocketState::ConnectedState, stateChanged()
*/

/*!
    \fn void QBluetoothSocket::disconnected()

    This signal is emitted when the socket is disconnected.

    \sa QBluetoothSocket::SocketState::UnconnectedState, stateChanged()
*/

/*!
    \fn void QBluetoothSocket::errorOccurred(QBluetoothSocket::SocketError error)

    This signal is emitted when an \a error occurs.

    \sa error()
    \since 6.2
*/

/*!
    \fn QBluetoothSocket::stateChanged(QBluetoothSocket::SocketState state)

    This signal is emitted when the socket state changes to \a state.

    \sa connected(), disconnected(), state(), QBluetoothSocket::SocketState
*/

/*!
    \fn void QBluetoothSocket::abort()

    Aborts the current connection and resets the socket. Unlike disconnectFromService(), this
    function immediately closes the socket, discarding any pending data in the write buffer.

    \note On Android, aborting the socket requires asynchronous interaction with Android threads.
    Therefore the associated \l disconnected() and \l stateChanged() signals are delayed
    until the threads have finished the closure.

    \sa disconnectFromService(), close()
*/

/*!
    \fn void QBluetoothSocket::close()

    Disconnects the socket's connection with the device.

    \note On Android, closing the socket requires asynchronous interaction with Android threads.
    Therefore the associated \l disconnected() and \l stateChanged() signals are delayed
    until the threads have finished the closure.

*/

/*!
    \fn void QBluetoothSocket::disconnectFromService()

    Attempts to close the socket. If there is pending data waiting to be written QBluetoothSocket
    will enter ClosingState and wait until all data has been written. Eventually, it will enter
    UnconnectedState and emit the disconnected() signal.

    \sa connectToService()
*/

/*!
    \fn QString QBluetoothSocket::localName() const

    Returns the name of the local device.

    Although some platforms may differ the socket must generally be connected to guarantee
    the return of a valid name. In particular, this is true when dealing with platforms
    that support multiple local Bluetooth adapters.
*/

/*!
    \fn QBluetoothAddress QBluetoothSocket::localAddress() const

    Returns the address of the local device.

    Although some platforms may differ the socket must generally be connected to guarantee
    the return of a valid address. In particular, this is true when dealing with platforms
    that support multiple local Bluetooth adapters.
*/

/*!
    \fn quint16 QBluetoothSocket::localPort() const

    Returns the port number of the local socket if available, otherwise returns 0.
    Although some platforms may differ the socket must generally be connected to guarantee
    the return of a valid port number.

    On Android and \macos, this feature is not supported and returns 0.
*/

/*!
    \fn QString QBluetoothSocket::peerName() const

    Returns the name of the peer device.
*/

/*!
    \fn QBluetoothAddress QBluetoothSocket::peerAddress() const

    Returns the address of the peer device.
*/

/*!
    \fn quint16 QBluetoothSocket::peerPort() const

    Return the port number of the peer socket if available, otherwise returns 0.
    On Android, this feature is not supported.
*/

/*!
    \fn qint64 QBluetoothSocket::readData(char *data, qint64 maxSize)

    \reimp
*/

/*!
    \fn qint64 QBluetoothSocket::writeData(const char *data, qint64 maxSize)

    \reimp
*/

static QBluetoothSocketBasePrivate *createSocketPrivate()
{
#if QT_CONFIG(bluez)
    if (bluetoothdVersion() >= QVersionNumber(5, 46)) {
        qCDebug(QT_BT) << "Using Bluetooth dbus socket implementation";
        return new QBluetoothSocketPrivateBluezDBus();
    } else {
        qCDebug(QT_BT) << "Using Bluetooth raw socket implementation";
        return new QBluetoothSocketPrivateBluez();
    }
#elif defined(QT_ANDROID_BLUETOOTH)
    return new QBluetoothSocketPrivateAndroid();
#elif defined(QT_WINRT_BLUETOOTH)
    return new QBluetoothSocketPrivateWinRT();
#elif defined(QT_OSX_BLUETOOTH)
    return new QBluetoothSocketPrivateDarwin();
#else
    return new QBluetoothSocketPrivateDummy();
#endif
}

/*!
    Constructs a Bluetooth socket of \a socketType type, with \a parent.
*/
QBluetoothSocket::QBluetoothSocket(QBluetoothServiceInfo::Protocol socketType, QObject *parent)
: QIODevice(parent)
{
    d_ptr = createSocketPrivate();
    d_ptr->q_ptr = this;

    Q_D(QBluetoothSocketBase);
    d->ensureNativeSocket(socketType);

    setOpenMode(QIODevice::NotOpen);
}

/*!
    Constructs a Bluetooth socket with \a parent.
*/
QBluetoothSocket::QBluetoothSocket(QObject *parent)
  : QIODevice(parent)
{
    d_ptr = createSocketPrivate();
    d_ptr->q_ptr = this;
    setOpenMode(QIODevice::NotOpen);
}

#if QT_CONFIG(bluez)

/*!
  \internal
*/
QBluetoothSocket::QBluetoothSocket(QBluetoothSocketBasePrivate *dPrivate,
                          QBluetoothServiceInfo::Protocol socketType,
                          QObject *parent)
    : QIODevice(parent)
{
    d_ptr = dPrivate;
    d_ptr->q_ptr = this;

    Q_D(QBluetoothSocketBase);
    d->ensureNativeSocket(socketType);

    setOpenMode(QIODevice::NotOpen);
}

#endif

/*!
    Destroys the Bluetooth socket.
*/
QBluetoothSocket::~QBluetoothSocket()
{
    delete d_ptr;
    d_ptr = nullptr;
}

/*!
    \reimp
*/
bool QBluetoothSocket::isSequential() const
{
    return true;
}

/*!
    Returns the number of incoming bytes that are waiting to be read.

    \sa bytesToWrite(), read()
*/
qint64 QBluetoothSocket::bytesAvailable() const
{
    Q_D(const QBluetoothSocketBase);
    return QIODevice::bytesAvailable() + d->bytesAvailable();
}

/*!
    Returns the number of bytes that are waiting to be written. The bytes are written when control
    goes back to the event loop.
*/
qint64 QBluetoothSocket::bytesToWrite() const
{
    Q_D(const QBluetoothSocketBase);
    return d->bytesToWrite();
}

/*!
    Attempts to connect to the service described by \a service.

    The socket is opened in the given \a openMode. The \l socketType() is ignored
    if \a service specifies a differing \l QBluetoothServiceInfo::socketProtocol().

    The socket first enters ConnectingState and attempts to connect to the device providing
    \a service. If a connection is established, QBluetoothSocket enters ConnectedState and
    emits connected().

    At any point, the socket can emit errorOccurred() to signal that an error occurred.

    Note that most platforms require a pairing prior to connecting to the remote device. Otherwise
    the connection process may fail.

    On Android, only RFCOMM connections are possible. This function ignores any socket protocol indicator
    and assumes RFCOMM.

    \sa state(), disconnectFromService()
*/
void QBluetoothSocket::connectToService(const QBluetoothServiceInfo &service, OpenMode openMode)
{
    Q_D(QBluetoothSocketBase);
    d->connectToService(service, openMode);
}

/*!
  \fn void QBluetoothSocket::connectToService(const QBluetoothAddress &address, QBluetoothUuid::ServiceClassUuid uuid, OpenMode mode = ReadWrite)

  \internal

  Exists to avoid QTBUG-65831.
*/

/*!
    Attempts to make a connection to the service identified by \a uuid on the device with address
    \a address.

    The socket is opened in the given \a openMode.

    For BlueZ, the socket first enters the \l ServiceLookupState and queries the connection parameters for
    \a uuid. If the service parameters are successfully retrieved the socket enters
    ConnectingState, and attempts to connect to \a address. If a connection is established,
    QBluetoothSocket enters \l ConnectedState and emits connected().

    On Android, the service connection can directly be established
    using the UUID of the remote service. Therefore the platform does not require
    the \l ServiceLookupState and \l socketType() is always set to
    \l QBluetoothServiceInfo::RfcommProtocol.

    At any point, the socket can emit errorOccurred() to signal that an error occurred.

    Note that most platforms require a pairing prior to connecting to the remote device. Otherwise
    the connection process may fail.

    \sa state(), disconnectFromService()
*/
void QBluetoothSocket::connectToService(const QBluetoothAddress &address, const QBluetoothUuid &uuid, OpenMode openMode)
{
    Q_D(QBluetoothSocketBase);
    d->connectToService(address, uuid, openMode);
}

/*!
    Attempts to make a connection with \a address on the given \a port.

    The socket is opened in the given \a openMode.

    The socket first enters ConnectingState, and attempts to connect to \a address. If a
    connection is established, QBluetoothSocket enters ConnectedState and emits connected().

    At any point, the socket can emit errorOccurred() to signal that an error occurred.

    On Android and BlueZ (version 5.46 or above), a connection to a service can not be established using a port.
    Calling this function will emit a \l {QBluetoothSocket::SocketError::ServiceNotFoundError}{ServiceNotFoundError}.

    Note that most platforms require a pairing prior to connecting to the remote device. Otherwise
    the connection process may fail.

    \sa state(), disconnectFromService()
*/
void QBluetoothSocket::connectToService(const QBluetoothAddress &address, quint16 port, OpenMode openMode)
{
    Q_D(QBluetoothSocketBase);
    d->connectToService(address, port, openMode);
}

/*!
    Returns the socket type. The socket automatically adjusts to the protocol
    offered by the remote service.

    Android only support \l{QBluetoothServiceInfo::RfcommProtocol}{RFCOMM}
    based sockets.
*/
QBluetoothServiceInfo::Protocol QBluetoothSocket::socketType() const
{
    Q_D(const QBluetoothSocketBase);
    return d->socketType;
}

/*!
    Returns the current state of the socket.
*/
QBluetoothSocket::SocketState QBluetoothSocket::state() const
{
    Q_D(const QBluetoothSocketBase);
    return d->state;
}

/*!
    Returns the last error.
*/
QBluetoothSocket::SocketError QBluetoothSocket::error() const
{
    Q_D(const QBluetoothSocketBase);
    return d->socketError;
}

/*!
    Returns a user displayable text string for the error.
 */
QString QBluetoothSocket::errorString() const
{
    Q_D(const QBluetoothSocketBase);
    return d->errorString;
}

/*!
    Sets the preferred security parameter for the connection attempt to
    \a flags. This value is incorporated when calling \l connectToService().
    Therefore it is required to reconnect to change this parameter for an
    existing connection.

    On Bluez this property is set to QBluetooth::Security::Authorization by default.

    On \macos, this value is ignored as the platform does not permit access
    to the security parameter of the socket. By default the platform prefers
    secure/encrypted connections though and therefore this function always
    returns \l QBluetooth::Security::Secure.

    Android only supports two levels of security (secure and non-secure).
    If this flag is set to \l QBluetooth::Security::NoSecurity the socket
    object will not employ any authentication or encryption. Any other
    security flag combination will trigger a secure Bluetooth connection.
    This flag is set to \l QBluetooth::Security::Secure by default.

    \note A secure connection requires a pairing between the two devices. On
    some platforms, the pairing is automatically initiated during the establishment
    of the connection. Other platforms require the application to manually trigger
    the pairing before attempting to connect.

    \sa preferredSecurityFlags()

    \since 5.6
*/
void QBluetoothSocket::setPreferredSecurityFlags(QBluetooth::SecurityFlags flags)
{
#ifdef QT_OSX_BLUETOOTH
    return; // not supported on macOS.
#endif
    Q_D(QBluetoothSocketBase);
    if (d->secFlags != flags)
        d->secFlags = flags;
}

/*!
    Returns the security parameters used for the initial connection
    attempt.

    The security parameters may be renegotiated between the two parties
    during or after the connection has been established. If such a change happens
    it is not reflected in the value of this flag.

    On \macos, this flag is always set to \l QBluetooth::Security::Secure.

    \sa setPreferredSecurityFlags()

    \since 5.6
*/
QBluetooth::SecurityFlags QBluetoothSocket::preferredSecurityFlags() const
{
#if QT_OSX_BLUETOOTH
    // not supported on macOS - platform always uses encryption
    return QBluetooth::Security::Secure;
#else
    Q_D(const QBluetoothSocketBase);
    return d->secFlags;
#endif // QT_OSX_BLUETOOTH
}

/*!
    Sets the socket state to \a state.
*/
void QBluetoothSocket::setSocketState(QBluetoothSocket::SocketState state)
{
    Q_D(QBluetoothSocketBase);
    const SocketState old = d->state;
    if (state == old)
        return;

    d->state = state;
    if(old != d->state)
        emit stateChanged(state);
    if (state == QBluetoothSocket::SocketState::ConnectedState) {
        emit connected();
    } else if ((old == QBluetoothSocket::SocketState::ConnectedState
                || old == QBluetoothSocket::SocketState::ClosingState)
               && state == QBluetoothSocket::SocketState::UnconnectedState) {
        emit disconnected();
    }
    if (state == SocketState::ListeningState){
#ifdef QT_OSX_BLUETOOTH
        qCWarning(QT_BT) << "listening socket is not supported by IOBluetooth";
#endif
        // TODO: look at this, is this really correct?
        // if we're a listening socket we can't handle connects?
        if (d->readNotifier) {
            d->readNotifier->setEnabled(false);
        }
    }
}

/*!
    Returns true if you can read at least one line from the device
 */

bool QBluetoothSocket::canReadLine() const
{
    Q_D(const QBluetoothSocketBase);
    return d->canReadLine() || QIODevice::canReadLine();
}

/*!
    Sets the type of error that last occurred to \a error_.
*/
void QBluetoothSocket::setSocketError(QBluetoothSocket::SocketError error_)
{
    Q_D(QBluetoothSocketBase);
    d->socketError = error_;
    emit errorOccurred(error_);
}

/*!
  Start device discovery for \a service and open the socket with \a openMode.  If the socket
  is created with a service uuid device address, use service discovery to find the
  port number to connect to.
*/

void QBluetoothSocket::doDeviceDiscovery(const QBluetoothServiceInfo &service, OpenMode openMode)
{
    Q_D(QBluetoothSocketBase);

    setSocketState(QBluetoothSocket::SocketState::ServiceLookupState);
    qCDebug(QT_BT) << "Starting Bluetooth service discovery";

    if(d->discoveryAgent) {
        d->discoveryAgent->stop();
        delete d->discoveryAgent;
    }

    d->discoveryAgent = new QBluetoothServiceDiscoveryAgent(this);
    d->discoveryAgent->setRemoteAddress(service.device().address());

    //qDebug() << "Got agent";

    connect(d->discoveryAgent, &QBluetoothServiceDiscoveryAgent::serviceDiscovered,
            this, &QBluetoothSocket::serviceDiscovered);
    connect(d->discoveryAgent, &QBluetoothServiceDiscoveryAgent::finished,
            this, &QBluetoothSocket::discoveryFinished);

    d->openMode = openMode;

    QList<QBluetoothUuid> filterUuids = service.serviceClassUuids();
    if(!service.serviceUuid().isNull())
        filterUuids.append(service.serviceUuid());

    if (!filterUuids.isEmpty())
        d->discoveryAgent->setUuidFilter(filterUuids);

    // we have to ID the service somehow
    Q_ASSERT(!d->discoveryAgent->uuidFilter().isEmpty());

    qCDebug(QT_BT) << "UUID filter" << d->discoveryAgent->uuidFilter();

    d->discoveryAgent->start(QBluetoothServiceDiscoveryAgent::FullDiscovery);
}

void QBluetoothSocket::serviceDiscovered(const QBluetoothServiceInfo &service)
{
    Q_D(QBluetoothSocketBase);
    qCDebug(QT_BT) << "FOUND SERVICE!" << service;
    if (service.protocolServiceMultiplexer() > 0 || service.serverChannel() > 0) {
        connectToService(service, d->openMode);
        d->discoveryAgent->deleteLater();
        d->discoveryAgent = nullptr;
#ifdef QT_WINRT_BLUETOOTH
    } else if (!service.attribute(0xBEEF).isNull()
               && !service.attribute(0xBEF0).isNull()) {
        connectToService(service, d->openMode);
        d->discoveryAgent->deleteLater();
        d->discoveryAgent = nullptr;
#endif
    } else {
        qCDebug(QT_BT) << "Could not find port/psm for potential remote service";
    }
}

void QBluetoothSocket::discoveryFinished()
{
    qCDebug(QT_BT) << "Socket discovery finished";
    Q_D(QBluetoothSocketBase);
    if (d->discoveryAgent){
        qCDebug(QT_BT) << "Didn't find any";
        d->errorString = tr("Service cannot be found");
        setSocketError(SocketError::ServiceNotFoundError);
        setSocketState(QBluetoothSocket::SocketState::UnconnectedState);
        d->discoveryAgent->deleteLater();
        d->discoveryAgent = nullptr;
    }
}

void QBluetoothSocket::abort()
{
    if (state() == SocketState::UnconnectedState)
        return;

    Q_D(QBluetoothSocketBase);
    setOpenMode(QIODevice::NotOpen);

    if (state() == SocketState::ServiceLookupState && d->discoveryAgent) {
        d->discoveryAgent->disconnect();
        d->discoveryAgent->stop();
        d->discoveryAgent = nullptr;
    }

    setSocketState(SocketState::ClosingState);
    d->abort();
}

void QBluetoothSocket::disconnectFromService()
{
    close();
}

QString QBluetoothSocket::localName() const
{
    Q_D(const QBluetoothSocketBase);
    return d->localName();
}

QBluetoothAddress QBluetoothSocket::localAddress() const
{
    Q_D(const QBluetoothSocketBase);
    return d->localAddress();
}

quint16 QBluetoothSocket::localPort() const
{
    Q_D(const QBluetoothSocketBase);
    return d->localPort();
}

QString QBluetoothSocket::peerName() const
{
    Q_D(const QBluetoothSocketBase);
    return d->peerName();
}

QBluetoothAddress QBluetoothSocket::peerAddress() const
{
    Q_D(const QBluetoothSocketBase);
    return d->peerAddress();
}

quint16 QBluetoothSocket::peerPort() const
{
    Q_D(const QBluetoothSocketBase);
    return d->peerPort();
}

qint64 QBluetoothSocket::writeData(const char *data, qint64 maxSize)
{
    Q_D(QBluetoothSocketBase);

    if (!data || maxSize <= 0) {
        d_ptr->errorString = tr("Invalid data/data size");
        setSocketError(QBluetoothSocket::SocketError::OperationError);
        return -1;
    }

    return d->writeData(data, maxSize);
}

qint64 QBluetoothSocket::readData(char *data, qint64 maxSize)
{
    Q_D(QBluetoothSocketBase);
    return d->readData(data, maxSize);
}

void QBluetoothSocket::close()
{
    if (state() == SocketState::UnconnectedState)
        return;

    Q_D(QBluetoothSocketBase);
    setOpenMode(QIODevice::NotOpen);

    if (state() == SocketState::ServiceLookupState && d->discoveryAgent) {
        d->discoveryAgent->disconnect();
        d->discoveryAgent->stop();
        d->discoveryAgent = nullptr;
    }

    setSocketState(SocketState::ClosingState);

    d->close();
}

/*!
  \fn bool QBluetoothSocket::setSocketDescriptor(int socketDescriptor, QBluetoothServiceInfo::Protocol socketType, SocketState socketState, OpenMode openMode)

  Sets the socket to use \a socketDescriptor with a type of \a socketType,
  which is in state \a socketState, and mode \a openMode.

  The socket descriptor is owned by the QBluetoothSocket instance and may
  be closed once finished.

  Returns \c true on success.
*/

// ### Qt 7 consider making this function private. The qbluetoothsocket_bluez backend is the
// the only backend providing publicly accessible support for this. Other backends implement
// similarly named, but private, overload
bool QBluetoothSocket::setSocketDescriptor(int socketDescriptor, QBluetoothServiceInfo::Protocol socketType,
                                           SocketState socketState, OpenMode openMode)
{
    Q_D(QBluetoothSocketBase);
    return d->setSocketDescriptor(socketDescriptor, socketType, socketState, openMode);
}

/*!
  Returns the platform-specific socket descriptor, if available.
  This function returns -1 if the descriptor is not available or an error has occurred.
*/

int QBluetoothSocket::socketDescriptor() const
{
    Q_D(const QBluetoothSocketBase);
    return d->socket;
}

QT_END_NAMESPACE

#include "moc_qbluetoothsocket.cpp"
