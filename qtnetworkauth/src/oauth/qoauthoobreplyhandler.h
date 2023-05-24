// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QOAUTHOOBREPLYHANDLER_H
#define QOAUTHOOBREPLYHANDLER_H

#ifndef QT_NO_HTTP

#include <QtNetworkAuth/qoauthglobal.h>
#include <QtNetworkAuth/qabstractoauthreplyhandler.h>

QT_BEGIN_NAMESPACE

class Q_OAUTH_EXPORT QOAuthOobReplyHandler : public QAbstractOAuthReplyHandler
{
    Q_OBJECT

public:
    explicit QOAuthOobReplyHandler(QObject *parent = nullptr);

    QString callback() const override;

protected:
    void networkReplyFinished(QNetworkReply *reply) override;

private:
    QVariantMap parseResponse(const QByteArray &response);
};

QT_END_NAMESPACE

#endif // QT_NO_HTTP

#endif // QOAUTHOOBREPLYHANDLER_H
