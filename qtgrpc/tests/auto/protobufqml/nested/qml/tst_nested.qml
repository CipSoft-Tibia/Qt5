// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtTest

import qtprotobufnamespace.tests.nested
import qtprotobufnamespace1.tests.nested as NestedFieldMessages1

TestCase {
    name: "qtprotobufNestedTest"

    // Messages from "qtprotobufnamespace.tests.nested"
    property nestedFieldMessage fieldMsg;
    property nestedMessage simpleMsg;
    property nestedFieldMessage2 fieldMessage2;
    property nestedMessageLevel1 messageLevel1;
    property nestedMessageLevel2 messageLevel2;
    property nestedExternal nestedExternalField;
    property nested nestedField;
    property nestedCyclingB bField;
    property nestedCyclingBB bbField;

    // Messages from "qtprotobufnamespace1.tests.nested"
    property NestedFieldMessages1.nestedFieldMessage fieldMsg1;
    property NestedFieldMessages1.nestedMessage simpleMsg1;

    function initTestCase() {
        simpleMsg.testFieldInt = 100
        fieldMsg.nested = simpleMsg
        messageLevel2.testFieldInt = 200
        fieldMessage2.nested2 = messageLevel2
        simpleMsg1.field = 500
        fieldMsg1.nested = simpleMsg1
        nestedExternalField.externalNested = simpleMsg1
        bField.testField = bbField
        bbField.testField = bField
        nestedField.testFieldInt = 500
    }

    function test_nestedMessage_data() {
        return [
                    {tag: "simpleMsg.testFieldInt == 100",
                        field: simpleMsg.testFieldInt, answer: 100 },
                    {tag: "simpleMsg1.field == 500",
                        field: simpleMsg1.field, answer: 500 },
                    {tag: "messageLevel2.testFieldInt == 200",
                        field: messageLevel2.testFieldInt, answer: 200 },
                    {tag: "Nested message assignment",
                        field: fieldMsg.nested, answer: simpleMsg },
                    {tag: "NestedFieldMessages1 message assignment",
                        field: fieldMsg1.nested, answer: simpleMsg1 },
                    {tag: "messageLevel2 message assignment",
                        field: fieldMessage2.nested2, answer: messageLevel2 },
                    {tag: "nestedExternalField.externalNested == simpleMsg1",
                        field: nestedExternalField.externalNested, answer: simpleMsg1 },
                    {tag: "bField.testField == bbField",
                        field: bField.testField, answer: bbField },
                    {tag: "bbField.testField == bField",
                        field: bbField.testField, answer: bField },
                    {tag: "nestedField.testFieldInt == 500",
                        field: nestedField.testFieldInt, answer: 500 },
                ]
    }

    function test_nestedMessage(data) {
        compare(data.field, data.answer)
    }

    function test_checkMessageTypes_data() {
        return [
                    { tag: "nestedFieldMessage is object",
                        field: typeof fieldMsg, answer: "object" },
                    { tag: "nestedMessage is object",
                        field: typeof simpleMsg, answer: "object" },
                    { tag: "nestedFieldMessage2 is object",
                        field: typeof fieldMessage2, answer: "object" },
                    { tag: "nestedMessageLevel1 is object",
                        field: typeof messageLevel1, answer: "object" },
                    { tag: "nestedMessageLevel2 is object",
                        field: typeof messageLevel2, answer: "object" },
                    { tag: "nestedExternal is object",
                        field: typeof nestedExternalField, answer: "object" },
                    { tag: "nested is object",
                        field: typeof nestedField, answer: "object" },
                    { tag: "nestedCyclingB is object",
                        field: typeof bField, answer: "object" },
                    { tag: "nestedCyclingBB is object",
                        field: typeof bbField, answer: "object" },
                ]
    }

    function test_checkMessageTypes(data) {
        compare(data.field, data.answer)
    }

}
