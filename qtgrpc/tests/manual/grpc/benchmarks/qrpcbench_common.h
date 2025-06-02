// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QRPCBENCH_COMMON_H
#define QRPCBENCH_COMMON_H

#include <QtCore/qcommandlineoption.h>
#include <QtCore/qcommandlineparser.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qsysinfo.h>

#include <chrono>
#include <concepts>
#include <format>
#include <iostream>
#include <string_view>

static constexpr std::string_view HostUri = "localhost:65002";

namespace Client {

template <typename T>
concept ClientConcept = requires(T t) {
    { T(uint64_t()) };
    { t.unaryCall() } -> std::same_as<void>;
    { t.serverStreaming() } -> std::same_as<void>;
    { t.clientStreaming() } -> std::same_as<void>;
    { t.bidiStreaming() } -> std::same_as<void>;
};

template <ClientConcept C>
inline void benchmarkMain(std::string_view name, int argc, char *argv[])
{
    QStringList args;
    for (int i = 0; i < argc; ++i)
        args.push_back(argv[i]);

    QCommandLineParser parser;
    parser.setApplicationDescription(name.data());
    parser.addHelpOption();

    QCommandLineOption calls({ "c", "calls" }, "Amount of calls made.", "size", "1000");
    QCommandLineOption payload({ "p", "payload" }, "Payload size in bytes", "size", "0");
    QCommandLineOption uniqueRpc({ "u", "unique" }, "Make each RPC on a fresh client");

    QCommandLineOption enableUnary("U", "Enable UnaryCalls");
    QCommandLineOption enableSStream("S", "Enable ServerStream");
    QCommandLineOption enableCStream("C", "Enable ClientStream");
    QCommandLineOption enableBStream("B", "Enable BiDiStream");

    parser.addOption(calls);
    parser.addOption(payload);
    parser.addOption(uniqueRpc);

    parser.addOption(enableUnary);
    parser.addOption(enableSStream);
    parser.addOption(enableCStream);
    parser.addOption(enableBStream);

    parser.process(args);

    bool defaultRun = !parser.isSet(enableUnary) && !parser.isSet(enableSStream)
        && !parser.isSet(enableCStream) && !parser.isSet(enableBStream);
    uint64_t amountCalls = parser.value(calls).toULong();
    qsizetype payloadSize = parser.value(payload).toLong();

    std::cout << std::format("#### Start of {} benchmark ####\n", name);
    std::cout << std::format("  cpu-arch: {}\n", QSysInfo::buildCpuArchitecture().toStdString());
    std::cout << std::format("  kernel: {}, {}\n", QSysInfo::kernelType().toStdString(),
                             QSysInfo::kernelVersion().toStdString());
    std::cout << std::format("  host URI: {}\n\n", HostUri);
    if (parser.isSet(payload))
        std::cout << std::format("  Option: payload per message {} bytes\n", payloadSize);
    if (parser.isSet(uniqueRpc))
        std::cout << std::format("  Option: unique client per RPC {}\n", parser.isSet(uniqueRpc));
    if (parser.isSet(payload) || parser.isSet(uniqueRpc))
        std::cout << "\n";

    if (parser.isSet(uniqueRpc)) {
        {
            C client(amountCalls, payloadSize);
            if (defaultRun || parser.isSet(enableUnary))
                client.unaryCall();
        }
        {
            C client(amountCalls, payloadSize);
            if (defaultRun || parser.isSet(enableSStream))
                client.serverStreaming();
        }
        {
            C client(amountCalls, payloadSize);
            if (defaultRun || parser.isSet(enableCStream))
                client.clientStreaming();
        }
        {
            C client(amountCalls, payloadSize);
            if (defaultRun || parser.isSet(enableBStream))
                client.bidiStreaming();
        }
    } else {
        C client(amountCalls, payloadSize);
        if (defaultRun || parser.isSet(enableUnary))
            client.unaryCall();
        if (defaultRun || parser.isSet(enableSStream))
            client.serverStreaming();
        if (defaultRun || parser.isSet(enableCStream))
            client.clientStreaming();
        if (defaultRun || parser.isSet(enableBStream))
            client.bidiStreaming();
    }

    std::cout << std::format("\n#### End of {} benchmark ####\n", name);
}

inline void printRpcResult(std::string benched, int64_t elapsedNs, uint64_t amountCalls,
                           uint64_t recvBytes = 0, uint64_t sendBytes = 0)
{
    using namespace std::chrono;

    std::cout << std::format("Finished benchmark: {}\n", benched);
    const auto ns = nanoseconds(elapsedNs);
    const auto us = duration_cast<microseconds>(ns);
    std::cout << std::format("  Completed calls: {}\n", amountCalls);
    std::cout << std::format("  Total time: {}, {}\n", us, ns);
    const auto avgNs = nanoseconds(ns / amountCalls);
    const auto avgUs = duration_cast<microseconds>(avgNs);
    std::cout << std::format("  Average time: {}, {}\n", avgUs, avgNs);
    if (recvBytes > 0 && sendBytes > 0) {
        std::cout << std::format("  Send bytes: {}, Recv bytes: {}\n", sendBytes, recvBytes);
        const auto sec = duration_cast<duration<double>>(ns);
        double totalTimeSec = sec.count();
        double totalBytes = static_cast<double>(recvBytes + sendBytes);
        double throughputKBPerSec = (totalBytes / 1024) / totalTimeSec;
        std::cout << std::format("  Throughput (KB/sec): {:.2f}\n", throughputKBPerSec);
    }
}

} // namespace Client

#endif // QRPCBENCH_COMMON_H
