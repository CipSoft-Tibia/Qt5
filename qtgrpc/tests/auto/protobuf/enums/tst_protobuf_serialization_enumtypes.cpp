// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2022 Alexey Edelev <semlanik@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "enummessages.qpb.h"

#include <QTest>
#include <QProtobufSerializer>

class QtProtobufEnumTypesSerializationTest : public QObject
{
    Q_OBJECT
private slots:
    void SimpleEnumMessageSerializeTest();
    void RepeatedEnumMessageTest();

    void init() {
        m_serializer.reset(new QProtobufSerializer);
    }
private:
    std::unique_ptr<QProtobufSerializer> m_serializer;
};

using namespace qtprotobufnamespace::tests;
using namespace Qt::Literals::StringLiterals;

void QtProtobufEnumTypesSerializationTest::SimpleEnumMessageSerializeTest()
{
    SimpleEnumMessage test;
    test.setLocalEnum(SimpleEnumMessage::LOCAL_ENUM_VALUE2);
    QByteArray result = test.serialize(m_serializer.get());
    QCOMPARE(result.size(), 2);
    QCOMPARE(result.toHex(), "0802"_ba);
}

void QtProtobufEnumTypesSerializationTest::RepeatedEnumMessageTest()
{
    RepeatedEnumMessage msg;

    msg.setLocalEnumList({RepeatedEnumMessage::LOCAL_ENUM_VALUE0,
                          RepeatedEnumMessage::LOCAL_ENUM_VALUE1,
                          RepeatedEnumMessage::LOCAL_ENUM_VALUE2,
                          RepeatedEnumMessage::LOCAL_ENUM_VALUE1,
                          RepeatedEnumMessage::LOCAL_ENUM_VALUE2,
                          RepeatedEnumMessage::LOCAL_ENUM_VALUE3});
    QByteArray result = msg.serialize(m_serializer.get());
    QCOMPARE(result.toHex(), "0a06000102010203"_ba);
    msg.setLocalEnumList({});
    result = msg.serialize(m_serializer.get());
    QCOMPARE(result.toHex(), ""_ba);
}

QTEST_MAIN(QtProtobufEnumTypesSerializationTest)
#include "tst_protobuf_serialization_enumtypes.moc"
