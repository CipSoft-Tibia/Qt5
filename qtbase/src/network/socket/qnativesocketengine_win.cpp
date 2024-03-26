// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <winsock2.h>
#include <ws2tcpip.h>

#include "qnativesocketengine_p_p.h"

#include <qabstracteventdispatcher.h>
#include <qsocketnotifier.h>
#include <qdebug.h>
#include <qdatetime.h>
#include <qnetworkinterface.h>
#include <qoperatingsystemversion.h>
#include <qvarlengtharray.h>

#include <algorithm>

//#define QNATIVESOCKETENGINE_DEBUG
#if defined(QNATIVESOCKETENGINE_DEBUG)
#include <private/qdebug_p.h>
#endif

QT_BEGIN_NAMESPACE

//Some distributions of mingw (including 4.7.2 from mingw.org) are missing this from headers.
//Also microsoft headers don't include it when building on XP and earlier.
#ifndef IPV6_V6ONLY
#define IPV6_V6ONLY 27
#endif
#ifndef IP_HOPLIMIT
#define IP_HOPLIMIT               21 // Receive packet hop limit.
#endif

#if defined(QNATIVESOCKETENGINE_DEBUG)

void verboseWSErrorDebug(int r)
{
    switch (r) {
        case WSANOTINITIALISED : qDebug("WSA error : WSANOTINITIALISED"); break;
        case WSAEINTR: qDebug("WSA error : WSAEINTR"); break;
        case WSAEBADF: qDebug("WSA error : WSAEBADF"); break;
        case WSAEACCES: qDebug("WSA error : WSAEACCES"); break;
        case WSAEFAULT: qDebug("WSA error : WSAEFAULT"); break;
        case WSAEINVAL: qDebug("WSA error : WSAEINVAL"); break;
        case WSAEMFILE: qDebug("WSA error : WSAEMFILE"); break;
        case WSAEWOULDBLOCK: qDebug("WSA error : WSAEWOULDBLOCK"); break;
        case WSAEINPROGRESS: qDebug("WSA error : WSAEINPROGRESS"); break;
        case WSAEALREADY: qDebug("WSA error : WSAEALREADY"); break;
        case WSAENOTSOCK: qDebug("WSA error : WSAENOTSOCK"); break;
        case WSAEDESTADDRREQ: qDebug("WSA error : WSAEDESTADDRREQ"); break;
        case WSAEMSGSIZE: qDebug("WSA error : WSAEMSGSIZE"); break;
        case WSAEPROTOTYPE: qDebug("WSA error : WSAEPROTOTYPE"); break;
        case WSAENOPROTOOPT: qDebug("WSA error : WSAENOPROTOOPT"); break;
        case WSAEPROTONOSUPPORT: qDebug("WSA error : WSAEPROTONOSUPPORT"); break;
        case WSAESOCKTNOSUPPORT: qDebug("WSA error : WSAESOCKTNOSUPPORT"); break;
        case WSAEOPNOTSUPP: qDebug("WSA error : WSAEOPNOTSUPP"); break;
        case WSAEPFNOSUPPORT: qDebug("WSA error : WSAEPFNOSUPPORT"); break;
        case WSAEAFNOSUPPORT: qDebug("WSA error : WSAEAFNOSUPPORT"); break;
        case WSAEADDRINUSE: qDebug("WSA error : WSAEADDRINUSE"); break;
        case WSAEADDRNOTAVAIL: qDebug("WSA error : WSAEADDRNOTAVAIL"); break;
        case WSAENETDOWN: qDebug("WSA error : WSAENETDOWN"); break;
        case WSAENETUNREACH: qDebug("WSA error : WSAENETUNREACH"); break;
        case WSAENETRESET: qDebug("WSA error : WSAENETRESET"); break;
        case WSAECONNABORTED: qDebug("WSA error : WSAECONNABORTED"); break;
        case WSAECONNRESET: qDebug("WSA error : WSAECONNRESET"); break;
        case WSAENOBUFS: qDebug("WSA error : WSAENOBUFS"); break;
        case WSAEISCONN: qDebug("WSA error : WSAEISCONN"); break;
        case WSAENOTCONN: qDebug("WSA error : WSAENOTCONN"); break;
        case WSAESHUTDOWN: qDebug("WSA error : WSAESHUTDOWN"); break;
        case WSAETOOMANYREFS: qDebug("WSA error : WSAETOOMANYREFS"); break;
        case WSAETIMEDOUT: qDebug("WSA error : WSAETIMEDOUT"); break;
        case WSAECONNREFUSED: qDebug("WSA error : WSAECONNREFUSED"); break;
        case WSAELOOP: qDebug("WSA error : WSAELOOP"); break;
        case WSAENAMETOOLONG: qDebug("WSA error : WSAENAMETOOLONG"); break;
        case WSAEHOSTDOWN: qDebug("WSA error : WSAEHOSTDOWN"); break;
        case WSAEHOSTUNREACH: qDebug("WSA error : WSAEHOSTUNREACH"); break;
        case WSAENOTEMPTY: qDebug("WSA error : WSAENOTEMPTY"); break;
        case WSAEPROCLIM: qDebug("WSA error : WSAEPROCLIM"); break;
        case WSAEUSERS: qDebug("WSA error : WSAEUSERS"); break;
        case WSAEDQUOT: qDebug("WSA error : WSAEDQUOT"); break;
        case WSAESTALE: qDebug("WSA error : WSAESTALE"); break;
        case WSAEREMOTE: qDebug("WSA error : WSAEREMOTE"); break;
        case WSAEDISCON: qDebug("WSA error : WSAEDISCON"); break;
        default: qDebug("WSA error : Unknown"); break;
    }
    qErrnoWarning(r, "more details");
}

#define WS_ERROR_DEBUG(x) verboseWSErrorDebug(x)

#else

#define WS_ERROR_DEBUG(x) Q_UNUSED(x)

#endif

#ifndef AF_INET6
#define AF_INET6        23              /* Internetwork Version 6 */
#endif

#ifndef SO_EXCLUSIVEADDRUSE
#define SO_EXCLUSIVEADDRUSE ((int)(~SO_REUSEADDR)) /* disallow local address reuse */
#endif

/*
    Extracts the port and address from a sockaddr, and stores them in
    \a port and \a addr if they are non-null.
*/
static inline void qt_socket_getPortAndAddress(SOCKET socketDescriptor, const qt_sockaddr *sa, quint16 *port, QHostAddress *address)
{
    if (sa->a.sa_family == AF_INET6) {
        const sockaddr_in6 *sa6 = &sa->a6;
        Q_IPV6ADDR tmp;
        for (int i = 0; i < 16; ++i)
            tmp.c[i] = sa6->sin6_addr.s6_addr[i];
        if (address) {
            QHostAddress a;
            a.setAddress(tmp);
            if (sa6->sin6_scope_id)
                a.setScopeId(QString::number(sa6->sin6_scope_id));
            *address = a;
        }
        if (port)
            WSANtohs(socketDescriptor, sa6->sin6_port, port);
    } else

    if (sa->a.sa_family == AF_INET) {
        const sockaddr_in *sa4 = &sa->a4;
        unsigned long addr;
        WSANtohl(socketDescriptor, sa4->sin_addr.s_addr, &addr);
        QHostAddress a;
        a.setAddress(addr);
        if (address)
            *address = a;
        if (port)
            WSANtohs(socketDescriptor, sa4->sin_port, port);
    }
}

static void convertToLevelAndOption(QNativeSocketEngine::SocketOption opt,
                                    QAbstractSocket::NetworkLayerProtocol socketProtocol, int &level, int &n)
{
    n = -1;
    level = SOL_SOCKET; // default

    switch (opt) {
    case QNativeSocketEngine::NonBlockingSocketOption:      // WSAIoctl
    case QNativeSocketEngine::TypeOfServiceOption:          // not supported
    case QNativeSocketEngine::MaxStreamsSocketOption:
        Q_UNREACHABLE();

    case QNativeSocketEngine::ReceiveBufferSocketOption:
        n = SO_RCVBUF;
        break;
    case QNativeSocketEngine::SendBufferSocketOption:
        n = SO_SNDBUF;
        break;
    case QNativeSocketEngine::BroadcastSocketOption:
        n = SO_BROADCAST;
        break;
    case QNativeSocketEngine::AddressReusable:
        n = SO_REUSEADDR;
        break;
    case QNativeSocketEngine::BindExclusively:
        n = SO_EXCLUSIVEADDRUSE;
        break;
    case QNativeSocketEngine::ReceiveOutOfBandData:
        n = SO_OOBINLINE;
        break;
    case QNativeSocketEngine::LowDelayOption:
        level = IPPROTO_TCP;
        n = TCP_NODELAY;
        break;
    case QNativeSocketEngine::KeepAliveOption:
        n = SO_KEEPALIVE;
        break;
    case QNativeSocketEngine::MulticastTtlOption:
        if (socketProtocol == QAbstractSocket::IPv6Protocol || socketProtocol == QAbstractSocket::AnyIPProtocol) {
            level = IPPROTO_IPV6;
            n = IPV6_MULTICAST_HOPS;
        } else
        {
            level = IPPROTO_IP;
            n = IP_MULTICAST_TTL;
        }
        break;
    case QNativeSocketEngine::MulticastLoopbackOption:
        if (socketProtocol == QAbstractSocket::IPv6Protocol || socketProtocol == QAbstractSocket::AnyIPProtocol) {
            level = IPPROTO_IPV6;
            n = IPV6_MULTICAST_LOOP;
        } else
        {
            level = IPPROTO_IP;
            n = IP_MULTICAST_LOOP;
        }
        break;
    case QNativeSocketEngine::ReceivePacketInformation:
        if (socketProtocol == QAbstractSocket::IPv6Protocol || socketProtocol == QAbstractSocket::AnyIPProtocol) {
            level = IPPROTO_IPV6;
            n = IPV6_PKTINFO;
        } else if (socketProtocol == QAbstractSocket::IPv4Protocol) {
            level = IPPROTO_IP;
            n = IP_PKTINFO;
        }
        break;
    case QNativeSocketEngine::ReceiveHopLimit:
        if (socketProtocol == QAbstractSocket::IPv6Protocol || socketProtocol == QAbstractSocket::AnyIPProtocol) {
            level = IPPROTO_IPV6;
            n = IPV6_HOPLIMIT;
        } else if (socketProtocol == QAbstractSocket::IPv4Protocol) {
            level = IPPROTO_IP;
            n = IP_HOPLIMIT;
        }
        break;

    case QAbstractSocketEngine::PathMtuInformation:
        break;          // not supported on Windows
    }
}

/*! \internal

*/
static inline QAbstractSocket::SocketType qt_socket_getType(qintptr socketDescriptor)
{
    int value = 0;
    QT_SOCKLEN_T valueSize = sizeof(value);
    if (::getsockopt(socketDescriptor, SOL_SOCKET, SO_TYPE,
                     reinterpret_cast<char *>(&value), &valueSize) != 0) {
        WS_ERROR_DEBUG(WSAGetLastError());
    } else {
        if (value == SOCK_STREAM)
            return QAbstractSocket::TcpSocket;
        else if (value == SOCK_DGRAM)
            return QAbstractSocket::UdpSocket;
    }
    return QAbstractSocket::UnknownSocketType;
}

// MS Transport Provider IOCTL to control
// reporting PORT_UNREACHABLE messages
// on UDP sockets via recv/WSARecv/etc.
// Path TRUE in input buffer to enable (default if supported),
// FALSE to disable.
#ifndef SIO_UDP_CONNRESET
#  ifndef IOC_VENDOR
#    define IOC_VENDOR 0x18000000
#  endif
#  ifndef _WSAIOW
#    define _WSAIOW(x,y) (IOC_IN|(x)|(y))
#  endif
#  define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR,12)
#endif

bool QNativeSocketEnginePrivate::createNewSocket(QAbstractSocket::SocketType socketType, QAbstractSocket::NetworkLayerProtocol &socketProtocol)
{
    //### SCTP not implemented
    if (socketType == QAbstractSocket::SctpSocket) {
        setError(QAbstractSocket::UnsupportedSocketOperationError,
                 ProtocolUnsupportedErrorString);
        return false;
    }

    //Windows XP and 2003 support IPv6 but not dual stack sockets
    int protocol = (socketProtocol == QAbstractSocket::IPv6Protocol
        || (socketProtocol == QAbstractSocket::AnyIPProtocol)) ? AF_INET6 : AF_INET;
    int type = (socketType == QAbstractSocket::UdpSocket) ? SOCK_DGRAM : SOCK_STREAM;

    // MSDN KB179942 states that on winnt 4 WSA_FLAG_OVERLAPPED is needed if socket is to be non
    // blocking and recommends always doing it for cross-windows-version compatibility.

    // WSA_FLAG_NO_HANDLE_INHERIT is atomic (like linux O_CLOEXEC)
#ifndef WSA_FLAG_NO_HANDLE_INHERIT
#define WSA_FLAG_NO_HANDLE_INHERIT 0x80
#endif

    SOCKET socket = ::WSASocket(protocol, type, 0, NULL, 0, WSA_FLAG_NO_HANDLE_INHERIT | WSA_FLAG_OVERLAPPED);
    if (socket == INVALID_SOCKET) {
        int err = WSAGetLastError();
        WS_ERROR_DEBUG(err);
        switch (err) {
        case WSANOTINITIALISED:
            //###
            break;
        case WSAEAFNOSUPPORT:
        case WSAESOCKTNOSUPPORT:
        case WSAEPROTOTYPE:
        case WSAEINVAL:
            setError(QAbstractSocket::UnsupportedSocketOperationError, ProtocolUnsupportedErrorString);
            break;
        case WSAEMFILE:
        case WSAENOBUFS:
            setError(QAbstractSocket::SocketResourceError, ResourceErrorString);
            break;
        default:
            break;
        }

        return false;
    }

    if (socketType == QAbstractSocket::UdpSocket) {
        // enable new behavior using
        // SIO_UDP_CONNRESET
        DWORD dwBytesReturned = 0;
        int bNewBehavior = 1;
        if (::WSAIoctl(socket, SIO_UDP_CONNRESET, &bNewBehavior, sizeof(bNewBehavior),
                       NULL, 0, &dwBytesReturned, NULL, NULL) == SOCKET_ERROR) {
            // not to worry isBogusUdpReadNotification() should handle this otherwise
            int err = WSAGetLastError();
            WS_ERROR_DEBUG(err);
        }
    }

    // get the pointer to sendmsg and recvmsg
    DWORD bytesReturned;
    GUID recvmsgguid = WSAID_WSARECVMSG;
    if (WSAIoctl(socket, SIO_GET_EXTENSION_FUNCTION_POINTER,
                 &recvmsgguid, sizeof(recvmsgguid),
                 &recvmsg, sizeof(recvmsg), &bytesReturned, NULL, NULL) == SOCKET_ERROR)
        recvmsg = 0;

    GUID sendmsgguid = WSAID_WSASENDMSG;
    if (WSAIoctl(socket, SIO_GET_EXTENSION_FUNCTION_POINTER,
                 &sendmsgguid, sizeof(sendmsgguid),
                 &sendmsg, sizeof(sendmsg), &bytesReturned, NULL, NULL) == SOCKET_ERROR)
        sendmsg = 0;

    socketDescriptor = socket;
    this->socketProtocol = socketProtocol;
    this->socketType = socketType;

    // Make the socket nonblocking.
    if (!setOption(QAbstractSocketEngine::NonBlockingSocketOption, 1)) {
        setError(QAbstractSocket::UnsupportedSocketOperationError, NonBlockingInitFailedErrorString);
        q_func()->close();
        return false;
    }

    return true;
}

/*! \internal

    Returns the value of the socket option \a opt.
*/
int QNativeSocketEnginePrivate::option(QNativeSocketEngine::SocketOption opt) const
{
    Q_Q(const QNativeSocketEngine);
    if (!q->isValid())
        return -1;

    // handle non-getsockopt
    switch (opt) {
    case QNativeSocketEngine::NonBlockingSocketOption: {
        unsigned long buf = 0;
        if (WSAIoctl(socketDescriptor, FIONBIO, 0,0, &buf, sizeof(buf), 0,0,0) == 0)
            return buf;
        else
            return -1;
        break;
    }
    case QNativeSocketEngine::TypeOfServiceOption:
    case QNativeSocketEngine::MaxStreamsSocketOption:
        return -1;

    default:
        break;
    }

#if Q_BYTE_ORDER != Q_LITTLE_ENDIAN
#error code assumes windows is little endian
#endif
    int n, level;
    int v = 0; //note: windows doesn't write to all bytes if the option type is smaller than int
    QT_SOCKOPTLEN_T len = sizeof(v);

    convertToLevelAndOption(opt, socketProtocol, level, n);
    if (n != -1) {
        if (getsockopt(socketDescriptor, level, n, (char *) &v, &len) == 0)
            return v;
        WS_ERROR_DEBUG(WSAGetLastError());
    }
    return -1;
}


/*! \internal
    Sets the socket option \a opt to \a v.
*/
bool QNativeSocketEnginePrivate::setOption(QNativeSocketEngine::SocketOption opt, int v)
{
    Q_Q(const QNativeSocketEngine);
    if (!q->isValid())
        return false;

    // handle non-setsockopt options
    switch (opt) {
    case QNativeSocketEngine::SendBufferSocketOption:
        // see QTBUG-30478 SO_SNDBUF should not be used on Vista or later
        return false;
    case QNativeSocketEngine::NonBlockingSocketOption:
        {
        unsigned long buf = v;
        unsigned long outBuf;
        DWORD sizeWritten = 0;
        if (::WSAIoctl(socketDescriptor, FIONBIO, &buf, sizeof(unsigned long), &outBuf, sizeof(unsigned long), &sizeWritten, 0,0) == SOCKET_ERROR) {
            WS_ERROR_DEBUG(WSAGetLastError());
            return false;
        }
        return true;
        }
    case QNativeSocketEngine::TypeOfServiceOption:
    case QNativeSocketEngine::MaxStreamsSocketOption:
        return false;

    default:
        break;
    }

    int n, level;
    convertToLevelAndOption(opt, socketProtocol, level, n);
    if (n == -1)
        return false;
    if (::setsockopt(socketDescriptor, level, n, (char*)&v, sizeof(v)) != 0) {
        WS_ERROR_DEBUG(WSAGetLastError());
        return false;
    }
    return true;
}

/*!
    Fetches information about both ends of the connection: whatever is
    available.
*/
bool QNativeSocketEnginePrivate::fetchConnectionParameters()
{
    localPort = 0;
    localAddress.clear();
    peerPort = 0;
    peerAddress.clear();
    inboundStreamCount = outboundStreamCount = 0;

    if (socketDescriptor == -1)
       return false;

    qt_sockaddr sa;
    QT_SOCKLEN_T sockAddrSize = sizeof(sa);

    // Determine local address
    memset(&sa, 0, sizeof(sa));
    if (::getsockname(socketDescriptor, &sa.a, &sockAddrSize) == 0) {
        qt_socket_getPortAndAddress(socketDescriptor, &sa, &localPort, &localAddress);
        // Determine protocol family
        switch (sa.a.sa_family) {
        case AF_INET:
            socketProtocol = QAbstractSocket::IPv4Protocol;
            break;
        case AF_INET6:
            socketProtocol = QAbstractSocket::IPv6Protocol;
            break;
        default:
            socketProtocol = QAbstractSocket::UnknownNetworkLayerProtocol;
            break;
        }
    } else {
        int err = WSAGetLastError();
        WS_ERROR_DEBUG(err);
        if (err == WSAENOTSOCK) {
            setError(QAbstractSocket::UnsupportedSocketOperationError,
                InvalidSocketErrorString);
            return false;
        }
    }

    // determine if local address is dual mode
    DWORD ipv6only = 0;
    QT_SOCKOPTLEN_T optlen = sizeof(ipv6only);
    if (localAddress == QHostAddress::AnyIPv6
        && !getsockopt(socketDescriptor, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&ipv6only, &optlen )) {
            if (!ipv6only) {
                socketProtocol = QAbstractSocket::AnyIPProtocol;
                localAddress = QHostAddress::Any;
            }
    }

    // Some Windows kernels return a v4-mapped QHostAddress::AnyIPv4 as a
    // local address of the socket which bound on both IPv4 and IPv6 interfaces.
    // This address does not match to any special address and should not be used
    // to send the data. So, replace it with QHostAddress::Any.
    const uchar ipv6MappedNet[] = {0,0,0,0, 0,0,0,0, 0,0,0xff,0xff, 0,0,0,0};
    if (localAddress.isInSubnet(QHostAddress(ipv6MappedNet), 128 - 32)) {
        bool ok = false;
        const quint32 localIPv4 = localAddress.toIPv4Address(&ok);
        if (ok && localIPv4 == INADDR_ANY) {
            socketProtocol = QAbstractSocket::AnyIPProtocol;
            localAddress = QHostAddress::Any;
        }
    }

    memset(&sa, 0, sizeof(sa));
    if (::getpeername(socketDescriptor, &sa.a, &sockAddrSize) == 0) {
        qt_socket_getPortAndAddress(socketDescriptor, &sa, &peerPort, &peerAddress);
        inboundStreamCount = outboundStreamCount = 1;
    } else {
        WS_ERROR_DEBUG(WSAGetLastError());
    }

    socketType = qt_socket_getType(socketDescriptor);

#if defined (QNATIVESOCKETENGINE_DEBUG)
    QString socketProtocolStr = QStringLiteral("UnknownProtocol");
    if (socketProtocol == QAbstractSocket::IPv4Protocol) socketProtocolStr = QStringLiteral("IPv4Protocol");
    else if (socketProtocol == QAbstractSocket::IPv6Protocol) socketProtocolStr = QStringLiteral("IPv6Protocol");

    QString socketTypeStr = QStringLiteral("UnknownSocketType");
    if (socketType == QAbstractSocket::TcpSocket) socketTypeStr = QStringLiteral("TcpSocket");
    else if (socketType == QAbstractSocket::UdpSocket) socketTypeStr = QStringLiteral("UdpSocket");

    qDebug("QNativeSocketEnginePrivate::fetchConnectionParameters() localAddress == %s, localPort = %i, peerAddress == %s, peerPort = %i, socketProtocol == %s, socketType == %s", localAddress.toString().toLatin1().constData(), localPort, peerAddress.toString().toLatin1().constData(), peerPort, socketProtocolStr.toLatin1().constData(), socketTypeStr.toLatin1().constData());
#endif

    return true;
}


static void setErrorFromWSAError(int error, QNativeSocketEnginePrivate *d)
{
    Q_ASSERT(d);
    switch (error) {
    case WSAEISCONN:
        d->socketState = QAbstractSocket::ConnectedState;
        break;
    case WSAEHOSTUNREACH:
        d->setError(QAbstractSocket::NetworkError, QNativeSocketEnginePrivate::HostUnreachableErrorString);
        d->socketState = QAbstractSocket::UnconnectedState;
        break;
    case WSAEADDRNOTAVAIL:
        d->setError(QAbstractSocket::NetworkError, QNativeSocketEnginePrivate::AddressNotAvailableErrorString);
        d->socketState = QAbstractSocket::UnconnectedState;
        break;
    case WSAEINPROGRESS:
        d->setError(QAbstractSocket::UnfinishedSocketOperationError, QNativeSocketEnginePrivate::InvalidSocketErrorString);
        d->socketState = QAbstractSocket::ConnectingState;
        break;
    case WSAEADDRINUSE:
        d->setError(QAbstractSocket::NetworkError, QNativeSocketEnginePrivate::AddressInuseErrorString);
        break;
    case WSAECONNREFUSED:
        d->setError(QAbstractSocket::ConnectionRefusedError, QNativeSocketEnginePrivate::ConnectionRefusedErrorString);
        d->socketState = QAbstractSocket::UnconnectedState;
        break;
    case WSAETIMEDOUT:
        d->setError(QAbstractSocket::NetworkError, QNativeSocketEnginePrivate::ConnectionTimeOutErrorString);
        d->socketState = QAbstractSocket::UnconnectedState;
        break;
    case WSAEACCES:
        d->setError(QAbstractSocket::SocketAccessError, QNativeSocketEnginePrivate::AccessErrorString);
        d->socketState = QAbstractSocket::UnconnectedState;
        break;
    case WSAENETUNREACH:
        d->setError(QAbstractSocket::NetworkError, QNativeSocketEnginePrivate::NetworkUnreachableErrorString);
        d->socketState = QAbstractSocket::UnconnectedState;
        break;
    case WSAEINVAL:
    case WSAEALREADY:
        d->setError(QAbstractSocket::UnfinishedSocketOperationError, QNativeSocketEnginePrivate::InvalidSocketErrorString);
        break;
    default:
        break;
    }
}

bool QNativeSocketEnginePrivate::nativeConnect(const QHostAddress &address, quint16 port)
{

#if defined (QNATIVESOCKETENGINE_DEBUG)
    qDebug("QNativeSocketEnginePrivate::nativeConnect() to %s :: %i", address.toString().toLatin1().constData(), port);
#endif

    qt_sockaddr aa;
    QT_SOCKLEN_T sockAddrSize = 0;

    setPortAndAddress(port, address, &aa, &sockAddrSize);

    if ((socketProtocol == QAbstractSocket::IPv6Protocol || socketProtocol == QAbstractSocket::AnyIPProtocol) && address.toIPv4Address()) {
        //IPV6_V6ONLY option must be cleared to connect to a V4 mapped address
        DWORD ipv6only = 0;
        ipv6only = ::setsockopt(socketDescriptor, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&ipv6only, sizeof(ipv6only) );
    }

    forever {
        int connectResult = ::WSAConnect(socketDescriptor, &aa.a, sockAddrSize, 0,0,0,0);
        if (connectResult == SOCKET_ERROR) {
            int err = WSAGetLastError();
            WS_ERROR_DEBUG(err);

            switch (err) {
            case WSANOTINITIALISED:
                //###
                break;
            case WSAEWOULDBLOCK: {
                // If WSAConnect returns WSAEWOULDBLOCK on the second
                // connection attempt, we have to check SO_ERROR's
                // value to detect ECONNREFUSED. If we don't get
                // ECONNREFUSED, we'll have to treat it as an
                // unfinished operation.
                int value = 0;
                QT_SOCKLEN_T valueSize = sizeof(value);
                bool tryAgain = false;
                bool errorDetected = false;
                int tries = 0;
                do {
                    if (::getsockopt(socketDescriptor, SOL_SOCKET, SO_ERROR, (char *) &value, &valueSize) == 0) {
                        if (value != NOERROR) {
                            WS_ERROR_DEBUG(value);
                            errorDetected = true;
                            // MSDN says getsockopt with SO_ERROR clears the error, but it's not actually cleared
                            // and this can affect all subsequent WSAConnect attempts, so clear it now.
                            const int val = NO_ERROR;
                            ::setsockopt(socketDescriptor, SOL_SOCKET, SO_ERROR, reinterpret_cast<const char*>(&val), sizeof val);
                        } else {
                            // When we get WSAEWOULDBLOCK the outcome was not known, so a
                            // NOERROR might indicate that the result of the operation
                            // is still unknown. We try again to increase the chance that we did
                            // get the correct result.
                            tryAgain = !tryAgain;
                        }
                        setErrorFromWSAError(value, this);
                    }
                    tries++;
                } while (tryAgain && (tries < 2));

                if (errorDetected)
                    break;
                // fall through to unfinished operation error handling
                err = WSAEINPROGRESS;
                Q_FALLTHROUGH();
            }

            default:
                setErrorFromWSAError(err, this);
                break;
            }
            if (socketState != QAbstractSocket::ConnectedState) {
#if defined (QNATIVESOCKETENGINE_DEBUG)
                qDebug("QNativeSocketEnginePrivate::nativeConnect(%s, %i) == false (%s)",
                        address.toString().toLatin1().constData(), port,
                        socketState == QAbstractSocket::ConnectingState
                        ? "Connection in progress" : socketErrorString.toLatin1().constData());
#endif
                return false;
            }
        }
        break;
    }

#if defined (QNATIVESOCKETENGINE_DEBUG)
    qDebug("QNativeSocketEnginePrivate::nativeConnect(%s, %i) == true",
           address.toString().toLatin1().constData(), port);
#endif

    socketState = QAbstractSocket::ConnectedState;
    return true;
}


bool QNativeSocketEnginePrivate::nativeBind(const QHostAddress &a, quint16 port)
{
    QHostAddress address = a;
    if (address.protocol() == QAbstractSocket::IPv4Protocol) {
        if ((address.toIPv4Address() & 0xffff0000) == 0xefff0000) {
            // binding to a multicast address
            address = QHostAddress(QHostAddress::AnyIPv4);
        }
    }

    qt_sockaddr aa;
    QT_SOCKLEN_T sockAddrSize = 0;
    setPortAndAddress(port, address, &aa, &sockAddrSize);

    if (aa.a.sa_family == AF_INET6) {
        // The default may change in future, so set it explicitly
        int ipv6only = 0;
        if (address.protocol() == QAbstractSocket::IPv6Protocol)
            ipv6only = 1;
        ::setsockopt(socketDescriptor, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&ipv6only, sizeof(ipv6only) );
    }


    int bindResult = ::bind(socketDescriptor, &aa.a, sockAddrSize);
    if (bindResult == SOCKET_ERROR && WSAGetLastError() == WSAEAFNOSUPPORT
            && address.protocol() == QAbstractSocket::AnyIPProtocol) {
        // retry with v4
        aa.a4.sin_family = AF_INET;
        aa.a4.sin_port = htons(port);
        aa.a4.sin_addr.s_addr = htonl(address.toIPv4Address());
        sockAddrSize = sizeof(aa.a4);
        bindResult = ::bind(socketDescriptor, &aa.a, sockAddrSize);
    }
    if (bindResult == SOCKET_ERROR) {
        int err = WSAGetLastError();
        WS_ERROR_DEBUG(err);
        switch (err) {
        case WSANOTINITIALISED:
            //###
            break;
        case WSAEADDRINUSE:
        case WSAEINVAL:
            setError(QAbstractSocket::AddressInUseError, AddressInuseErrorString);
            break;
        case WSAEACCES:
            setError(QAbstractSocket::SocketAccessError, AddressProtectedErrorString);
            break;
        case WSAEADDRNOTAVAIL:
            setError(QAbstractSocket::SocketAddressNotAvailableError, AddressNotAvailableErrorString);
            break;
        default:
            break;
        }

#if defined (QNATIVESOCKETENGINE_DEBUG)
        qDebug("QNativeSocketEnginePrivate::nativeBind(%s, %i) == false (%s)",
               address.toString().toLatin1().constData(), port, socketErrorString.toLatin1().constData());
#endif

        return false;
    }

#if defined (QNATIVESOCKETENGINE_DEBUG)
    qDebug("QNativeSocketEnginePrivate::nativeBind(%s, %i) == true",
           address.toString().toLatin1().constData(), port);
#endif
    socketState = QAbstractSocket::BoundState;
    return true;
}


bool QNativeSocketEnginePrivate::nativeListen(int backlog)
{
    if (::listen(socketDescriptor, backlog) == SOCKET_ERROR) {
        int err = WSAGetLastError();
        WS_ERROR_DEBUG(err);
        switch (err) {
        case WSANOTINITIALISED:
            //###
            break;
        case WSAEADDRINUSE:
            setError(QAbstractSocket::AddressInUseError,
                     PortInuseErrorString);
            break;
        default:
            break;
        }

#if defined (QNATIVESOCKETENGINE_DEBUG)
        qDebug("QNativeSocketEnginePrivate::nativeListen(%i) == false (%s)",
               backlog, socketErrorString.toLatin1().constData());
#endif
        return false;
    }

#if defined (QNATIVESOCKETENGINE_DEBUG)
    qDebug("QNativeSocketEnginePrivate::nativeListen(%i) == true", backlog);
#endif

    socketState = QAbstractSocket::ListeningState;
    return true;
}

qintptr QNativeSocketEnginePrivate::nativeAccept()
{
    SOCKET acceptedDescriptor = WSAAccept(socketDescriptor, 0,0,0,0);
    if (acceptedDescriptor == INVALID_SOCKET) {
        int err = WSAGetLastError();
        switch (err) {
        case WSAEACCES:
            setError(QAbstractSocket::SocketAccessError, AccessErrorString);
            break;
        case WSAECONNREFUSED:
            setError(QAbstractSocket::ConnectionRefusedError, ConnectionRefusedErrorString);
            break;
        case WSAECONNRESET:
            setError(QAbstractSocket::NetworkError, RemoteHostClosedErrorString);
            break;
        case WSAENETDOWN:
            setError(QAbstractSocket::NetworkError, NetworkUnreachableErrorString);
            break;
        case WSAENOTSOCK:
            setError(QAbstractSocket::SocketResourceError, NotSocketErrorString);
            break;
        case WSAEINVAL:
        case WSAEOPNOTSUPP:
            setError(QAbstractSocket::UnsupportedSocketOperationError, ProtocolUnsupportedErrorString);
            break;
        case WSAEFAULT:
        case WSAEMFILE:
        case WSAENOBUFS:
            setError(QAbstractSocket::SocketResourceError, ResourceErrorString);
            break;
        case WSAEWOULDBLOCK:
            setError(QAbstractSocket::TemporaryError, TemporaryErrorString);
            break;
        default:
            setError(QAbstractSocket::UnknownSocketError, UnknownSocketErrorString);
            break;
        }
    } else if (acceptedDescriptor != INVALID_SOCKET && QAbstractEventDispatcher::instance()) {
        // Because of WSAAsyncSelect() WSAAccept returns a non blocking socket
        // with the same attributes as the listening socket including the current
        // WSAAsyncSelect(). To be able to change the socket to blocking mode the
        // WSAAsyncSelect() call must be canceled.
        QSocketNotifier n(acceptedDescriptor, QSocketNotifier::Read);
        n.setEnabled(true);
        n.setEnabled(false);
    }
#if defined (QNATIVESOCKETENGINE_DEBUG)
    qDebug("QNativeSocketEnginePrivate::nativeAccept() == %lld", qint64(acceptedDescriptor));
#endif
    return qintptr(acceptedDescriptor);
}

static bool multicastMembershipHelper(QNativeSocketEnginePrivate *d,
                                      int how6,
                                      int how4,
                                      const QHostAddress &groupAddress,
                                      const QNetworkInterface &iface)
{
    int level = 0;
    int sockOpt = 0;
    char *sockArg;
    int sockArgSize;

    struct ip_mreq mreq4;
    struct ipv6_mreq mreq6;

    if (groupAddress.protocol() == QAbstractSocket::IPv6Protocol) {
        level = IPPROTO_IPV6;
        sockOpt = how6;
        sockArg = reinterpret_cast<char *>(&mreq6);
        sockArgSize = sizeof(mreq6);
        memset(&mreq6, 0, sizeof(mreq6));
        Q_IPV6ADDR ip6 = groupAddress.toIPv6Address();
        memcpy(&mreq6.ipv6mr_multiaddr, &ip6, sizeof(ip6));
        mreq6.ipv6mr_interface = iface.index();
    } else if (groupAddress.protocol() == QAbstractSocket::IPv4Protocol) {
        level = IPPROTO_IP;
        sockOpt = how4;
        sockArg = reinterpret_cast<char *>(&mreq4);
        sockArgSize = sizeof(mreq4);
        memset(&mreq4, 0, sizeof(mreq4));
        mreq4.imr_multiaddr.s_addr = htonl(groupAddress.toIPv4Address());

        if (iface.isValid()) {
            const QList<QNetworkAddressEntry> addressEntries = iface.addressEntries();
            bool found = false;
            for (const QNetworkAddressEntry &entry : addressEntries) {
                const QHostAddress ip = entry.ip();
                if (ip.protocol() == QAbstractSocket::IPv4Protocol) {
                    mreq4.imr_interface.s_addr = htonl(ip.toIPv4Address());
                    found = true;
                    break;
                }
            }
            if (!found) {
                d->setError(QAbstractSocket::NetworkError,
                            QNativeSocketEnginePrivate::NetworkUnreachableErrorString);
                return false;
            }
        } else {
            mreq4.imr_interface.s_addr = INADDR_ANY;
        }
    } else {
        // unreachable
        d->setError(QAbstractSocket::UnsupportedSocketOperationError,
                    QNativeSocketEnginePrivate::ProtocolUnsupportedErrorString);
        return false;
    }

    int res = setsockopt(d->socketDescriptor, level, sockOpt, sockArg, sockArgSize);
    if (res == -1) {
        d->setError(QAbstractSocket::UnsupportedSocketOperationError,
                    QNativeSocketEnginePrivate::OperationUnsupportedErrorString);
        return false;
    }
    return true;
}

bool QNativeSocketEnginePrivate::nativeJoinMulticastGroup(const QHostAddress &groupAddress,
                                                          const QNetworkInterface &iface)
{
    return multicastMembershipHelper(this,
                                     IPV6_JOIN_GROUP,
                                     IP_ADD_MEMBERSHIP,
                                     groupAddress,
                                     iface);
}

bool QNativeSocketEnginePrivate::nativeLeaveMulticastGroup(const QHostAddress &groupAddress,
                                                           const QNetworkInterface &iface)
{
    return multicastMembershipHelper(this,
                                     IPV6_LEAVE_GROUP,
                                     IP_DROP_MEMBERSHIP,
                                     groupAddress,
                                     iface);
}

QNetworkInterface QNativeSocketEnginePrivate::nativeMulticastInterface() const
{
    if (socketProtocol == QAbstractSocket::IPv6Protocol || socketProtocol == QAbstractSocket::AnyIPProtocol) {
        uint v;
        QT_SOCKOPTLEN_T sizeofv = sizeof(v);
        if (::getsockopt(socketDescriptor, IPPROTO_IPV6, IPV6_MULTICAST_IF, (char *) &v, &sizeofv) == -1)
            return QNetworkInterface();
        return QNetworkInterface::interfaceFromIndex(v);
    }

    struct in_addr v;
    v.s_addr = 0;
    QT_SOCKOPTLEN_T sizeofv = sizeof(v);
    if (::getsockopt(socketDescriptor, IPPROTO_IP, IP_MULTICAST_IF, (char *) &v, &sizeofv) == -1)
        return QNetworkInterface();
    if (v.s_addr != 0 && sizeofv >= QT_SOCKOPTLEN_T(sizeof(v))) {
        QHostAddress ipv4(ntohl(v.s_addr));
        QList<QNetworkInterface> ifaces = QNetworkInterface::allInterfaces();
        for (int i = 0; i < ifaces.count(); ++i) {
            const QNetworkInterface &iface = ifaces.at(i);
            if (!(iface.flags() & QNetworkInterface::CanMulticast))
                continue;
            QList<QNetworkAddressEntry> entries = iface.addressEntries();
            for (int j = 0; j < entries.count(); ++j) {
                const QNetworkAddressEntry &entry = entries.at(j);
                if (entry.ip() == ipv4)
                    return iface;
            }
        }
    }
    return QNetworkInterface();
}

bool QNativeSocketEnginePrivate::nativeSetMulticastInterface(const QNetworkInterface &iface)
{

    if (socketProtocol == QAbstractSocket::IPv6Protocol || socketProtocol == QAbstractSocket::AnyIPProtocol) {
        uint v = iface.isValid() ? iface.index() : 0;
        return (::setsockopt(socketDescriptor, IPPROTO_IPV6, IPV6_MULTICAST_IF, (char *) &v, sizeof(v)) != -1);
    }

    struct in_addr v;
    if (iface.isValid()) {
        QList<QNetworkAddressEntry> entries = iface.addressEntries();
        for (int i = 0; i < entries.count(); ++i) {
            const QNetworkAddressEntry &entry = entries.at(i);
            const QHostAddress &ip = entry.ip();
            if (ip.protocol() == QAbstractSocket::IPv4Protocol) {
                v.s_addr = htonl(ip.toIPv4Address());
                int r = ::setsockopt(socketDescriptor, IPPROTO_IP, IP_MULTICAST_IF, (char *) &v, sizeof(v));
                if (r != -1)
                    return true;
            }
        }
        return false;
    }

    v.s_addr = INADDR_ANY;
    return (::setsockopt(socketDescriptor, IPPROTO_IP, IP_MULTICAST_IF, (char *) &v, sizeof(v)) != -1);
}

qint64 QNativeSocketEnginePrivate::nativeBytesAvailable() const
{
    unsigned long  nbytes = 0;
    unsigned long dummy = 0;
    DWORD sizeWritten = 0;
    if (::WSAIoctl(socketDescriptor, FIONREAD, &dummy, sizeof(dummy), &nbytes, sizeof(nbytes), &sizeWritten, 0,0) == SOCKET_ERROR) {
        WS_ERROR_DEBUG(WSAGetLastError());
        return -1;
    }

    // ioctlsocket sometimes reports 1 byte available for datagrams
    // while the following recvfrom returns -1 and claims connection
    // was reset (udp is connectionless). so we peek one byte to
    // catch this case and return 0 bytes available if recvfrom
    // fails.
    if (nbytes == 1 && socketType == QAbstractSocket::UdpSocket) {
        char c;
        WSABUF buf;
        buf.buf = &c;
        buf.len = sizeof(c);
        DWORD bytesReceived;
        DWORD flags = MSG_PEEK;
        if (::WSARecvFrom(socketDescriptor, &buf, 1, &bytesReceived, &flags, 0,0,0,0) == SOCKET_ERROR) {
            int err = WSAGetLastError();
            if (err != WSAECONNRESET && err != WSAENETRESET)
                return 0;
        } else {
            return bytesReceived;
        }
    }
    return nbytes;
}


bool QNativeSocketEnginePrivate::nativeHasPendingDatagrams() const
{
    // Create a sockaddr struct and reset its port number.
    qt_sockaddr storage;
    QT_SOCKLEN_T storageSize = sizeof(storage);
    memset(&storage, 0, storageSize);

    bool result = false;

    // Peek 0 bytes into the next message. The size of the message may
    // well be 0, so we check if there was a sender.
    char c;
    WSABUF buf;
    buf.buf = &c;
    buf.len = sizeof(c);
    DWORD available = 0;
    DWORD flags = MSG_PEEK;
    int ret = ::WSARecvFrom(socketDescriptor, &buf, 1, &available, &flags, &storage.a, &storageSize,0,0);
    int err = WSAGetLastError();
    if (ret == SOCKET_ERROR && err !=  WSAEMSGSIZE) {
        WS_ERROR_DEBUG(err);
        result = (err == WSAECONNRESET || err == WSAENETRESET);
    } else {
        // If there's no error, or if our buffer was too small, there must be
        // a pending datagram.
        result = true;
    }

#if defined (QNATIVESOCKETENGINE_DEBUG)
    qDebug("QNativeSocketEnginePrivate::nativeHasPendingDatagrams() == %s",
           result ? "true" : "false");
#endif
    return result;
}


qint64 QNativeSocketEnginePrivate::nativePendingDatagramSize() const
{
    qint64 ret = -1;
    int recvResult = 0;
    DWORD flags;
    // We increase the amount we peek by 2048 * 5 on each iteration
    // Grabs most cases fast and early.
    char udpMessagePeekBuffer[2048];
    const int increments = 5;
    QVarLengthArray<WSABUF, 10> buf;
    for (;;) {
        buf.reserve(buf.size() + increments);
        std::fill_n(std::back_inserter(buf), increments, WSABUF{sizeof(udpMessagePeekBuffer), udpMessagePeekBuffer});

        flags = MSG_PEEK;
        DWORD bytesRead = 0;
        recvResult = ::WSARecv(socketDescriptor, buf.data(), DWORD(buf.size()), &bytesRead, &flags, nullptr, nullptr);
        int err = WSAGetLastError();
        if (recvResult != SOCKET_ERROR) {
            ret = qint64(bytesRead);
            break;
        } else {
            switch (err) {
            case WSAEMSGSIZE:
                continue;
            case WSAECONNRESET:
            case WSAENETRESET:
                ret = 0;
                break;
            default:
                WS_ERROR_DEBUG(err);
                ret = -1;
                break;
            }
            break;
        }
    }

#if defined (QNATIVESOCKETENGINE_DEBUG)
    qDebug("QNativeSocketEnginePrivate::nativePendingDatagramSize() == %lli", ret);
#endif

    return ret;
}

qint64 QNativeSocketEnginePrivate::nativeReceiveDatagram(char *data, qint64 maxLength, QIpPacketHeader *header,
                                                         QAbstractSocketEngine::PacketHeaderOptions options)
{
    union {
        char cbuf[WSA_CMSG_SPACE(sizeof(struct in6_pktinfo)) + WSA_CMSG_SPACE(sizeof(int))];
        WSACMSGHDR align;    // only to ensure alignment
    };
    WSAMSG msg;
    WSABUF buf;
    qt_sockaddr aa;
    char c;
    memset(&msg, 0, sizeof(msg));
    memset(&aa, 0, sizeof(aa));

    // we need to receive at least one byte, even if our user isn't interested in it
    buf.buf = maxLength ? data : &c;
    buf.len = maxLength ? maxLength : 1;
    msg.lpBuffers = &buf;
    msg.dwBufferCount = 1;
    msg.name = reinterpret_cast<LPSOCKADDR>(&aa);
    msg.namelen = sizeof(aa);
    msg.Control.buf = cbuf;
    msg.Control.len = sizeof(cbuf);

    DWORD flags = 0;
    DWORD bytesRead = 0;
    qint64 ret;

    if (recvmsg)
        ret = recvmsg(socketDescriptor, &msg, &bytesRead, 0,0);
    else
        ret = ::WSARecvFrom(socketDescriptor, &buf, 1, &bytesRead, &flags, msg.name, &msg.namelen,0,0);
    if (ret == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err == WSAEMSGSIZE) {
            // it is ok the buffer was to small if bytesRead is larger than
            // maxLength then assume bytes read is really maxLenth
            ret = qint64(bytesRead) > maxLength ? maxLength : qint64(bytesRead);
            if (options & QNativeSocketEngine::WantDatagramSender)
                qt_socket_getPortAndAddress(socketDescriptor, &aa, &header->senderPort, &header->senderAddress);
        } else {
            WS_ERROR_DEBUG(err);
            switch (err) {
            case WSAENETRESET:
                setError(QAbstractSocket::NetworkError, NetworkDroppedConnectionErrorString);
                break;
            case WSAECONNRESET:
                setError(QAbstractSocket::ConnectionRefusedError, ConnectionResetErrorString);
                break;
            default:
                setError(QAbstractSocket::NetworkError, ReceiveDatagramErrorString);
                break;
            }
            ret = -1;
            if (header)
                header->clear();
        }
    } else {
        ret = qint64(bytesRead);
        if (options & QNativeSocketEngine::WantDatagramSender)
            qt_socket_getPortAndAddress(socketDescriptor, &aa, &header->senderPort, &header->senderAddress);
    }

    if (ret != -1 && recvmsg && options != QAbstractSocketEngine::WantNone) {
        // get the ancillary data
        header->destinationPort = localPort;
        WSACMSGHDR *cmsgptr;
        for (cmsgptr = WSA_CMSG_FIRSTHDR(&msg); cmsgptr != NULL;
             cmsgptr = WSA_CMSG_NXTHDR(&msg, cmsgptr)) {
            if (cmsgptr->cmsg_level == IPPROTO_IPV6 && cmsgptr->cmsg_type == IPV6_PKTINFO
                    && cmsgptr->cmsg_len >= WSA_CMSG_LEN(sizeof(in6_pktinfo))) {
                in6_pktinfo *info = reinterpret_cast<in6_pktinfo *>(WSA_CMSG_DATA(cmsgptr));

                header->destinationAddress.setAddress(reinterpret_cast<quint8 *>(&info->ipi6_addr));
                header->ifindex = info->ipi6_ifindex;
                if (header->ifindex)
                    header->destinationAddress.setScopeId(QString::number(info->ipi6_ifindex));
            }
            if (cmsgptr->cmsg_level == IPPROTO_IP && cmsgptr->cmsg_type == IP_PKTINFO
                    && cmsgptr->cmsg_len >= WSA_CMSG_LEN(sizeof(in_pktinfo))) {
                in_pktinfo *info = reinterpret_cast<in_pktinfo *>(WSA_CMSG_DATA(cmsgptr));
                u_long addr;
                WSANtohl(socketDescriptor, info->ipi_addr.s_addr, &addr);
                header->destinationAddress.setAddress(addr);
                header->ifindex = info->ipi_ifindex;
            }

            if (cmsgptr->cmsg_len == WSA_CMSG_LEN(sizeof(int))
                    && ((cmsgptr->cmsg_level == IPPROTO_IPV6 && cmsgptr->cmsg_type == IPV6_HOPLIMIT)
                        || (cmsgptr->cmsg_level == IPPROTO_IP && cmsgptr->cmsg_type == IP_TTL))) {
                header->hopLimit = *reinterpret_cast<int *>(WSA_CMSG_DATA(cmsgptr));
            }
        }
    }

#if defined (QNATIVESOCKETENGINE_DEBUG)
    bool printSender = (ret != -1 && (options & QNativeSocketEngine::WantDatagramSender) != 0);
    qDebug("QNativeSocketEnginePrivate::nativeReceiveDatagram(%p \"%s\", %lli, %s, %i) == %lli",
           data, QtDebugUtils::toPrintable(data, ret, 16).constData(), maxLength,
           printSender ? header->senderAddress.toString().toLatin1().constData() : "(unknown)",
           printSender ? header->senderPort : 0, ret);
#endif

    return ret;
}


qint64 QNativeSocketEnginePrivate::nativeSendDatagram(const char *data, qint64 len,
                                                      const QIpPacketHeader &header)
{
    union {
        char cbuf[WSA_CMSG_SPACE(sizeof(struct in6_pktinfo)) + WSA_CMSG_SPACE(sizeof(int))];
        WSACMSGHDR align;    // ensures alignment
    };
    WSACMSGHDR *cmsgptr = &align;
    WSAMSG msg;
    WSABUF buf;
    qt_sockaddr aa;

    memset(&msg, 0, sizeof(msg));
    memset(&aa, 0, sizeof(aa));
    buf.buf = len ? (char*)data : 0;
    msg.lpBuffers = &buf;
    msg.dwBufferCount = 1;
    msg.name = &aa.a;
    buf.len = len;

    setPortAndAddress(header.destinationPort, header.destinationAddress, &aa, &msg.namelen);

    uint oldIfIndex = 0;
    bool mustSetIpv6MulticastIf = false;

    if (msg.namelen == sizeof(aa.a6)) {
        // sending IPv6
        if (header.hopLimit != -1) {
            msg.Control.len += WSA_CMSG_SPACE(sizeof(int));
            cmsgptr->cmsg_len = WSA_CMSG_LEN(sizeof(int));
            cmsgptr->cmsg_level = IPPROTO_IPV6;
            cmsgptr->cmsg_type = IPV6_HOPLIMIT;
            memcpy(WSA_CMSG_DATA(cmsgptr), &header.hopLimit, sizeof(int));
            cmsgptr = reinterpret_cast<WSACMSGHDR *>(reinterpret_cast<char *>(cmsgptr)
                                                     + WSA_CMSG_SPACE(sizeof(int)));
        }
        if (!header.senderAddress.isNull()) {
            struct in6_pktinfo *data = reinterpret_cast<in6_pktinfo *>(WSA_CMSG_DATA(cmsgptr));
            memset(data, 0, sizeof(*data));
            msg.Control.len += WSA_CMSG_SPACE(sizeof(*data));
            cmsgptr->cmsg_len = WSA_CMSG_LEN(sizeof(*data));
            cmsgptr->cmsg_level = IPPROTO_IPV6;
            cmsgptr->cmsg_type = IPV6_PKTINFO;
            data->ipi6_ifindex = header.ifindex;

            Q_IPV6ADDR tmp = header.senderAddress.toIPv6Address();
            memcpy(&data->ipi6_addr, &tmp, sizeof(tmp));
            cmsgptr = reinterpret_cast<WSACMSGHDR *>(reinterpret_cast<char *>(cmsgptr)
                                                     + WSA_CMSG_SPACE(sizeof(*data)));
        } else if (header.ifindex != 0) {
            // Unlike other operating systems, setting the interface index in the in6_pktinfo
            // structure above and leaving the ipi6_addr set to :: will cause the packets to be
            // sent with source address ::. So we have to use IPV6_MULTICAST_IF, which MSDN is
            // quite clear that "This option does not change the default interface for receiving
            // IPv6 multicast traffic."
            QT_SOCKOPTLEN_T len = sizeof(oldIfIndex);
            if (::getsockopt(socketDescriptor, IPPROTO_IPV6, IPV6_MULTICAST_IF,
                             reinterpret_cast<char *>(&oldIfIndex), &len) == -1
                    || ::setsockopt(socketDescriptor, IPPROTO_IPV6, IPV6_MULTICAST_IF,
                                    reinterpret_cast<const char *>(&header.ifindex), sizeof(header.ifindex)) == -1) {
                setError(QAbstractSocket::NetworkError, SendDatagramErrorString);
                return -1;
            }
            mustSetIpv6MulticastIf = true;
        }
    } else {
        // sending IPv4
        if (header.hopLimit != -1) {
            msg.Control.len += WSA_CMSG_SPACE(sizeof(int));
            cmsgptr->cmsg_len = WSA_CMSG_LEN(sizeof(int));
            cmsgptr->cmsg_level = IPPROTO_IP;
            cmsgptr->cmsg_type = IP_TTL;
            memcpy(WSA_CMSG_DATA(cmsgptr), &header.hopLimit, sizeof(int));
            cmsgptr = reinterpret_cast<WSACMSGHDR *>(reinterpret_cast<char *>(cmsgptr)
                                                     + WSA_CMSG_SPACE(sizeof(int)));
        }
        if (header.ifindex != 0 || !header.senderAddress.isNull()) {
            struct in_pktinfo *data = reinterpret_cast<in_pktinfo *>(WSA_CMSG_DATA(cmsgptr));
            memset(data, 0, sizeof(*data));
            msg.Control.len += WSA_CMSG_SPACE(sizeof(*data));
            cmsgptr->cmsg_len = WSA_CMSG_LEN(sizeof(*data));
            cmsgptr->cmsg_level = IPPROTO_IP;
            cmsgptr->cmsg_type = IP_PKTINFO;
            data->ipi_ifindex = header.ifindex;
            WSAHtonl(socketDescriptor, header.senderAddress.toIPv4Address(), &data->ipi_addr.s_addr);
            cmsgptr = reinterpret_cast<WSACMSGHDR *>(reinterpret_cast<char *>(cmsgptr)
                                                     + WSA_CMSG_SPACE(sizeof(*data)));
        }
    }

    if (msg.Control.len != 0)
        msg.Control.buf = cbuf;

    DWORD flags = 0;
    DWORD bytesSent = 0;
    qint64 ret = -1;
    if (sendmsg) {
        ret = sendmsg(socketDescriptor, &msg, flags, &bytesSent, 0,0);
    } else {
        ret = ::WSASendTo(socketDescriptor, &buf, 1, &bytesSent, flags, msg.name, msg.namelen, 0,0);
    }
    if (ret == SOCKET_ERROR) {
        int err = WSAGetLastError();
        WS_ERROR_DEBUG(err);
        switch (err) {
        case WSAEMSGSIZE:
            setError(QAbstractSocket::DatagramTooLargeError, DatagramTooLargeErrorString);
            break;
        default:
            setError(QAbstractSocket::NetworkError, SendDatagramErrorString);
            break;
        }
        ret = -1;
    } else {
        ret = qint64(bytesSent);
    }

    if (mustSetIpv6MulticastIf) {
        // undo what we did above
        ::setsockopt(socketDescriptor, IPPROTO_IPV6, IPV6_MULTICAST_IF,
                     reinterpret_cast<char *>(&oldIfIndex), sizeof(oldIfIndex));
    }

#if defined (QNATIVESOCKETENGINE_DEBUG)
    qDebug("QNativeSocketEnginePrivate::nativeSendDatagram(%p \"%s\", %lli, \"%s\", %i) == %lli",
           data, QtDebugUtils::toPrintable(data, len, 16).constData(), len,
           header.destinationAddress.toString().toLatin1().constData(),
           header.destinationPort, ret);
#endif

    return ret;
}


qint64 QNativeSocketEnginePrivate::nativeWrite(const char *data, qint64 len)
{
    Q_Q(QNativeSocketEngine);
    qint64 ret = 0;
    qint64 bytesToSend = len;

    for (;;) {
        WSABUF buf;
        buf.buf = const_cast<char*>(data) + ret;
        buf.len = bytesToSend;
        DWORD flags = 0;
        DWORD bytesWritten = 0;

        int socketRet = ::WSASend(socketDescriptor, &buf, 1, &bytesWritten, flags, 0,0);

        ret += qint64(bytesWritten);

        int err;
        if (socketRet != SOCKET_ERROR) {
            if (ret == len || bytesToSend != qint64(bytesWritten))
                break;
        } else if ((err = WSAGetLastError()) == WSAEWOULDBLOCK) {
            break;
        } else if (err == WSAENOBUFS) {
            // this function used to not send more than 49152 per call to WSASendTo
            // to avoid getting a WSAENOBUFS. However this is a performance regression
            // and we think it only appears with old windows versions. We now handle the
            // WSAENOBUFS and hope it never appears anyway.
            // just go on, the next loop run we will try a smaller number
        } else {
            WS_ERROR_DEBUG(err);
            switch (err) {
            case WSAECONNRESET:
            case WSAECONNABORTED:
                ret = -1;
                setError(QAbstractSocket::NetworkError, WriteErrorString);
                q->close();
                break;
            default:
                break;
            }
            break;
        }

        // for next send:
        bytesToSend = qMin<qint64>(49152, len - ret);
    }

#if defined (QNATIVESOCKETENGINE_DEBUG)
    qDebug("QNativeSocketEnginePrivate::nativeWrite(%p \"%s\", %lli) == %lli",
           data, QtDebugUtils::toPrintable(data, ret, 16).constData(), len, ret);
#endif

    return ret;
}

qint64 QNativeSocketEnginePrivate::nativeRead(char *data, qint64 maxLength)
{
    qint64 ret = -1;
    WSABUF buf;
    buf.buf = data;
    buf.len = maxLength;
    DWORD flags = 0;
    DWORD bytesRead = 0;
    if (::WSARecv(socketDescriptor, &buf, 1, &bytesRead, &flags, 0,0) ==  SOCKET_ERROR) {
        int err = WSAGetLastError();
        WS_ERROR_DEBUG(err);
        switch (err) {
        case WSAEWOULDBLOCK:
            ret = -2;
            break;
        case WSAEBADF:
        case WSAEINVAL:
            //error string is now set in read(), not here in nativeRead()
            break;
        case WSAECONNRESET:
        case WSAECONNABORTED:
            // for tcp sockets this will be handled in QNativeSocketEngine::read
            ret = 0;
            break;
        default:
            break;
        }
    } else {
        if (WSAGetLastError() == WSAEWOULDBLOCK)
            ret = -2;
        else
            ret = qint64(bytesRead);
    }

#if defined (QNATIVESOCKETENGINE_DEBUG)
    if (ret != -2) {
        qDebug("QNativeSocketEnginePrivate::nativeRead(%p \"%s\", %lli) == %lli", data,
               QtDebugUtils::toPrintable(data, bytesRead, 16).constData(), maxLength, ret);
    } else {
        qDebug("QNativeSocketEnginePrivate::nativeRead(%p, %lli) == -2 (WOULD BLOCK)",
               data, maxLength);
    }
#endif

    return ret;
}

int QNativeSocketEnginePrivate::nativeSelect(int timeout, bool selectForRead) const
{
    bool readEnabled = selectForRead && readNotifier && readNotifier->isEnabled();
    if (readEnabled)
        readNotifier->setEnabled(false);

    fd_set fds;

    int ret = 0;

    memset(&fds, 0, sizeof(fd_set));
    fds.fd_count = 1;
    fds.fd_array[0] = (SOCKET)socketDescriptor;

    struct timeval tv;
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;

    if (selectForRead) {
        ret = select(0, &fds, 0, 0, timeout < 0 ? 0 : &tv);
    } else {
        // select for write

        // Windows needs this to report errors when connecting a socket ...
        fd_set fdexception;
        FD_ZERO(&fdexception);
        FD_SET((SOCKET)socketDescriptor, &fdexception);

        ret = select(0, 0, &fds, &fdexception, timeout < 0 ? 0 : &tv);

        // ... but if it is actually set, pretend it did not happen
        if (ret > 0 && FD_ISSET((SOCKET)socketDescriptor, &fdexception))
            ret--;
    }

    if (readEnabled)
        readNotifier->setEnabled(true);

    return ret;
}

int QNativeSocketEnginePrivate::nativeSelect(int timeout,
                                      bool checkRead, bool checkWrite,
                                      bool *selectForRead, bool *selectForWrite) const
{
    bool readEnabled = checkRead && readNotifier && readNotifier->isEnabled();
    if (readEnabled)
        readNotifier->setEnabled(false);

    fd_set fdread;
    fd_set fdwrite;
    fd_set fdexception;

    int ret = 0;

    memset(&fdread, 0, sizeof(fd_set));
    if (checkRead) {
        fdread.fd_count = 1;
        fdread.fd_array[0] = (SOCKET)socketDescriptor;
    }
    memset(&fdwrite, 0, sizeof(fd_set));
    FD_ZERO(&fdexception);
    if (checkWrite) {
        fdwrite.fd_count = 1;
        fdwrite.fd_array[0] = (SOCKET)socketDescriptor;

        // Windows needs this to report errors when connecting a socket
        FD_SET((SOCKET)socketDescriptor, &fdexception);
    }

    struct timeval tv;
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;

    ret = select(socketDescriptor + 1, &fdread, &fdwrite, &fdexception, timeout < 0 ? 0 : &tv);

     //... but if it is actually set, pretend it did not happen
    if (ret > 0 && FD_ISSET((SOCKET)socketDescriptor, &fdexception))
        ret--;

    if (readEnabled)
        readNotifier->setEnabled(true);

    if (ret <= 0)
        return ret;

    *selectForRead = FD_ISSET((SOCKET)socketDescriptor, &fdread);
    *selectForWrite = FD_ISSET((SOCKET)socketDescriptor, &fdwrite);

    return ret;
}

void QNativeSocketEnginePrivate::nativeClose()
{
#if defined (QTCPSOCKETENGINE_DEBUG)
    qDebug("QNativeSocketEnginePrivate::nativeClose()");
#endif
    // We were doing a setsockopt here before with SO_DONTLINGER. (However with kind of wrong
    // usage of parameters, it wants a BOOL but we used a struct and pretended it to be bool).
    // We don't think setting this option should be done here, if a user wants it she/he can
     // do it manually with socketDescriptor()/setSocketDescriptor();
    ::closesocket(socketDescriptor);
}

QT_END_NAMESPACE
