// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2022 Alexey Edelev <semlanik@gmail.com>, Viktor Kopp <vifactor@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "enummessages.qpb.h"

#include <QMetaProperty>
#include <QSignalSpy>
#include <QTest>

#include <qtprotobuftestscommon.h>

class QtProtobufEnumTypesGenerationTest : public QObject
{
    Q_OBJECT
private slots:
    void BasicTest();
    void LocalEnumTest();
    void LocalEnumListTest();
    void MixedEnumUsageTest();
    void FileEnumsTest();
    void StepChildEnumMessageTest();
    void StepChildEnumListMessageTest();
};

using namespace qtprotobufnamespace::tests;

void QtProtobufEnumTypesGenerationTest::BasicTest()
{
    QVERIFY(TestEnumGadget::staticMetaObject.enumeratorCount() > 0);
    QMetaEnum testEnum;
    for (int i = 0; i < TestEnumGadget::staticMetaObject.enumeratorCount(); i++) {
        QMetaEnum tmp = TestEnumGadget::staticMetaObject.enumerator(i);
        if (QString(tmp.name()) == QString("TestEnum")) {
            testEnum = tmp;
            break;
        }
    }
    QVERIFY(testEnum.isValid());
    QCOMPARE(testEnum.key(0), "TEST_ENUM_VALUE0");
    QCOMPARE(testEnum.key(1), "TEST_ENUM_VALUE1");
    QCOMPARE(testEnum.key(2), "TEST_ENUM_VALUE2");
    QCOMPARE(testEnum.key(3), "TEST_ENUM_VALUE3");
    QCOMPARE(testEnum.key(4), "TEST_ENUM_VALUE4");

    QCOMPARE(testEnum.value(0), 0);
    QCOMPARE(testEnum.value(1), 1);
    QCOMPARE(testEnum.value(2), 2);
    QCOMPARE(testEnum.value(3), 4);
    QCOMPARE(testEnum.value(4), 3);

    for (int i = 0; i < TestEnumSecondInFileGadget::staticMetaObject.enumeratorCount(); i++) {
        QMetaEnum tmp = TestEnumSecondInFileGadget::staticMetaObject.enumerator(i);
        if (QString(tmp.name()) == QString("TestEnumSecondInFile")) {
            testEnum = tmp;
            break;
        }
    }

    QVERIFY(testEnum.isValid());
    QCOMPARE(testEnum.key(0), "TEST_ENUM_SIF_VALUE0");
    QCOMPARE(testEnum.key(1), "TEST_ENUM_SIF_VALUE1");
    QCOMPARE(testEnum.key(2), "TEST_ENUM_SIF_VALUE2");

    QCOMPARE(testEnum.value(0), 0);
    QCOMPARE(testEnum.value(1), 1);
    QCOMPARE(testEnum.value(2), 2);
}

void QtProtobufEnumTypesGenerationTest::LocalEnumTest()
{
    const auto &metaObject = SimpleEnumMessage_QtProtobufNested::staticMetaObject;
    QVERIFY(metaObject.enumeratorCount() > 0);
    QMetaEnum simpleEnum;
    for (int i = 0; i < metaObject.enumeratorCount(); i++) {
        QMetaEnum tmp = metaObject.enumerator(i);
        qDebug() << tmp.name();
        if (QString(tmp.name()) == QString("LocalEnum")) {
            simpleEnum = tmp;
            break;
        }
    }
    QVERIFY(simpleEnum.isValid());
    QCOMPARE(simpleEnum.key(0), "LOCAL_ENUM_VALUE0");
    QCOMPARE(simpleEnum.key(1), "LOCAL_ENUM_VALUE1");
    QCOMPARE(simpleEnum.key(2), "LOCAL_ENUM_VALUE2");
    QCOMPARE(simpleEnum.key(3), "LOCAL_ENUM_VALUE3");

    QCOMPARE(simpleEnum.value(0), 0);
    QCOMPARE(simpleEnum.value(1), 1);
    QCOMPARE(simpleEnum.value(2), 2);
    QCOMPARE(simpleEnum.value(3), 3);
}

void QtProtobufEnumTypesGenerationTest::MixedEnumUsageTest()
{
    QVERIFY(MixedEnumUsageMessage_QtProtobufNested::staticMetaObject.enumeratorCount() > 0);

    const char *propertyName = "localEnumMap";
    qProtobufAssertMessagePropertyRegistered<MixedEnumUsageMessage,
                                             MixedEnumUsageMessage::LocalEnumMapEntry>(
            3, "MixedEnumUsageMessage::LocalEnumMapEntry", propertyName);

    MixedEnumUsageMessage::LocalEnumMapEntry value(
            { { "value1", MixedEnumUsageMessage::LocalEnum::LOCAL_ENUM_VALUE2 },
              { "value2", MixedEnumUsageMessage::LocalEnum::LOCAL_ENUM_VALUE2 },
              { "value3", MixedEnumUsageMessage::LocalEnum::LOCAL_ENUM_VALUE1 },
              { "value4", MixedEnumUsageMessage::LocalEnum::LOCAL_ENUM_VALUE3 } });

    MixedEnumUsageMessage test;
    QVERIFY(test.setProperty(propertyName,
                             QVariant::fromValue<MixedEnumUsageMessage::LocalEnumMapEntry>(value)));
    QCOMPARE(test.property(propertyName).value<MixedEnumUsageMessage::LocalEnumMapEntry>(), value);
    QCOMPARE(test.localEnumMap(), value);
}

void QtProtobufEnumTypesGenerationTest::LocalEnumListTest()
{
    QVERIFY(RepeatedEnumMessage_QtProtobufNested::staticMetaObject.enumeratorCount() > 0);

    const char *propertyName = "localEnumList";
    qProtobufAssertMessagePropertyRegistered<RepeatedEnumMessage, RepeatedEnumMessage::LocalEnumRepeated>(1,  "RepeatedEnumMessage::LocalEnumRepeated", propertyName);

    RepeatedEnumMessage::LocalEnumRepeated value({RepeatedEnumMessage::LocalEnum::LOCAL_ENUM_VALUE2,
                                                RepeatedEnumMessage::LocalEnum::LOCAL_ENUM_VALUE2,
                                                RepeatedEnumMessage::LocalEnum::LOCAL_ENUM_VALUE1,
                                                RepeatedEnumMessage::LocalEnum::LOCAL_ENUM_VALUE3});

    RepeatedEnumMessage test;
    QVERIFY(test.setProperty(propertyName, QVariant::fromValue<RepeatedEnumMessage::LocalEnumRepeated>(value)));
    QCOMPARE(test.property(propertyName).value<RepeatedEnumMessage::LocalEnumRepeated>(), value);
    QCOMPARE(test.localEnumList(), value);
}

void QtProtobufEnumTypesGenerationTest::FileEnumsTest()
{
    const char *propertyName = "globalEnumList";
    qProtobufAssertMessagePropertyRegistered<SimpleFileEnumMessage, TestEnumGadget::TestEnumRepeated>(2, "TestEnumGadget::TestEnumRepeated", propertyName);

    TestEnumGadget::TestEnumRepeated value{TestEnumGadget::TestEnum::TEST_ENUM_VALUE1,
                                     TestEnumGadget::TestEnum::TEST_ENUM_VALUE3,
                                     TestEnumGadget::TestEnum::TEST_ENUM_VALUE4,
                                     TestEnumGadget::TestEnum::TEST_ENUM_VALUE2,
                                     TestEnumGadget::TestEnum::TEST_ENUM_VALUE1};
    SimpleFileEnumMessage test;
    QVERIFY(test.setProperty(propertyName, QVariant::fromValue<TestEnumGadget::TestEnumRepeated>(value)));
    QCOMPARE(test.property(propertyName).value<TestEnumGadget::TestEnumRepeated>(), value);
    QCOMPARE(test.globalEnumList(), value);
}

void QtProtobufEnumTypesGenerationTest::StepChildEnumMessageTest()
{
    const char *propertyName = "localStepChildEnum";
    qProtobufAssertMessagePropertyRegistered<StepChildEnumMessage, SimpleEnumMessage::LocalEnum>(1, "SimpleEnumMessage::LocalEnum", propertyName);

    StepChildEnumMessage test;
    QVERIFY(test.setProperty(propertyName, QVariant::fromValue<SimpleEnumMessage::LocalEnum>(SimpleEnumMessage::LocalEnum::LOCAL_ENUM_VALUE2)));
    QCOMPARE(test.property(propertyName).value<SimpleEnumMessage::LocalEnum>(), SimpleEnumMessage::LocalEnum::LOCAL_ENUM_VALUE2);
    QCOMPARE(test.localStepChildEnum(), SimpleEnumMessage::LocalEnum::LOCAL_ENUM_VALUE2);
}

void QtProtobufEnumTypesGenerationTest::StepChildEnumListMessageTest()
{
    const char *propertyName = "localStepChildList";
    qProtobufAssertMessagePropertyRegistered<StepChildEnumMessage, SimpleEnumMessage::LocalEnumRepeated>(2, "SimpleEnumMessage::LocalEnumRepeated", propertyName);

    SimpleEnumMessage::LocalEnumRepeated value({SimpleEnumMessage::LocalEnum::LOCAL_ENUM_VALUE2,
                                            SimpleEnumMessage::LocalEnum::LOCAL_ENUM_VALUE2,
                                            SimpleEnumMessage::LocalEnum::LOCAL_ENUM_VALUE1,
                                            SimpleEnumMessage::LocalEnum::LOCAL_ENUM_VALUE3});
    StepChildEnumMessage test;
    QVERIFY(test.setProperty(propertyName, QVariant::fromValue<SimpleEnumMessage::LocalEnumRepeated>(value)));
    QCOMPARE(test.property(propertyName).value<SimpleEnumMessage::LocalEnumRepeated>(), value);
    QCOMPARE(test.localStepChildList(), value);
}

QTEST_MAIN(QtProtobufEnumTypesGenerationTest)
#include "tst_protobuf_enumtypes.moc"
