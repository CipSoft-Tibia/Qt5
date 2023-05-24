// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtTest

import QmlTestUri

pragma ValueTypeBehavior: Addressable

TestCase {
    name: "qtprotobufBasicTest"
    property simpleBoolMessage boolMsg;
    property simpleIntMessage int32Msg;
    property simpleSIntMessage sint32Msg;
    property simpleUIntMessage uint32Msg;
    property simpleFixedInt32Message fixed32Msg;
    property simpleSFixedInt32Message sfixed32Msg;
    property simpleStringMessage stringMsg;
    property complexMessage complexMsg;
    property simpleStringMessage outerMessage;
    property simpleStringMessage innerMessage;
    property complexMessage complexMsgNoInit;
    property simpleInt64Message int64Msg;
    property simpleSInt64Message sint64Msg;
    property simpleUInt64Message uint64Msg;
    property simpleFixedInt64Message fixed64Msg;
    property simpleSFixedInt64Message sfixed64Msg;

    // Messages with fields not compatible with QML
    property simpleBytesMessage bytesMessage;

    function test_1initialization() {
        // 32 size
        int32Msg.testFieldInt = 2147483647
        sint32Msg.testFieldInt = 2147483647
        uint32Msg.testFieldInt = 4294967295
        fixed32Msg.testFieldFixedInt32 = 4294967295
        sfixed32Msg.testFieldFixedInt32 = 2147483647

        // 64 size
        int64Msg.testFieldInt = 2147483647
        sint64Msg.testFieldInt = 2147483647
        uint64Msg.testFieldInt = 4294967295
        fixed64Msg.testFieldFixedInt64 = 4294967295
        sfixed64Msg.testFieldFixedInt64 = 2147483647

        // strings
        stringMsg.testFieldString = "Test string"
        outerMessage.testFieldString = "outer"
        innerMessage.testFieldString = "inner"
        complexMsg.testComplexField = innerMessage
        complexMsg.testComplexField.testFieldString = "inner"
    }

    function test_basicTypesOperations_data() {
        return [
                    // int32
                    {
                        tag: "int32 '=' operation",
                        field: int32Msg.testFieldInt = 10,
                        answer: 10
                    },
                    {
                        tag: "int32 '-' operation",
                        field: int32Msg.testFieldInt
                               = int32Msg.testFieldInt - int32Msg.testFieldInt,
                        answer: 0
                    },
                    {
                        tag: "int32 '+' operation",
                        field: int32Msg.testFieldInt = int32Msg.testFieldInt + 10,
                        answer: 10
                    },
                    {
                        tag: "int32 '/' operation",
                        field: int32Msg.testFieldInt
                               = int32Msg.testFieldInt / int32Msg.testFieldInt,
                        answer: 1
                    },
                    {
                        tag: "int32 '*' operation",
                        field: int32Msg.testFieldInt = int32Msg.testFieldInt * 10,
                        answer: 10
                    },
                    {
                        tag: "int32 '--' operation",
                        field: --int32Msg.testFieldInt,
                        answer: 9
                    },
                    {
                        tag: "int32 '++' operation",
                        field: ++int32Msg.testFieldInt,
                        answer: 10
                    },
                    // sint32
                    {
                        tag: "sint32 '=' operation",
                        field: sint32Msg.testFieldInt = 100,
                        answer: 100
                    },
                    {
                        tag: "sint32 '-' operation",
                        field: sint32Msg.testFieldInt
                               = sint32Msg.testFieldInt - sint32Msg.testFieldInt,
                        answer: 0
                    },
                    {
                        tag: "sint32 '+' operation",
                        field: sint32Msg.testFieldInt
                               = sint32Msg.testFieldInt + 100,
                        answer: 100
                    },
                    {
                        tag: "sint32 '/' operation",
                        field: sint32Msg.testFieldInt
                               = sint32Msg.testFieldInt / sint32Msg.testFieldInt,
                        answer: 1
                    },
                    {
                        tag: "sint32 '*' operation",
                        field: sint32Msg.testFieldInt = sint32Msg.testFieldInt * 10,
                        answer: 10
                    },
                    {
                        tag: "sint32 '--' operation",
                        field: --sint32Msg.testFieldInt,
                        answer: 9
                    },
                    {
                        tag: "sint32 '++' operation",
                        field: ++sint32Msg.testFieldInt,
                        answer: 10
                    },
                    // uint32
                    {
                        tag: "uint32 '=' operation",
                        field: uint32Msg.testFieldInt = 100,
                        answer: 100
                    },
                    {
                        tag: "uint32 '-' operation",
                        field: uint32Msg.testFieldInt
                               = uint32Msg.testFieldInt - uint32Msg.testFieldInt,
                        answer: 0
                    },
                    {
                        tag: "uint32 '+' operation",
                        field: uint32Msg.testFieldInt
                               = uint32Msg.testFieldInt + 100,
                        answer: 100
                    },
                    {
                        tag: "uint32 '/' operation",
                        field: uint32Msg.testFieldInt
                               = uint32Msg.testFieldInt / uint32Msg.testFieldInt,
                        answer: 1
                    },
                    {
                        tag: "uint32 '*' operation",
                        field: uint32Msg.testFieldInt = uint32Msg.testFieldInt * 10,
                        answer: 10
                    },
                    {
                        tag: "uint32 '--' operation",
                        field: --uint32Msg.testFieldInt,
                        answer: 9
                    },
                    {
                        tag: "uint32 '++' operation",
                        field: ++uint32Msg.testFieldInt,
                        answer: 10
                    },
                    // fixedInt32
                    {
                        tag: "fixedInt32 '=' operation",
                        field: fixed32Msg.testFieldFixedInt32 = 1000,
                        answer: 1000
                    },
                    {
                        tag: "fixedInt32 '-' operation",
                        field: fixed32Msg.testFieldFixedInt32
                               = fixed32Msg.testFieldFixedInt32 - fixed32Msg.testFieldFixedInt32,
                        answer: 0
                    },
                    {
                        tag: "fixedInt32 '+' operation",
                        field: fixed32Msg.testFieldFixedInt32
                               = fixed32Msg.testFieldFixedInt32 + 1000,
                        answer: 1000
                    },
                    {
                        tag: "fixedInt32 '/' operation",
                        field: fixed32Msg.testFieldFixedInt32
                               = fixed32Msg.testFieldFixedInt32 / fixed32Msg.testFieldFixedInt32,
                        answer: 1
                    },
                    {
                        tag: "fixedInt32 '*' operation",
                        field: fixed32Msg.testFieldFixedInt32
                               = fixed32Msg.testFieldFixedInt32 * 10000,
                        answer: 10000
                    },
                    {
                        tag: "fixedInt32 '--' operation",
                        field: --fixed32Msg.testFieldFixedInt32,
                        answer: 9999
                    },
                    {
                        tag: "fixedInt32 '++' operation",
                        field: ++fixed32Msg.testFieldFixedInt32,
                        answer: 10000
                    },
                    // sfixed32
                    {
                        tag: "sfixed32 '=' operation",
                        field: sfixed32Msg.testFieldFixedInt32 = 1000,
                        answer: 1000
                    },
                    {
                        tag: "sfixed32 '-' operation",
                        field: sfixed32Msg.testFieldFixedInt32
                               = sfixed32Msg.testFieldFixedInt32 - sfixed32Msg.testFieldFixedInt32,
                        answer: 0
                    },
                    {
                        tag: "sfixed32 '+' operation",
                        field: sfixed32Msg.testFieldFixedInt32
                               = sfixed32Msg.testFieldFixedInt32 + 1000,
                        answer: 1000
                    },
                    {
                        tag: "sfixed32 '/' operation",
                        field: sfixed32Msg.testFieldFixedInt32
                               = sfixed32Msg.testFieldFixedInt32 / sfixed32Msg.testFieldFixedInt32,
                        answer: 1
                    },
                    {
                        tag: "sfixed32 '*' operation",
                        field: sfixed32Msg.testFieldFixedInt32
                               = sfixed32Msg.testFieldFixedInt32 * 10000,
                        answer: 10000
                    },
                    {
                        tag: "sfixed32 '--' operation",
                        field: --sfixed32Msg.testFieldFixedInt32,
                        answer: 9999
                    },
                    {
                        tag: "sfixed32 '++' operation",
                        field: ++sfixed32Msg.testFieldFixedInt32,
                        answer: 10000
                    },
                    // int64
                    {
                        tag: "int64 '=' operation",
                        field: int64Msg.testFieldInt = 1000,
                        answer: 1000
                    },
                    {
                        tag: "int64 '-' operation",
                        field: int64Msg.testFieldInt
                               = int64Msg.testFieldInt - int64Msg.testFieldInt,
                        answer: 0
                    },
                    {
                        tag: "int64 '+' operation",
                        field: int64Msg.testFieldInt = int64Msg.testFieldInt + 1010,
                        answer: 1010
                    },
                    {
                        tag: "int64 '/' operation",
                        field: int64Msg.testFieldInt
                               = int64Msg.testFieldInt / int64Msg.testFieldInt,
                        answer: 1
                    },
                    {
                        tag: "int64 '*' operation",
                        field: int64Msg.testFieldInt = int64Msg.testFieldInt * 10,
                        answer: 10
                    },
                    {
                        tag: "int64 '--' operation",
                        field: --int64Msg.testFieldInt,
                        answer: 9
                    },
                    {
                        tag: "int64 '++' operation",
                        field: ++int64Msg.testFieldInt,
                        answer: 10
                    },
                    // sint64
                    {
                        tag: "sint64 '=' operation",
                        field: sint64Msg.testFieldInt = 100,
                        answer: 100
                    },
                    {
                        tag: "sint64 '-' operation",
                        field: sint64Msg.testFieldInt
                               = sint64Msg.testFieldInt - sint64Msg.testFieldInt,
                        answer: 0
                    },
                    {
                        tag: "sint64 '+' operation",
                        field: sint64Msg.testFieldInt
                               = sint64Msg.testFieldInt + 100,
                        answer: 100
                    },
                    {
                        tag: "sint64 '/' operation",
                        field: sint64Msg.testFieldInt
                               = sint64Msg.testFieldInt / sint64Msg.testFieldInt,
                        answer: 1
                    },
                    {
                        tag: "sint64 '*' operation",
                        field: sint64Msg.testFieldInt = sint64Msg.testFieldInt * 10,
                        answer: 10
                    },
                    {
                        tag: "sint64 '--' operation",
                        field: --sint64Msg.testFieldInt,
                        answer: 9
                    },
                    {
                        tag: "sint64 '++' operation",
                        field: ++sint64Msg.testFieldInt,
                        answer: 10
                    },
                    // uint64
                    {
                        tag: "uint64 '=' operation",
                        field: uint64Msg.testFieldInt = 100,
                        answer: 100
                    },
                    {
                        tag: "uint64 '-' operation",
                        field: uint64Msg.testFieldInt
                               = uint64Msg.testFieldInt - uint64Msg.testFieldInt,
                        answer: 0
                    },
                    {
                        tag: "uint64 '+' operation",
                        field: uint64Msg.testFieldInt
                               = uint64Msg.testFieldInt + 100,
                        answer: 100
                    },
                    {
                        tag: "uint64 '/' operation",
                        field: uint64Msg.testFieldInt
                               = uint64Msg.testFieldInt / uint64Msg.testFieldInt,
                        answer: 1
                    },
                    {
                        tag: "uint64 '*' operation",
                        field: uint64Msg.testFieldInt = uint64Msg.testFieldInt * 10,
                        answer: 10
                    },
                    {
                        tag: "uint64 '--' operation",
                        field: --uint64Msg.testFieldInt,
                        answer: 9
                    },
                    {
                        tag: "uint64 '++' operation",
                        field: ++uint64Msg.testFieldInt,
                        answer: 10
                    },
                    // fixedInt64
                    {
                        tag: "fixedInt64 '=' operation",
                        field: fixed64Msg.testFieldFixedInt64 = 1000,
                        answer: 1000
                    },
                    {
                        tag: "fixedInt64 '-' operation",
                        field: fixed64Msg.testFieldFixedInt64
                               = fixed64Msg.testFieldFixedInt64 - fixed64Msg.testFieldFixedInt64,
                        answer: 0
                    },
                    {
                        tag: "fixedInt64 '+' operation",
                        field: fixed64Msg.testFieldFixedInt64
                               = fixed64Msg.testFieldFixedInt64 + 1000,
                        answer: 1000
                    },
                    {
                        tag: "fixedInt64 '/' operation",
                        field: fixed64Msg.testFieldFixedInt64
                               = fixed64Msg.testFieldFixedInt64 / fixed64Msg.testFieldFixedInt64,
                        answer: 1
                    },
                    {
                        tag: "fixedInt64 '*' operation",
                        field: fixed64Msg.testFieldFixedInt64
                               = fixed64Msg.testFieldFixedInt64 * 10000,
                        answer: 10000
                    },
                    {
                        tag: "fixedInt64 '--' operation",
                        field: --fixed64Msg.testFieldFixedInt64,
                        answer: 9999
                    },
                    {
                        tag: "fixedInt64 '++' operation",
                        field: ++fixed64Msg.testFieldFixedInt64,
                        answer: 10000
                    },
                    // sfixed64
                    {
                        tag: "sfixed64 '=' operation",
                        field: sfixed64Msg.testFieldFixedInt64 = 1000,
                        answer: 1000
                    },
                    {
                        tag: "sfixed64 '-' operation",
                        field: sfixed64Msg.testFieldFixedInt64
                               = sfixed64Msg.testFieldFixedInt64 - sfixed64Msg.testFieldFixedInt64,
                        answer: 0
                    },
                    {
                        tag: "sfixed64 '+' operation",
                        field: sfixed64Msg.testFieldFixedInt64
                               = sfixed64Msg.testFieldFixedInt64 + 1000,
                        answer: 1000
                    },
                    {
                        tag: "sfixed64 '/' operation",
                        field: sfixed64Msg.testFieldFixedInt64
                               = sfixed64Msg.testFieldFixedInt64 / sfixed64Msg.testFieldFixedInt64,
                        answer: 1
                    },
                    {
                        tag: "sfixed64 '*' operation",
                        field: sfixed64Msg.testFieldFixedInt64
                               = sfixed64Msg.testFieldFixedInt64 * 10000,
                        answer: 10000
                    },
                    {
                        tag: "sfixed64 '--' operation",
                        field: --sfixed64Msg.testFieldFixedInt64,
                        answer: 9999
                    },
                    {
                        tag: "sfixed64 '++' operation",
                        field: ++sfixed64Msg.testFieldFixedInt64,
                        answer: 10000
                    }
                ]
    }

    function test_basicTypesOperations(data) {
        compare(data.field, data.answer)
    }

    function test_protobufTypesTypeCheck_data() {
        return [
                    {
                        tag: "SimpleBoolMessage testFieldBool type",
                        field: typeof boolMsg.testFieldBool,
                        answer: "boolean"
                    },
                    {
                        tag: "SimpleIntMessage testFieldInt type",
                        field: typeof int32Msg.testFieldInt,
                        answer: "number"
                    },
                    {
                        tag: "SimpleSIntMessage testFieldInt type",
                        field: typeof sint32Msg.testFieldInt,
                        answer: "number"
                    },
                    {
                        tag: "SimpleUIntMessage testFieldInt type",
                        field: typeof uint32Msg.testFieldInt,
                        answer: "number"
                    },
                    {
                        tag: "SimpleFixedInt32Message testFieldInt type",
                        field: typeof fixed32Msg.testFieldFixedInt32,
                        answer: "number"
                    },
                    {
                        tag: "SimpleSFixedInt32Message testFieldInt type",
                        field: typeof sfixed32Msg.testFieldFixedInt32,
                        answer: "number"
                    },
                    {
                        tag: "SimpleStringMessage testFieldString type",
                        field: typeof stringMsg.testFieldString,
                        answer: "string"
                    },
                    {
                        tag: "SimpleSInt64Message testFieldInt type",
                        field: typeof sint64Msg.testFieldInt,
                        answer: "number"
                    },
                    {
                        tag: "SimpleUInt64Message testFieldInt type",
                        field: typeof uint64Msg.testFieldInt,
                        answer: "number"
                    },
                    {
                        tag: "SimpleInt64Message testFieldInt type",
                        field: typeof int64Msg.testFieldInt,
                        answer: "number"
                    },
                    {
                        tag: "SimpleFixedInt64Message testFieldInt type",
                        field: typeof fixed64Msg.testFieldFixedInt64,
                        answer: "number"
                    },
                    {
                        tag: "SimpleSFixedInt64Message testFieldInt type",
                        field: typeof sfixed64Msg.testFieldFixedInt64,
                        answer: "number"
                    }
                ]
    }

    function test_protobufTypesTypeCheck(data) {
        //TODO: int64, fixed64, sfixed64 not recognized as numbers. See QTBUG-113516.
        expectFail("SimpleInt64Message testFieldInt type",
            "Proper type support is not implemented")
        expectFail("SimpleFixedInt64Message testFieldInt type",
            "Proper type support is not implemented")
        expectFail("SimpleSFixedInt64Message testFieldInt type",
            "Proper type support is not implemented")
        compare(data.field, data.answer)
    }

    function test_unsupportedProtobufTypesTypeCheck_data() {
        // The below types contain fields that are not supported by QML so the property is not
        // accessible but we can still check that it's valid. typeof check returns 'object' for
        // these fields.
        return [
                    {
                        tag: "SimpleBytesMessage testFieldBytes type",
                        field: typeof bytesMessage.testFieldBytes,
                        answer: "object"
                    },
                ]
    }

    function test_unsupportedProtobufTypesTypeCheck(data) {
        compare(data.field, data.answer)
    }

    function test_complexMessageTypeCheck_data() {
        return [
                    {
                        tag: "ComplexMessage testComplexField type",
                        field: typeof complexMsgNoInit.testComplexField,
                        answer: "object"
                    },
                    {
                        tag: "ComplexMessage testComplexField exact type",
                        field: complexMsgNoInit.testComplexField as simpleStringMessage,
                        answer: "qtprotobufnamespace::tests::SimpleStringMessage()"
                    },
                    {
                        tag: "ComplexMessage testComplexField exact invalid type",
                        field: complexMsgNoInit.testComplexField as simpleSIntMessage,
                        answer: null
                    },
                    {
                        tag: "ComplexMessage testComplexField.testFieldString type",
                        field: typeof complexMsgNoInit.testComplexField.testFieldString,
                        answer: "string"
                    },
                    {
                        tag: "ComplexMessage testFieldInt type",
                        field: typeof complexMsgNoInit.testFieldInt,
                        answer: "number"
                    },
                ]
    }

    function test_complexMessageTypeCheck(data) {
        compare(data.field, data.answer)
    }

    function test_1initializationCheck_data() {
        return [
                    {tag: "SimpleIntMessage initialization",
                        field: int32Msg.testFieldInt, answer: 2147483647 },
                    {tag: "SimpleSIntMessage initialization",
                        field: sint32Msg.testFieldInt, answer: 2147483647 },
                    {tag: "SimpleUIntMessage initialization",
                        field: uint32Msg.testFieldInt, answer: 4294967295 },
                    {tag: "SimpleFixedInt32Message initialization",
                        field: fixed32Msg.testFieldFixedInt32, answer: 4294967295 },
                    {tag: "SimpleSFixedInt32Message initialization",
                        field: sfixed32Msg.testFieldFixedInt32, answer: 2147483647 },
                ]
    }

    function test_1initializationCheck(data) {
        compare(data.field, data.answer)
    }

    function test_simpleboolmessage_data() {
        return [
                    {tag: "SimpleBoolMessage == true",
                        field: boolMsg.testFieldBool = true, answer: true },
                    {tag: "SimpleBoolMessage == false",
                        field: boolMsg.testFieldBool = false, answer: false },
                ]
    }

    function test_simpleboolmessage(data) {
        compare(data.field, data.answer)
    }

    function test_simpleintmessage_data() {
        return [
                    {tag: "SimpleIntMessage == 0",
                        field: int32Msg.testFieldInt = 0, answer: 0 },
                    {tag: "SimpleIntMessage == -128",
                        field: int32Msg.testFieldInt = -128, answer: -128 },
                    {tag: "SimpleIntMessage == 127",
                        field: int32Msg.testFieldInt = 127, answer: 127 },
                    {tag: "SimpleIntMessage == -256",
                        field: int32Msg.testFieldInt = -256, answer: -256 },
                    {tag: "SimpleIntMessage == 255",
                        field: int32Msg.testFieldInt = 255, answer: 255 },
                    {tag: "SimpleIntMessage == -32768",
                        field: int32Msg.testFieldInt = -32768, answer: -32768 },
                    {tag: "SimpleIntMessage == 32767",
                        field: int32Msg.testFieldInt = 32767, answer: 32767 },
                    {tag: "SimpleIntMessage == -65536",
                        field: int32Msg.testFieldInt = -65536, answer: -65536 },
                    {tag: "SimpleIntMessage == 65535",
                        field: int32Msg.testFieldInt = 65535, answer: 65535 },
                    {tag: "SimpleIntMessage == -2147483648",
                        field: int32Msg.testFieldInt = -2147483648, answer: -2147483648 },
                    {tag: "SimpleIntMessage == 2147483647",
                        field: int32Msg.testFieldInt = 2147483647, answer: 2147483647 },
                ]
    }

    function test_simpleintmessage(data) {
        compare(data.field, data.answer)
    }

    function test_simplesintmessage_data() {
        return [
                    {tag: "SimpleSIntMessage == 0",
                        field: sint32Msg.testFieldInt = 0, answer: 0 },
                    {tag: "SimpleSIntMessage == -128",
                        field: sint32Msg.testFieldInt = -128, answer: -128 },
                    {tag: "SimpleSIntMessage == 127",
                        field: sint32Msg.testFieldInt = 127, answer: 127 },
                    {tag: "SimpleSIntMessage == -256",
                        field: sint32Msg.testFieldInt = -256, answer: -256 },
                    {tag: "SimpleSIntMessage == 255",
                        field: sint32Msg.testFieldInt = 255, answer: 255 },
                    {tag: "SimpleSIntMessage == -32768",
                        field: sint32Msg.testFieldInt = -32768, answer: -32768 },
                    {tag: "SimpleSIntMessage == 32767",
                        field: sint32Msg.testFieldInt = 32767, answer: 32767 },
                    {tag: "SimpleSIntMessage == -65536",
                        field: sint32Msg.testFieldInt = -65536, answer: -65536 },
                    {tag: "SimpleSIntMessage == 65535",
                        field: sint32Msg.testFieldInt = 65535, answer: 65535 },
                    {tag: "SimpleSIntMessage == -2147483648",
                        field: sint32Msg.testFieldInt = -2147483648, answer: -2147483648 },
                    {tag: "SimpleSIntMessage == 2147483647",
                        field: sint32Msg.testFieldInt = 2147483647, answer: 2147483647 },
                ]
    }

    function test_simplesintmessage(data) {
        compare(data.field, data.answer)
    }

    function test_simpleuintmessage_data() {
        return [
                    {tag: "SimpleUIntMessage == 0",
                        field: uint32Msg.testFieldInt = 0, answer: 0 },
                    {tag: "SimpleUIntMessage == 127",
                        field: uint32Msg.testFieldInt = 127, answer: 127 },
                    {tag: "SimpleUIntMessage == 255",
                        field: uint32Msg.testFieldInt = 255, answer: 255 },
                    {tag: "SimpleUIntMessage == 32767",
                        field: uint32Msg.testFieldInt = 32767, answer: 32767 },
                    {tag: "SimpleUIntMessage == 65535",
                        field: uint32Msg.testFieldInt = 65535, answer: 65535 },
                    {tag: "SimpleUIntMessage == -2147483648",
                        field: uint32Msg.testFieldInt = 2147483647, answer: 2147483647 },
                    {tag: "SimpleUIntMessage == 4294967295",
                        field: uint32Msg.testFieldInt = 4294967295, answer: 4294967295 },
                ]
    }

    function test_simpleuintmessage(data) {
        compare(data.field, data.answer)
    }

    function test_simplefixed32message_data() {
        return [
                    {tag: "SimpleFixedInt32Message == 0",
                        field: fixed32Msg.testFieldInt = 0, answer: 0 },
                    {tag: "SimpleFixedInt32Message == 127",
                        field: fixed32Msg.testFieldInt = 127, answer: 127 },
                    {tag: "SimpleFixedInt32Message == 255",
                        field: fixed32Msg.testFieldInt = 255, answer: 255 },
                    {tag: "SimpleFixedInt32Message == 32767",
                        field: fixed32Msg.testFieldInt = 32767, answer: 32767 },
                    {tag: "SimpleFixedInt32Message == 65535",
                        field: fixed32Msg.testFieldInt = 65535, answer: 65535 },
                    {tag: "SimpleFixedInt32Message == -2147483648",
                        field: fixed32Msg.testFieldInt = 2147483647, answer: 2147483647 },
                    {tag: "SimpleFixedInt32Message == 4294967295",
                        field: fixed32Msg.testFieldInt = 4294967295, answer: 4294967295 },
                ]
    }

    function test_simplefixed32message(data) {
        compare(data.field, data.answer)
    }

    function test_simplesfixed32message_data() {
        return [
                    {tag: "SimpleSFixedInt32Message == 0",
                        field: sfixed32Msg.testFieldInt = 0, answer: 0 },
                    {tag: "SimpleSFixedInt32Message == -128",
                        field: sfixed32Msg.testFieldInt = -128, answer: -128 },
                    {tag: "SimpleSFixedInt32Message == 127",
                        field: sfixed32Msg.testFieldInt = 127, answer: 127 },
                    {tag: "SimpleSFixedInt32Message == -256",
                        field: sfixed32Msg.testFieldInt = -256, answer: -256 },
                    {tag: "SimpleSFixedInt32Message == 255",
                        field: sfixed32Msg.testFieldInt = 255, answer: 255 },
                    {tag: "SimpleSFixedInt32Message == -32768",
                        field: sfixed32Msg.testFieldInt = -32768, answer: -32768 },
                    {tag: "SimpleSFixedInt32Message == 32767",
                        field: sfixed32Msg.testFieldInt = 32767, answer: 32767 },
                    {tag: "SimpleSFixedInt32Message == -65536",
                        field: sfixed32Msg.testFieldInt = -65536, answer: -65536 },
                    {tag: "SimpleSFixedInt32Message == 65535",
                        field: sfixed32Msg.testFieldInt = 65535, answer: 65535 },
                    {tag: "SimpleSFixedInt32Message == -2147483648",
                        field: sfixed32Msg.testFieldInt = -2147483648, answer: -2147483648 },
                    {tag: "SimpleSFixedInt32Message == 2147483647",
                        field: sfixed32Msg.testFieldInt = 2147483647, answer: 2147483647 },
                ]
    }
    function test_simplesfixed32message(data) {
        compare(data.field, data.answer)
    }

    function test_simplesstringmessage() {
        compare(stringMsg.testFieldString, "Test string", "SimpleStringMessage")
    }

    function test_int32ImplicitConversion_data() {
        return [
                    {tag: "int32ImplicitConversion int32Msg == 0",
                        field: int32Msg.testFieldInt = 0, answer: false },
                    {tag: "int32ImplicitConversion int32Msg == 1",
                        field: int32Msg.testFieldInt = 1, answer: true },
                ]
    }

    function test_int32ImplicitConversion(data) {
        compare(data.field ? true : false, data.answer,
                "Invalid implicit conversion: " + data.field + " should be false")
        compare(data.field ? true : false, data.answer,
                "Invalid implicit conversion: " + data.field + " should be true")
    }

    function test_int32LocaleStringConversion() {
        compare(int32Msg.testFieldInt.toLocaleString(Qt.locale()),
                Number(int32Msg.testFieldInt).toLocaleString(Qt.locale()),
                "Locale number string is not match "
                + int32Msg.testFieldInt.toLocaleString(Qt.locale())
                + " != " + Number(int32Msg.testFieldInt).toLocaleString(Qt.locale()))
    }

    function test_fixed32ImplicitConversion_data() {
        return [
                    {tag: "fixed32ImplicitConversion fixed32Msg == 0",
                        field: fixed32Msg.testFieldFixedInt32 = 0, answer: false },
                    {tag: "fixed32ImplicitConversion fixed32Msg == 1",
                        field: fixed32Msg.testFieldFixedInt32 = 1, answer: true },
                ]
    }

    function test_fixed32ImplicitConversion(data) {
        compare(data.field ? true : false, data.answer,
                "Invalid implicit conversion: " + data.field + " should be false")
        compare(data.field ? true : false, data.answer,
                "Invalid implicit conversion: " + data.field + " should be true")
    }

    function test_fixed32LocaleStringConversion() {
        compare(fixed32Msg.testFieldFixedInt32.toLocaleString(Qt.locale()),
                Number(fixed32Msg.testFieldFixedInt32).toLocaleString(Qt.locale()),
                "Locale number string is not match "
                + fixed32Msg.testFieldFixedInt32.toLocaleString(Qt.locale())
                + " != " + Number(fixed32Msg.testFieldFixedInt32).toLocaleString(Qt.locale()))
    }

    function test_sint32ImplicitConversion_data() {
        return [
                    {tag: "sint32ImplicitConversion testFieldInt == 0",
                        field: fixed32Msg.testFieldInt = 0, answer: false },
                    {tag: "sint32ImplicitConversion testFieldInt == 1",
                        field: fixed32Msg.testFieldInt = 1, answer: true },
                ]
    }

    function test_sint32ImplicitConversion(data) {
        compare(data.field ? true : false, data.answer,
                "Invalid implicit conversion: " + data.field + " should be false")
        compare(data.field ? true : false, data.answer,
                "Invalid implicit conversion: " + data.field + " should be true")
    }

    function test_sint32LocaleStringConversion() {
        compare(sint32Msg.testFieldInt.toLocaleString(Qt.locale()),
                Number(sint32Msg.testFieldInt).toLocaleString(Qt.locale()),
                "Locale number string is not match "
                + sint32Msg.testFieldInt.toLocaleString(Qt.locale())
                + " != " + Number(sint32Msg.testFieldInt).toLocaleString(Qt.locale()))
    }

    function test_sfixed32ImplicitConversion_data() {
        return [
                    {tag: "sfixed32ImplicitConversion sfixed32Msg == 0",
                        field: sfixed32Msg.testFieldInt = 0, answer: false },
                    {tag: "sfixed32ImplicitConversion sfixed32Msg == 1",
                        field: sfixed32Msg.testFieldInt = 1, answer: true },
                ]
    }

    function test_sfixed32ImplicitConversion(data) {
        compare(data.field ? true : false, data.answer,
                "Invalid implicit conversion: " + data.field + " should be false")
        compare(data.field ? true : false, data.answer,
                "Invalid implicit conversion: " + data.field + " should be true")
    }

    function test_sfixed32LocaleStringConversion() {
        compare(sfixed32Msg.testFieldFixedInt32.toLocaleString(Qt.locale()),
                Number(sfixed32Msg.testFieldFixedInt32).toLocaleString(Qt.locale()),
                "Locale number string is not match "
                + sfixed32Msg.testFieldFixedInt32.toLocaleString(Qt.locale())
                + " != " + Number(sfixed32Msg.testFieldFixedInt32).toLocaleString(Qt.locale()))
    }

    function test_complexMessage_data() {
        return [
                    {tag: "inner text",
                        field: complexMsg.testComplexField.testFieldString, answer: "inner" },
                    {tag: "My test text",
                        field: complexMsg.testComplexField.testFieldString = "My test",
                        answer: "My test" },
                    {tag: "outer massage",
                        field: complexMsg.testComplexField = outerMessage, answer: outerMessage },
                    {tag: "inner massage",
                        field: complexMsg.testComplexField = innerMessage, answer: innerMessage },
                    {tag: "innerMessage text",
                        field: complexMsg.testComplexField.testFieldString, answer: "inner" },
                ]
    }

    function test_complexMessage(data) {
        compare(data.field, data.answer)
    }
}
