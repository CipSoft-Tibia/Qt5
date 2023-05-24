// Copyright (C) 2020 Alexey Edelev <semlanik@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#define private public
#define protected public
#include "basicmessages.qpb.h"
#undef private
#undef protected

#include <QTest>

using namespace qtprotobufnamespace::tests;

class QtProtobufInternalsTest : public QObject
{
    Q_OBJECT
public:
    QtProtobufInternalsTest() = default;

private slots:
    void NullPointerMessageTest();
    void NullPointerGetterMessageTest();
};

void QtProtobufInternalsTest::NullPointerMessageTest()
{
    SimpleStringMessage stringMsg;
    stringMsg.setTestFieldString({ "not null" });
    ComplexMessage msg;
    msg.setTestFieldInt(0);
    msg.setTestComplexField(stringMsg);

    msg.setTestComplexField_p(nullptr);
    QVERIFY(msg.testComplexField().testFieldString().isEmpty());
    QVERIFY(msg.testComplexField_p() != nullptr);
}

void QtProtobufInternalsTest::NullPointerGetterMessageTest()
{
    ComplexMessage msg;
    QVERIFY(msg.testComplexField_p() == nullptr);
    msg.setTestComplexField_p(nullptr);
    QVERIFY(msg.testComplexField().testFieldString().isEmpty());
    QVERIFY(msg.testComplexField_p() != nullptr);
}

QTEST_MAIN(QtProtobufInternalsTest)
#include "tst_protobuf_internals.moc"
