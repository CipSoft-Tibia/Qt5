// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2022 Alexey Edelev <semlanik@gmail.com>, Viktor Kopp <vifactor@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "basicmessages.qpb.h"
#include "repeatedmessages.qpb.h"

#include <QMetaProperty>
#include <QSignalSpy>
#include <QTest>

#include <qtprotobuftestscommon.h>

class QtProtobufRepeatedTypesGenerationTest : public QObject
{
    Q_OBJECT
private slots:
    void RepeatedStringMessageTest();
    void RepeatedIntMessageTest();
    void RepeatedDoubleMessageTest();
    void RepeatedFloatMessageTest();
    void RepeatedBytesMessageTest();
    void RepeatedSIntMessageTest();
    void RepeatedUIntMessageTest();
    void RepeatedInt64MessageTest();
    void RepeatedSInt64MessageTest();
    void RepeatedUInt64MessageTest();
    void RepeatedFixedIntMessageTest();
    void RepeatedFixedInt64MessageTest();
    void RepeatedSFixedIntMessageTest();
    void RepeatedSFixedInt64MessageTest();
    void RepeatedBoolTest();

    void MoveOperatorRepeatedTest();
    void RepeatedComplexMessageCompareTest();
};

using namespace qtprotobufnamespace::tests;

void QtProtobufRepeatedTypesGenerationTest::RepeatedStringMessageTest()
{
    const char *propertyName = "testRepeatedString";
    qProtobufAssertMessagePropertyRegistered<RepeatedStringMessage, QStringList>(1, "QStringList", propertyName);

    RepeatedStringMessage test;
    QVERIFY(test.setProperty(propertyName, QVariant::fromValue<QStringList>({"Text", "tryam"})));
    QCOMPARE(test.property(propertyName).value<QStringList>(), QStringList({"Text", "tryam"}));
    QCOMPARE(test.testRepeatedString(), QStringList({"Text", "tryam"}));
}

void QtProtobufRepeatedTypesGenerationTest::RepeatedIntMessageTest()
{
    const char *propertyName = "testRepeatedInt";
    qProtobufAssertMessagePropertyRegistered<RepeatedIntMessage, QtProtobuf::int32List>(1, "QtProtobuf::int32List", propertyName);

    RepeatedIntMessage test;
    QVERIFY(test.setProperty(propertyName, QVariant::fromValue<QtProtobuf::int32List>({1, 2, 3, 4, 5})));
    QCOMPARE(test.property(propertyName).value<QtProtobuf::int32List>(), QtProtobuf::int32List({1, 2, 3, 4, 5}));
    QCOMPARE(test.testRepeatedInt(), QtProtobuf::int32List({1, 2, 3, 4, 5}));

    test.testRepeatedInt().append(66);
    QCOMPARE(test.testRepeatedInt(), QtProtobuf::int32List({1, 2, 3, 4, 5, 66}));

    test.testRepeatedInt().pop_back();
    QCOMPARE(test.testRepeatedInt(), QtProtobuf::int32List({1, 2, 3, 4, 5}));
}

void QtProtobufRepeatedTypesGenerationTest::RepeatedDoubleMessageTest()
{
    const char *propertyName = "testRepeatedDouble";
    qProtobufAssertMessagePropertyRegistered<RepeatedDoubleMessage, QtProtobuf::doubleList>(1, "QtProtobuf::doubleList", propertyName);

    RepeatedDoubleMessage test;
    QVERIFY(test.setProperty(propertyName, QVariant::fromValue<QtProtobuf::doubleList>({1.0, 2.3, 3, 4.7, 5.9})));
    QCOMPARE(test.property(propertyName).value<QtProtobuf::doubleList>(), QtProtobuf::doubleList({1.0, 2.3, 3, 4.7, 5.9}));
    QCOMPARE(test.testRepeatedDouble(), QtProtobuf::doubleList({1.0, 2.3, 3, 4.7, 5.9}));

    test.testRepeatedDouble().append(6.6);
    QCOMPARE(test.testRepeatedDouble(), QtProtobuf::doubleList({1.0, 2.3, 3, 4.7, 5.9, 6.6}));

    test.testRepeatedDouble().pop_back();
    QCOMPARE(test.testRepeatedDouble(), QtProtobuf::doubleList({1.0, 2.3, 3, 4.7, 5.9}));
}

void QtProtobufRepeatedTypesGenerationTest::RepeatedFloatMessageTest()
{
    const char *propertyName = "testRepeatedFloat";
    qProtobufAssertMessagePropertyRegistered<RepeatedFloatMessage, QtProtobuf::floatList>(1, "QtProtobuf::floatList", propertyName);

    RepeatedFloatMessage test;
    QVERIFY(test.setProperty(propertyName, QVariant::fromValue<QtProtobuf::floatList>({1.0f, 2.3f, 3, 4.7f, 5.9f})));
    QCOMPARE(test.property(propertyName).value<QtProtobuf::floatList>(), QtProtobuf::floatList({1.0f, 2.3f, 3, 4.7f, 5.9f}));
    QCOMPARE(test.testRepeatedFloat(), QtProtobuf::floatList({1.0f, 2.3f, 3, 4.7f, 5.9f}));

    test.testRepeatedFloat().append(6.6f);
    QCOMPARE(test.testRepeatedFloat(), QtProtobuf::floatList({1.0f, 2.3f, 3, 4.7f, 5.9f, 6.6f}));

    test.testRepeatedFloat().pop_back();
    QCOMPARE(test.testRepeatedFloat(), QtProtobuf::floatList({1.0f, 2.3f, 3, 4.7f, 5.9f}));
}

void QtProtobufRepeatedTypesGenerationTest::RepeatedBytesMessageTest()
{
    const char *propertyName = "testRepeatedBytes";
    qProtobufAssertMessagePropertyRegistered<RepeatedBytesMessage, QByteArrayList>(1, "QByteArrayList", propertyName);

    QByteArrayList bList;
    bList << "\x01\x02\x03\x04\x05";
    bList << "\x01\x05\x03\x04\x03";

    RepeatedBytesMessage test;
    QVERIFY(test.setProperty(propertyName, QVariant::fromValue<QByteArrayList>(bList)));
    QCOMPARE(test.property(propertyName).value<QByteArrayList>(), bList);
    QCOMPARE(test.testRepeatedBytes(), bList);

    bList << "\x01\x05\x03\x03";
    test.testRepeatedBytes() << "\x01\x05\x03\x03";
    QCOMPARE(test.testRepeatedBytes(), bList);

    bList.pop_back();
    test.testRepeatedBytes().pop_back();
    QCOMPARE(test.testRepeatedBytes(), bList);
}

void QtProtobufRepeatedTypesGenerationTest::RepeatedSIntMessageTest()
{
    const char *propertyName = "testRepeatedInt";
    qProtobufAssertMessagePropertyRegistered<RepeatedSIntMessage, QtProtobuf::sint32List>(1, "QtProtobuf::sint32List", propertyName);

    RepeatedSIntMessage test;
    QVERIFY(test.setProperty(propertyName, QVariant::fromValue<QtProtobuf::sint32List>({1, 2, 3, 4, 5})));
    QCOMPARE(test.property(propertyName).value<QtProtobuf::sint32List>(), QtProtobuf::sint32List({1, 2, 3, 4, 5}));
    QCOMPARE(test.testRepeatedInt(), QtProtobuf::sint32List({1, 2, 3, 4, 5}));

    test.testRepeatedInt() << 6;
    QCOMPARE(test.testRepeatedInt(), QtProtobuf::sint32List({1, 2, 3, 4, 5, 6}));

    test.testRepeatedInt().pop_back();
    QCOMPARE(test.testRepeatedInt(), QtProtobuf::sint32List({1, 2, 3, 4, 5}));
}

void QtProtobufRepeatedTypesGenerationTest::RepeatedUIntMessageTest()
{
    const char *propertyName = "testRepeatedInt";
    qProtobufAssertMessagePropertyRegistered<RepeatedUIntMessage, QtProtobuf::uint32List>(1, "QtProtobuf::uint32List", propertyName);

    RepeatedUIntMessage test;
    QVERIFY(test.setProperty(propertyName, QVariant::fromValue<QtProtobuf::uint32List>({1, 2, 3, 4, 5})));
    QCOMPARE(test.property(propertyName).value<QtProtobuf::uint32List>(), QtProtobuf::uint32List({1, 2, 3, 4, 5}));
    QCOMPARE(test.testRepeatedInt(), QtProtobuf::uint32List({1, 2, 3, 4, 5}));

    test.testRepeatedInt().append(6);
    QCOMPARE(test.testRepeatedInt(), QtProtobuf::uint32List({1, 2, 3, 4, 5,6}));

    test.testRepeatedInt().pop_back();
    QCOMPARE(test.testRepeatedInt(), QtProtobuf::uint32List({1, 2, 3, 4, 5}));
}

void QtProtobufRepeatedTypesGenerationTest::RepeatedInt64MessageTest()
{
    const char *propertyName = "testRepeatedInt";
    qProtobufAssertMessagePropertyRegistered<RepeatedInt64Message, QtProtobuf::int64List>(1, "QtProtobuf::int64List", propertyName);

    RepeatedInt64Message test;
    QVERIFY(test.setProperty(propertyName, QVariant::fromValue<QtProtobuf::int64List>({1, 2, 3, 4, 5})));
    QCOMPARE(test.property(propertyName).value<QtProtobuf::int64List>(), QtProtobuf::int64List({1, 2, 3, 4, 5}));
    QCOMPARE(test.testRepeatedInt(), QtProtobuf::int64List({1, 2, 3, 4, 5}));

    test.testRepeatedInt().append(69);
    QCOMPARE(test.testRepeatedInt(), QtProtobuf::int64List({1, 2, 3, 4, 5, 69}));

    test.testRepeatedInt().pop_back();
    QCOMPARE(test.testRepeatedInt(), QtProtobuf::int64List({1, 2, 3, 4, 5}));
}

void QtProtobufRepeatedTypesGenerationTest::RepeatedSInt64MessageTest()
{
    const char *propertyName = "testRepeatedInt";
    qProtobufAssertMessagePropertyRegistered<RepeatedSInt64Message, QtProtobuf::sint64List>(1, "QtProtobuf::sint64List", propertyName);

    RepeatedSInt64Message test;
    QVERIFY(test.setProperty(propertyName, QVariant::fromValue<QtProtobuf::sint64List>({1, 2, 3, 4, 5})));
    QCOMPARE(test.property(propertyName).value<QtProtobuf::sint64List>(), QtProtobuf::sint64List({1, 2, 3, 4, 5}));
    QCOMPARE(test.testRepeatedInt(), QtProtobuf::sint64List({1, 2, 3, 4, 5}));

    test.testRepeatedInt() << 96;
    QCOMPARE(test.testRepeatedInt(), QtProtobuf::sint64List({1, 2, 3, 4, 5, 96}));

    test.testRepeatedInt().pop_back();
    QCOMPARE(test.testRepeatedInt(), QtProtobuf::sint64List({1, 2, 3, 4, 5}));
}

void QtProtobufRepeatedTypesGenerationTest::RepeatedUInt64MessageTest()
{
    const char *propertyName = "testRepeatedInt";
    qProtobufAssertMessagePropertyRegistered<RepeatedUInt64Message, QtProtobuf::uint64List>(1, "QtProtobuf::uint64List", propertyName);

    RepeatedUInt64Message test;
    QVERIFY(test.setProperty(propertyName, QVariant::fromValue<QtProtobuf::uint64List>({1, 2, 3, 4, 5})));
    QCOMPARE(test.property(propertyName).value<QtProtobuf::uint64List>(), QtProtobuf::uint64List({1, 2, 3, 4, 5}));
    QCOMPARE(test.testRepeatedInt(), QtProtobuf::uint64List({1, 2, 3, 4, 5}));

    test.testRepeatedInt().append(96);
    QCOMPARE(test.testRepeatedInt(), QtProtobuf::uint64List({1, 2, 3, 4, 5, 96}));

    test.testRepeatedInt().pop_back();
    QCOMPARE(test.testRepeatedInt(), QtProtobuf::uint64List({1, 2, 3, 4, 5}));
}

void QtProtobufRepeatedTypesGenerationTest::RepeatedFixedIntMessageTest()
{
    const char *propertyName = "testRepeatedInt";
    qProtobufAssertMessagePropertyRegistered<RepeatedFixedIntMessage, QtProtobuf::fixed32List>(1, "QtProtobuf::fixed32List", propertyName);

    RepeatedFixedIntMessage test;
    QVERIFY(test.setProperty(propertyName, QVariant::fromValue<QtProtobuf::fixed32List>({1, 2, 3, 4, 5})));
    QCOMPARE(test.property(propertyName).value<QtProtobuf::fixed32List>(), QtProtobuf::fixed32List({1, 2, 3, 4, 5}));
    QCOMPARE(test.testRepeatedInt(), QtProtobuf::fixed32List({1, 2, 3, 4, 5}));

    test.testRepeatedInt() << 0;
    QCOMPARE(test.testRepeatedInt(), QtProtobuf::fixed32List({1, 2, 3, 4, 5, 0}));

    test.testRepeatedInt().pop_back();
    QCOMPARE(test.testRepeatedInt(), QtProtobuf::fixed32List({1, 2, 3, 4, 5}));
}

void QtProtobufRepeatedTypesGenerationTest::RepeatedFixedInt64MessageTest()
{
    const char *propertyName = "testRepeatedInt";
    qProtobufAssertMessagePropertyRegistered<RepeatedFixedInt64Message, QtProtobuf::fixed64List>(1, "QtProtobuf::fixed64List", propertyName);

    RepeatedFixedInt64Message test;
    QVERIFY(test.setProperty(propertyName, QVariant::fromValue<QtProtobuf::fixed64List>({1, 2, 3, 4, 5})));
    QCOMPARE(test.property(propertyName).value<QtProtobuf::fixed64List>(), QtProtobuf::fixed64List({1, 2, 3, 4, 5}));
    QCOMPARE(test.testRepeatedInt(), QtProtobuf::fixed64List({1, 2, 3, 4, 5}));

    test.testRepeatedInt() << 0;
    QCOMPARE(test.testRepeatedInt(), QtProtobuf::fixed64List({1, 2, 3, 4, 5, 0}));

    test.testRepeatedInt().pop_back();
    QCOMPARE(test.testRepeatedInt(), QtProtobuf::fixed64List({1, 2, 3, 4, 5}));
}

void QtProtobufRepeatedTypesGenerationTest::RepeatedSFixedIntMessageTest()
{
    const char *propertyName = "testRepeatedInt";
    qProtobufAssertMessagePropertyRegistered<RepeatedSFixedIntMessage, QtProtobuf::sfixed32List>(1, "QtProtobuf::sfixed32List", propertyName);

    RepeatedSFixedIntMessage test;
    QVERIFY(test.setProperty(propertyName, QVariant::fromValue<QtProtobuf::sfixed32List>({1, 2, 3, 4, 5})));
    QCOMPARE(test.property(propertyName).value<QtProtobuf::sfixed32List>(), QtProtobuf::sfixed32List({1, 2, 3, 4, 5}));
    QCOMPARE(test.testRepeatedInt(), QtProtobuf::sfixed32List({1, 2, 3, 4, 5}));

    test.testRepeatedInt() << 0;
    QCOMPARE(test.testRepeatedInt(), QtProtobuf::sfixed32List({1, 2, 3, 4, 5, 0}));

    test.testRepeatedInt().pop_back();
    QCOMPARE(test.testRepeatedInt(), QtProtobuf::sfixed32List({1, 2, 3, 4, 5}));
}

void QtProtobufRepeatedTypesGenerationTest::RepeatedSFixedInt64MessageTest()
{
    const char *propertyName = "testRepeatedInt";
    qProtobufAssertMessagePropertyRegistered<RepeatedSFixedInt64Message, QtProtobuf::sfixed64List>(1, "QtProtobuf::sfixed64List", propertyName);

    RepeatedSFixedInt64Message test;
    QVERIFY(test.setProperty(propertyName, QVariant::fromValue<QtProtobuf::sfixed64List>({1, 2, 3, 4, 5})));
    QCOMPARE(test.property(propertyName).value<QtProtobuf::sfixed64List>(), QtProtobuf::sfixed64List({1, 2, 3, 4, 5}));
    QCOMPARE(test.testRepeatedInt(), QtProtobuf::sfixed64List({1, 2, 3, 4, 5}));

    test.testRepeatedInt() << 0;
    QCOMPARE(test.testRepeatedInt(), QtProtobuf::sfixed64List({1, 2, 3, 4, 5, 0}));

    test.testRepeatedInt().pop_back();
    QCOMPARE(test.testRepeatedInt(), QtProtobuf::sfixed64List({1, 2, 3, 4, 5}));
}

void QtProtobufRepeatedTypesGenerationTest::RepeatedBoolTest()
{
    const char *propertyName = "testRepeatedBool";
    qProtobufAssertMessagePropertyRegistered<RepeatedBoolMessage, QtProtobuf::boolList>(
            1, "QtProtobuf::boolList", propertyName);

    RepeatedBoolMessage test;
    QtProtobuf::boolList list { false, false, false, true, false };
    QVERIFY(test.setProperty(propertyName, QVariant::fromValue(list)));
    QCOMPARE(test.property(propertyName).value<QtProtobuf::boolList>(), list);
    QCOMPARE(test.testRepeatedBool(), list);

    test.testRepeatedBool() << true;
    QtProtobuf::boolList listCopy = list;
    listCopy.append(true);
    QCOMPARE(test.testRepeatedBool(), listCopy);

    test.testRepeatedBool().pop_back();
    QCOMPARE(test.testRepeatedBool(), list);
}

void QtProtobufRepeatedTypesGenerationTest::MoveOperatorRepeatedTest()
{
    const char *propertyName = "testRepeatedInt";
    RepeatedIntMessage test;
    RepeatedIntMessage test2;
    test.setProperty(propertyName, QVariant::fromValue<QtProtobuf::int32List>({55}));

    test2.setTestRepeatedInt({ 55, 44, 11, 33 });
    test = std::move(test2);
    QCOMPARE(QtProtobuf::int32List({ 55, 44, 11, 33 }), test.testRepeatedInt());

    RepeatedIntMessage test3;
    test3.setTestRepeatedInt({ 55, 44, 11, 35 });
    RepeatedIntMessage test4(std::move(test3));

    QCOMPARE(QtProtobuf::int32List({ 55, 44, 11, 35 }), test4.testRepeatedInt());
}

void QtProtobufRepeatedTypesGenerationTest::RepeatedComplexMessageCompareTest()
{
    SimpleStringMessage stringMsg;

    ComplexMessage msg1;
    stringMsg.setTestFieldString("qwerty");
    msg1.setTestFieldInt(10);
    msg1.setTestComplexField(stringMsg);

    ComplexMessage msg2;
    stringMsg.setTestFieldString("ytrewq");
    msg2.setTestFieldInt(20);
    msg2.setTestComplexField(stringMsg);

    ComplexMessage msg3;
    stringMsg.setTestFieldString("qwerty");
    msg3.setTestFieldInt(10);
    msg3.setTestComplexField(stringMsg);

    ComplexMessage msg4;
    stringMsg.setTestFieldString("ytrewq");
    msg4.setTestFieldInt(20);
    msg4.setTestComplexField(stringMsg);

    QCOMPARE(msg1, msg3);
    QCOMPARE(msg2, msg4);

    RepeatedComplexMessage test1;
    test1.setTestRepeatedComplex({ msg1, msg2 });

    RepeatedComplexMessage test2;
    test2.setTestRepeatedComplex({ msg3, msg4 });

    RepeatedComplexMessage test3;
    test3.setTestRepeatedComplex({ msg4, msg3 });

    QCOMPARE(test1, test2);
    QVERIFY(!(test3 == test2));
}

QTEST_MAIN(QtProtobufRepeatedTypesGenerationTest)
#include "tst_protobuf_repeatedtypes.moc"
