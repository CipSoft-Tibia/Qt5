// Copyright (C) 2016 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Milian Wolff <milian.wolff@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef WEBSOCKETTRANSPORT_H
#define WEBSOCKETTRANSPORT_H

#include <QWebChannelAbstractTransport>

QT_BEGIN_NAMESPACE
class QWebSocket;
QT_END_NAMESPACE

class WebSocketTransport : public QWebChannelAbstractTransport
{
    Q_OBJECT
public:
    explicit WebSocketTransport(QWebSocket *socket);
    virtual ~WebSocketTransport();

    void sendMessage(const QJsonObject &message) override;

private slots:
    void textMessageReceived(const QString &message);

private:
    QWebSocket *m_socket;
};

#endif // WEBSOCKETTRANSPORT_H
