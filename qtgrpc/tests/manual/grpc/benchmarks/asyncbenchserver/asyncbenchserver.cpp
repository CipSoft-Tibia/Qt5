// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "rpcjobs.h"
#include <qrpcbench_common.h>

#include <grpcpp/completion_queue.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>

#include <absl/log/initialize.h>

#include <atomic>
#include <cassert>
#include <memory>
#include <string>
#include <string_view>

class AsyncServer
{
public:
    enum class State { Created, Running, Finished };

    void run(std::string_view address = "0.0.0.0:0")
    {
        State expected = State::Created;
        while (!mState.compare_exchange_weak(expected, State::Running)) {
            if (expected != State::Created)
                return;
        }

        {
            grpc::ServerBuilder builder;
            builder.AddListeningPort(address.data(), grpc::InsecureServerCredentials(),
                                     &mSelectedPort);
            builder.RegisterService(&mService);
            mCompletionQueue = builder.AddCompletionQueue();
            mAddressUri = address.substr(0, address.find_last_of(':'));
            mServer = builder.BuildAndStart();
        }
        std::cout << std::format("Server listening on URI: {}, Port: {}\n", mAddressUri,
                                 mSelectedPort);

        new UnaryCall(mCompletionQueue.get(), &mService);
        new ServerStreaming(mCompletionQueue.get(), &mService);
        new ClientStreaming(mCompletionQueue.get(), &mService);
        new BiDiStreaming(mCompletionQueue.get(), &mService);

        AsyncServer::processRPCs(mCompletionQueue.get());
    }

private:
    qt::bench::BenchmarkService::AsyncService mService;
    std::unique_ptr<grpc::Server> mServer;
    std::unique_ptr<grpc::ServerCompletionQueue> mCompletionQueue;

    std::string mAddressUri;
    int mSelectedPort = -1;

    std::atomic<State> mState = { State::Created };
    static_assert(std::atomic<State>::is_always_lock_free);

    // A single thread works on the completion queue.
    static void processRPCs(grpc::ServerCompletionQueue *cq)
    {
        assert(cq);
        void *rawTag = nullptr;
        bool ok = false;
        while (cq->Next(&rawTag, &ok))
            static_cast<RpcJob *>(rawTag)->onProcess(ok);
    }
};

int main()
{
    absl::InitializeLog();
    AsyncServer server;
    server.run(HostUri);
}
