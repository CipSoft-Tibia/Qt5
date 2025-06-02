// Copyright (C) 2020 Alexey Edelev <semlanik@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "basicmessages.qpb.h"

#include <QTest>

using namespace qtprotobufnamespace::tests;

class QtProtobufInternalsTest : public QObject
{
    Q_OBJECT
public:
    QtProtobufInternalsTest() = default;

private Q_SLOTS:
    void nullPointerMessageTest();
    void nullPointerGetterMessageTest();
};

void QtProtobufInternalsTest::nullPointerMessageTest()
{
    SimpleStringMessage stringMsg;
    stringMsg.setTestFieldString({ "not null" });
    ComplexMessage msg;
    msg.setTestFieldInt(0);
    msg.setTestComplexField(stringMsg);

    msg.setProperty("testComplexField_p",
                    QVariant::fromValue(static_cast<SimpleStringMessage *>(nullptr)));
    QVERIFY(msg.testComplexField().testFieldString().isEmpty());
    QVERIFY(msg.property("testComplexField_p").value<SimpleStringMessage *>() != nullptr);
}

void QtProtobufInternalsTest::nullPointerGetterMessageTest()
{
    ComplexMessage msg;
    QVERIFY(!msg.hasTestComplexField());
    QVERIFY(msg.property("testComplexField_p").value<SimpleStringMessage *>() != nullptr);
    msg.setProperty("testComplexField_p",
                    QVariant::fromValue(static_cast<SimpleStringMessage *>(nullptr)));
    QVERIFY(msg.testComplexField().testFieldString().isEmpty());
    QVERIFY(msg.property("testComplexField_p").value<SimpleStringMessage *>() != nullptr);
}

QTEST_MAIN(QtProtobufInternalsTest)
#include "tst_protobuf_internals.moc"
