// Copyright (C) 2023 The Qt Company Ltd.
// Copyright (C) 2019 Alexey Edelev <semlanik@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "simplechatengine.h"

#include <QGrpcHttp2Channel>

#include <QDebug>
#include <QFile>
#include <QCryptographicHash>
#include <QDateTime>
#include <QClipboard>
#include <QGuiApplication>
#include <QMimeData>
#include <QImage>
#include <QByteArray>
#include <QBuffer>

SimpleChatEngine::SimpleChatEngine(QObject *parent)
    : QObject(parent),
      m_state(Disconnected),
      m_client(new qtgrpc::examples::chat::SimpleChat::Client),
      m_clipBoard(QGuiApplication::clipboard())
{
    if (m_clipBoard) {
        QObject::connect(m_clipBoard, &QClipboard::dataChanged, this,
                         &SimpleChatEngine::clipBoardContentTypeChanged);
    }
}

SimpleChatEngine::~SimpleChatEngine()
{
    delete m_client;
}

void SimpleChatEngine::login(const QString &name, const QString &password)
{
    if (m_state != Disconnected)
        return;

    setState(Connecting);
    QUrl url("http://localhost:65002");

    // ![0]
    QGrpcChannelOptions channelOptions(url);
    QGrpcMetadata metadata = {
        { "user-name", { name.toUtf8() } },
        { "user-password", { password.toUtf8() } },
    };
    channelOptions.withMetadata(metadata);
    std::shared_ptr<QAbstractGrpcChannel> channel = std::make_shared<QGrpcHttp2Channel>(
            channelOptions);
    // ![0]

    m_client->attachChannel(channel);

    // ![1]
    auto stream = m_client->streamMessageList(qtgrpc::examples::chat::None());
    QObject::connect(stream.get(), &QGrpcServerStream::errorOccurred, this,
                     [this, stream](const QGrpcStatus &status) {
                         qCritical()
                                 << "Stream error(" << status.code() << "):" << status.message();
                         if (status.code() == QGrpcStatus::Unauthenticated) {
                             emit authFailed();
                         } else {
                             emit networkError(status.message());
                             setState(Disconnected);
                         }
                     });

    QObject::connect(stream.get(), &QGrpcServerStream::finished, this,
                     [this, stream]() { setState(Disconnected); });

    QObject::connect(stream.get(), &QGrpcServerStream::messageReceived, this,
                     [this, name, password, stream]() {
                         if (m_userName != name) {
                             m_userName = name;
                             m_password = password;
                             emit userNameChanged();
                         }
                         setState(Connected);
                         m_messages.append(
                                 stream->read<qtgrpc::examples::chat::ChatMessages>().messages());
                     });
    // ![1]
}

void SimpleChatEngine::sendMessage(const QString &content)
{
    // ![2]
    qtgrpc::examples::chat::ChatMessage msg;
    msg.setContent(content.toUtf8());
    msg.setType(qtgrpc::examples::chat::ChatMessage::ContentType::Text);
    msg.setTimestamp(QDateTime::currentMSecsSinceEpoch());
    msg.setFrom(m_userName);
    m_client->sendMessage(msg);
    // ![2]
}

SimpleChatEngine::ContentType SimpleChatEngine::clipBoardContentType() const
{
    if (m_clipBoard != nullptr) {
        const QMimeData *mime = m_clipBoard->mimeData();
        if (mime != nullptr) {
            if (mime->hasImage() || mime->hasUrls())
                return SimpleChatEngine::ContentType::Image;
            else if (mime->hasText())
                return SimpleChatEngine::ContentType::Text;
        }
    }
    return SimpleChatEngine::ContentType::Unknown;
}

void SimpleChatEngine::sendImageFromClipboard()
{
    if (m_clipBoard == nullptr)
        return;

    QByteArray imgData;
    const QMimeData *mime = m_clipBoard->mimeData();
    if (mime != nullptr) {
        if (mime->hasImage()) {
            QImage img = mime->imageData().value<QImage>();
            img = img.scaled(300, 300, Qt::KeepAspectRatio);
            QBuffer buffer(&imgData);
            buffer.open(QIODevice::WriteOnly);
            img.save(&buffer, "PNG");
            buffer.close();
        } else if (mime->hasUrls()) {
            QUrl imgUrl = mime->urls().at(0);
            if (!imgUrl.isLocalFile()) {
                qWarning() << "Only supports transfer of local images";
                return;
            }
            QImage img(imgUrl.toLocalFile());
            if (img.isNull()) {
                qWarning() << "Invalid image format";
                return;
            }

            QBuffer buffer(&imgData);
            buffer.open(QIODevice::WriteOnly);
            img.save(&buffer, "PNG");
            buffer.close();
        }
    }

    if (imgData.isEmpty())
        return;

    qtgrpc::examples::chat::ChatMessage msg;
    msg.setContent(imgData);
    msg.setType(qtgrpc::examples::chat::ChatMessage::ContentType::Image);
    msg.setTimestamp(QDateTime::currentMSecsSinceEpoch());
    msg.setFrom(m_userName);
    m_client->sendMessage(msg);
}

ChatMessageModel *SimpleChatEngine::messages()
{
    return &m_messages;
}

QString SimpleChatEngine::userName() const
{
    return m_userName;
}

SimpleChatEngine::State SimpleChatEngine::state() const
{
    return m_state;
}

void SimpleChatEngine::setState(SimpleChatEngine::State state)
{
    if (m_state != state) {
        m_state = state;
        emit stateChanged();
    }
}
