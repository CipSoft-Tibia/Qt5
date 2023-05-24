// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QOAUTHHTTPSERVERREPLYHANDLER_H
#define QOAUTHHTTPSERVERREPLYHANDLER_H

#ifndef QT_NO_HTTP

#include <QtNetworkAuth/qoauthglobal.h>
#include <QtNetworkAuth/qoauthoobreplyhandler.h>

#include <QtNetwork/qhostaddress.h>

QT_BEGIN_NAMESPACE

class QUrlQuery;

class QOAuthHttpServerReplyHandlerPrivate;
class Q_OAUTH_EXPORT QOAuthHttpServerReplyHandler : public QOAuthOobReplyHandler
{
    Q_OBJECT

public:
    explicit QOAuthHttpServerReplyHandler(QObject *parent = nullptr);
    explicit QOAuthHttpServerReplyHandler(quint16 port, QObject *parent = nullptr);
    explicit QOAuthHttpServerReplyHandler(const QHostAddress &address, quint16 port,
                                          QObject *parent = nullptr);
    ~QOAuthHttpServerReplyHandler();

    QString callback() const override;

    QString callbackPath() const;
    void setCallbackPath(const QString &path);

    QString callbackText() const;
    void setCallbackText(const QString &text);

    quint16 port() const;

    bool listen(const QHostAddress &address = QHostAddress::Any, quint16 port = 0);
    void close();
    bool isListening() const;

private:
    Q_DECLARE_PRIVATE(QOAuthHttpServerReplyHandler)
    QScopedPointer<QOAuthHttpServerReplyHandlerPrivate> d_ptr;
};

QT_END_NAMESPACE

#endif // QT_NO_HTTP

#endif // QOAUTHHTTPSERVERREPLYHANDLER_H
