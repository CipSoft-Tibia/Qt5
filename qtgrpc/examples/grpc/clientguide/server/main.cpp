// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "clientguide.grpc.pb.h"

#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>

#include <chrono>
#include <iostream>
#include <string_view>

using client::guide::Request;
using client::guide::Response;

static constexpr std::string_view ServerUri = "localhost:50056";

namespace {

int64_t now()
{
    return std::chrono::system_clock::now().time_since_epoch().count();
}

std::ostream &operator<<(std::ostream &stream, const Request &request)
{
    return stream << "Request( time: " << request.time() << ", num: " << request.num() << " )";
}

}

class ClientGuideService : public client::guide::ClientGuideService::Service
{
    grpc::Status UnaryCall(grpc::ServerContext * /* context */, const Request *request,
                           Response *response) override
    {
        std::cout << "Server (UnaryCall): " << *request << std::endl;
        //! [time]
        const auto time = now();
        if (request->time() > time)
            return { grpc::StatusCode::INVALID_ARGUMENT, "Request time is in the future!" };
        //! [time]

        //! [response]
        response->set_num(request->num());
        response->set_time(time);
        return grpc::Status::OK;
        //! [response]
    }

    grpc::Status ServerStreaming(grpc::ServerContext * /* context */, const Request *request,
                                 grpc::ServerWriter<Response> *writer) override
    {
        std::cout << "Server (ServerStreaming): " << *request << std::endl;
        if (request->time() > now())
            return { grpc::StatusCode::INVALID_ARGUMENT, "Request time is in the future!" };

        Response response;
        for (int32_t i = 0; i < request->num(); ++i) {
            response.set_num(i);
            response.set_time(now());
            writer->Write(response);
        }

        return grpc::Status::OK;
    }

    grpc::Status ClientStreaming(grpc::ServerContext * /* context */,
                                 grpc::ServerReader<Request> *reader, Response *response) override
    {
        Request request;
        int32_t count = 0;
        while (reader->Read(&request)) {
            std::cout << "Server (ClientStreaming): " << request << std::endl;
            if (request.time() > now())
                return { grpc::StatusCode::INVALID_ARGUMENT, "Request time is in the future!" };
            ++count;
        }
        response->set_num(count);
        response->set_time(now());
        return grpc::Status::OK;
    }

    grpc::Status
    BidirectionalStreaming(grpc::ServerContext * /* context */,
                           grpc::ServerReaderWriter<Response, Request> *stream) override
    {
        Request request;
        Response response;

        while (stream->Read(&request)) {
            std::cout << "Server (BidirectionalStreaming): " << request << std::endl;
            const auto time = now();
            if (request.time() > time)
                return { grpc::StatusCode::INVALID_ARGUMENT, "Request time is in the future!" };
            response.set_num(request.num());
            response.set_time(time);
            if (!stream->Write(response))
                return grpc::Status::CANCELLED;
        }

        return grpc::Status::OK;
    }
};

int main(int /* argc */, char * /* argv */[])
{
    std::unique_ptr<grpc::Server> server;
    ClientGuideService service;
    {
        grpc::ServerBuilder builder;
        builder.AddListeningPort(ServerUri.data(), grpc::InsecureServerCredentials());
        builder.RegisterService(&service);
        server = builder.BuildAndStart();
    }
    std::cout << "Server listening on: " << ServerUri.data() << std::endl;
    server->Wait();
}
