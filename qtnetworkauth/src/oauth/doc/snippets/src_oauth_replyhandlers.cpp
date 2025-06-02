// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtNetworkAuth/qoauth2authorizationcodeflow.h>
#include <QtNetworkAuth/qoauthhttpserverreplyhandler.h>
#include <QtNetworkAuth/qoauthurischemereplyhandler.h>

#include <QtNetwork/qnetworkrequestfactory.h>

#include <QtGui/qdesktopservices.h>
#include <QtGui/qguiapplication.h>

#include <QtCore/qobject.h>
#include <QtCore/qurl.h>

using namespace Qt::StringLiterals;

class HttpServerExample : public QObject
{
    Q_OBJECT
public:

    void setup()
    {
        //! [httpserver-oauth-setup]
        m_oauth.setAuthorizationUrl(QUrl("https://some.authorization.service/v3/authorize"_L1));
        m_oauth.setAccessTokenUrl(QUrl("https://some.authorization.service/v3/access_token"_L1));
        m_oauth.setClientIdentifier("a_client_id"_L1);
        m_oauth.setScope("read"_L1);

        m_handler = new QOAuthHttpServerReplyHandler(1234, this);

        connect(&m_oauth, &QAbstractOAuth::authorizeWithBrowser, this, &QDesktopServices::openUrl);
        connect(&m_oauth, &QAbstractOAuth::granted, this, [this]() {
            // Here we use QNetworkRequestFactory to store the access token
            m_api.setBearerToken(m_oauth.token().toLatin1());
            m_handler->close();
        });
        //! [httpserver-oauth-setup]

        //! [httpserver-handler-setup]
        m_oauth.setReplyHandler(m_handler);

        // Initiate the authorization
        if (m_handler->isListening()) {
            m_oauth.grant();
        }
        //! [httpserver-handler-setup]
    }

private:
    //! [httpserver-variables]
    QOAuth2AuthorizationCodeFlow m_oauth;
    QOAuthHttpServerReplyHandler *m_handler = nullptr;
    //! [httpserver-variables]
    QNetworkRequestFactory m_api;
};

class UriSchemeExample : public QObject
{
    Q_OBJECT
public:

    void setup()
    {
        //! [uri-oauth-setup]
        m_oauth.setAuthorizationUrl(QUrl("https://some.authorization.service/v3/authorize"_L1));
        m_oauth.setAccessTokenUrl(QUrl("https://some.authorization.service/v3/access_token"_L1));
        m_oauth.setClientIdentifier("a_client_id"_L1);
        m_oauth.setScope("read"_L1);

        connect(&m_oauth, &QAbstractOAuth::authorizeWithBrowser, this, &QDesktopServices::openUrl);
        connect(&m_oauth, &QAbstractOAuth::granted, this, [this]() {
            // Here we use QNetworkRequestFactory to store the access token
            m_api.setBearerToken(m_oauth.token().toLatin1());
            m_handler.close();
        });
        //! [uri-oauth-setup]

        //! [uri-handler-setup]
        m_handler.setRedirectUrl(QUrl{"com.my.app:/oauth2redirect"_L1});
        m_oauth.setReplyHandler(&m_handler);

        // Initiate the authorization
        if (m_handler.listen()) {
            m_oauth.grant();
        }
        //! [uri-handler-setup]
    }

private:

    //! [uri-variables]
    QOAuth2AuthorizationCodeFlow m_oauth;
    QOAuthUriSchemeReplyHandler m_handler;
    //! [uri-variables]
    QNetworkRequestFactory m_api;
};

int main(int argc, char *argv[])
{
    QGuiApplication a(argc, argv);

    HttpServerExample httpServer;
    httpServer.setup();

    UriSchemeExample uriScheme;
    uriScheme.setup();

    return a.exec();
}

#include "src_oauth_replyhandlers.moc"
