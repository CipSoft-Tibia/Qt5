// Copyright (C) 2023 The Qt Company Ltd.
// Copyright (C) 2019 Alexey Edelev <semlanik@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef SIMPLECHATENGINE_H
#define SIMPLECHATENGINE_H

#include <QObject>
#include <QQmlEngine>

#include "simplechat.qpb.h"
#include "simplechat_client.grpc.qpb.h"
#include "chatmessagemodel.h"

QT_BEGIN_NAMESPACE
class QClipboard;
QT_END_NAMESPACE

class SimpleChatEngine : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QString userName READ userName NOTIFY userNameChanged)
    Q_PROPERTY(ChatMessageModel *messages READ messages CONSTANT)
    Q_PROPERTY(SimpleChatEngine::ContentType clipBoardContentType READ clipBoardContentType NOTIFY
                       clipBoardContentTypeChanged)
    Q_PROPERTY(SimpleChatEngine::State state READ state NOTIFY stateChanged)
public:
    enum ContentType {
        Unknown = qtgrpc::examples::chat::ChatMessage::ContentType::Unknown,
        Text = qtgrpc::examples::chat::ChatMessage::ContentType::Text,
        Image = qtgrpc::examples::chat::ChatMessage::ContentType::Image,
    };
    Q_ENUM(ContentType)

    enum State {
        Disconnected = 0,
        Connecting = 1,
        Connected = 2,
    };
    Q_ENUM(State)

    explicit SimpleChatEngine(QObject *parent = nullptr);
    ~SimpleChatEngine() override;
    Q_INVOKABLE void login(const QString &name, const QString &password);
    Q_INVOKABLE void sendMessage(const QString &message);

    Q_INVOKABLE void sendImageFromClipboard();

    QString userName() const;
    ChatMessageModel *messages();
    SimpleChatEngine::ContentType clipBoardContentType() const;

    SimpleChatEngine::State state() const;

Q_SIGNALS:
    void networkError(const QString &);
    void authFailed();
    void clipBoardContentTypeChanged();
    void userNameChanged();
    void stateChanged();

private:
    void setState(SimpleChatEngine::State state);

    SimpleChatEngine::State m_state;
    ChatMessageModel m_messages;
    qtgrpc::examples::chat::SimpleChat::Client *m_client;
    QClipboard *m_clipBoard;
    QString m_userName;
    QString m_password;
};

Q_DECLARE_METATYPE(ChatMessageModel *)

#endif // SIMPLECHATENGINE_H
