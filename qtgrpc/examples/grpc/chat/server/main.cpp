// Copyright (C) 2023 The Qt Company Ltd.
// Copyright (C) 2019 Alexey Edelev <semlanik@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <iostream>
#include <string_view>
#include <list>
#include <cassert>
#include <optional>

#include <grpc++/grpc++.h>
#include <simplechat.pb.h>
#include <simplechat.grpc.pb.h>
using namespace qtgrpc::examples::chat;

class MessageListHandler;
class UserListHandler;

constexpr std::string_view nameHeader("user-name");
constexpr std::string_view passwordHeader("user-password");
class SimpleChatService final : public SimpleChat::WithAsyncMethod_messageList<SimpleChat::Service>
{
public:
    SimpleChatService();
    void run(std::string_view address);

    grpc::Status sendMessage(grpc::ServerContext *context, const ChatMessage *request,
                               None *) override;

private:
    std::optional<std::string> checkUserCredentials(grpc::ServerContext *context);
    void addMessageHandler(MessageListHandler *userHandler);
    void sendMessageToClients(const ChatMessage *message);

    Users m_usersDatabase;
    ChatMessages m_messages;
    std::list<MessageListHandler *> m_activeClients;
};

struct HandlerTag
{
    enum Type { Request = 1, Reply, Disconnect, Reject };

    HandlerTag(HandlerTag::Type t, MessageListHandler *h) : tag(t), handler(h) { }

    HandlerTag::Type tag;
    MessageListHandler *handler;
};

class MessageListHandler
{
public:
    MessageListHandler(SimpleChatService *service, grpc::ServerCompletionQueue *queue)
        : writer(&ctx), cq(queue)
    {
        ctx.AsyncNotifyWhenDone(new HandlerTag(HandlerTag::Disconnect, this));
        service->RequestmessageList(&ctx, &request, &writer, cq, cq,
                                    new HandlerTag(HandlerTag::Request, this));
    }

    None request;
    grpc::ServerAsyncWriter<qtgrpc::examples::chat::ChatMessages> writer;
    grpc::ServerContext ctx;
    grpc::ServerCompletionQueue *cq;
};

void SimpleChatService::run(std::string_view address)
{
    grpc::ServerBuilder builder;
    builder.AddListeningPort(std::string(address), grpc::InsecureServerCredentials());
    builder.RegisterService(this);
    std::unique_ptr<grpc::ServerCompletionQueue> cq = builder.AddCompletionQueue();
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << address << '\n';

    MessageListHandler *pending = new MessageListHandler(this, cq.get());
    while (true) {
        HandlerTag *tag = nullptr;
        bool ok = false;
        cq->Next(reinterpret_cast<void **>(&tag), &ok);

        if (tag == nullptr)
            continue;

        if (!ok) {
            std::cout << "Unable to proccess tag from the completion queue\n";
            delete tag;
            continue;
        }

        switch (tag->tag) {
        case HandlerTag::Request: {
            std::cout << "New connection request received\n";
            const auto name = checkUserCredentials(&(pending->ctx));
            if (!name.has_value()) {
                std::cout << "Authentication failed\n";
                pending->writer.Finish(grpc::Status(grpc::StatusCode::UNAUTHENTICATED,
                                                      "User or login are invalid"),
                                       new HandlerTag(HandlerTag::Reject, pending));
            } else {
                std::cout << "User " << name.value() << " connected to chat\n";
                addMessageHandler(pending);
                pending->writer.Write(m_messages, nullptr);
            }
            pending = new MessageListHandler(this, cq.get());
        } break;
        case HandlerTag::Disconnect: {
            auto it = std::find(m_activeClients.begin(), m_activeClients.end(), tag->handler);
            if (it != m_activeClients.end()) {
                std::cout << "Client disconnected\n";
                m_activeClients.erase(it);
                delete tag->handler;
            }
        } break;
        case HandlerTag::Reject:
            std::cout << "Connection rejected\n";
            m_activeClients.remove(tag->handler);
            delete tag->handler;
            break;
        case HandlerTag::Reply:
            std::cout << "Sending data to users\n";
            break;
        }
        delete tag;
    }
}

std::optional<std::string> SimpleChatService::checkUserCredentials(grpc::ServerContext *context)
{
    assert(context != nullptr);

    std::string name;
    std::string password;
    for (const auto &[key, value] : std::as_const(context->client_metadata())) {
        if (std::string(key.data(), key.size()) == nameHeader) {
            name = std::string(value.data(), value.size());
        }
        if (std::string(key.data(), key.size()) == passwordHeader) {
            password = std::string(value.data(), value.size());
        }
    }

    return std::find_if(m_usersDatabase.users().begin(), m_usersDatabase.users().end(),
                        [&name, &password](const auto &it) {
                            return it.name() == name && it.password() == password;
                        })
                    != m_usersDatabase.users().end()
            ? std::optional{ name }
            : std::nullopt;
}

SimpleChatService::SimpleChatService()
{
    // All passwords are 'qwerty' by default
    User *newUser = m_usersDatabase.add_users();
    newUser->set_name("user1");
    newUser->set_password("qwerty");
    newUser = m_usersDatabase.add_users();
    newUser->set_name("user2");
    newUser->set_password("qwerty");
    newUser = m_usersDatabase.add_users();
    newUser->set_name("user3");
    newUser->set_password("qwerty");
    newUser = m_usersDatabase.add_users();
    newUser->set_name("user4");
    newUser->set_password("qwerty");
    newUser = m_usersDatabase.add_users();
    newUser->set_name("user5");
    newUser->set_password("qwerty");
}

void SimpleChatService::sendMessageToClients(const ChatMessage *message)
{
    assert(message != nullptr);

    // Send only new messages after users received all the messages for this session after stream
    // call.
    ChatMessages messages;
    ChatMessage *msg = messages.add_messages();
    *msg = *message;

    for (auto client : m_activeClients)
        client->writer.Write(messages, new HandlerTag(HandlerTag::Reply, client));
}

void SimpleChatService::addMessageHandler(MessageListHandler *handler)
{
    assert(handler != nullptr);
    m_activeClients.push_back(handler);
}

grpc::Status SimpleChatService::sendMessage(grpc::ServerContext *context,
                                              const ChatMessage *request, None *)
{
    assert(context != nullptr);
    assert(request != nullptr);

    auto name = checkUserCredentials(context);
    if (!name)
        return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "Login or password are invalid");

    auto msg = m_messages.add_messages();
    *msg = *request;
    msg->set_from(*name);
    sendMessageToClients(msg);
    return grpc::Status();
}

int main(int, char *[])
{
    SimpleChatService srv;
    srv.run("localhost:65002");
}
