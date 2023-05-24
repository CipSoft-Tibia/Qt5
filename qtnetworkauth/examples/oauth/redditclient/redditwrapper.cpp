// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "redditwrapper.h"

#include <QtNetworkAuth/qoauth2authorizationcodeflow.h>
#include <QtNetworkAuth/qoauthhttpserverreplyhandler.h>

#include <QtGui/qdesktopservices.h>

using namespace Qt::StringLiterals;

static constexpr auto hotUrl("https://oauth.reddit.com/hot"_L1);
static constexpr auto authorizationUrl("https://www.reddit.com/api/v1/authorize"_L1);
static constexpr auto accessTokenUrl("https://www.reddit.com/api/v1/access_token"_L1);
static constexpr auto scope("identity read"_L1);

RedditWrapper::RedditWrapper(QObject *parent) : QObject(parent)
{
    auto replyHandler = new QOAuthHttpServerReplyHandler(1337, this);
    oauth2.setReplyHandler(replyHandler);
    oauth2.setAuthorizationUrl(QUrl(authorizationUrl));
    oauth2.setAccessTokenUrl(QUrl(accessTokenUrl));
    oauth2.setScope(scope);

    connect(&oauth2, &QOAuth2AuthorizationCodeFlow::granted, this, &RedditWrapper::authenticated);
    connect(&oauth2, &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser, this,
            &QDesktopServices::openUrl);
}

RedditWrapper::RedditWrapper(const QString &clientIdentifier, QObject *parent) :
    RedditWrapper(parent)
{
    oauth2.setClientIdentifier(clientIdentifier);
}

QNetworkReply *RedditWrapper::requestHotThreads()
{
    qDebug() << "Getting hot threads...";
    return oauth2.get(QUrl(hotUrl));
}

void RedditWrapper::grant()
{
    oauth2.grant();
}
