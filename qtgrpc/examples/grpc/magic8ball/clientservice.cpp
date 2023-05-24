// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "clientservice.h"

#include <QDebug>
#include <QGrpcHttp2Channel>

using namespace qtgrpc::examples;

ClientService::ClientService(QObject *parent) :
    QObject(parent),
    m_client(new ExampleService::Client),
    m_response(new AnswerResponse)
{
    connect(m_client.get(), &ExampleService::Client::errorOccurred,
            this, &ClientService::errorOccurred);

    //! [0]
    QGrpcChannelOptions channelOptions(QUrl("http://localhost:50051", QUrl::StrictMode));
    m_client->attachChannel(std::make_shared<QGrpcHttp2Channel>(channelOptions));
    //! [0]
}

void ClientService::errorOccurred()
{
    qWarning() << "Connection error occurred. Have you started server part:"
                  " ../magic8ball/SimpleGrpcServer?";

    emit errorRecieved("No connection\nto\nserver");
}

void ClientService::sendRequest()
{
    // clean error on UI before new request
    emit errorRecieved("");

    AnswerRequest request;
    request.setMessage("sleep");

    m_client->answerMethod(request, m_response.get());
}

void ClientService::setMessage()
{
    emit messageRecieved(m_response.get()->message());
}
