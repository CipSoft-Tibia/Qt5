// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2022 Alexey Edelev <semlanik@gmail.com>, Viktor Kopp <vifactor@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "syntax.qpb.h"

#include <QMetaProperty>
#include <QSignalSpy>
#include <QTest>

#include <qtprotobuftestscommon.h>

class QtProtobufSyntaxTest : public QObject
{
    Q_OBJECT
private slots:
    void UnderscoresTest();
    void UpperCaseTest();
    void ReservedTest();
    void ReservedUpperCaseTest();
    void ReservedEnumTest();
    void LowerCaseEnumTest();
    void UpperCaseEnumTest();
};

using namespace qtprotobufnamespace::tests;

void QtProtobufSyntaxTest::UnderscoresTest()
{
    //Sanity compilation checks
    Message_Uderscore_name msg1;
    MessageUderscorename msg2;
    MessageUnderscoreField msg3;
    PriorMessageUnderscoreField msg4;
    FollowingMessageUnderscoreField msg5;
    CombinedMessageUnderscoreField msg6;

    qProtobufAssertMessagePropertyRegistered<MessageUnderscoreField, QtProtobuf::sint32>(1, "QtProtobuf::sint32", "underScoreMessageField");
    qProtobufAssertMessagePropertyRegistered<PriorMessageUnderscoreField, QtProtobuf::sint32>(1, "QtProtobuf::sint32", "underScoreMessageField");
    qProtobufAssertMessagePropertyRegistered<PriorMessageUnderscoreField, QtProtobuf::sint32>(1, "QtProtobuf::sint32", "underScoreMessageField");
    qProtobufAssertMessagePropertyRegistered<FollowingMessageUnderscoreField , QtProtobuf::sint32>(1, "QtProtobuf::sint32", "underScoreMessageField");
    qProtobufAssertMessagePropertyRegistered<CombinedMessageUnderscoreField , QtProtobuf::sint32>(1, "QtProtobuf::sint32", "underScoreMessageField");
}

void QtProtobufSyntaxTest::UpperCaseTest()
{
    qProtobufAssertMessagePropertyRegistered<MessageUpperCase, QtProtobuf::sint32>(1, "QtProtobuf::sint32", "testField");
}

void QtProtobufSyntaxTest::ReservedTest()
{
    qProtobufAssertMessagePropertyRegistered<MessageReserved, QtProtobuf::sint32>(1, "QtProtobuf::sint32", "import_proto");
    qProtobufAssertMessagePropertyRegistered<MessageReserved, QtProtobuf::sint32>(2, "QtProtobuf::sint32", "property_proto");
    qProtobufAssertMessagePropertyRegistered<MessageReserved, QtProtobuf::sint32>(3, "QtProtobuf::sint32", "id_proto");
}

void QtProtobufSyntaxTest::ReservedUpperCaseTest()
{
    qProtobufAssertMessagePropertyRegistered<MessageUpperCaseReserved, QtProtobuf::sint32>(1, "QtProtobuf::sint32", "import_proto");
    qProtobufAssertMessagePropertyRegistered<MessageUpperCaseReserved, QtProtobuf::sint32>(2, "QtProtobuf::sint32", "property_proto");
    qProtobufAssertMessagePropertyRegistered<MessageUpperCaseReserved, QtProtobuf::sint32>(3, "QtProtobuf::sint32", "id_proto");
}

void QtProtobufSyntaxTest::ReservedEnumTest()
{
    const auto &metaObject = MessageEnumReserved_QtProtobufNested::staticMetaObject;
    QVERIFY(metaObject.enumeratorCount() > 0);
    QMetaEnum simpleEnum;
    for (int i = 0; i < metaObject.enumeratorCount(); i++) {
        QMetaEnum tmp = metaObject.enumerator(i);
        if (QString(tmp.name()) == QString("ReservedEnum")) {
            simpleEnum = tmp;
            break;
        }
    }
    QVERIFY(simpleEnum.isValid());
    QCOMPARE(simpleEnum.key(0), "Import");
    QCOMPARE(simpleEnum.key(1), "Property");
    QCOMPARE(simpleEnum.key(2), "Id");

    QCOMPARE(simpleEnum.value(0), 0);
    QCOMPARE(simpleEnum.value(1), 1);
    QCOMPARE(simpleEnum.value(2), 2);
}

void QtProtobufSyntaxTest::LowerCaseEnumTest()
{
    const auto &metaObject = MessageEnumReserved_QtProtobufNested::staticMetaObject;
    QVERIFY(metaObject.enumeratorCount() > 0);
    QMetaEnum simpleEnum;
    for (int i = 0; i < metaObject.enumeratorCount(); i++) {
        QMetaEnum tmp = metaObject.enumerator(i);
        if (QString(tmp.name()) == QString("LowerCaseEnum")) {
            simpleEnum = tmp;
            break;
        }
    }
    QVERIFY(simpleEnum.isValid());
    QCOMPARE(simpleEnum.key(0), "enumValue0");
    QCOMPARE(simpleEnum.key(1), "enumValue1");
    QCOMPARE(simpleEnum.key(2), "enumValue2");
}

void QtProtobufSyntaxTest::UpperCaseEnumTest()
{
    const auto &metaObject = MessageEnumReserved_QtProtobufNested::staticMetaObject;
    QVERIFY(metaObject.enumeratorCount() > 0);
    QMetaEnum simpleEnum;
    for (int i = 0; i < metaObject.enumeratorCount(); i++) {
        QMetaEnum tmp = metaObject.enumerator(i);
        if (QString(tmp.name()) == QString("UpperCaseEnum")) {
            simpleEnum = tmp;
            break;
        }
    }
    QVERIFY(simpleEnum.isValid());
    QCOMPARE(simpleEnum.key(0), "EnumValue0");
    QCOMPARE(simpleEnum.key(1), "EnumValue1");
    QCOMPARE(simpleEnum.key(2), "EnumValue2");
}

QTEST_MAIN(QtProtobufSyntaxTest)
#include "tst_protobuf_syntax.moc"
