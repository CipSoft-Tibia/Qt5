// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtTest

import nopackage.uri.test

TestCase {
    name: "qtprotobufNopackageTest"
    property noPackageMessage noPackageMessageUser;
    property noPackageExternalMessage noPackageExternal;
    property simpleIntMessageExt intMsgExt;
    property emptyMessage emptyMsg;

    function test_1init() {
        noPackageMessageUser.testField.testFieldInt = 42
        noPackageExternal.testField.testFieldInt = 100
        intMsgExt.testFieldInt = -100
    }

    function test_enumValues_data() {
        return [
                    // TestEnum
                    { tag: "TestEnum.LOCAL_ENUM_VALUE0 == 0",
                        field: TestEnum.LOCAL_ENUM_VALUE0, answer: 0 },
                    { tag: "TestEnum.LOCAL_ENUM_VALUE1 == 1",
                        field: TestEnum.LOCAL_ENUM_VALUE1, answer: 1 },
                    { tag: "TestEnum.LOCAL_ENUM_VALUE2 == 2",
                        field: TestEnum.LOCAL_ENUM_VALUE2, answer: 2 },
                    { tag: "TestEnum.LOCAL_ENUM_VALUE3 == 5",
                        field: TestEnum.LOCAL_ENUM_VALUE3, answer: 5 },
                ]
    }

    function test_enumValues(data) {
         compare(data.field, data.answer)
    }

    function test_messageValuesTypes_data() {
        return [
                    // TestEnum
                    { tag: "NoPackageMessage->testFieldInt is a number",
                        field: typeof noPackageMessageUser.testField.testFieldInt,
                        answer: "number" },
                    { tag: "NoPackageExternalMessage->testFieldInt is a number",
                        field: typeof noPackageExternal.testField.testFieldInt,
                        answer: "number" },
                    { tag: "SimpleIntMessageExt->testFieldInt is a number",
                        field: typeof intMsgExt.testFieldInt, answer: "number" },
                    { tag: "EmptyMessage is an object",
                        field: typeof emptyMsg, answer: "object" },
                ]
    }

    function test_messageValuesTypes(data) {
         compare(data.field, data.answer)
    }

    function test_messageValues_data() {
        return [
                    { tag: "Test noPackageMessage field",
                        field: noPackageMessageUser.testField.testFieldInt, answer: 42 },
                    { tag: "Test noPackageExternalMessage field",
                        field: noPackageExternal.testField.testFieldInt, answer: 100 },
                    { tag: "Test simpleIntMessageExt field",
                        field: intMsgExt.testFieldInt, answer: -100 },
                ]
    }

    function test_messageValues(data) {
         compare(data.field, data.answer)
    }

    function test_messageValuesUpdate() {
        noPackageMessageUser.testField.testFieldInt = 43;
        compare(noPackageMessageUser.testField.testFieldInt, 43,
                "noPackageExternalMessage message contains invalid value");
        noPackageExternal.testField.testFieldInt = -99
        compare(noPackageExternal.testField.testFieldInt, -99,
                "noPackageExternal message contains invalid value");
        intMsgExt.testFieldInt = 0
        compare(intMsgExt.testFieldInt, 0, "simpleIntMessageExt message contains invalid value");
    }
}
