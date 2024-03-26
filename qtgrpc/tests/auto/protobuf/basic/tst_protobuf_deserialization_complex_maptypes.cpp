// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2022 Alexey Edelev <semlanik@gmail.com>, Viktor Kopp <vifactor@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "basicmessages.qpb.h"
#include "mapmessages.qpb.h"

#include <QObject>

#include <QProtobufSerializer>
#include <QTest>

#include <limits>

using namespace qtprotobufnamespace::tests;

class QtProtobufMapTypesDeserializationTest : public QObject
{
    Q_OBJECT
private slots:
    void init() { serializer.reset(new QProtobufSerializer); }

    void SimpleFixed32ComplexMapDeserializeTest();
    void SimpleSFixed32ComplexMapDeserializeTest();
    void SimpleInt32ComplexMapDeserializeTest();
    void SimpleSInt32ComplexMapDeserializeTest();
    void SimpleUInt32ComplexMapDeserializeTest();
    void SimpleFixed64ComplexMapDeserializeTest();
    void SimpleSFixed64ComplexMapDeserializeTest();
    void SimpleInt64ComplexMapDeserializeTest();
    void SimpleSInt64ComplexMapDeserializeTest();
    void SimpleUInt64ComplexMapDeserializeTest();
    void SimpleStringComplexMapDeserializeTest();
    void SimpleUInt64ComplexMapInvalidLengthDeserializeTest();
    void SimpleStringComplexMapInvalidLengthDeserializeTest();
    void SimpleUInt64ComplexMapCorruptedDeserializeTest();

private:
    std::unique_ptr<QProtobufSerializer> serializer;
};

void QtProtobufMapTypesDeserializationTest::SimpleFixed32ComplexMapDeserializeTest()
{
    SimpleFixed32ComplexMessageMapMessage test;
    test.deserialize(serializer.get(),
                     QByteArray::fromHex("3a180d0a00000012110810120d320b74656e207369787465656e3a230"
                                         "d2a000000121c080a12183216666f757274792074776f2074656e2073"
                                         "69787465656e3a110d13000100120a080a120632045755543f"));
    QCOMPARE(QAbstractProtobufSerializer::NoError, serializer->deserializationError());

    qtprotobufnamespace::tests::SimpleStringMessage stringMsg;

    stringMsg.setTestFieldString({ "ten sixteen" });
    ComplexMessage expected1;
    expected1.setTestFieldInt(16);
    expected1.setTestComplexField(stringMsg);

    stringMsg.setTestFieldString({ "fourty two ten sixteen" });
    ComplexMessage expected2;
    expected2.setTestFieldInt(10);
    expected2.setTestComplexField(stringMsg);

    stringMsg.setTestFieldString({ "WUT?" });
    ComplexMessage expected3;
    expected3.setTestFieldInt(10);
    expected3.setTestComplexField(stringMsg);

    QCOMPARE(test.mapField()[10], expected1);
    QCOMPARE(test.mapField()[42], expected2);
    QCOMPARE(test.mapField()[65555], expected3);
}

void QtProtobufMapTypesDeserializationTest::SimpleSFixed32ComplexMapDeserializeTest()
{
    SimpleSFixed32ComplexMessageMapMessage test;
    test.deserialize(
            serializer.get(),
            QByteArray::fromHex("4a290dd6ffffff1222121e321c6d696e757320666f757274792074776f2074656e"
                                "207369787465656e080a4a180d0a0000001211120d320b74656e20736978746565"
                                "6e08104a110d13000100120a120632045755543f080a"));
    QCOMPARE(QAbstractProtobufSerializer::NoError, serializer->deserializationError());

    qtprotobufnamespace::tests::SimpleStringMessage stringMsg;

    stringMsg.setTestFieldString({ "ten sixteen" });
    ComplexMessage expected1;
    expected1.setTestFieldInt(16);
    expected1.setTestComplexField(stringMsg);

    stringMsg.setTestFieldString({ "minus fourty two ten sixteen" });
    ComplexMessage expected2;
    expected2.setTestFieldInt(10);
    expected2.setTestComplexField(stringMsg);

    stringMsg.setTestFieldString({ "WUT?" });
    ComplexMessage expected3;
    expected3.setTestFieldInt(10);
    expected3.setTestComplexField(stringMsg);

    QCOMPARE(test.mapField()[10], expected1);
    QCOMPARE(test.mapField()[-42], expected2);
    QCOMPARE(test.mapField()[65555], expected3);
}

void QtProtobufMapTypesDeserializationTest::SimpleInt32ComplexMapDeserializeTest()
{
    SimpleInt32ComplexMessageMapMessage test;
    test.deserialize(
            serializer.get(),
            QByteArray::fromHex("1a2f08d6ffffffffffffffff011222121e321c6d696e757320666f757274792074"
                                "776f2074656e207369787465656e080a1a15080a1211120d320b74656e20736978"
                                "7465656e08101a1008938004120a120632045755543f080a"));
    QCOMPARE(serializer->deserializationError(), QAbstractProtobufSerializer::NoError);

    qtprotobufnamespace::tests::SimpleStringMessage stringMsg;

    stringMsg.setTestFieldString({ "ten sixteen" });
    ComplexMessage expected1;
    expected1.setTestFieldInt(16);
    expected1.setTestComplexField(stringMsg);

    stringMsg.setTestFieldString({ "minus fourty two ten sixteen" });
    ComplexMessage expected2;
    expected2.setTestFieldInt(10);
    expected2.setTestComplexField(stringMsg);

    stringMsg.setTestFieldString({ "WUT?" });
    ComplexMessage expected3;
    expected3.setTestFieldInt(10);
    expected3.setTestComplexField(stringMsg);

    QCOMPARE(test.mapField()[10], expected1);
    QCOMPARE(test.mapField()[-42], expected2);
    QCOMPARE(test.mapField()[65555], expected3);
}

void QtProtobufMapTypesDeserializationTest::SimpleSInt32ComplexMapDeserializeTest()
{
    SimpleSInt32ComplexMessageMapMessage test;
    test.deserialize(serializer.get(),
                     QByteArray::fromHex("0a1608a580081210120c320a6d696e7573205755543f080a0a1508141"
                                         "211120d320b74656e207369787465656e08100a200854121c12183216"
                                         "666f757274792074776f2074656e207369787465656e080a"));
    QCOMPARE(QAbstractProtobufSerializer::NoError, serializer->deserializationError());

    qtprotobufnamespace::tests::SimpleStringMessage stringMsg;

    stringMsg.setTestFieldString({ "ten sixteen" });
    ComplexMessage expected1;
    expected1.setTestFieldInt(16);
    expected1.setTestComplexField(stringMsg);

    stringMsg.setTestFieldString({ "fourty two ten sixteen" });
    ComplexMessage expected2;
    expected2.setTestFieldInt(10);
    expected2.setTestComplexField(stringMsg);

    stringMsg.setTestFieldString({ "minus WUT?" });
    ComplexMessage expected3;
    expected3.setTestFieldInt(10);
    expected3.setTestComplexField(stringMsg);

    QCOMPARE(test.mapField()[10], expected1);
    QCOMPARE(test.mapField()[42], expected2);
    QCOMPARE(test.mapField()[-65555], expected3);
}

void QtProtobufMapTypesDeserializationTest::SimpleUInt32ComplexMapDeserializeTest()
{
    SimpleUInt32ComplexMessageMapMessage test;
    test.deserialize(
            serializer.get(),
            QByteArray::fromHex(
                    "2a15080a1211120d320b74656e207369787465656e08102a20082a121c12183216666f75727479"
                    "2074776f2074656e207369787465656e080a2a1008938004120a120632045755543f080a"));
    QCOMPARE(QAbstractProtobufSerializer::NoError, serializer->deserializationError());

    qtprotobufnamespace::tests::SimpleStringMessage stringMsg;

    stringMsg.setTestFieldString({ "ten sixteen" });
    ComplexMessage expected1;
    expected1.setTestFieldInt(16);
    expected1.setTestComplexField(stringMsg);

    stringMsg.setTestFieldString({ "fourty two ten sixteen" });
    ComplexMessage expected2;
    expected2.setTestFieldInt(10);
    expected2.setTestComplexField(stringMsg);

    stringMsg.setTestFieldString({ "WUT?" });
    ComplexMessage expected3;
    expected3.setTestFieldInt(10);
    expected3.setTestComplexField(stringMsg);

    QCOMPARE(test.mapField()[10], expected1);
    QCOMPARE(test.mapField()[42], expected2);
    QCOMPARE(test.mapField()[65555], expected3);
}

void QtProtobufMapTypesDeserializationTest::SimpleFixed64ComplexMapDeserializeTest()
{
    SimpleFixed64ComplexMessageMapMessage test;
    test.deserialize(serializer.get(),
                     QByteArray::fromHex(
                             "421c090a000000000000001211120d320b74656e207369787465656e0810421509130"
                             "0010000000000120a120632045755543f080a422b09ffffffffffffffff1220121c32"
                             "1a6d696e757320666f757274792074776f2074656e204d41414158082a"));
    QCOMPARE(QAbstractProtobufSerializer::NoError, serializer->deserializationError());

    qtprotobufnamespace::tests::SimpleStringMessage stringMsg;

    stringMsg.setTestFieldString({ "ten sixteen" });
    ComplexMessage expected1;
    expected1.setTestFieldInt(16);
    expected1.setTestComplexField(stringMsg);

    stringMsg.setTestFieldString({ "minus fourty two ten MAAAX" });
    ComplexMessage expected2;
    expected2.setTestFieldInt(42);
    expected2.setTestComplexField(stringMsg);

    stringMsg.setTestFieldString({ "WUT?" });
    ComplexMessage expected3;
    expected3.setTestFieldInt(10);
    expected3.setTestComplexField(stringMsg);

    QCOMPARE(test.mapField()[10], expected1);
    QCOMPARE(test.mapField()[UINT64_MAX], expected2);
    QCOMPARE(test.mapField()[65555], expected3);
}

void QtProtobufMapTypesDeserializationTest::SimpleSFixed64ComplexMapDeserializeTest()
{
    SimpleSFixed64ComplexMessageMapMessage test;
    test.deserialize(serializer.get(),
                     QByteArray::fromHex(
                             "522d09d6ffffffffffffff1222121e321c6d696e757320666f757274792074776f207"
                             "4656e207369787465656e080a521c090a000000000000001211120d320b74656e2073"
                             "69787465656e08105215091300010000000000120a120632045755543f080a"));
    QCOMPARE(QAbstractProtobufSerializer::NoError, serializer->deserializationError());

    qtprotobufnamespace::tests::SimpleStringMessage stringMsg;

    stringMsg.setTestFieldString({ "ten sixteen" });
    ComplexMessage expected1;
    expected1.setTestFieldInt(16);
    expected1.setTestComplexField(stringMsg);

    stringMsg.setTestFieldString({ "minus fourty two ten sixteen" });
    ComplexMessage expected2;
    expected2.setTestFieldInt(10);
    expected2.setTestComplexField(stringMsg);

    stringMsg.setTestFieldString({ "WUT?" });
    ComplexMessage expected3;
    expected3.setTestFieldInt(10);
    expected3.setTestComplexField(stringMsg);

    QCOMPARE(test.mapField()[10], expected1);
    QCOMPARE(test.mapField()[-42], expected2);
    QCOMPARE(test.mapField()[65555], expected3);
}

void QtProtobufMapTypesDeserializationTest::SimpleInt64ComplexMapDeserializeTest()
{
    SimpleInt64ComplexMessageMapMessage test;
    test.deserialize(
            serializer.get(),
            QByteArray::fromHex("222f08d6ffffffffffffffff011222121e321c6d696e757320666f757274792074"
                                "776f2074656e207369787465656e080a2215080a1211120d320b74656e20736978"
                                "7465656e0810221008938004120a120632045755543f080a"));
    QCOMPARE(QAbstractProtobufSerializer::NoError, serializer->deserializationError());

    qtprotobufnamespace::tests::SimpleStringMessage stringMsg;

    stringMsg.setTestFieldString({ "ten sixteen" });
    ComplexMessage expected1;
    expected1.setTestFieldInt(16);
    expected1.setTestComplexField(stringMsg);

    stringMsg.setTestFieldString({ "minus fourty two ten sixteen" });
    ComplexMessage expected2;
    expected2.setTestFieldInt(10);
    expected2.setTestComplexField(stringMsg);

    stringMsg.setTestFieldString({ "WUT?" });
    ComplexMessage expected3;
    expected3.setTestFieldInt(10);
    expected3.setTestComplexField(stringMsg);

    QCOMPARE(test.mapField()[10], expected1);
    QCOMPARE(test.mapField()[-42], expected2);
    QCOMPARE(test.mapField()[65555], expected3);
}

void QtProtobufMapTypesDeserializationTest::SimpleSInt64ComplexMapDeserializeTest()
{
    SimpleSInt64ComplexMessageMapMessage test;
    test.deserialize(serializer.get(),
                     QByteArray::fromHex("122608531222121e321c6d696e757320666f757274792074776f20746"
                                         "56e207369787465656e080a121508141211120d320b74656e20736978"
                                         "7465656e0810121008a68008120a120632045755543f080a"));
    QCOMPARE(QAbstractProtobufSerializer::NoError, serializer->deserializationError());

    qtprotobufnamespace::tests::SimpleStringMessage stringMsg;

    stringMsg.setTestFieldString({ "ten sixteen" });
    ComplexMessage expected1;
    expected1.setTestFieldInt(16);
    expected1.setTestComplexField(stringMsg);

    stringMsg.setTestFieldString({ "minus fourty two ten sixteen" });
    ComplexMessage expected2;
    expected2.setTestFieldInt(10);
    expected2.setTestComplexField(stringMsg);

    stringMsg.setTestFieldString({ "WUT?" });
    ComplexMessage expected3;
    expected3.setTestFieldInt(10);
    expected3.setTestComplexField(stringMsg);

    QCOMPARE(test.mapField()[10], expected1);
    QCOMPARE(test.mapField()[-42], expected2);
    QCOMPARE(test.mapField()[65555], expected3);
}

void QtProtobufMapTypesDeserializationTest::SimpleUInt64ComplexMapDeserializeTest()
{
    SimpleUInt64ComplexMessageMapMessage test;
    test.deserialize(
            serializer.get(),
            QByteArray::fromHex(
                    "3214080a1210120c320a74656e20656c6576656e080b3220082a121c12183216666f7572747920"
                    "74776f2074656e207369787465656e080a321008938004120a120632045755543f080a"));
    QCOMPARE(QAbstractProtobufSerializer::NoError, serializer->deserializationError());

    qtprotobufnamespace::tests::SimpleStringMessage stringMsg;

    stringMsg.setTestFieldString({ "ten eleven" });
    ComplexMessage expected1;
    expected1.setTestFieldInt(11);
    expected1.setTestComplexField(stringMsg);

    stringMsg.setTestFieldString({ "fourty two ten sixteen" });
    ComplexMessage expected2;
    expected2.setTestFieldInt(10);
    expected2.setTestComplexField(stringMsg);

    stringMsg.setTestFieldString({ "WUT?" });
    ComplexMessage expected3;
    expected3.setTestFieldInt(10);
    expected3.setTestComplexField(stringMsg);

    QCOMPARE(test.mapField()[10], expected1);
    QCOMPARE(test.mapField()[42], expected2);
    QCOMPARE(test.mapField()[65555], expected3);
}

void QtProtobufMapTypesDeserializationTest::SimpleStringComplexMapDeserializeTest()
{
    SimpleStringComplexMessageMapMessage test;
    test.deserialize(serializer.get(),
                     QByteArray::fromHex(
                             "6a140a055755543f3f120b120732053f5755543f080a6a170a0362656e1210120c320"
                             "a74656e20656c6576656e080b6a350a157768657265206973206d7920636172206475"
                             "64653f121c12183216666f757274792074776f2074656e207369787465656e080a"));
    QCOMPARE(QAbstractProtobufSerializer::NoError, serializer->deserializationError());

    qtprotobufnamespace::tests::SimpleStringMessage stringMsg;

    stringMsg.setTestFieldString({ "ten eleven" });
    ComplexMessage expected1;
    expected1.setTestFieldInt(11);
    expected1.setTestComplexField(stringMsg);

    stringMsg.setTestFieldString({ "fourty two ten sixteen" });
    ComplexMessage expected2;
    expected2.setTestFieldInt(10);
    expected2.setTestComplexField(stringMsg);

    stringMsg.setTestFieldString({ "?WUT?" });
    ComplexMessage expected3;
    expected3.setTestFieldInt(10);
    expected3.setTestComplexField(stringMsg);

    QCOMPARE(test.mapField()["ben"], expected1);
    QCOMPARE(test.mapField()["where is my car dude?"], expected2);
    QCOMPARE(test.mapField()["WUT??"], expected3);
}

void QtProtobufMapTypesDeserializationTest::SimpleUInt64ComplexMapInvalidLengthDeserializeTest()
{
    SimpleUInt64ComplexMessageMapMessage test;
    test.deserialize(serializer.get(),
                     QByteArray::fromHex(
                             "3214080a1210120c320a74656e20656c6576656e080b3220082a121c12183216666f7"
                             "57274792074776f2074656e207369787465656e080a321008938004120a120"));
    QCOMPARE(serializer->deserializationError(), QAbstractProtobufSerializer::InvalidHeaderError);
    QVERIFY(test.mapField().isEmpty());
}

void QtProtobufMapTypesDeserializationTest::SimpleStringComplexMapInvalidLengthDeserializeTest()
{
    SimpleStringComplexMessageMapMessage test;
    test.deserialize(serializer.get(),
                     QByteArray::fromHex(
                             "6a140a055755543f3f120b120732053f5755543f080a6a170a0362656e1210120c320"
                             "a74656e20656c6576656e080b6a350a157768657265206973206d7920636172206475"
                             "64653f121c12183216666f757274792074776f2074656e20736978746565"));
    QCOMPARE(serializer->deserializationError(), QAbstractProtobufSerializer::InvalidHeaderError);

    qtprotobufnamespace::tests::SimpleStringMessage stringMsg;

    stringMsg.setTestFieldString({ "ten eleven" });
    ComplexMessage expected1;
    expected1.setTestFieldInt(11);
    expected1.setTestComplexField(stringMsg);

    stringMsg.setTestFieldString({ "?WUT?" });
    ComplexMessage expected2;
    expected2.setTestFieldInt(10);
    expected2.setTestComplexField(stringMsg);

    QCOMPARE(test.mapField()["ben"], expected1);
    QCOMPARE(test.mapField()["WUT??"], expected2);
    QVERIFY(!test.mapField().contains("where is my car dude?"));
}

void QtProtobufMapTypesDeserializationTest::SimpleUInt64ComplexMapCorruptedDeserializeTest()
{
    SimpleUInt64ComplexMessageMapMessage test;
    test.deserialize(
            serializer.get(),
            QByteArray::fromHex(
                    "3214080a1210120c320a74656e20656c6576656e080b3221233522345b2183216666f757274792"
                    "074776f2074656e207369787465656e080a321008938004120a120632045755543f080a"));
    QCOMPARE(serializer->deserializationError(), QAbstractProtobufSerializer::InvalidHeaderError);
    QVERIFY(test.mapField().isEmpty());
}

QTEST_MAIN(QtProtobufMapTypesDeserializationTest)
#include "tst_protobuf_deserialization_complex_maptypes.moc"
