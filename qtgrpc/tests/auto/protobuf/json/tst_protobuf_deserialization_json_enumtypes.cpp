// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "enummessages.qpb.h"

#include <QProtobufJsonSerializer>
#include <QTest>

class QtProtobufEnumTypesDeserializationTest : public QObject
{
    Q_OBJECT
private slots:
    void init() { m_serializer.reset(new QProtobufJsonSerializer); }
    void SimpleEnumMessageDeserializeTest();
    void RepeatedEnumMessageTest();
    void MalformedJsonTest();
    void InvalidTypeTest();
private:
    std::unique_ptr<QProtobufJsonSerializer> m_serializer;
};

using namespace qtprotobufnamespace::tests;

void QtProtobufEnumTypesDeserializationTest::SimpleEnumMessageDeserializeTest()
{
    SimpleEnumMessage test;
    test.deserialize(m_serializer.get(), "{\"localEnum\":\"LOCAL_ENUM_VALUE2\"}");
    QCOMPARE(test.localEnum(), SimpleEnumMessage::LocalEnum::LOCAL_ENUM_VALUE2);
}

void QtProtobufEnumTypesDeserializationTest::RepeatedEnumMessageTest()
{
    RepeatedEnumMessage msg;

    msg.deserialize(m_serializer.get(), QByteArray());
    QVERIFY(msg.localEnumList().isEmpty());

    msg.deserialize(m_serializer.get(),
                    "{\"localEnumList\":[\"LOCAL_ENUM_VALUE0\","
                    "\"LOCAL_ENUM_VALUE1\",\"LOCAL_ENUM_VALUE2\","
                    "\"LOCAL_ENUM_VALUE1\",\"LOCAL_ENUM_VALUE2\","
                    "\"LOCAL_ENUM_VALUE3\"]}");
    QVERIFY((msg.localEnumList()
             == RepeatedEnumMessage::LocalEnumRepeated{
                 RepeatedEnumMessage::LocalEnum::LOCAL_ENUM_VALUE0,
                 RepeatedEnumMessage::LocalEnum::LOCAL_ENUM_VALUE1,
                 RepeatedEnumMessage::LocalEnum::LOCAL_ENUM_VALUE2,
                 RepeatedEnumMessage::LocalEnum::LOCAL_ENUM_VALUE1,
                 RepeatedEnumMessage::LocalEnum::LOCAL_ENUM_VALUE2,
                 RepeatedEnumMessage::LocalEnum::LOCAL_ENUM_VALUE3 }));
}

void QtProtobufEnumTypesDeserializationTest::MalformedJsonTest()
{
    SimpleEnumMessage test;
    // more braces
    test.deserialize(m_serializer.get(), "{\"localEnum\":\"LOCAL_ENUM_VALUE2\"}}");

    QCOMPARE(m_serializer->deserializationError(),
             QAbstractProtobufSerializer::UnexpectedEndOfStreamError);

    RepeatedEnumMessage msg;
    // no ']'
    msg.deserialize(m_serializer.get(),
                    "{\"localEnumList\":[\"LOCAL_ENUM_VALUE0\","
                    "\"LOCAL_ENUM_VALUE1\",\"LOCAL_ENUM_VALUE2\","
                    "\"LOCAL_ENUM_VALUE1\",\"LOCAL_ENUM_VALUE2\","
                    "\"LOCAL_ENUM_VALUE3\"}");

    QCOMPARE(m_serializer->deserializationError(),
             QAbstractProtobufSerializer::UnexpectedEndOfStreamError);
}

void QtProtobufEnumTypesDeserializationTest::InvalidTypeTest()
{
    // no LOCAL_ENUM_VALUE240
    SimpleEnumMessage invalidTest;
    invalidTest.deserialize(m_serializer.get(), "{\"localEnum\":\"LOCAL_ENUM_VALUE240\"}");
    QCOMPARE(m_serializer->deserializationError(),
             QAbstractProtobufSerializer::InvalidFormatError);

    RepeatedEnumMessage msg, msg2;
    // 'false'
    msg.deserialize(m_serializer.get(),
                    "{\"localEnumList\":[\"false\","
                    "\"LOCAL_ENUM_VALUE1\",\"LOCAL_ENUM_VALUE2\","
                    "\"LOCAL_ENUM_VALUE1\",\"LOCAL_ENUM_VALUE2\","
                    "\"LOCAL_ENUM_VALUE3\"]}");
    QCOMPARE(m_serializer->deserializationError(),
             QAbstractProtobufSerializer::InvalidFormatError);

    // no LOCAL_ENUM_VALUE_100
    msg2.deserialize(m_serializer.get(),
                     "{\"localEnumList\":[\"LOCAL_ENUM_VALUE_100\","
                     "\"LOCAL_ENUM_VALUE1\",\"LOCAL_ENUM_VALUE2\","
                     "\"LOCAL_ENUM_VALUE1\",\"LOCAL_ENUM_VALUE2\","
                     "\"LOCAL_ENUM_VALUE3\"]}");
    QCOMPARE(m_serializer->deserializationError(),
             QAbstractProtobufSerializer::InvalidFormatError);
}

QTEST_MAIN(QtProtobufEnumTypesDeserializationTest)
#include "tst_protobuf_deserialization_json_enumtypes.moc"
