// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "testserverrunner.h"

#include <QDebug>
#include <QFile>
#include <QString>
#include <QThread>

#include "testservice.grpc.pb.h"

#include <grpc++/grpc++.h>

#include <memory>

namespace {

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;
using qtgrpc::tests::Empty;
using qtgrpc::tests::BlobMessage;
using qtgrpc::tests::SimpleIntMessage;
using qtgrpc::tests::SimpleStringMessage;
using qtgrpc::tests::TestService;

// Logic and data behind the server's behavior.
class TestServiceServiceImpl final : public qtgrpc::tests::TestService::Service
{
public:
    TestServiceServiceImpl(qint64 latency) : m_latency(latency) { }

private:
    grpc::Status testMethod(grpc::ServerContext *, const SimpleStringMessage *request,
                            SimpleStringMessage *response) override;

    grpc::Status testMethodServerStream(grpc::ServerContext *, const SimpleStringMessage *request,
                                        grpc::ServerWriter<SimpleStringMessage> *writer) override;

    grpc::Status testMethodBlobServerStream(grpc::ServerContext *, const BlobMessage *request,
                                            grpc::ServerWriter<BlobMessage> *writer) override;
    grpc::Status testMethodStatusMessage(grpc::ServerContext *, const SimpleStringMessage *request,
                                         SimpleStringMessage *) override;
    grpc::Status testMethodNonCompatibleArgRet(grpc::ServerContext *,
                                               const SimpleIntMessage *request,
                                               SimpleStringMessage *response) override;
    grpc::Status testMetadata(grpc::ServerContext *,
                                                const Empty *,
                                                qtgrpc::tests::Empty *) override;

    grpc::Status
    testMethodClientStream(::grpc::ServerContext *,
                           ::grpc::ServerReader<::qtgrpc::tests::SimpleStringMessage> *reader,
                           ::qtgrpc::tests::SimpleStringMessage *response) override;

    grpc::Status testMethodBiStream(
            ::grpc::ServerContext *context,
            ::grpc::ServerReaderWriter<::qtgrpc::tests::SimpleStringMessage,
                                       ::qtgrpc::tests::SimpleStringMessage> *stream) override;
    qint64 m_latency;
};
}

Status TestServiceServiceImpl::testMethod(grpc::ServerContext *, const SimpleStringMessage *request,
                                          SimpleStringMessage *response)
{
    qInfo() << "testMethod called with: " << request->testfieldstring();
    response->set_testfieldstring(request->testfieldstring());
    if (request->testfieldstring() == "sleep") {
        QThread::msleep(m_latency);
    }
    return Status();
}

Status TestServiceServiceImpl::testMethodServerStream(grpc::ServerContext *,
                                                      const SimpleStringMessage *request,
                                                      ServerWriter<SimpleStringMessage> *writer)

{
    qInfo() << "testMethodServerStream called with: " << request->testfieldstring();
    SimpleStringMessage msg;

    msg.set_testfieldstring(request->testfieldstring() + "1");
    QThread::msleep(m_latency);
    qInfo() << "send back " << (request->testfieldstring() + "1");
    writer->Write(msg);

    msg.set_testfieldstring(request->testfieldstring() + "2");
    QThread::msleep(m_latency);
    qInfo() << "send back " << (request->testfieldstring() + "2");
    writer->Write(msg);

    msg.set_testfieldstring(request->testfieldstring() + "3");
    QThread::msleep(m_latency);
    qInfo() << "send back " << (request->testfieldstring() + "3");
    writer->Write(msg);

    msg.set_testfieldstring(request->testfieldstring() + "4");
    QThread::msleep(m_latency);
    qInfo() << "send back " << (request->testfieldstring() + "4");
    writer->WriteLast(msg, grpc::WriteOptions());

    return Status();
}

Status TestServiceServiceImpl::testMethodBlobServerStream(grpc::ServerContext *,
                                                          const BlobMessage *request,
                                                          ServerWriter<BlobMessage> *writer)

{
    qInfo() << "testMethodBlobServerStream called with testbytes size: "
            << request->testbytes().size();
    BlobMessage msg;
    msg.set_allocated_testbytes(new std::string(request->testbytes()));
    writer->Write(msg);
    return Status();
}

Status TestServiceServiceImpl::testMethodStatusMessage(grpc::ServerContext *,
                                                       const SimpleStringMessage *request,
                                                       SimpleStringMessage *)
{
    return Status(grpc::StatusCode::UNIMPLEMENTED, request->testfieldstring());
}

Status TestServiceServiceImpl::testMethodNonCompatibleArgRet(grpc::ServerContext *,
                                                             const SimpleIntMessage *request,
                                                             SimpleStringMessage *response)
{
    qInfo() << "testMethodNonCompatibleArgRet called with: " << request->testfield();
    response->set_testfieldstring(std::to_string(request->testfield()));
    return Status();
}

Status TestServiceServiceImpl::testMetadata(grpc::ServerContext *ctx, const Empty *,
                                            qtgrpc::tests::Empty *)
{
    std::string client_return_header;
    for (const auto &header : ctx->client_metadata()) {
        if (header.first == "client_header") {
            ctx->AddTrailingMetadata("server_header",
                                     std::string(header.second.data(), header.second.size()));
        } else if (header.first == "client_return_header") {
            if (client_return_header.empty())
                client_return_header = std::string(header.second.data(), header.second.size());
            else
                client_return_header = "invalid_value";
        }
    }

    ctx->AddTrailingMetadata("client_return_header", client_return_header);
    return Status();
}

grpc::Status TestServiceServiceImpl::testMethodClientStream(
        ::grpc::ServerContext *, ::grpc::ServerReader<::qtgrpc::tests::SimpleStringMessage> *reader,
        ::qtgrpc::tests::SimpleStringMessage *response)
{
    ::qtgrpc::tests::SimpleStringMessage req;
    std::string rspString;
    for (int i = 0; i < 4; ++i) {
        if (!reader->Read(&req)) {
            qInfo() << "Unable to read message from client stream";
            return grpc::Status(grpc::StatusCode::DATA_LOSS, rspString);
        }
        rspString += req.testfieldstring();
        rspString += std::to_string(i + 1);
    }

    response->set_testfieldstring(rspString);
    return {};
}

grpc::Status TestServiceServiceImpl::testMethodBiStream(
        ::grpc::ServerContext *,
        ::grpc::ServerReaderWriter<::qtgrpc::tests::SimpleStringMessage,
                                   ::qtgrpc::tests::SimpleStringMessage> *stream)
{
    ::qtgrpc::tests::SimpleStringMessage req;
    for (int i = 0; i < 4; ++i) {
        if (!stream->Read(&req)) {
            qInfo() << "Unable to read message from bidirectional stream";
            return grpc::Status(grpc::StatusCode::DATA_LOSS, "Read failed");
        }
        std::string rspString = req.testfieldstring() + std::to_string(i + 1);
        ::qtgrpc::tests::SimpleStringMessage rsp;
        rsp.set_testfieldstring(rspString);
        if (!stream->Write(rsp, {})) {
            qInfo() << "Unable to write message to bidirectional stream";
            return grpc::Status(grpc::StatusCode::DATA_LOSS, "Write failed");
        }
        QThread::msleep(m_latency);
    }

    return {};
}

void TestServer::run(qint64 latency)
{
    TestServiceServiceImpl service(latency);

    grpc::ServerBuilder builder;
    QFile cfile(":/assets/cert.pem");
    cfile.open(QFile::ReadOnly);
    QString cert = cfile.readAll();

    QFile kfile(":/assets/key.pem");
    kfile.open(QFile::ReadOnly);
    QString key = kfile.readAll();

    grpc::SslServerCredentialsOptions opts(GRPC_SSL_DONT_REQUEST_CLIENT_CERTIFICATE);
    opts.pem_key_cert_pairs.push_back({ key.toStdString(), cert.toStdString() });

    QString httpURI("127.0.0.1:50051");
    builder.AddListeningPort(httpURI.toStdString(), grpc::InsecureServerCredentials());
    QString httpsURI("127.0.0.1:50052");
    builder.AddListeningPort(httpsURI.toStdString(), grpc::SslServerCredentials(opts));
#ifndef Q_OS_WINDOWS
    QString unixUri("unix:///tmp/qtgrpc_test.sock");
    builder.AddListeningPort(unixUri.toStdString(), grpc::InsecureServerCredentials());
#endif
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    if (!server) {
        qDebug() << "Creating grpc::Server failed.";
        return;
    }
#ifdef Q_OS_WINDOWS
    qDebug() << "Server listening on " << httpURI << httpsURI;
#else
    qDebug() << "Server listening on " << httpURI << httpsURI << "and" << unixUri;
#endif
    server->Wait();
}
