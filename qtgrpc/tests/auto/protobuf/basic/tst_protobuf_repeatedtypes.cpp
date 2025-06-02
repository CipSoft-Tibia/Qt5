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
private Q_SLOTS:
    void repeatedStringMessageTest();
    void repeatedIntMessageTest();
    void repeatedDoubleMessageTest();
    void repeatedFloatMessageTest();
    void repeatedBytesMessageTest();
    void repeatedSIntMessageTest();
    void repeatedUIntMessageTest();
    void repeatedInt64MessageTest();
    void repeatedSInt64MessageTest();
    void repeatedUInt64MessageTest();
    void repeatedFixedIntMessageTest();
    void repeatedFixedInt64MessageTest();
    void repeatedSFixedIntMessageTest();
    void repeatedSFixedInt64MessageTest();
    void repeatedBoolTest();

    void moveOperatorRepeatedTest();
    void repeatedComplexMessageCompareTest();
    void rvalueSettersTest();
};

using namespace qtprotobufnamespace::tests;

void QtProtobufRepeatedTypesGenerationTest::repeatedStringMessageTest()
{
    const char *propertyName = "testRepeatedString";
    qProtobufAssertMessagePropertyRegistered<RepeatedStringMessage, QStringList>(1, "QStringList", propertyName);
    QVERIFY(RepeatedStringMessage::staticPropertyOrdering.fieldFlags(0)
            & QtProtobufPrivate::FieldFlag::Repeated);

    RepeatedStringMessage test;
    QVERIFY(test.setProperty(propertyName, QVariant::fromValue<QStringList>({"Text", "tryam"})));
    QCOMPARE(test.property(propertyName).value<QStringList>(), QStringList({"Text", "tryam"}));
    QCOMPARE(test.testRepeatedString(), QStringList({"Text", "tryam"}));
}

void QtProtobufRepeatedTypesGenerationTest::repeatedIntMessageTest()
{
    const char *propertyName = "testRepeatedInt";
    qProtobufAssertMessagePropertyRegistered<RepeatedIntMessage, QtProtobuf::int32List>(1, "QtProtobuf::int32List", propertyName);
    QVERIFY(RepeatedIntMessage::staticPropertyOrdering.fieldFlags(0)
            & QtProtobufPrivate::FieldFlag::Repeated);

    RepeatedIntMessage test;
    QVERIFY(test.setProperty(propertyName, QVariant::fromValue<QtProtobuf::int32List>({1, 2, 3, 4, 5})));
    QCOMPARE(test.property(propertyName).value<QtProtobuf::int32List>(), QtProtobuf::int32List({1, 2, 3, 4, 5}));
    QCOMPARE(test.testRepeatedInt(), QtProtobuf::int32List({1, 2, 3, 4, 5}));
}

void QtProtobufRepeatedTypesGenerationTest::repeatedDoubleMessageTest()
{
    const char *propertyName = "testRepeatedDouble";
    qProtobufAssertMessagePropertyRegistered<RepeatedDoubleMessage, QtProtobuf::doubleList>(1, "QtProtobuf::doubleList", propertyName);
    QVERIFY(RepeatedDoubleMessage::staticPropertyOrdering.fieldFlags(0)
            & QtProtobufPrivate::FieldFlag::Repeated);

    RepeatedDoubleMessage test;
    QVERIFY(test.setProperty(propertyName, QVariant::fromValue<QtProtobuf::doubleList>({1.0, 2.3, 3, 4.7, 5.9})));
    QCOMPARE(test.property(propertyName).value<QtProtobuf::doubleList>(), QtProtobuf::doubleList({1.0, 2.3, 3, 4.7, 5.9}));
    QCOMPARE(test.testRepeatedDouble(), QtProtobuf::doubleList({1.0, 2.3, 3, 4.7, 5.9}));
}

void QtProtobufRepeatedTypesGenerationTest::repeatedFloatMessageTest()
{
    const char *propertyName = "testRepeatedFloat";
    qProtobufAssertMessagePropertyRegistered<RepeatedFloatMessage, QtProtobuf::floatList>(1, "QtProtobuf::floatList", propertyName);
    QVERIFY(RepeatedFloatMessage::staticPropertyOrdering.fieldFlags(0)
            & QtProtobufPrivate::FieldFlag::Repeated);

    RepeatedFloatMessage test;
    QVERIFY(test.setProperty(propertyName, QVariant::fromValue<QtProtobuf::floatList>({1.0f, 2.3f, 3, 4.7f, 5.9f})));
    QCOMPARE(test.property(propertyName).value<QtProtobuf::floatList>(), QtProtobuf::floatList({1.0f, 2.3f, 3, 4.7f, 5.9f}));
    QCOMPARE(test.testRepeatedFloat(), QtProtobuf::floatList({1.0f, 2.3f, 3, 4.7f, 5.9f}));
}

void QtProtobufRepeatedTypesGenerationTest::repeatedBytesMessageTest()
{
    const char *propertyName = "testRepeatedBytes";
    qProtobufAssertMessagePropertyRegistered<RepeatedBytesMessage, QByteArrayList>(1, "QByteArrayList", propertyName);
    QVERIFY(RepeatedBytesMessage::staticPropertyOrdering.fieldFlags(0)
            & QtProtobufPrivate::FieldFlag::Repeated);

    QByteArrayList bList;
    bList << "\x01\x02\x03\x04\x05";
    bList << "\x01\x05\x03\x04\x03";

    RepeatedBytesMessage test;
    QVERIFY(test.setProperty(propertyName, QVariant::fromValue<QByteArrayList>(bList)));
    QCOMPARE(test.property(propertyName).value<QByteArrayList>(), bList);
    QCOMPARE(test.testRepeatedBytes(), bList);
}

void QtProtobufRepeatedTypesGenerationTest::repeatedSIntMessageTest()
{
    const char *propertyName = "testRepeatedInt";
    qProtobufAssertMessagePropertyRegistered<RepeatedSIntMessage, QtProtobuf::sint32List>(1, "QtProtobuf::sint32List", propertyName);
    QVERIFY(RepeatedSIntMessage::staticPropertyOrdering.fieldFlags(0)
            & QtProtobufPrivate::FieldFlag::Repeated);

    RepeatedSIntMessage test;
    QVERIFY(test.setProperty(propertyName, QVariant::fromValue<QtProtobuf::sint32List>({1, 2, 3, 4, 5})));
    QCOMPARE(test.property(propertyName).value<QtProtobuf::sint32List>(), QtProtobuf::sint32List({1, 2, 3, 4, 5}));
    QCOMPARE(test.testRepeatedInt(), QtProtobuf::sint32List({1, 2, 3, 4, 5}));
}

void QtProtobufRepeatedTypesGenerationTest::repeatedUIntMessageTest()
{
    const char *propertyName = "testRepeatedInt";
    qProtobufAssertMessagePropertyRegistered<RepeatedUIntMessage, QtProtobuf::uint32List>(1, "QtProtobuf::uint32List", propertyName);
    QVERIFY(RepeatedUIntMessage::staticPropertyOrdering.fieldFlags(0)
            & QtProtobufPrivate::FieldFlag::Repeated);

    RepeatedUIntMessage test;
    QVERIFY(test.setProperty(propertyName, QVariant::fromValue<QtProtobuf::uint32List>({1, 2, 3, 4, 5})));
    QCOMPARE(test.property(propertyName).value<QtProtobuf::uint32List>(), QtProtobuf::uint32List({1, 2, 3, 4, 5}));
    QCOMPARE(test.testRepeatedInt(), QtProtobuf::uint32List({1, 2, 3, 4, 5}));
}

void QtProtobufRepeatedTypesGenerationTest::repeatedInt64MessageTest()
{
    const char *propertyName = "testRepeatedInt";
    qProtobufAssertMessagePropertyRegistered<RepeatedInt64Message, QtProtobuf::int64List>(1, "QtProtobuf::int64List", propertyName);
    QVERIFY(RepeatedInt64Message::staticPropertyOrdering.fieldFlags(0)
            & QtProtobufPrivate::FieldFlag::Repeated);

    RepeatedInt64Message test;
    QVERIFY(test.setProperty(propertyName, QVariant::fromValue<QtProtobuf::int64List>({1, 2, 3, 4, 5})));
    QCOMPARE(test.property(propertyName).value<QtProtobuf::int64List>(), QtProtobuf::int64List({1, 2, 3, 4, 5}));
    QCOMPARE(test.testRepeatedInt(), QtProtobuf::int64List({1, 2, 3, 4, 5}));
}

void QtProtobufRepeatedTypesGenerationTest::repeatedSInt64MessageTest()
{
    const char *propertyName = "testRepeatedInt";
    qProtobufAssertMessagePropertyRegistered<RepeatedSInt64Message, QtProtobuf::sint64List>(1, "QtProtobuf::sint64List", propertyName);
    QVERIFY(RepeatedSInt64Message::staticPropertyOrdering.fieldFlags(0)
            & QtProtobufPrivate::FieldFlag::Repeated);

    RepeatedSInt64Message test;
    QVERIFY(test.setProperty(propertyName, QVariant::fromValue<QtProtobuf::sint64List>({1, 2, 3, 4, 5})));
    QCOMPARE(test.property(propertyName).value<QtProtobuf::sint64List>(), QtProtobuf::sint64List({1, 2, 3, 4, 5}));
    QCOMPARE(test.testRepeatedInt(), QtProtobuf::sint64List({1, 2, 3, 4, 5}));
}

void QtProtobufRepeatedTypesGenerationTest::repeatedUInt64MessageTest()
{
    const char *propertyName = "testRepeatedInt";
    qProtobufAssertMessagePropertyRegistered<RepeatedUInt64Message, QtProtobuf::uint64List>(1, "QtProtobuf::uint64List", propertyName);
    QVERIFY(RepeatedUInt64Message::staticPropertyOrdering.fieldFlags(0)
            & QtProtobufPrivate::FieldFlag::Repeated);

    RepeatedUInt64Message test;
    QVERIFY(test.setProperty(propertyName, QVariant::fromValue<QtProtobuf::uint64List>({1, 2, 3, 4, 5})));
    QCOMPARE(test.property(propertyName).value<QtProtobuf::uint64List>(), QtProtobuf::uint64List({1, 2, 3, 4, 5}));
    QCOMPARE(test.testRepeatedInt(), QtProtobuf::uint64List({1, 2, 3, 4, 5}));
}

void QtProtobufRepeatedTypesGenerationTest::repeatedFixedIntMessageTest()
{
    const char *propertyName = "testRepeatedInt";
    qProtobufAssertMessagePropertyRegistered<RepeatedFixedIntMessage, QtProtobuf::fixed32List>(1, "QtProtobuf::fixed32List", propertyName);
    QVERIFY(RepeatedFixedIntMessage::staticPropertyOrdering.fieldFlags(0)
            & QtProtobufPrivate::FieldFlag::Repeated);

    RepeatedFixedIntMessage test;
    QVERIFY(test.setProperty(propertyName, QVariant::fromValue<QtProtobuf::fixed32List>({1, 2, 3, 4, 5})));
    QCOMPARE(test.property(propertyName).value<QtProtobuf::fixed32List>(), QtProtobuf::fixed32List({1, 2, 3, 4, 5}));
    QCOMPARE(test.testRepeatedInt(), QtProtobuf::fixed32List({1, 2, 3, 4, 5}));
}

void QtProtobufRepeatedTypesGenerationTest::repeatedFixedInt64MessageTest()
{
    const char *propertyName = "testRepeatedInt";
    qProtobufAssertMessagePropertyRegistered<RepeatedFixedInt64Message, QtProtobuf::fixed64List>(1, "QtProtobuf::fixed64List", propertyName);
    QVERIFY(RepeatedFixedInt64Message::staticPropertyOrdering.fieldFlags(0)
            & QtProtobufPrivate::FieldFlag::Repeated);

    RepeatedFixedInt64Message test;
    QVERIFY(test.setProperty(propertyName, QVariant::fromValue<QtProtobuf::fixed64List>({1, 2, 3, 4, 5})));
    QCOMPARE(test.property(propertyName).value<QtProtobuf::fixed64List>(), QtProtobuf::fixed64List({1, 2, 3, 4, 5}));
    QCOMPARE(test.testRepeatedInt(), QtProtobuf::fixed64List({1, 2, 3, 4, 5}));
}

void QtProtobufRepeatedTypesGenerationTest::repeatedSFixedIntMessageTest()
{
    const char *propertyName = "testRepeatedInt";
    qProtobufAssertMessagePropertyRegistered<RepeatedSFixedIntMessage, QtProtobuf::sfixed32List>(1, "QtProtobuf::sfixed32List", propertyName);
    QVERIFY(RepeatedSFixedIntMessage::staticPropertyOrdering.fieldFlags(0)
            & QtProtobufPrivate::FieldFlag::Repeated);

    RepeatedSFixedIntMessage test;
    QVERIFY(test.setProperty(propertyName, QVariant::fromValue<QtProtobuf::sfixed32List>({1, 2, 3, 4, 5})));
    QCOMPARE(test.property(propertyName).value<QtProtobuf::sfixed32List>(), QtProtobuf::sfixed32List({1, 2, 3, 4, 5}));
    QCOMPARE(test.testRepeatedInt(), QtProtobuf::sfixed32List({1, 2, 3, 4, 5}));
}

void QtProtobufRepeatedTypesGenerationTest::repeatedSFixedInt64MessageTest()
{
    const char *propertyName = "testRepeatedInt";
    qProtobufAssertMessagePropertyRegistered<RepeatedSFixedInt64Message, QtProtobuf::sfixed64List>(1, "QtProtobuf::sfixed64List", propertyName);
    QVERIFY(RepeatedSFixedInt64Message::staticPropertyOrdering.fieldFlags(0)
            & QtProtobufPrivate::FieldFlag::Repeated);

    RepeatedSFixedInt64Message test;
    QVERIFY(test.setProperty(propertyName, QVariant::fromValue<QtProtobuf::sfixed64List>({1, 2, 3, 4, 5})));
    QCOMPARE(test.property(propertyName).value<QtProtobuf::sfixed64List>(), QtProtobuf::sfixed64List({1, 2, 3, 4, 5}));
    QCOMPARE(test.testRepeatedInt(), QtProtobuf::sfixed64List({1, 2, 3, 4, 5}));
}

void QtProtobufRepeatedTypesGenerationTest::repeatedBoolTest()
{
    const char *propertyName = "testRepeatedBool";
    qProtobufAssertMessagePropertyRegistered<RepeatedBoolMessage, QtProtobuf::boolList>(
            1, "QtProtobuf::boolList", propertyName);
    QVERIFY(RepeatedBoolMessage::staticPropertyOrdering.fieldFlags(0)
            & QtProtobufPrivate::FieldFlag::Repeated);

    RepeatedBoolMessage test;
    QtProtobuf::boolList list { false, false, false, true, false };
    QVERIFY(test.setProperty(propertyName, QVariant::fromValue(list)));
    QCOMPARE(test.property(propertyName).value<QtProtobuf::boolList>(), list);
    QCOMPARE(test.testRepeatedBool(), list);
}

void QtProtobufRepeatedTypesGenerationTest::moveOperatorRepeatedTest()
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

void QtProtobufRepeatedTypesGenerationTest::repeatedComplexMessageCompareTest()
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

void QtProtobufRepeatedTypesGenerationTest::rvalueSettersTest()
{
    RepeatedIntMessage test;
}

QTEST_MAIN(QtProtobufRepeatedTypesGenerationTest)
#include "tst_protobuf_repeatedtypes.moc"
