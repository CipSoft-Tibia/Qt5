// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "serverrunner.h"
#include "exampleservice.grpc.pb.h"

#include <QThread>
#include <QDebug>
#include <QRandomGenerator>

#include <grpc++/grpc++.h>
#include <memory>
#include <array>
#include <random>

namespace {

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;
using qtgrpc::examples::AnswerRequest;
using qtgrpc::examples::AnswerResponse;
using qtgrpc::examples::ExampleService;

static const std::array<std::string_view, 10> answers = {"Yes",
                                                         "Yep",
                                                         "Most\nlikely",
                                                         "It is\ncertain",
                                                         "No",
                                                         "Nope",
                                                         "Try later",
                                                         "Are you\nsure?",
                                                         "Maybe",
                                                         "Very\ndoubtful"};

// Generates random index value.
static int generateRandomIndex()
{
    static std::uniform_int_distribution<int> dist(0, answers.size() - 1);
    return dist(*QRandomGenerator::global());
}

// Logic and data behind the server's behavior.
class ExampleServiceServiceImpl final : public qtgrpc::examples::ExampleService::Service
{
    grpc::Status answerMethod(grpc::ServerContext *, const AnswerRequest *request,
                              AnswerResponse *response) override;
};
}

Status ExampleServiceServiceImpl::answerMethod(grpc::ServerContext *,
                                               const AnswerRequest *request,
                                               AnswerResponse *response)
{
    if (request->message() == "sleep")
        QThread::msleep(2000);

    response->set_message(std::string(answers[generateRandomIndex()]));
    return Status();
}

void ExampleServer::run()
{
    std::string serverUri("127.0.0.1:50051");
    ExampleServiceServiceImpl service;

    grpc::ServerBuilder builder;
    builder.AddListeningPort(serverUri, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    if (!server) {
        qDebug() << "Creating grpc::Server failed.";
        return;
    }

    qDebug() << "Server listening on " << serverUri;
    server->Wait();
}
