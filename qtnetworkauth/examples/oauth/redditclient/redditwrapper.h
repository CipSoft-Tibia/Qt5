// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef REDDITWRAPPER_H
#define REDDITWRAPPER_H

#include <QtNetworkAuth/qoauth2authorizationcodeflow.h>

class RedditWrapper : public QObject
{
    Q_OBJECT

public:
    RedditWrapper(QObject *parent = nullptr);
    RedditWrapper(const QString &clientIdentifier, QObject *parent = nullptr);

    QNetworkReply *requestHotThreads();

public slots:
    void grant();

signals:
    void authenticated();

private:
    QOAuth2AuthorizationCodeFlow oauth2;
};

#endif // REDDITWRAPPER_H
