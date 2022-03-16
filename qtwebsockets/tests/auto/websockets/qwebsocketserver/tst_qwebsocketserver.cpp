/****************************************************************************
**
** Copyright (C) 2016 Kurt Pattyn <pattyn.kurt@gmail.com>.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <QString>
#include <QtTest>
#include <QNetworkProxy>
#include <QTcpServer>
#ifndef QT_NO_OPENSSL
#include <QtNetwork/qsslpresharedkeyauthenticator.h>
#endif
#ifndef QT_NO_SSL
#include <QtNetwork/qsslcipher.h>
#include <QtNetwork/qsslkey.h>
#include <QtNetwork/qsslsocket.h>
#endif
#include <QtWebSockets/QWebSocketServer>
#include <QtWebSockets/QWebSocket>
#include <QtWebSockets/QWebSocketCorsAuthenticator>
#include <QtWebSockets/qwebsocketprotocol.h>

QT_USE_NAMESPACE

Q_DECLARE_METATYPE(QWebSocketProtocol::Version)
Q_DECLARE_METATYPE(QWebSocketProtocol::CloseCode)
Q_DECLARE_METATYPE(QWebSocketServer::SslMode)
Q_DECLARE_METATYPE(QWebSocketCorsAuthenticator *)
#ifndef QT_NO_SSL
Q_DECLARE_METATYPE(QSslError)
#endif

#ifndef QT_NO_OPENSSL
// Use this cipher to force PSK key sharing.
static const QString PSK_CIPHER_WITHOUT_AUTH = QStringLiteral("PSK-AES256-CBC-SHA");
static const QByteArray PSK_CLIENT_PRESHAREDKEY = QByteArrayLiteral("\x1a\x2b\x3c\x4d\x5e\x6f");
static const QByteArray PSK_SERVER_IDENTITY_HINT = QByteArrayLiteral("QtTestServerHint");
static const QByteArray PSK_CLIENT_IDENTITY = QByteArrayLiteral("Client_identity");

class PskProvider : public QObject
{
    Q_OBJECT

public:
    bool m_server = false;
    QByteArray m_identity;
    QByteArray m_psk;

public slots:
    void providePsk(QSslPreSharedKeyAuthenticator *authenticator)
    {
        QVERIFY(authenticator);
        QCOMPARE(authenticator->identityHint(), PSK_SERVER_IDENTITY_HINT);
        if (m_server)
            QCOMPARE(authenticator->maximumIdentityLength(), 0);
        else
            QVERIFY(authenticator->maximumIdentityLength() > 0);

        QVERIFY(authenticator->maximumPreSharedKeyLength() > 0);

        if (!m_identity.isEmpty()) {
            authenticator->setIdentity(m_identity);
            QCOMPARE(authenticator->identity(), m_identity);
        }

        if (!m_psk.isEmpty()) {
            authenticator->setPreSharedKey(m_psk);
            QCOMPARE(authenticator->preSharedKey(), m_psk);
        }
    }
};
#endif

class tst_QWebSocketServer : public QObject
{
    Q_OBJECT

public:
    tst_QWebSocketServer();

private Q_SLOTS:
    void init();
    void initTestCase();
    void cleanupTestCase();
    void tst_initialisation();
    void tst_settersAndGetters();
    void tst_listening();
    void tst_connectivity();
    void tst_preSharedKey();
    void tst_maxPendingConnections();
    void tst_serverDestroyedWhileSocketConnected();
    void tst_scheme(); // qtbug-55927
    void tst_handleConnection();

private:
    bool m_shouldSkipUnsupportedIpv6Test;
#ifdef SHOULD_CHECK_SYSCALL_SUPPORT
    bool ipv6GetsockoptionMissing(int level, int optname);
#endif
};

tst_QWebSocketServer::tst_QWebSocketServer() : m_shouldSkipUnsupportedIpv6Test(false)
{
}

void tst_QWebSocketServer::init()
{
    qRegisterMetaType<QWebSocketProtocol::Version>("QWebSocketProtocol::Version");
    qRegisterMetaType<QWebSocketProtocol::CloseCode>("QWebSocketProtocol::CloseCode");
    qRegisterMetaType<QWebSocketServer::SslMode>("QWebSocketServer::SslMode");
    qRegisterMetaType<QWebSocketCorsAuthenticator *>("QWebSocketCorsAuthenticator *");
#ifndef QT_NO_SSL
    qRegisterMetaType<QSslError>("QSslError");
#ifndef QT_NO_OPENSSL
    qRegisterMetaType<QSslPreSharedKeyAuthenticator *>();
#endif
#endif
}

#ifdef SHOULD_CHECK_SYSCALL_SUPPORT
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>

bool tst_QWebSocketServer::ipv6GetsockoptionMissing(int level, int optname)
{
    int testSocket;

    testSocket = socket(PF_INET6, SOCK_STREAM, 0);

    // If we can't test here, assume it's not missing
    if (testSocket == -1)
        return false;

    bool result = false;
    if (getsockopt(testSocket, level, optname, nullptr, 0) == -1) {
        if (errno == EOPNOTSUPP) {
            result = true;
        }
    }

    close(testSocket);
    return result;
}

#endif //SHOULD_CHECK_SYSCALL_SUPPORT

void tst_QWebSocketServer::initTestCase()
{
#ifdef SHOULD_CHECK_SYSCALL_SUPPORT
    // Qemu does not have required support for IPV6 socket options.
    // If this is detected, skip the test
    m_shouldSkipUnsupportedIpv6Test = ipv6GetsockoptionMissing(SOL_IPV6, IPV6_V6ONLY);
#endif
}

void tst_QWebSocketServer::cleanupTestCase()
{
}

void tst_QWebSocketServer::tst_initialisation()
{
    {
        QWebSocketServer server(QString(), QWebSocketServer::NonSecureMode);

        QVERIFY(server.serverName().isEmpty());
        QCOMPARE(server.secureMode(), QWebSocketServer::NonSecureMode);
        QVERIFY(!server.isListening());
        QCOMPARE(server.maxPendingConnections(), 30);
        QCOMPARE(server.serverPort(), quint16(0));
        QCOMPARE(server.serverAddress(), QHostAddress());
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
        QCOMPARE(server.socketDescriptor(), -1);
#else // ### Qt 6: Remove leftovers
        QCOMPARE(server.nativeDescriptor(), -1);
#endif // (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
        QVERIFY(!server.hasPendingConnections());
        QVERIFY(!server.nextPendingConnection());
        QCOMPARE(server.error(), QWebSocketProtocol::CloseCodeNormal);
        QVERIFY(server.errorString().isEmpty());
    #ifndef QT_NO_NETWORKPROXY
        QCOMPARE(server.proxy().type(), QNetworkProxy::DefaultProxy);
    #endif
    #ifndef QT_NO_SSL
        QCOMPARE(server.sslConfiguration(), QSslConfiguration::defaultConfiguration());
    #endif
        QCOMPARE(server.supportedVersions().count(), 1);
        QCOMPARE(server.supportedVersions().at(0), QWebSocketProtocol::VersionLatest);
        QCOMPARE(server.supportedVersions().at(0), QWebSocketProtocol::Version13);

        server.close();
        //closing a server should not affect any of the parameters
        //certainly if the server was not opened before

        QVERIFY(server.serverName().isEmpty());
        QCOMPARE(server.secureMode(), QWebSocketServer::NonSecureMode);
        QVERIFY(!server.isListening());
        QCOMPARE(server.maxPendingConnections(), 30);
        QCOMPARE(server.serverPort(), quint16(0));
        QCOMPARE(server.serverAddress(), QHostAddress());
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
        QCOMPARE(server.socketDescriptor(), -1);
#else // ### Qt 6: Remove leftovers
        QCOMPARE(server.nativeDescriptor(), -1);
#endif // (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
        QVERIFY(!server.hasPendingConnections());
        QVERIFY(!server.nextPendingConnection());
        QCOMPARE(server.error(), QWebSocketProtocol::CloseCodeNormal);
        QVERIFY(server.errorString().isEmpty());
    #ifndef QT_NO_NETWORKPROXY
        QCOMPARE(server.proxy().type(), QNetworkProxy::DefaultProxy);
    #endif
    #ifndef QT_NO_SSL
        QCOMPARE(server.sslConfiguration(), QSslConfiguration::defaultConfiguration());
    #endif
        QCOMPARE(server.supportedVersions().count(), 1);
        QCOMPARE(server.supportedVersions().at(0), QWebSocketProtocol::VersionLatest);
        QCOMPARE(server.supportedVersions().at(0), QWebSocketProtocol::Version13);
        QCOMPARE(server.serverUrl(), QUrl());
    }

    {
#ifndef QT_NO_SSL
        QWebSocketServer sslServer(QString(), QWebSocketServer::SecureMode);
        QCOMPARE(sslServer.secureMode(), QWebSocketServer::SecureMode);
#endif
    }
}

void tst_QWebSocketServer::tst_settersAndGetters()
{
    QWebSocketServer server(QString(), QWebSocketServer::NonSecureMode);

    server.setMaxPendingConnections(23);
    QCOMPARE(server.maxPendingConnections(), 23);
    server.setMaxPendingConnections(INT_MIN);
    QCOMPARE(server.maxPendingConnections(), INT_MIN);
    server.setMaxPendingConnections(INT_MAX);
    QCOMPARE(server.maxPendingConnections(), INT_MAX);

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    QVERIFY(!server.setSocketDescriptor(-2));
    QCOMPARE(server.socketDescriptor(), -1);
#else // ### Qt 6: Remove leftovers
    QVERIFY(!server.setNativeDescriptor(-2));
    QCOMPARE(server.nativeDescriptor(), -1);
#endif // (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))

    server.setServerName(QStringLiteral("Qt WebSocketServer"));
    QCOMPARE(server.serverName(), QStringLiteral("Qt WebSocketServer"));

#ifndef QT_NO_NETWORKPROXY
    QNetworkProxy proxy(QNetworkProxy::Socks5Proxy);
    server.setProxy(proxy);
    QCOMPARE(server.proxy(), proxy);
#endif
#ifndef QT_NO_SSL
    //cannot set an ssl configuration on a non secure server
    QSslConfiguration sslConfiguration = QSslConfiguration::defaultConfiguration();
    sslConfiguration.setPeerVerifyDepth(sslConfiguration.peerVerifyDepth() + 1);
    server.setSslConfiguration(sslConfiguration);
    QVERIFY(server.sslConfiguration() != sslConfiguration);
    QCOMPARE(server.sslConfiguration(), QSslConfiguration::defaultConfiguration());

    QWebSocketServer sslServer(QString(), QWebSocketServer::SecureMode);
    sslServer.setSslConfiguration(sslConfiguration);
    QCOMPARE(sslServer.sslConfiguration(), sslConfiguration);
    QVERIFY(sslServer.sslConfiguration() != QSslConfiguration::defaultConfiguration());
#endif
}

void tst_QWebSocketServer::tst_listening()
{
    //These listening tests are not too extensive, as the implementation of QWebSocketServer
    //relies on QTcpServer

    QWebSocketServer server(QString(), QWebSocketServer::NonSecureMode);

    QSignalSpy serverAcceptErrorSpy(&server, SIGNAL(acceptError(QAbstractSocket::SocketError)));
    QSignalSpy serverConnectionSpy(&server, SIGNAL(newConnection()));
    QSignalSpy serverErrorSpy(&server,
                              SIGNAL(serverError(QWebSocketProtocol::CloseCode)));
    QSignalSpy corsAuthenticationSpy(&server,
                              SIGNAL(originAuthenticationRequired(QWebSocketCorsAuthenticator*)));
    QSignalSpy serverClosedSpy(&server, SIGNAL(closed()));
#ifndef QT_NO_SSL
    QSignalSpy peerVerifyErrorSpy(&server, SIGNAL(peerVerifyError(QSslError)));
    QSignalSpy sslErrorsSpy(&server, SIGNAL(sslErrors(QList<QSslError>)));
#endif

    QVERIFY(server.listen());   //listen on all network interface, choose an appropriate port
    QVERIFY(server.isListening());
    QCOMPARE(serverClosedSpy.count(), 0);
    server.close();
    QTRY_COMPARE(serverClosedSpy.count(), 1);
    QVERIFY(!server.isListening());
    QCOMPARE(serverErrorSpy.count(), 0);

    QVERIFY(!server.listen(QHostAddress(QStringLiteral("1.2.3.4")), 0));
    QCOMPARE(server.error(), QWebSocketProtocol::CloseCodeAbnormalDisconnection);
    QCOMPARE(server.errorString().toLatin1().constData(), "The address is not available");
    QVERIFY(!server.isListening());

    QCOMPARE(serverAcceptErrorSpy.count(), 0);
    QCOMPARE(serverConnectionSpy.count(), 0);
    QCOMPARE(corsAuthenticationSpy.count(), 0);
#ifndef QT_NO_SSL
    QCOMPARE(peerVerifyErrorSpy.count(), 0);
    QCOMPARE(sslErrorsSpy.count(), 0);
#endif
    QCOMPARE(serverErrorSpy.count(), 1);
    QCOMPARE(serverClosedSpy.count(), 1);
}

void tst_QWebSocketServer::tst_connectivity()
{
    if (m_shouldSkipUnsupportedIpv6Test)
        QSKIP("Syscalls needed for ipv6 sockoptions missing functionality");

    QWebSocketServer server(QString(), QWebSocketServer::NonSecureMode);
    QSignalSpy serverConnectionSpy(&server, SIGNAL(newConnection()));
    QSignalSpy serverErrorSpy(&server,
                              SIGNAL(serverError(QWebSocketProtocol::CloseCode)));
    QSignalSpy corsAuthenticationSpy(&server,
                              SIGNAL(originAuthenticationRequired(QWebSocketCorsAuthenticator*)));
    QSignalSpy serverClosedSpy(&server, SIGNAL(closed()));
#ifndef QT_NO_SSL
    QSignalSpy peerVerifyErrorSpy(&server, SIGNAL(peerVerifyError(QSslError)));
    QSignalSpy sslErrorsSpy(&server, SIGNAL(sslErrors(QList<QSslError>)));
#endif
    QWebSocket socket;
    QSignalSpy socketConnectedSpy(&socket, SIGNAL(connected()));

    QVERIFY(server.listen());
    QCOMPARE(server.serverAddress(), QHostAddress(QHostAddress::Any));
    QCOMPARE(server.serverUrl(), QUrl(QStringLiteral("ws://") + QHostAddress(QHostAddress::LocalHost).toString() +
                                 QStringLiteral(":").append(QString::number(server.serverPort()))));

    socket.open(server.serverUrl().toString());

    QTRY_COMPARE(socketConnectedSpy.count(), 1);
    QCOMPARE(socket.state(), QAbstractSocket::ConnectedState);
    QCOMPARE(serverConnectionSpy.count(), 1);
    QCOMPARE(corsAuthenticationSpy.count(), 1);

    QCOMPARE(serverClosedSpy.count(), 0);

    server.close();

    QTRY_COMPARE(serverClosedSpy.count(), 1);
#ifndef QT_NO_SSL
    QCOMPARE(peerVerifyErrorSpy.count(), 0);
    QCOMPARE(sslErrorsSpy.count(), 0);
#endif
    QCOMPARE(serverErrorSpy.count(), 0);
}

void tst_QWebSocketServer::tst_preSharedKey()
{
    if (m_shouldSkipUnsupportedIpv6Test)
        QSKIP("Syscalls needed for ipv6 sockoptions missing functionality");

#ifndef QT_NO_OPENSSL
    QWebSocketServer server(QString(), QWebSocketServer::SecureMode);

    bool cipherFound = false;
    const QList<QSslCipher> supportedCiphers = QSslSocket::supportedCiphers();
    for (const QSslCipher &cipher : supportedCiphers) {
        if (cipher.name() == PSK_CIPHER_WITHOUT_AUTH) {
            cipherFound = true;
            break;
        }
    }

    if (!cipherFound)
        QSKIP("SSL implementation does not support the necessary cipher");

    QSslCipher cipher(PSK_CIPHER_WITHOUT_AUTH);
    QList<QSslCipher> list;
    list << cipher;

    QSslConfiguration config = QSslConfiguration::defaultConfiguration();
    if (QSslSocket::sslLibraryVersionNumber() >= 0x10101000L)
        config.setProtocol(QSsl::TlsV1_2); // With TLS 1.3 there are some issues with PSK, force 1.2
    config.setCiphers(list);
    config.setPeerVerifyMode(QSslSocket::VerifyNone);
    config.setPreSharedKeyIdentityHint(PSK_SERVER_IDENTITY_HINT);
    server.setSslConfiguration(config);

    PskProvider providerServer;
    providerServer.m_server = true;
    providerServer.m_identity = PSK_CLIENT_IDENTITY;
    providerServer.m_psk = PSK_CLIENT_PRESHAREDKEY;
    connect(&server, &QWebSocketServer::preSharedKeyAuthenticationRequired, &providerServer, &PskProvider::providePsk);

    QSignalSpy serverPskRequiredSpy(&server, &QWebSocketServer::preSharedKeyAuthenticationRequired);
    QSignalSpy serverConnectionSpy(&server, &QWebSocketServer::newConnection);
    QSignalSpy serverErrorSpy(&server,
                              SIGNAL(serverError(QWebSocketProtocol::CloseCode)));
    QSignalSpy serverClosedSpy(&server, &QWebSocketServer::closed);
    QSignalSpy sslErrorsSpy(&server, SIGNAL(sslErrors(QList<QSslError>)));

    QWebSocket socket;
    QSslConfiguration socketConfig = QSslConfiguration::defaultConfiguration();
    if (QSslSocket::sslLibraryVersionNumber() >= 0x10101000L)
        socketConfig.setProtocol(QSsl::TlsV1_2); // With TLS 1.3 there are some issues with PSK, force 1.2
    socketConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    socketConfig.setCiphers(list);
    socket.setSslConfiguration(socketConfig);

    PskProvider providerClient;
    providerClient.m_identity = PSK_CLIENT_IDENTITY;
    providerClient.m_psk = PSK_CLIENT_PRESHAREDKEY;
    connect(&socket, &QWebSocket::preSharedKeyAuthenticationRequired, &providerClient, &PskProvider::providePsk);
    QSignalSpy socketPskRequiredSpy(&socket, &QWebSocket::preSharedKeyAuthenticationRequired);
    QSignalSpy socketConnectedSpy(&socket, &QWebSocket::connected);

    QVERIFY(server.listen());
    QCOMPARE(server.serverAddress(), QHostAddress(QHostAddress::Any));
    QCOMPARE(server.serverUrl(), QUrl(QString::asprintf("wss://%ls:%d",
                                 qUtf16Printable(QHostAddress(QHostAddress::LocalHost).toString()), server.serverPort())));

    socket.open(server.serverUrl().toString());

    QTRY_COMPARE(socketConnectedSpy.count(), 1);
    QCOMPARE(socket.state(), QAbstractSocket::ConnectedState);
    QCOMPARE(serverConnectionSpy.count(), 1);
    QCOMPARE(serverPskRequiredSpy.count(), 1);
    QCOMPARE(socketPskRequiredSpy.count(), 1);

    QCOMPARE(serverClosedSpy.count(), 0);

    server.close();

    QTRY_COMPARE(serverClosedSpy.count(), 1);
    QCOMPARE(sslErrorsSpy.count(), 0);
    QCOMPARE(serverErrorSpy.count(), 0);
#endif
}

void tst_QWebSocketServer::tst_maxPendingConnections()
{
    if (m_shouldSkipUnsupportedIpv6Test)
        QSKIP("Syscalls needed for ipv6 sockoptions missing functionality");

    //tests if maximum connections are respected
    //also checks if there are no side-effects like signals that are unexpectedly thrown
    QWebSocketServer server(QString(), QWebSocketServer::NonSecureMode);
    server.setMaxPendingConnections(2);
    QSignalSpy serverConnectionSpy(&server, SIGNAL(newConnection()));
    QSignalSpy serverErrorSpy(&server,
                              SIGNAL(serverError(QWebSocketProtocol::CloseCode)));
    QSignalSpy corsAuthenticationSpy(&server,
                              SIGNAL(originAuthenticationRequired(QWebSocketCorsAuthenticator*)));
    QSignalSpy serverClosedSpy(&server, SIGNAL(closed()));
#ifndef QT_NO_SSL
    QSignalSpy peerVerifyErrorSpy(&server, SIGNAL(peerVerifyError(QSslError)));
    QSignalSpy sslErrorsSpy(&server, SIGNAL(sslErrors(QList<QSslError>)));
#endif
    QSignalSpy serverAcceptErrorSpy(&server, SIGNAL(acceptError(QAbstractSocket::SocketError)));

    QWebSocket socket1;
    QWebSocket socket2;
    QWebSocket socket3;

    QSignalSpy socket1ConnectedSpy(&socket1, SIGNAL(connected()));
    QSignalSpy socket2ConnectedSpy(&socket2, SIGNAL(connected()));
    QSignalSpy socket3ConnectedSpy(&socket3, SIGNAL(connected()));

    QVERIFY(server.listen());

    socket1.open(server.serverUrl().toString());

    QTRY_COMPARE(socket1ConnectedSpy.count(), 1);
    QCOMPARE(socket1.state(), QAbstractSocket::ConnectedState);
    QCOMPARE(serverConnectionSpy.count(), 1);
    QCOMPARE(corsAuthenticationSpy.count(), 1);
    socket2.open(server.serverUrl().toString());
    QTRY_COMPARE(socket2ConnectedSpy.count(), 1);
    QCOMPARE(socket2.state(), QAbstractSocket::ConnectedState);
    QCOMPARE(serverConnectionSpy.count(), 2);
    QCOMPARE(corsAuthenticationSpy.count(), 2);
    socket3.open(server.serverUrl().toString());
    QVERIFY(!socket3ConnectedSpy.wait(250));
    QCOMPARE(socket3ConnectedSpy.count(), 0);
    QCOMPARE(socket3.state(), QAbstractSocket::UnconnectedState);
    QCOMPARE(serverConnectionSpy.count(), 2);
    QCOMPARE(corsAuthenticationSpy.count(), 2);

    QVERIFY(server.hasPendingConnections());
    QWebSocket *pSocket = server.nextPendingConnection();
    QVERIFY(pSocket);
    delete pSocket;
    QVERIFY(server.hasPendingConnections());
    pSocket = server.nextPendingConnection();
    QVERIFY(pSocket);
    delete pSocket;
    QVERIFY(!server.hasPendingConnections());
    QVERIFY(!server.nextPendingConnection());

//will resolve in another commit
#ifndef Q_OS_WIN
    QCOMPARE(serverErrorSpy.count(), 1);
    QCOMPARE(serverErrorSpy.at(0).at(0).value<QWebSocketProtocol::CloseCode>(),
             QWebSocketProtocol::CloseCodeAbnormalDisconnection);
#endif
    QCOMPARE(serverClosedSpy.count(), 0);

    server.close();

    QTRY_COMPARE(serverClosedSpy.count(), 1);
#ifndef QT_NO_SSL
    QCOMPARE(peerVerifyErrorSpy.count(), 0);
    QCOMPARE(sslErrorsSpy.count(), 0);
#endif
    QCOMPARE(serverAcceptErrorSpy.count(), 0);
}

void tst_QWebSocketServer::tst_serverDestroyedWhileSocketConnected()
{
    if (m_shouldSkipUnsupportedIpv6Test)
        QSKIP("Syscalls needed for ipv6 sockoptions missing functionality");

    QWebSocketServer * server = new QWebSocketServer(QString(), QWebSocketServer::NonSecureMode);
    QSignalSpy serverConnectionSpy(server, SIGNAL(newConnection()));
    QSignalSpy corsAuthenticationSpy(server,
                              SIGNAL(originAuthenticationRequired(QWebSocketCorsAuthenticator*)));
    QSignalSpy serverClosedSpy(server, SIGNAL(closed()));

    QWebSocket socket;
    QSignalSpy socketConnectedSpy(&socket, SIGNAL(connected()));
    QSignalSpy socketDisconnectedSpy(&socket, SIGNAL(disconnected()));

    QVERIFY(server->listen());
    QCOMPARE(server->serverAddress(), QHostAddress(QHostAddress::Any));
    QCOMPARE(server->serverUrl(), QUrl(QStringLiteral("ws://") + QHostAddress(QHostAddress::LocalHost).toString() +
                                  QStringLiteral(":").append(QString::number(server->serverPort()))));

    socket.open(server->serverUrl().toString());

    QTRY_COMPARE(socketConnectedSpy.count(), 1);
    QCOMPARE(socket.state(), QAbstractSocket::ConnectedState);
    QCOMPARE(serverConnectionSpy.count(), 1);
    QCOMPARE(corsAuthenticationSpy.count(), 1);

    QCOMPARE(serverClosedSpy.count(), 0);

    delete server;

    QTRY_COMPARE(socketDisconnectedSpy.count(), 1);
}

void tst_QWebSocketServer::tst_scheme()
{
    if (m_shouldSkipUnsupportedIpv6Test)
        QSKIP("Syscalls needed for ipv6 sockoptions missing functionality");

    QWebSocketServer plainServer(QString(), QWebSocketServer::NonSecureMode);
    QSignalSpy plainServerConnectionSpy(&plainServer, SIGNAL(newConnection()));

    QVERIFY(plainServer.listen());

    QWebSocket plainSocket;
    plainSocket.open(plainServer.serverUrl().toString());

    QTRY_COMPARE(plainServerConnectionSpy.count(), 1);
    QScopedPointer<QWebSocket> plainServerSocket(plainServer.nextPendingConnection());
    QVERIFY(!plainServerSocket.isNull());
    QCOMPARE(plainServerSocket->requestUrl().scheme(), QStringLiteral("ws"));
    plainServer.close();

#ifndef QT_NO_SSL
    QWebSocketServer secureServer(QString(), QWebSocketServer::SecureMode);
    QSslConfiguration sslConfiguration;
    QFile certFile(QStringLiteral(":/localhost.cert"));
    QFile keyFile(QStringLiteral(":/localhost.key"));
    QVERIFY(certFile.open(QIODevice::ReadOnly));
    QVERIFY(keyFile.open(QIODevice::ReadOnly));
    QSslCertificate certificate(&certFile, QSsl::Pem);
    QSslKey sslKey(&keyFile, QSsl::Rsa, QSsl::Pem);
    certFile.close();
    keyFile.close();
    sslConfiguration.setPeerVerifyMode(QSslSocket::VerifyNone);
    sslConfiguration.setLocalCertificate(certificate);
    sslConfiguration.setPrivateKey(sslKey);
    sslConfiguration.setProtocol(QSsl::TlsV1SslV3);
    secureServer.setSslConfiguration(sslConfiguration);
    QSignalSpy secureServerConnectionSpy(&secureServer, SIGNAL(newConnection()));

    QVERIFY(secureServer.listen());

    QSslCipher sessionCipher;
    QWebSocket secureSocket;
    connect(&secureSocket, &QWebSocket::sslErrors,
            &secureSocket, [&] {
                secureSocket.ignoreSslErrors();
                sessionCipher = secureSocket.sslConfiguration().sessionCipher();
            });
    secureSocket.open(secureServer.serverUrl().toString());

    QTRY_COMPARE(secureServerConnectionSpy.count(), 1);
    QScopedPointer<QWebSocket> secureServerSocket(secureServer.nextPendingConnection());
    QVERIFY(!secureServerSocket.isNull());
    QCOMPARE(secureServerSocket->requestUrl().scheme(), QStringLiteral("wss"));
    secureServer.close();
    QVERIFY(!sessionCipher.isNull());
#endif
}

void tst_QWebSocketServer::tst_handleConnection()
{
    QWebSocketServer wsServer(QString(), QWebSocketServer::NonSecureMode);
    QSignalSpy wsServerConnectionSpy(&wsServer, &QWebSocketServer::newConnection);

    QTcpServer tcpServer;
    connect(&tcpServer, &QTcpServer::newConnection,
            [&tcpServer, &wsServer]() {
        wsServer.handleConnection(tcpServer.nextPendingConnection());
    });
    QVERIFY(tcpServer.listen());

    QWebSocket webSocket;
    QSignalSpy wsConnectedSpy(&webSocket, &QWebSocket::connected);
    webSocket.open(QStringLiteral("ws://localhost:%1").arg(tcpServer.serverPort()));
    QTRY_COMPARE(wsConnectedSpy.count(), 1);

    QTRY_COMPARE(wsServerConnectionSpy.count(), 1);

    QScopedPointer<QWebSocket> webServerSocket(wsServer.nextPendingConnection());
    QVERIFY(!webServerSocket.isNull());

    QSignalSpy wsMessageReceivedSpy(webServerSocket.data(), &QWebSocket::textMessageReceived);
    webSocket.sendTextMessage("dummy");

    QTRY_COMPARE(wsMessageReceivedSpy.count(), 1);
    QList<QVariant> arguments = wsMessageReceivedSpy.takeFirst();
    QCOMPARE(arguments.first().toString(), QString("dummy"));

    QSignalSpy clientMessageReceivedSpy(&webSocket, &QWebSocket::textMessageReceived);
    webServerSocket->sendTextMessage("hello");
    QVERIFY(webServerSocket->bytesToWrite() > 5); // 5 + a few extra bytes for header
    QTRY_COMPARE(webServerSocket->bytesToWrite(), 0);
    QTRY_COMPARE(clientMessageReceivedSpy.count(), 1);
    arguments = clientMessageReceivedSpy.takeFirst();
    QCOMPARE(arguments.first().toString(), QString("hello"));
}

QTEST_MAIN(tst_QWebSocketServer)

#include "tst_qwebsocketserver.moc"
