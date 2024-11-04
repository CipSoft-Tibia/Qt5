// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2022 Alexey Edelev <semlanik@gmail.com>, Viktor Kopp <vifactor@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "basicmessages.qpb.h"
#include "mapmessages.qpb.h"

#include <QMetaProperty>
#include <QSignalSpy>
#include <QTest>

#include <qtprotobuftestscommon.h>

using namespace Qt::Literals::StringLiterals;

class QtProtobufMapTypesGenerationTest : public QObject
{
    Q_OBJECT
private slots:
    void SInt32StringMapMessageTest();
    void StringStringMapMessageTest();

    void SimpleInt32ComplexMessageMapMessageCompareTest();
};

using namespace qtprotobufnamespace::tests;

void QtProtobufMapTypesGenerationTest::SInt32StringMapMessageTest()
{
    const char *propertyName = "mapField";
    qProtobufAssertMessagePropertyRegistered<SimpleSInt32StringMapMessage, SimpleSInt32StringMapMessage::MapFieldEntry>(1, "qtprotobufnamespace::tests::SimpleSInt32StringMapMessage::MapFieldEntry", propertyName);

    QVERIFY(QMetaType::fromType<SimpleSInt32StringMapMessage::MapFieldEntry>().isRegistered());

    SimpleSInt32StringMapMessage::MapFieldEntry testMap = {{10, {"Some 10"}}, {0, {"Some 0"}}, {44, {"Some 44"}}};
    SimpleSInt32StringMapMessage test;
    test.setMapField(testMap);
    QCOMPARE(test.property(propertyName).value<SimpleSInt32StringMapMessage::MapFieldEntry>(), testMap);
    QCOMPARE(test.mapField(), testMap);
    QCOMPARE(test.mapField()[10], "Some 10"_L1);
    QCOMPARE(test.mapField()[0], "Some 0"_L1);
    QCOMPARE(test.mapField()[44], "Some 44"_L1);

    test.mapField()[66] = "Some 66";
    QCOMPARE(test.mapField()[66], "Some 66"_L1);

    test.mapField()[66] = "Some default";
    QCOMPARE(test.mapField()[66], "Some default"_L1);
}

void QtProtobufMapTypesGenerationTest::StringStringMapMessageTest()
{
    const char *propertyName = "mapField";
    qProtobufAssertMessagePropertyRegistered<SimpleStringStringMapMessage, SimpleStringStringMapMessage::MapFieldEntry>(13, "qtprotobufnamespace::tests::SimpleStringStringMapMessage::MapFieldEntry", propertyName);

    QVERIFY(QMetaType::fromType<SimpleStringStringMapMessage::MapFieldEntry>().isRegistered());

    SimpleStringStringMapMessage::MapFieldEntry testMap = {{"key 10", "Some 10"}, {"key 0", "Some 0"}, {"key 44", "Some 44"}};
    SimpleStringStringMapMessage test;
    test.setMapField(testMap);
    QCOMPARE(test.property(propertyName).value<SimpleStringStringMapMessage::MapFieldEntry>(), testMap);
    QCOMPARE(test.mapField(), testMap);
    QCOMPARE(test.mapField()["key 10"], "Some 10"_L1);
    QCOMPARE(test.mapField()["key 0"], "Some 0"_L1);
    QCOMPARE(test.mapField()["key 44"], "Some 44"_L1);

    test.mapField()["key 66"] = "Some 66";
    QCOMPARE(test.mapField()["key 66"], "Some 66"_L1);

    test.mapField()["key 66"] = "Some default";
    QCOMPARE(test.mapField()["key 66"], "Some default"_L1);
}

void QtProtobufMapTypesGenerationTest::SimpleInt32ComplexMessageMapMessageCompareTest()
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

    SimpleInt32ComplexMessageMapMessage test1;
    test1.setMapField({ { 20, msg1 }, { 30, msg2 } });
    SimpleInt32ComplexMessageMapMessage test2;
    test2.setMapField({ { 20, msg3 }, { 30, msg4 } });

    QCOMPARE(test1, test2);
}

QTEST_MAIN(QtProtobufMapTypesGenerationTest)
#include "tst_protobuf_maptypes.moc"
