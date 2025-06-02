// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef RPCJOBS_H
#define RPCJOBS_H

#include <proto/bench.grpc.pb.h>
#include <proto/bench.pb.h>

#include <grpcpp/server_context.h>

using qt::bench::BenchmarkService;

class RpcJob
{
public:
    RpcJob() = default;
    virtual ~RpcJob() = default;

    RpcJob(const RpcJob &) = delete;
    RpcJob &operator=(const RpcJob &) = delete;

    RpcJob(RpcJob &&) = delete;
    RpcJob &operator=(RpcJob &&) = delete;

    virtual void onProcess(bool ok) = 0;
    virtual void onDone() = 0;

    [[nodiscard]] bool isDone() const noexcept { return mOnDoneCalled; }

private:
    std::atomic<bool> mOnDoneCalled = false;
    friend class NotifyWhenDone;
};

class NotifyWhenDone final : public RpcJob
{
public:
    explicit NotifyWhenDone(RpcJob *parent) : mParent(parent) { }

    void onProcess(bool /* ok */) override
    {
        assert(mParent);
        mParent->mOnDoneCalled = true;
        onDone();
    }

    void onDone() override { delete this; }

private:
    RpcJob *mParent;
};

class UnaryCall final : public RpcJob
{
    using Request = qt::bench::UnaryCallRequest;
    using Response = qt::bench::UnaryCallResponse;

public:
    explicit UnaryCall(grpc::ServerCompletionQueue *cq, BenchmarkService::AsyncService *service)
        : mCQ(cq), mService(service), mWriter(&mContext)
    {
        mService->RequestUnaryCall(&mContext, &mRequest, &mWriter, cq, cq, this);
        mContext.AsyncNotifyWhenDone(new NotifyWhenDone(this));
    }

    void onProcess(bool ok) override
    {
        if (!ok || isDone())
            return onDone();

        if (mState == State::Created) {
            new UnaryCall(mCQ, mService);
            mResponse.set_pong(mRequest.ping());
            mWriter.Finish(mResponse, grpc::Status::OK, this);
            mState = State::Processed;
        } else {
            return onDone();
        }
    }

    void onDone() override { delete this; }

private:
    grpc::ServerContext mContext;
    grpc::ServerCompletionQueue *mCQ;
    BenchmarkService::AsyncService *mService;

    Request mRequest;
    Response mResponse;
    grpc::ServerAsyncResponseWriter<Response> mWriter;

    enum class State { Created, Processed };
    State mState = State::Created;
};

class ServerStreaming final : public RpcJob
{
    using Request = qt::bench::ServerStreamingRequest;
    using Response = qt::bench::ServerStreamingResponse;

public:
    explicit ServerStreaming(grpc::ServerCompletionQueue *cq,
                             BenchmarkService::AsyncService *service)
        : mCQ(cq), mService(service), mServerStream(&mContext)
    {
        mService->RequestServerStreaming(&mContext, &mRequest, &mServerStream, mCQ, mCQ, this);
        mContext.AsyncNotifyWhenDone(new NotifyWhenDone(this));
    }

    void onProcess(bool ok) override
    {
        if (!ok || isDone())
            return onDone();

        switch (mState) {
        case State::Created: {
            new ServerStreaming(mCQ, mService);

            if (mRequest.has_payload())
                mResponse.mutable_payload()->assign(mRequest.payload().size(), 'x');
            mResponse.set_pong(mPongCount);

            mServerStream.Write(mResponse, this);
            mState = State::Writing;
        } break;
        case State::Writing: {
            if (++mPongCount >= mRequest.ping()) {
                mServerStream.Finish(grpc::Status::OK, this);
                mState = State::Finished;
            } else {
                mResponse.set_pong(mPongCount);
                mServerStream.Write(mResponse, this);
            }
        } break;
        case State::Finished: {
            // Finish will invoke onDone directly.
        } break;
        }
    }

    void onDone() override
    {
        std::cout << std::format("serverstream done, {} pongs\n", mPongCount);
        delete this;
    }

private:
    grpc::ServerContext mContext;
    grpc::ServerCompletionQueue *mCQ;
    BenchmarkService::AsyncService *mService;
    grpc::ServerAsyncWriter<Response> mServerStream;

    enum class State { Created, Writing, Finished };
    State mState = State::Created;

    Request mRequest;
    Response mResponse;
    uint64_t mPongCount = 0;
};

class ClientStreaming final : public RpcJob
{
    using Request = qt::bench::ClientStreamingRequest;
    using Response = qt::bench::ClientStreamingResponse;

public:
    explicit ClientStreaming(grpc::ServerCompletionQueue *cq,
                             BenchmarkService::AsyncService *service)
        : mCQ(cq), mService(service), mClientStream(&mContext)
    {
        mService->RequestClientStreaming(&mContext, &mClientStream, mCQ, mCQ, this);
        mContext.AsyncNotifyWhenDone(new NotifyWhenDone(this));
    }

    void onProcess(bool ok) override
    {
        if (isDone())
            return onDone();

        switch (mState) {
        case State::Created: {
            new ClientStreaming(mCQ, mService);
            if (!ok)
                return onDone();
            mClientStream.Read(&mRequest, this);
            mState = State::Reading;
        } break;
        case State::Reading: {
            if (!ok) {
                mResponse.set_pong(mPingCount);
                mClientStream.Finish(mResponse, grpc::Status::OK, this);
                mState = State::Finished;
            } else {
                ++mPingCount;
                if (mRequest.has_payload() && !mResponse.has_payload())
                    mResponse.mutable_payload()->assign(mRequest.payload().size(), 'x');
                mRequest.set_ping(mPingCount);
                mClientStream.Read(&mRequest, this);
            }
        } break;
        case State::Finished: {
            // Finish will invoke onDone directly.
        } break;
        }
    }

    void onDone() override
    {
        std::cout << std::format("clientstream done, {} pings\n", mPingCount);
        delete this;
    }

private:
    grpc::ServerContext mContext;
    grpc::ServerCompletionQueue *mCQ;
    BenchmarkService::AsyncService *mService;
    grpc::ServerAsyncReader<Response, Request> mClientStream;

    enum class State { Created, Reading, Finished };
    State mState = State::Created;

    Request mRequest;
    Response mResponse;
    uint64_t mPingCount = 0;
};

class BiDiStreaming final : public RpcJob
{
    using Request = qt::bench::BiDiStreamingRequest;
    using Response = qt::bench::BiDiStreamingResponse;

    // A simple Ping - Pong BiDi streaming implementation.
    // gRPC would however also allow to have Read and Write
    // operation running simultaniously.
public:
    explicit BiDiStreaming(grpc::ServerCompletionQueue *cq, BenchmarkService::AsyncService *service)
        : mCQ(cq), mService(service), mStream(&mContext)
    {
        mService->RequestBiDiStreaming(&mContext, &mStream, mCQ, mCQ, this);
        mContext.AsyncNotifyWhenDone(new NotifyWhenDone(this));
    }

    void onProcess(bool ok) override
    {
        if (isDone())
            return onDone();

        switch (mState) {
        case State::Created: {
            new BiDiStreaming(mCQ, mService);
            if (!ok)
                return onDone();
            mStream.Read(&mRequest, this);
            mState = State::Reading;
        } break;
        case State::Reading: {
            if (!ok) {
                mStream.Finish(grpc::Status::OK, this);
                mState = State::Finished;
                return;
            }
            ++mPingCount;
            if (mRequest.has_payload() && !mResponse.has_payload())
                mResponse.mutable_payload()->assign(mRequest.payload().size(), 'x');
            mResponse.set_pong(mRequest.ping());

            mStream.Write(mResponse, this);
            mState = State::Writing;
        } break;
        case State::Writing: {
            if (!ok)
                return onDone();
            ++mPongCount;
            mStream.Read(&mRequest, this);
            mState = State::Reading;
        } break;
        case State::Finished: {
            // Finish will invoke onDone directly.
        } break;
        }
    }

    void onDone() override
    {
        std::cout << std::format("bidistream done, {} pings, {} pongs\n", mPingCount, mPongCount);
        delete this;
    }

private:
    grpc::ServerContext mContext;
    grpc::ServerCompletionQueue *mCQ;
    BenchmarkService::AsyncService *mService;

    grpc::ServerAsyncReaderWriter<Response, Request> mStream;

    enum class State { Created, Reading, Writing, Finished };
    State mState = State::Created;

    Request mRequest;
    Response mResponse;

    uint64_t mPingCount = 0;
    uint64_t mPongCount = 0;
};

#endif // RPCJOBS_H
