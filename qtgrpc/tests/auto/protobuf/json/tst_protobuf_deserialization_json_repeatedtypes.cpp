// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

//#include "basicmessages.qpb.h"
#include "repeatedmessages.qpb.h"

#include <QJsonDocument>
#include <QProtobufJsonSerializer>
#include <QTest>

using namespace Qt::Literals::StringLiterals;

class QtProtobufRepeatedTypesJsonDeserializationTest : public QObject
{
    Q_OBJECT
private slots:
    void init()
    {
        serializer.reset(new QProtobufJsonSerializer);
    }

    void RepeatedStringMessageTest();
    void RepeatedFloatMessageTest();
    void RepeatedBytesMessageTest();
    void RepeatedDoubleMessageTest();
    void RepeatedIntMessageTest();
    void RepeatedSIntMessageTest();
    void RepeatedUIntMessageTest();
    void RepeatedInt64MessageTest();
    void RepeatedSInt64MessageTest();
    void RepeatedUInt64MessageTest();
    void RepeatedFixedIntMessageTest();
    void RepeatedSFixedIntMessageTest();
    void RepeatedFixedInt64MessageTest();
    void RepeatedSFixedInt64MessageTest();
    void RepeatedComplexMessageTest();
    void RepeatedBoolMessageTest();
    void MalformedJsonTest();
    void InvalidTypeTest();
private:
    std::unique_ptr<QProtobufJsonSerializer> serializer;
};

using namespace qtprotobufnamespace::tests;

void QtProtobufRepeatedTypesJsonDeserializationTest::RepeatedStringMessageTest()
{
    RepeatedStringMessage test;
    test.deserialize(serializer.get(), "{\"testRepeatedString\":[\"aaaa\",\"bbbbb\",\"ccc\",\"dddddd\",\"eeeee\",\"\"]}"_ba);
    QCOMPARE(test.testRepeatedString().count(), 6);
    QCOMPARE(test.testRepeatedString(), QStringList({"aaaa","bbbbb","ccc","dddddd","eeeee",""}));

    test.deserialize(serializer.get(), "{\"testRepeatedString\":[]"_ba);
    QVERIFY(test.testRepeatedString().isEmpty());
}

void QtProtobufRepeatedTypesJsonDeserializationTest::RepeatedBytesMessageTest()
{
    RepeatedBytesMessage test;
    test.deserialize(serializer.get(), "{\"testRepeatedBytes\":[\"AQIDBAUG\",\"/////w==\",\"6urq6uo=\",\"AQIDBAUG\"]}"_ba);
    QCOMPARE(test.testRepeatedBytes().count(), 4);
    QCOMPARE(test.testRepeatedBytes(), QByteArrayList({QByteArray::fromHex("010203040506"),
                                                             QByteArray::fromHex("ffffffff"),
                                                             QByteArray::fromHex("eaeaeaeaea"),
                                                             QByteArray::fromHex("010203040506")}));
    //TODO: Serialization for list types works partially incorrect because of appending of values to existing
    //Need to make decision if deserialize should reset all protobuf properties or not
    RepeatedBytesMessage test2;
    test2.deserialize(serializer.get(), "{\"testRepeatedBytes\":[\"AQIDBAUG\",\"\",\"6urq6uo=\",\"AQIDBAUG\"]}"_ba);
    QCOMPARE(test2.testRepeatedBytes().count(), 4);
    QCOMPARE(test2.testRepeatedBytes(), QByteArrayList({QByteArray::fromHex("010203040506"),
                                                             QByteArray::fromHex(""),
                                                             QByteArray::fromHex("eaeaeaeaea"),
                                                             QByteArray::fromHex("010203040506")}));
    test.deserialize(serializer.get(), "{\"testRepeatedBytes\":[]"_ba);
    QVERIFY(test.testRepeatedBytes().isEmpty());
}

void QtProtobufRepeatedTypesJsonDeserializationTest::RepeatedFloatMessageTest()
{
    RepeatedFloatMessage test;
    test.deserialize(serializer.get(), "{\"testRepeatedFloat\":[0.4,1.2,0.5,3.912348,0.6]}"_ba);
    QCOMPARE(test.testRepeatedFloat().count(), 5);
    QCOMPARE(test.testRepeatedFloat(), QtProtobuf::floatList({0.4f, 1.2f, 0.5f, 3.912348f, 0.6f}));

    test.deserialize(serializer.get(), "{\"testRepeatedFloat\":[]"_ba);
    QVERIFY(test.testRepeatedFloat().isEmpty());
}

void QtProtobufRepeatedTypesJsonDeserializationTest::RepeatedDoubleMessageTest()
{
    RepeatedDoubleMessage test;
    test.deserialize(serializer.get(), "{\"testRepeatedDouble\":[0.1,0.2,0.3,3.912348239293,0.5]}"_ba);
    QCOMPARE(test.testRepeatedDouble().count(), 5);
    QCOMPARE(test.testRepeatedDouble(), QtProtobuf::doubleList({0.1,0.2,0.3,3.912348239293,0.5}));

    test.deserialize(serializer.get(), "{\"testRepeatedDouble\":[]"_ba);
    QVERIFY(test.testRepeatedDouble().isEmpty());
}

void QtProtobufRepeatedTypesJsonDeserializationTest::RepeatedIntMessageTest()
{
    RepeatedIntMessage test;
    test.deserialize(serializer.get(), "{\"testRepeatedInt\":[0,1,321,-65999,123245,-3,3]}"_ba);
    QCOMPARE(test.testRepeatedInt().count(), 7);
    QCOMPARE(test.testRepeatedInt(), QtProtobuf::int32List({0, 1, 321, -65999, 123245, -3, 3}));

    test.deserialize(serializer.get(), "{\"testRepeatedInt\":[]"_ba);
    QVERIFY(test.testRepeatedInt().isEmpty());
}

void QtProtobufRepeatedTypesJsonDeserializationTest::RepeatedSIntMessageTest()
{
    RepeatedSIntMessage test;
    test.deserialize(serializer.get(), "{\"testRepeatedInt\":[1,321,-65999,123245,-3,3]}"_ba);
    QCOMPARE(test.testRepeatedInt().count(), 6);
    QCOMPARE(test.testRepeatedInt(), QtProtobuf::sint32List({1, 321, -65999, 123245, -3, 3}));

    test.deserialize(serializer.get(), "{\"testRepeatedInt\":[]"_ba);
    QVERIFY(test.testRepeatedInt().isEmpty());
}

void QtProtobufRepeatedTypesJsonDeserializationTest::RepeatedUIntMessageTest()
{
    RepeatedUIntMessage test;
    test.deserialize(serializer.get(), "{\"testRepeatedInt\":[1,0,321,65999,123245,3]}"_ba);
    QCOMPARE(test.testRepeatedInt().count(), 6);
    QCOMPARE(test.testRepeatedInt(), QtProtobuf::uint32List({1, 0, 321, 65999, 123245, 3}));

    test.deserialize(serializer.get(), "{\"testRepeatedInt\":[]"_ba);
    QVERIFY(test.testRepeatedInt().isEmpty());
}

void QtProtobufRepeatedTypesJsonDeserializationTest::RepeatedInt64MessageTest()
{
    RepeatedInt64Message test;
    test.deserialize(serializer.get(), "{\"testRepeatedInt\":[1,321,-65999,12324523123123,-3,0,3]}"_ba);
    QCOMPARE(test.testRepeatedInt().count(), 7);
    QCOMPARE(test.testRepeatedInt(), QtProtobuf::int64List({1, 321, -65999, 12324523123123, -3, 0, 3}));

    test.deserialize(serializer.get(), "{\"testRepeatedInt\":[]"_ba);
    QVERIFY(test.testRepeatedInt().isEmpty());
}

void QtProtobufRepeatedTypesJsonDeserializationTest::RepeatedSInt64MessageTest()
{
    RepeatedSInt64Message test;
    test.deserialize(serializer.get(), "{\"testRepeatedInt\":[1,321,-65999,12324523123123,-3,3]}"_ba);
    QCOMPARE(test.testRepeatedInt().count(), 6);
    QCOMPARE(test.testRepeatedInt(), QtProtobuf::sint64List({1, 321, -65999, 12324523123123, -3, 3}));

    test.deserialize(serializer.get(), "{\"testRepeatedInt\":[]"_ba);
    QVERIFY(test.testRepeatedInt().isEmpty());
}

void QtProtobufRepeatedTypesJsonDeserializationTest::RepeatedUInt64MessageTest()
{
    RepeatedUInt64Message test;
    test.deserialize(serializer.get(), "{\"testRepeatedInt\":[1,321,65999,123245,123245324235425234,3]}"_ba);
    QCOMPARE(test.testRepeatedInt().count(), 6);
    QCOMPARE(test.testRepeatedInt(), QtProtobuf::uint64List({1, 321, 65999, 123245, 123245324235425234, 3}));

    test.deserialize(serializer.get(), "{\"testRepeatedInt\":[]"_ba);
    QVERIFY(test.testRepeatedInt().isEmpty());
}

void QtProtobufRepeatedTypesJsonDeserializationTest::RepeatedFixedIntMessageTest()
{
    RepeatedFixedIntMessage test;
    test.deserialize(serializer.get(), "{\"testRepeatedInt\":[1,321,65999,12324523,3,3]}"_ba);
    QCOMPARE(test.testRepeatedInt().count(), 6);
    QCOMPARE(test.testRepeatedInt(), QtProtobuf::fixed32List({1, 321, 65999, 12324523, 3, 3}));

    test.deserialize(serializer.get(), "{\"testRepeatedInt\":[]"_ba);
    QVERIFY(test.testRepeatedInt().isEmpty());
}

void QtProtobufRepeatedTypesJsonDeserializationTest::RepeatedSFixedIntMessageTest()
{
    RepeatedSFixedIntMessage test;
    test.deserialize(serializer.get(), "{\"testRepeatedInt\":[0,1,321,-65999,12324523,-3,3]}"_ba);
    QCOMPARE(test.testRepeatedInt().count(), 7);
    QCOMPARE(test.testRepeatedInt(), QtProtobuf::sfixed32List({0, 1, 321, -65999, 12324523, -3, 3}));

    test.deserialize(serializer.get(), "{\"testRepeatedInt\":[]"_ba);
    QVERIFY(test.testRepeatedInt().isEmpty());
}

void QtProtobufRepeatedTypesJsonDeserializationTest::RepeatedFixedInt64MessageTest()
{
    RepeatedFixedInt64Message test;
    test.deserialize(serializer.get(), "{\"testRepeatedInt\":[1,321,65999,123245324235425234,3,3]}"_ba);
    QCOMPARE(test.testRepeatedInt().count(), 6);
    QCOMPARE(test.testRepeatedInt(), QtProtobuf::fixed64List({1, 321, 65999, 123245324235425234, 3, 3}));

    test.deserialize(serializer.get(), "{\"testRepeatedInt\":[]"_ba);
    QVERIFY(test.testRepeatedInt().isEmpty());
}

void QtProtobufRepeatedTypesJsonDeserializationTest::RepeatedSFixedInt64MessageTest()
{
    RepeatedSFixedInt64Message test;
    test.deserialize(serializer.get(), "{\"testRepeatedInt\":[1,321,-65999,123245324235425234,-3,3]}"_ba);
    QCOMPARE(test.testRepeatedInt().count(), 6);
    QCOMPARE(test.testRepeatedInt(), QtProtobuf::sfixed64List({1, 321, -65999, 123245324235425234, -3, 3}));

    test.deserialize(serializer.get(), "{\"testRepeatedInt\":[]"_ba);
    QVERIFY(test.testRepeatedInt().isEmpty());
}

void QtProtobufRepeatedTypesJsonDeserializationTest::RepeatedComplexMessageTest()
{
    RepeatedComplexMessage test;
    test.deserialize(serializer.get(), "{\"testRepeatedComplex\":[{\"testFieldInt\":21,\"testComplexField\":"
                                       "{\"testFieldString\":\"12345\"}},{\"testFieldInt\":22,\"testComplexField\":"
                                       "{\"testFieldString\":\"qwerty\"}},{\"testFieldInt\":23,\"testComplexField\":"
                                       "{\"testFieldString\":\"\"}}]}"_ba);

    QCOMPARE(test.testRepeatedComplex().count(), 3);
    QCOMPARE(test.testRepeatedComplex().at(0).testFieldInt(), 21);
    QCOMPARE(test.testRepeatedComplex().at(0).testComplexField().testFieldString(), QString("12345"));
    QCOMPARE(test.testRepeatedComplex().at(1).testFieldInt(), 22);
    QCOMPARE(test.testRepeatedComplex().at(1).testComplexField().testFieldString(), QString("qwerty"));
    QCOMPARE(test.testRepeatedComplex().at(2).testFieldInt(), 23);
    QCOMPARE(test.testRepeatedComplex().at(2).testComplexField().testFieldString(), QString(""));

    test.deserialize(serializer.get(), "{\"testRepeatedComplex\":[]}"_ba);
    QVERIFY(test.testRepeatedComplex().isEmpty());
}

void QtProtobufRepeatedTypesJsonDeserializationTest::RepeatedBoolMessageTest()
{
    RepeatedBoolMessage boolMsg;
    boolMsg.deserialize(serializer.get(), "{\"testRepeatedBool\":[true,true,true,false,false,false,false,false,false,false,false,false,true]}"_ba);
    QCOMPARE(serializer->deserializationError(), QAbstractProtobufSerializer::NoError);
    QtProtobuf::boolList expected({ true,  true,  true,  false, false, false, false,
                                    false, false, false, false, false, true });
    QCOMPARE(boolMsg.testRepeatedBool().count(), 13);
    QCOMPARE(boolMsg.testRepeatedBool(), expected);
}

void QtProtobufRepeatedTypesJsonDeserializationTest::MalformedJsonTest()
{
    // no '['
    RepeatedBoolMessage boolMsg;
    boolMsg.deserialize(serializer.get(), "{\"testRepeatedBool\":true,true,true,false,false,false,false,false,false,false,false,false,true]}"_ba);
    QVERIFY(boolMsg.testRepeatedBool().size() == 0);
    QCOMPARE(serializer->deserializationError(), QAbstractProtobufSerializer::UnexpectedEndOfStreamError);

    // twice ]
    RepeatedSInt64Message test;
    test.deserialize(serializer.get(), "{\"testRepeatedInt\":[]1,321,-65999,12324523123123,-3,3]}"_ba);
    QVERIFY(test.testRepeatedInt().size() == 0);
    QCOMPARE(serializer->deserializationError(), QAbstractProtobufSerializer::UnexpectedEndOfStreamError);

}

void QtProtobufRepeatedTypesJsonDeserializationTest::InvalidTypeTest()
{
    // expected sint, string is used
    RepeatedSInt64Message test;
    test.deserialize(serializer.get(), "{\"testRepeatedInt\":[1,321,\"abcd\",12324523123123,-3,3]}"_ba);
    QVERIFY(test.testRepeatedInt().size() == 0);
    QCOMPARE(serializer->deserializationError(), QAbstractProtobufSerializer::InvalidFormatError);

    // expected bool, float is used
    RepeatedBoolMessage boolMsg;
    boolMsg.deserialize(serializer.get(),
                        "{\"testRepeatedBool\":[true,true,true,7.8,false,false,false,false,false,false,false,false,true]}"_ba);
    QVERIFY(test.testRepeatedInt().size() == 0);
    QCOMPARE(serializer->deserializationError(), QAbstractProtobufSerializer::InvalidFormatError);

}

QTEST_MAIN(QtProtobufRepeatedTypesJsonDeserializationTest)
#include "tst_protobuf_deserialization_json_repeatedtypes.moc"
