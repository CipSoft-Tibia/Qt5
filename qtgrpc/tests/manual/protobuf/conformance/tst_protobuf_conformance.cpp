// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <conformance.qpb.h>
#include <test_messages_proto3.qpb.h>

#include <cstdio>
#include <memory>

#include <QtCore/QCoreApplication>
#include <QtCore/QSocketNotifier>
#include <QtProtobuf/QProtobufSerializer>

class ConformaceServer : public QObject
{
public:
    ConformaceServer();
    void readStdin(int socket, QSocketNotifier::Type type);

    QByteArray runTest(const QByteArray &reqData);

private:
    QProtobufSerializer m_serializer;
    std::unique_ptr<QSocketNotifier> m_stdinNotifier;
};

ConformaceServer::ConformaceServer()
    : QObject(),
      m_stdinNotifier(std::make_unique<QSocketNotifier>(fileno(stdin), QSocketNotifier::Read, this))
{
    connect(m_stdinNotifier.get(), &QSocketNotifier::activated, this, &ConformaceServer::readStdin);
}

void ConformaceServer::readStdin(int, QSocketNotifier::Type)
{
    uint32_t dataSize = 0;
    if (fread(&dataSize, sizeof(dataSize), 1, stdin) != 1) {
        fprintf(stderr, "Unable to read the message size from stdin. Assume the channel is closed.\n");
        QCoreApplication::quit();
        return;
    }

    if (dataSize == 0) {
        fprintf(stderr, "Received message of size 0. Potential issue\n");
        return;
    }

    QByteArray reqData(dataSize, Qt::Uninitialized);
    if (fread(reqData.data(), sizeof(char), dataSize, stdin) != dataSize) {
        fprintf(stderr, "Unable read message of size %d\n", dataSize);
    }

    auto respData = runTest(reqData);
    dataSize = respData.size();

    if (fwrite(&dataSize, sizeof(dataSize), 1, stdout) != 1) {
        fprintf(stderr, "Unable to write the message size to stdout\n");
        QCoreApplication::exit(1);
        return;
    }

    if (fwrite(respData.constData(), sizeof(char), dataSize, stdout) != dataSize) {
        fprintf(stderr, "Unable to write the message to stdout\n");
        QCoreApplication::exit(1);
        return;
    }
    fflush(stdout);
}

QByteArray ConformaceServer::runTest(const QByteArray &reqData)
{
    conformance::ConformanceRequest request;
    request.deserialize(&m_serializer, reqData);

    conformance::ConformanceResponse response;

    if (request.requestedOutputFormat() != conformance::WireFormatGadget::PROTOBUF) {
        response.setSkipped(
                QString("Unsupported response format %1").arg(request.requestedOutputFormat()));
        return response.serialize(&m_serializer);
    }

    if (conformance::ConformanceRequest::PayloadFields::ProtobufPayload != request.payloadField()) {
        response.setSkipped(QString("Unsuported payload type %1").arg(int(request.payloadField())));
        return response.serialize(&m_serializer);
    }

    auto msg = QProtobufMessage::constructByName(request.messageType());
    if (!msg) {
        response.setSkipped(QString("Unknown message %1").arg(request.messageType()));
        return response.serialize(&m_serializer);
    }

    if (request.messageType() == "conformance.FailureSet") {
        conformance::FailureSet failures;
        response.setProtobufPayload(failures.serialize(&m_serializer));
        return response.serialize(&m_serializer);
    }

    if (!m_serializer.deserializeRawMessage(msg.get(), request.protobufPayload())
        || m_serializer.deserializationError() != QAbstractProtobufSerializer::NoError) {
        response.setParseError(m_serializer.deserializationErrorString());
        return response.serialize(&m_serializer);
    }

    response.setProtobufPayload(m_serializer.serializeRawMessage(msg.get()));
    return response.serialize(&m_serializer);
}

int main(int argc, char *argv[])
{
    // Ensure we don't output anything to stdout
    qInstallMessageHandler([](QtMsgType, const QMessageLogContext &context, const QString &msg) {
        QByteArray localMsg = msg.toLocal8Bit();
        const char *file = context.file ? context.file : "";
        const char *function = context.function ? context.function : "";
        fprintf(stderr, "%s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        return;
    });

    QCoreApplication app(argc, argv);
    ConformaceServer server;
    return app.exec();
}
