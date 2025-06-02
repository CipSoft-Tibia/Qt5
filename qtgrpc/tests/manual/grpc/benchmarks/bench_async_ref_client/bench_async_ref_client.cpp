// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qrpcbench_common.h>
#include <proto/bench.grpc.pb.h>
#include <proto/bench.pb.h>

#include <grpcpp/grpcpp.h>

#include <absl/log/initialize.h>

class AsyncGrpcClientBenchmark
{
public:
    explicit AsyncGrpcClientBenchmark(uint64_t calls, size_t payload = 0) : mCalls(calls)
    {
        if (payload > 0)
            sData.assign(payload, 'x');
        auto channel = grpc::CreateChannel(HostUri.data(), grpc::InsecureChannelCredentials());
        mStub = qt::bench::BenchmarkService::NewStub(std::move(channel));
    }

    void unaryCall();
    void serverStreaming();
    void clientStreaming();
    void bidiStreaming();

private:
    std::unique_ptr<qt::bench::BenchmarkService::Stub> mStub;
    QElapsedTimer mTimer;
    uint64_t mCalls;

    inline static std::string sData;
};

void AsyncGrpcClientBenchmark::unaryCall()
{
    struct UnaryCallData
    {
        grpc::ClientContext context;
        grpc::Status status;
        std::unique_ptr<grpc::ClientAsyncResponseReader<qt::bench::UnaryCallResponse>> reader;

        qt::bench::UnaryCallRequest request;
        qt::bench::UnaryCallResponse response;
    };

    grpc::CompletionQueue cq;
    void *rawTag = nullptr;
    bool ok = false;

    const auto startCall = [this, &cq](uint64_t pong) {
        auto *call = new UnaryCallData();
        call->request.set_ping(pong);
        call->reader = mStub->AsyncUnaryCall(&call->context, call->request, &cq);
        call->reader->Finish(&call->response, &call->status, call);
    };

    uint64_t counter = 0;
    startCall(counter);

    while (cq.Next(&rawTag, &ok)) {
        auto *rpcResult = static_cast<UnaryCallData *>(rawTag);
        if (counter == 0)
            mTimer.start();
        if (rpcResult->status.ok()) {
            if (++counter < mCalls) {
                delete rpcResult;
                startCall(counter);
                continue;
            } else {
                delete rpcResult;
                break;
            }
        } else {
            std::cout << "FAILED: " << rpcResult->status.error_message() << '\n';
        }
        delete rpcResult;
    }
    Client::printRpcResult("UnaryCall", mTimer.nsecsElapsed(), counter);
}

void AsyncGrpcClientBenchmark::serverStreaming()
{

    struct ServerStreamingData
    {
        grpc::ClientContext context;
        grpc::Status status;
        std::unique_ptr<grpc::ClientAsyncReader<qt::bench::ServerStreamingResponse>> stream;

        qt::bench::ServerStreamingRequest request;
        qt::bench::ServerStreamingResponse response;

        std::function<bool(bool)> readHandler;
        std::function<bool(bool)> finishHandler;
        std::function<bool(bool)> callHandler;
    };

    grpc::CompletionQueue cq;
    void *rawTag = nullptr;
    bool ok = false;

    uint64_t counter = 0;
    uint64_t recvBytes = 0;

    auto *call = new ServerStreamingData();
    call->readHandler = [call, &counter, &recvBytes](bool ok) {
        if (ok) {
            call->stream->Read(&call->response, &call->readHandler);
            if (call->response.has_payload())
                recvBytes += call->response.payload().size();
            ++counter;
        }
        return true;
    };
    call->finishHandler = [this, call, &counter, &recvBytes](bool ok) {
        if (ok && call->status.ok()) {
            Client::printRpcResult("ServerStreaming", mTimer.nsecsElapsed(), counter, recvBytes,
                                   sData.size());
        } else {
            std::cout << "FAILED: " << call->status.error_message();
        }
        return false;
    };
    call->callHandler = [this, call](bool ok) {
        if (ok) {
            call->stream->Read(&call->response, &call->readHandler);
            call->stream->Finish(&call->status, &call->finishHandler);
            mTimer.start();
            return true;
        } else {
            std::cout << "FAILED: serverStreamingRequest\n";
            return false;
        }
    };

    if (!sData.empty())
        call->request.set_payload(sData);
    call->request.set_ping(mCalls);
    call->stream = mStub->AsyncServerStreaming(&call->context, call->request, &cq,
                                               &call->callHandler);

    while (cq.Next(&rawTag, &ok)) {
        auto rpcTag = *static_cast<std::function<bool(bool)> *>(rawTag);
        if (!rpcTag(ok))
            break;
    }

    delete call;
}

void AsyncGrpcClientBenchmark::clientStreaming()
{
    struct ClientStreamingData
    {
        grpc::ClientContext context;
        grpc::Status status;
        std::unique_ptr<grpc::ClientAsyncWriter<qt::bench::ClientStreamingRequest>> stream;

        qt::bench::ClientStreamingRequest request;
        qt::bench::ClientStreamingResponse response;

        std::function<bool(bool)> writeHandler;
        std::function<bool(bool)> writesDoneHandler;
        std::function<bool(bool)> finishHandler;
        std::function<bool(bool)> callHandler;
    };

    grpc::CompletionQueue cq;
    void *rawTag = nullptr;
    bool ok = false;

    uint64_t counter = 0;
    uint64_t sendBytes = 0;

    auto *call = new ClientStreamingData();
    call->writeHandler = [this, call, &counter, &sendBytes](bool ok) {
        if (ok && counter < mCalls) {
            call->request.set_ping(counter);
            call->stream->Write(call->request, &call->writeHandler);
            if (call->request.has_payload())
                sendBytes += call->request.payload().size();
            ++counter;
        } else if (ok && counter >= mCalls) {
            call->stream->WritesDone(&call->writesDoneHandler);
        }
        return true;
    };
    call->writesDoneHandler = [](bool) { return true; };
    call->finishHandler = [this, call, &counter, &sendBytes](bool ok) {
        if (ok && call->status.ok()) {
            uint64_t recvBytes = 0;
            if (call->response.has_payload())
                recvBytes += call->response.payload().size();
            Client::printRpcResult("ClientStreaming", mTimer.nsecsElapsed(), counter, recvBytes,
                                   sendBytes);
        } else {
            std::cout << "FAILED: " << call->status.error_message();
        }
        return false;
    };
    call->callHandler = [this, call](bool ok) {
        if (ok) {
            call->stream->Finish(&call->status, &call->finishHandler);
            call->stream->Write(call->request, &call->writeHandler);
            mTimer.start();
            return true;
        } else {
            std::cout << "FAILED: clientStreaming\n";
            return false;
        }
    };

    if (!sData.empty())
        call->request.set_payload(sData);
    call->stream = mStub->AsyncClientStreaming(&call->context, &call->response, &cq,
                                               &call->callHandler);

    while (cq.Next(&rawTag, &ok)) {
        auto rpcTag = *static_cast<std::function<bool(bool)> *>(rawTag);
        if (!rpcTag(ok))
            break;
    }

    delete call;
}

void AsyncGrpcClientBenchmark::bidiStreaming()
{
    struct BidiStreamingData
    {
        grpc::ClientContext context;
        grpc::Status status;
        std::unique_ptr<grpc::ClientAsyncReaderWriter<qt::bench::BiDiStreamingRequest,
                                                      qt::bench::BiDiStreamingResponse>>
            stream;

        qt::bench::BiDiStreamingRequest request;
        qt::bench::BiDiStreamingResponse response;

        std::function<bool(bool)> finishHandler;
        std::function<bool(bool)> callHandler;
        std::function<bool(bool)> writeHandler;
        std::function<bool(bool)> writesDoneHandler;
        std::function<bool(bool)> readHandler;
    };
    grpc::CompletionQueue cq;
    void *rawTag = nullptr;
    bool ok = false;

    uint64_t counter = 0;
    uint64_t recvBytes = 0;
    uint64_t sendBytes = 0;

    auto *call = new BidiStreamingData();
    call->writeHandler = [this, call, &counter, &sendBytes](bool ok) {
        if (ok && counter < mCalls) {
            if (call->request.has_payload())
                sendBytes += call->request.payload().size();
            call->request.set_ping(counter);
            call->stream->Write(call->request, &call->writeHandler);
            ++counter;
        } else if (ok && counter >= mCalls) {
            call->stream->WritesDone(&call->writesDoneHandler);
        }
        return true;
    };
    call->writesDoneHandler = [](bool) { return true; };
    call->readHandler = [call, &recvBytes](bool ok) {
        if (ok) {
            if (call->response.has_payload())
                recvBytes += call->response.payload().size();
            call->stream->Read(&call->response, &call->readHandler);
        }
        return true;
    };
    call->finishHandler = [this, call, &counter, &recvBytes, &sendBytes](bool ok) {
        if (ok && call->status.ok()) {
            Client::printRpcResult("BidiStreaming", mTimer.nsecsElapsed(), counter, recvBytes,
                                   sendBytes);
        } else {
            std::cout << "FAILED: " << call->status.error_message();
        }
        return false;
    };
    call->callHandler = [this, call](bool ok) {
        if (ok) {
            mTimer.start();
            call->stream->Finish(&call->status, &call->finishHandler);
            call->writeHandler(true);
            call->readHandler(true);
            return true;
        } else {
            std::cout << "FAILED: clientStreaming\n";
            return false;
        }
    };

    if (!sData.empty())
        call->request.set_payload(sData);
    call->stream = mStub->AsyncBiDiStreaming(&call->context, &cq, &call->callHandler);

    while (cq.Next(&rawTag, &ok)) {
        auto rpcTag = *static_cast<std::function<bool(bool)> *>(rawTag);
        if (!rpcTag(ok))
            break;
    }

    delete call;
}

int main(int argc, char *argv[])
{
    absl::InitializeLog();
    Client::benchmarkMain<AsyncGrpcClientBenchmark>("AsyncReferenceClient", argc, argv);
}
