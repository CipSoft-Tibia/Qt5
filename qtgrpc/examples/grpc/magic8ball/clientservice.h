// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef CLIENT_SERVICE_H
#define CLIENT_SERVICE_H

#include <QObject>
#include <QString>
#include <qqmlregistration.h>

#include <memory>

#include "exampleservice.qpb.h"
#include "exampleservice_client.grpc.qpb.h"

class ClientService : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    explicit ClientService(QObject *parent = nullptr);
    Q_INVOKABLE void sendRequest();
    Q_INVOKABLE void setMessage();
    void errorOccurred();

signals:
    void messageRecieved(const QString &value);
    void errorRecieved(const QString &value);

private:
    std::unique_ptr<qtgrpc::examples::ExampleService::Client> m_client;
    std::unique_ptr<qtgrpc::examples::AnswerResponse> m_response;
};

#endif // CLIENT_SERVICE_H
