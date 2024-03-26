// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "grpcclienttestbase.h"

#include <QtGrpc/QGrpcChannelOptions>
#include <QtNetwork/qtnetwork-config.h>
#include <message_latency_defs.h>

void GrpcClientTestBase::initTestCase_data()
{
    QTest::addColumn<GrpcClientTestBase::Channels>("type");
    QTest::addColumn<std::shared_ptr<QAbstractGrpcChannel>>("channel");

    if (m_channels.testFlag(Channel::Qt)) {
        QTest::newRow("Http2Client")
                << QFlags{ Channel::Qt }
                << std::shared_ptr<QAbstractGrpcChannel>(new QGrpcHttp2Channel(QGrpcChannelOptions{
                           QUrl("http://localhost:50051", QUrl::StrictMode) }));
    }

#if QT_CONFIG(native_grpc)
    if (m_channels.testFlag(Channel::Native)) {
#  ifndef Q_OS_WINDOWS
        QTest::newRow("GrpcSocket")
            << QFlags{ Channel::Native }
            << std::shared_ptr<QAbstractGrpcChannel>(
                   new QGrpcChannel(QGrpcChannelOptions{ QUrl("unix:///tmp/qtgrpc_test.sock") },
                                    QGrpcChannel::InsecureChannelCredentials));
#  endif
        QTest::newRow("GrpcHttp") << QFlags{ Channel::Native }
                                  << std::shared_ptr<QAbstractGrpcChannel>(new QGrpcChannel(
                                             QGrpcChannelOptions{ QUrl("localhost:50051") },
                                             QGrpcChannel::InsecureChannelCredentials));
    }
#endif

#if QT_CONFIG(ssl)
    if (m_channels.testFlag(Channel::Ssl)) {
        QFile caCerificateFile(":/assets/cert.pem");
        QVERIFY2(caCerificateFile.open(QFile::ReadOnly), "Unable to open ssl ca certificate file");
        QSslConfiguration sslConfig;
        QSslCertificate caCert(caCerificateFile.readAll());
        sslConfig.setProtocol(QSsl::TlsV1_2);
        sslConfig.addCaCertificate(caCert);
        sslConfig.setAllowedNextProtocols({ QByteArray("h2") });
        QTest::newRow("Http2ClientSSL")
            << QFlags{ Channel::Qt, Channel::Ssl }
            << std::shared_ptr<QAbstractGrpcChannel>(new QGrpcHttp2Channel(
                   QGrpcChannelOptions{ QUrl("https://localhost:50052", QUrl::StrictMode) }
                       .withSslConfiguration(sslConfig)));
    }

    if (m_channels.testFlag(Channel::SslNoCredentials)) {
        QSslConfiguration sslConfig;
        sslConfig.setProtocol(QSsl::TlsV1_2);
        sslConfig.setAllowedNextProtocols({ QByteArray("h2") });
        QGrpcChannelOptions channelOptions(QUrl("https://localhost:50052", QUrl::StrictMode));
        channelOptions.withSslConfiguration(sslConfig);

        QTest::newRow("Http2ClientSSLNoCredentials")
            << QFlags{ Channel::Qt, Channel::SslNoCredentials }
            << std::shared_ptr<QAbstractGrpcChannel>(new QGrpcHttp2Channel(channelOptions));
#  if QT_CONFIG(native_grpc)
        QTest::newRow("GrpcHttpSSLNoCredentials")
            << QFlags{ Channel::Native, Channel::SslNoCredentials }
            << std::shared_ptr<QAbstractGrpcChannel>(new QGrpcChannel(channelOptions,
                                                          QGrpcChannel::SslDefaultCredentials));
#  endif
    }
#endif

    if (m_channels.testFlag(Channel::WithChannelDeadline)) {
        constexpr auto
            channelTimeout = std::chrono::milliseconds(static_cast<int64_t>(MessageLatency * 0.25));
        QTest::newRow("Http2ClientDeadline")
            << QFlags{ Channel::Qt, Channel::WithChannelDeadline }
            << std::shared_ptr<
                   QAbstractGrpcChannel>(new QGrpcHttp2Channel(QGrpcChannelOptions{
                   QUrl("http://localhost:50051", QUrl::StrictMode) }
                                                                   .withDeadline(channelTimeout)));
    }
}

void GrpcClientTestBase::init()
{
    QFETCH_GLOBAL(GrpcClientTestBase::Channels, type);
    m_channelType = type;
    QFETCH_GLOBAL(std::shared_ptr<QAbstractGrpcChannel>, channel);
    m_client = std::make_shared<qtgrpc::tests::TestService::Client>();
    m_client->attachChannel(std::move(channel));

    if (m_serverProccess.state() != QProcess::ProcessState::Running) {
        qInfo() << "Restarting server";
        m_serverProccess.restart();
        QVERIFY2(m_serverProccess.state() == QProcess::ProcessState::Running,
                 "Precondition failed - Server cannot be started.");
    }
}

std::shared_ptr<qtgrpc::tests::TestService::Client> GrpcClientTestBase::client()
{
    return m_client;
}

GrpcClientTestBase::Channels GrpcClientTestBase::channelType()
{
    return m_channelType;
}
