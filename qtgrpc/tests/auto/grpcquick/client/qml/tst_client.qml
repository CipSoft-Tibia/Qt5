// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtTest
import QmlTestUri
import qtgrpc.tests

Item {
    id: root

    Timer {
        id: timer
        running: false
        repeat: false
        interval: 10000
        onTriggered:  testCase.when = true;
    }

    property simpleStringMessage messageArg;
    property simpleStringMessage messageResponse;

    property bool calbackCalled: false
    property var clientQml
    property var grpcChannel
    property var grpcChannelDeadline
    property var setResponse: function(value) { root.messageResponse = value; testCase.when = true; }
    property var errorCallback: function() { root.calbackCalled = true; testCase.when = true; }

    function createClientItem() {
        return Qt.createQmlObject("import QtQuick; import QtGrpc; \
                                   import qtgrpc.tests; TestServiceClient {}", root)
    }

    function createGrpcChannelItem() {
        return Qt.createQmlObject("import QtQuick; import QtGrpc; GrpcHttp2Channel { \
                                   options: GrpcChannelOptions { \
                                   host: \"http://localhost:50051\"; \
                                   deadline: { 2000 } } }", root)
    }

    function createGrpcChannelWithDeadlineItem() {
        return Qt.createQmlObject("import QtQuick; import QtGrpc; GrpcHttp2Channel { \
                                   options: GrpcChannelOptions { \
                                   host: \"http://localhost:50051\"; \
                                   deadline: { 1000 } }  }", root)
    }

    TestCase {
        name: "qtgrpcClientRegistration"
        function test_1clientTypes_data() {
            return [
                        { tag: "Grpc Client created",
                            field: typeof clientQml, answer: "object" },
                        { tag: "Grpc Http2 Channel created",
                            field: typeof grpcChannel, answer: "object" },
                        { tag: "Grpc Http2 Deadline Channel created",
                            field: typeof grpcChannelDeadline, answer: "object" }
                    ]
        }

        function test_1clientTypes(data) {
            compare(data.field, data.answer)
        }

        function test_ChannelOptions_data() {
            return [
                        { tag: "grpcChannelOptions URL is set",
                            field: grpcChannel.options.host, answer: "http://localhost:50051" },
                        { tag: "grpcChannelOptions deadline is set",
                            field: grpcChannelDeadline.options.deadline, answer: 1000 }
                    ]
        }

        function test_ChannelOptions(data) {
            compare(data.field, data.answer)
        }

        function test_testMethodCall() {
            clientQml.testMethod(root.messageArg, root.setResponse, root.errorCallback);
            timer.start()
        }
    }

    TestCase {
        id: testCase
        name: "qtgrpcClientTestCall"
        when: false

        function test_testMethodCallCheck() {
            verify(root.messageResponse == root.messageArg)
            verify(!root.calbackCalled)
        }
    }

    Component.onCompleted: {
        clientQml = root.createClientItem()
        grpcChannel = root.createGrpcChannelItem()
        grpcChannelDeadline = root.createGrpcChannelWithDeadlineItem()
        clientQml.channel = grpcChannel.channel
    }
}
