// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the Network Access API.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#ifndef QOAUTH2AUTHORIZATIONCODEFLOW_P_H
#define QOAUTH2AUTHORIZATIONCODEFLOW_P_H

#ifndef QT_NO_HTTP

#include <private/qabstractoauth2_p.h>

#include <QtNetworkAuth/qoauthglobal.h>
#include <QtNetworkAuth/qoauth2authorizationcodeflow.h>

#include <QtCore/qstring.h>
#include <QtCore/qdatetime.h>

QT_BEGIN_NAMESPACE

class QOAuth2AuthorizationCodeFlowPrivate : public QAbstractOAuth2Private
{
public:
    Q_DECLARE_PUBLIC(QOAuth2AuthorizationCodeFlow)

    QOAuth2AuthorizationCodeFlowPrivate(const QUrl &authorizationUrl,
                                        const QUrl &accessTokenUrl,
                                        const QString &clientIdentifier,
                                        QNetworkAccessManager *manager = nullptr);

    void _q_handleCallback(const QVariantMap &data);
    void _q_accessTokenRequestFinished(const QVariantMap &values);
    void _q_accessTokenRequestFailed(QAbstractOAuth::Error error, const QString &errorString = {});
    void _q_authenticate(QNetworkReply *reply, QAuthenticator *authenticator);

    QUrl accessTokenUrl;
    QString tokenType;
    QPointer<QNetworkReply> currentReply;
};

QT_END_NAMESPACE

#endif // QT_NO_HTTP

#endif // QOAUTH2AUTHORIZATIONCODEFLOW_P_H
