// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2022 Alexey Edelev <semlanik@gmail.com>, Viktor Kopp <vifactor@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "enummessages.qpb.h"

#include <QTest>
#include <QProtobufSerializer>

class QtProtobufEnumTypesDeserializationTest : public QObject
{
    Q_OBJECT
private slots:
    void init()
    {
        m_serializer.reset(new QProtobufSerializer);
    }
    void SimpleEnumMessageDeserializeTest();
    void RepeatedEnumMessageTest();
private:
    std::unique_ptr<QProtobufSerializer> m_serializer;
};

using namespace qtprotobufnamespace::tests;

void QtProtobufEnumTypesDeserializationTest::SimpleEnumMessageDeserializeTest()
{
    SimpleEnumMessage test;
    test.deserialize(m_serializer.get(), QByteArray::fromHex("0803"));
    QCOMPARE(test.localEnum(), SimpleEnumMessage::LOCAL_ENUM_VALUE3);
}

void QtProtobufEnumTypesDeserializationTest::RepeatedEnumMessageTest()
{
    RepeatedEnumMessage msg;

    msg.deserialize(m_serializer.get(), QByteArray());
    QVERIFY(msg.localEnumList().isEmpty());

    msg.deserialize(m_serializer.get(), QByteArray::fromHex("0a06000102010203"));
    QVERIFY((msg.localEnumList() == RepeatedEnumMessage::LocalEnumRepeated {RepeatedEnumMessage::LOCAL_ENUM_VALUE0,
                RepeatedEnumMessage::LOCAL_ENUM_VALUE1,
                RepeatedEnumMessage::LOCAL_ENUM_VALUE2,
                RepeatedEnumMessage::LOCAL_ENUM_VALUE1,
                RepeatedEnumMessage::LOCAL_ENUM_VALUE2,
                RepeatedEnumMessage::LOCAL_ENUM_VALUE3}));
}

QTEST_MAIN(QtProtobufEnumTypesDeserializationTest)
#include "tst_protobuf_deserialization_enumtypes.moc"
