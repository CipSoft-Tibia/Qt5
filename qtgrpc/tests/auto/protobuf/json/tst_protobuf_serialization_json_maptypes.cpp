// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "mapmessages.qpb.h"

#include <QObject>
#include <QProtobufJsonSerializer>
#include <QTest>

using namespace qtprotobufnamespace::tests;
using namespace Qt::Literals::StringLiterals;

class QtProtobufJsonMapTypesSerializationTest : public QObject
{
    Q_OBJECT
private slots:
    void init() { m_serializer.reset(new QProtobufJsonSerializer); }
    void SimpleFixed32StringMapSerializeTest();
    void SimpleSFixed32StringMapSerializeTest();
    void SimpleInt32StringMapSerializeTest();
    void SimpleSInt32StringMapSerializeTest();
    void SimpleUInt32StringMapSerializeTest();
    void SimpleFixed64StringMapSerializeTest();
    void SimpleSFixed64StringMapSerializeTest();
    void SimpleInt64StringMapSerializeTest();
    void SimpleSInt64StringMapSerializeTest();
    void SimpleUInt64StringMapSerializeTest();
    void SimpleStringStringMapSerializeTest();
    void SimpleSInt32ComplexMessageMapSerializeTest();
    void SimpleSFixed32ComplexMapSerializeTest();
    void SimpleInt32ComplexMapSerializeTest();
    void SimpleSInt32ComplexMapSerializeTest();
    void SimpleUInt32ComplexMapSerializeTest();
    void SimpleFixed64ComplexMapSerializeTest();
    void SimpleSFixed64ComplexMapSerializeTest();
    void SimpleInt64ComplexMapSerializeTest();
    void SimpleSInt64ComplexMapSerializeTest();
    void SimpleUInt64ComplexMapSerializeTest();
    void SimpleStringComplexMapSerializeTest();

private:
    std::unique_ptr<QProtobufJsonSerializer> m_serializer;
};

void QtProtobufJsonMapTypesSerializationTest::SimpleFixed32StringMapSerializeTest()
{
    SimpleFixed32StringMapMessage test;
    test.setMapField({ { 10, { "ten" } },
                       { 42, { "fourty two" } },
                       { 15, { "fifteen" } },
                       { 0, { "" } } });
    QByteArray result = test.serialize(m_serializer.get());
    QCOMPARE(
        result,
        "{\"mapField\":{\"0\":\"\",\"10\":\"ten\",\"15\":\"fifteen\",\"42\":\"fourty two\"}}"_ba);
}

void QtProtobufJsonMapTypesSerializationTest::SimpleSFixed32StringMapSerializeTest()
{
    SimpleSFixed32StringMapMessage test;
    test.setMapField({ { 10, { "ten" } }, { -42, { "minus fourty two" } }, { 15, { "fifteen" } } });
    QByteArray result = test.serialize(m_serializer.get());
    QCOMPARE(result,
             "{\"mapField\":{\"-42\":\"minus fourty two\",\"10\":\"ten\",\"15\":\"fifteen\"}}"_ba);
}

void QtProtobufJsonMapTypesSerializationTest::SimpleInt32StringMapSerializeTest()
{
    SimpleInt32StringMapMessage test;
    test.setMapField({ { -10, { "minus ten" } }, { 42, { "fourty two" } }, { 15, { "fifteen" } } });
    QByteArray result = test.serialize(m_serializer.get());
    QCOMPARE(result,
             "{\"mapField\":{\"-10\":\"minus ten\",\"15\":\"fifteen\",\"42\":\"fourty two\"}}"_ba);
}

void QtProtobufJsonMapTypesSerializationTest::SimpleSInt32StringMapSerializeTest()
{
    SimpleSInt32StringMapMessage test;
    test.setMapField({ { 10, { "ten" } }, { -42, { "minus fourty two" } }, { 15, { "fifteen" } } });
    QByteArray result = test.serialize(m_serializer.get());
    QCOMPARE(result,
             "{\"mapField\":{\"-42\":\"minus fourty two\",\"10\":\"ten\",\"15\":\"fifteen\"}}"_ba);
}

void QtProtobufJsonMapTypesSerializationTest::SimpleUInt32StringMapSerializeTest()
{
    SimpleUInt32StringMapMessage test;
    test.setMapField({ { 10, { "ten" } }, { 42, { "fourty two" } }, { 15, { "fifteen" } } });
    QByteArray result = test.serialize(m_serializer.get());
    QCOMPARE(result, "{\"mapField\":{\"10\":\"ten\",\"15\":\"fifteen\",\"42\":\"fourty two\"}}"_ba);
}

void QtProtobufJsonMapTypesSerializationTest::SimpleFixed64StringMapSerializeTest()
{
    SimpleFixed64StringMapMessage test;
    test.setMapField({ { 10, { "ten" } }, { 42, { "fourty two" } }, { 15, { "fifteen" } } });
    QByteArray result = test.serialize(m_serializer.get());
    QCOMPARE(result, "{\"mapField\":{\"10\":\"ten\",\"15\":\"fifteen\",\"42\":\"fourty two\"}}"_ba);
}

void QtProtobufJsonMapTypesSerializationTest::SimpleSFixed64StringMapSerializeTest()
{
    SimpleSFixed64StringMapMessage test;
    test.setMapField({ { 10, { "ten" } }, { -42, { "minus fourty two" } }, { 15, { "fifteen" } } });
    QByteArray result = test.serialize(m_serializer.get());
    QCOMPARE(result,
             "{\"mapField\":{\"-42\":\"minus fourty two\",\"10\":\"ten\",\"15\":\"fifteen\"}}"_ba);
}

void QtProtobufJsonMapTypesSerializationTest::SimpleInt64StringMapSerializeTest()
{
    SimpleInt64StringMapMessage test;
    test.setMapField({ { -10, { "minus ten" } }, { 42, { "fourty two" } }, { 15, { "fifteen" } } });
    QByteArray result = test.serialize(m_serializer.get());
    QCOMPARE(result,
             "{\"mapField\":{\"-10\":\"minus ten\",\"15\":\"fifteen\",\"42\":\"fourty two\"}}"_ba);
}

void QtProtobufJsonMapTypesSerializationTest::SimpleSInt64StringMapSerializeTest()
{
    SimpleSInt64StringMapMessage test;
    test.setMapField({ { 10, { "ten" } }, { -42, { "minus fourty two" } }, { 15, { "fifteen" } } });
    QByteArray result = test.serialize(m_serializer.get());
    QCOMPARE(result,
             "{\"mapField\":{\"-42\":\"minus fourty two\",\"10\":\"ten\",\"15\":\"fifteen\"}}"_ba);
}

void QtProtobufJsonMapTypesSerializationTest::SimpleUInt64StringMapSerializeTest()
{
    SimpleUInt64StringMapMessage test;
    test.setMapField({ { 10, { "ten" } }, { 42, { "fourty two" } }, { 15, { "fifteen" } } });
    QByteArray result = test.serialize(m_serializer.get());
    QCOMPARE(result, "{\"mapField\":{\"10\":\"ten\",\"15\":\"fifteen\",\"42\":\"fourty two\"}}"_ba);
}

void QtProtobufJsonMapTypesSerializationTest::SimpleStringStringMapSerializeTest()
{
    SimpleStringStringMapMessage test;
    test.setMapField({ { "ben", "ten" },
                       { "what is the answer?", "fourty two" },
                       { "sweet", "fifteen" } });
    QByteArray result = test.serialize(m_serializer.get());
    QCOMPARE(result,
             "{\"mapField\":{\"ben\":\"ten\",\"sweet\":\"fifteen\","
             "\"what is the answer?\":\"fourty two\"}}"_ba);
}

void QtProtobufJsonMapTypesSerializationTest::SimpleSInt32ComplexMessageMapSerializeTest()
{
    qtprotobufnamespace::tests::SimpleStringMessage stringMsg;

    stringMsg.setTestFieldString({ "ten sixteen" });
    ComplexMessage expected1;
    expected1.setTestFieldInt(16);
    expected1.setTestComplexField(stringMsg);

    stringMsg.setTestFieldString({ "fourty two ten sixteen" });
    ComplexMessage expected2;
    expected2.setTestFieldInt(10);
    expected2.setTestComplexField(stringMsg);

    stringMsg.setTestFieldString({ "WUT?" });
    ComplexMessage expected3;
    expected3.setTestFieldInt(10);
    expected3.setTestComplexField(stringMsg);

    SimpleFixed32ComplexMessageMapMessage test;
    test.setMapField({ { 10, ComplexMessage(expected1) },
                       { 42, ComplexMessage(expected2) },
                       { 65555, ComplexMessage(expected3) } });
    QByteArray result = test.serialize(m_serializer.get());
    QCOMPARE(result,
             "{\"mapField\":{\"10\":{\"testComplexField\":{\"testFieldString\":\"ten sixteen\"},"
             "\"testFieldInt\":16},\"42\":{\"testComplexField\":{\"testFieldString\":"
             "\"fourty two ten sixteen\"},\"testFieldInt\":10},\"65555\":{\"testComplexField\":"
             "{\"testFieldString\":\"WUT?\"},\"testFieldInt\":10}}}"_ba);
}

void QtProtobufJsonMapTypesSerializationTest::SimpleSFixed32ComplexMapSerializeTest()
{
    qtprotobufnamespace::tests::SimpleStringMessage stringMsg;

    stringMsg.setTestFieldString({ "ten sixteen" });
    ComplexMessage expected1;
    expected1.setTestFieldInt(16);
    expected1.setTestComplexField(stringMsg);

    stringMsg.setTestFieldString({ "minus fourty two ten sixteen" });
    ComplexMessage expected2;
    expected2.setTestFieldInt(10);
    expected2.setTestComplexField(stringMsg);

    stringMsg.setTestFieldString({ "WUT?" });
    ComplexMessage expected3;
    expected3.setTestFieldInt(10);
    expected3.setTestComplexField(stringMsg);

    SimpleSFixed32ComplexMessageMapMessage test;
    test.setMapField({ { 10, ComplexMessage(expected1) },
                       { -42, ComplexMessage(expected2) },
                       { 65555, ComplexMessage(expected3) } });
    QByteArray result = test.serialize(m_serializer.get());

    QCOMPARE(
        result,
        "{\"mapField\":{\"-42\":{\"testComplexField\":{\"testFieldString\":"
        "\"minus fourty two ten sixteen\"},\"testFieldInt\":10},\"10\":"
        "{\"testComplexField\":{\"testFieldString\":\"ten sixteen\"},\"testFieldInt\":16},"
        "\"65555\":{\"testComplexField\":{\"testFieldString\":\"WUT?\"},\"testFieldInt\":10}}}"_ba);
}

void QtProtobufJsonMapTypesSerializationTest::SimpleInt32ComplexMapSerializeTest()
{
    qtprotobufnamespace::tests::SimpleStringMessage stringMsg;

    stringMsg.setTestFieldString({ "ten sixteen" });
    ComplexMessage expected1;
    expected1.setTestFieldInt(16);
    expected1.setTestComplexField(stringMsg);

    stringMsg.setTestFieldString({ "minus fourty two ten sixteen" });
    ComplexMessage expected2;
    expected2.setTestFieldInt(10);
    expected2.setTestComplexField(stringMsg);

    stringMsg.setTestFieldString({ "WUT?" });
    ComplexMessage expected3;
    expected3.setTestFieldInt(10);
    expected3.setTestComplexField(stringMsg);

    SimpleInt32ComplexMessageMapMessage test;
    test.setMapField({ { 10, ComplexMessage(expected1) },
                       { -42, ComplexMessage(expected2) },
                       { 65555, ComplexMessage(expected3) } });
    QByteArray result = test.serialize(m_serializer.get());

    QCOMPARE(result,
             "{\"mapField\":{\"-42\":{\"testComplexField\":{\"testFieldString\":"
             "\"minus fourty two ten sixteen\"},\"testFieldInt\":10},\"10\":{\"testComplexField\":"
             "{\"testFieldString\":\"ten sixteen\"},\"testFieldInt\":16},\"65555\":"
             "{\"testComplexField\":{\"testFieldString\":\"WUT?\"},\"testFieldInt\":10}}}"_ba);
}

void QtProtobufJsonMapTypesSerializationTest::SimpleSInt32ComplexMapSerializeTest()
{
    qtprotobufnamespace::tests::SimpleStringMessage stringMsg;

    stringMsg.setTestFieldString({ "ten sixteen" });
    ComplexMessage expected1;
    expected1.setTestFieldInt(16);
    expected1.setTestComplexField(stringMsg);

    stringMsg.setTestFieldString({ "fourty two ten sixteen" });
    ComplexMessage expected2;
    expected2.setTestFieldInt(10);
    expected2.setTestComplexField(stringMsg);

    stringMsg.setTestFieldString({ "minus WUT?" });
    ComplexMessage expected3;
    expected3.setTestFieldInt(10);
    expected3.setTestComplexField(stringMsg);

    SimpleSInt32ComplexMessageMapMessage test;
    test.setMapField({ { 10, ComplexMessage(expected1) },
                       { 42, ComplexMessage(expected2) },
                       { -65555, ComplexMessage(expected3) } });
    QByteArray result = test.serialize(m_serializer.get());

    QCOMPARE(result,
             "{\"mapField\":{\"-65555\":{\"testComplexField\":{\"testFieldString\":\"minus WUT?\"},"
             "\"testFieldInt\":10},\"10\":{\"testComplexField\":{\"testFieldString\":\"ten "
             "sixteen\"},"
             "\"testFieldInt\":16},\"42\":{\"testComplexField\":{\"testFieldString\":"
             "\"fourty two ten sixteen\"},\"testFieldInt\":10}}}"_ba);
}

void QtProtobufJsonMapTypesSerializationTest::SimpleUInt32ComplexMapSerializeTest()
{
    qtprotobufnamespace::tests::SimpleStringMessage stringMsg;

    stringMsg.setTestFieldString({ "ten sixteen" });
    ComplexMessage expected1;
    expected1.setTestFieldInt(16);
    expected1.setTestComplexField(stringMsg);

    stringMsg.setTestFieldString({ "fourty two ten sixteen" });
    ComplexMessage expected2;
    expected2.setTestFieldInt(10);
    expected2.setTestComplexField(stringMsg);

    stringMsg.setTestFieldString({ "WUT?" });
    ComplexMessage expected3;
    expected3.setTestFieldInt(10);
    expected3.setTestComplexField(stringMsg);

    SimpleUInt32ComplexMessageMapMessage test;
    test.setMapField({ { 10, ComplexMessage(expected1) },
                       { 42, ComplexMessage(expected2) },
                       { 65555, ComplexMessage(expected3) } });
    QByteArray result = test.serialize(m_serializer.get());

    QCOMPARE(result,
             "{\"mapField\":{\"10\":{\"testComplexField\":{\"testFieldString\":\"ten sixteen\"},"
             "\"testFieldInt\":16},\"42\":{\"testComplexField\":{\"testFieldString\":"
             "\"fourty two ten sixteen\"},\"testFieldInt\":10},\"65555\":{\"testComplexField\":"
             "{\"testFieldString\":\"WUT?\"},\"testFieldInt\":10}}}"_ba);
}

void QtProtobufJsonMapTypesSerializationTest::SimpleFixed64ComplexMapSerializeTest()
{
    qtprotobufnamespace::tests::SimpleStringMessage stringMsg;

    stringMsg.setTestFieldString({ "ten sixteen" });
    ComplexMessage expected1;
    expected1.setTestFieldInt(16);
    expected1.setTestComplexField(stringMsg);

    stringMsg.setTestFieldString({ "minus fourty two ten MAAAX" });
    ComplexMessage expected2;
    expected2.setTestFieldInt(42);
    expected2.setTestComplexField(stringMsg);

    stringMsg.setTestFieldString({ "WUT?" });
    ComplexMessage expected3;
    expected3.setTestFieldInt(10);
    expected3.setTestComplexField(stringMsg);

    SimpleFixed64ComplexMessageMapMessage test;
    test.setMapField({ { 10, ComplexMessage(expected1) },
                       { UINT64_MAX, ComplexMessage(expected2) },
                       { 65555, ComplexMessage(expected3) } });
    QByteArray result = test.serialize(m_serializer.get());

    QCOMPARE(result,
             "{\"mapField\":{\"10\":{\"testComplexField\":{\"testFieldString\":\"ten sixteen\"},"
             "\"testFieldInt\":16},\"18446744073709551615\":{\"testComplexField\":{"
             "\"testFieldString\":"
             "\"minus fourty two ten MAAAX\"},\"testFieldInt\":42},\"65555\":{\"testComplexField\":"
             "{\"testFieldString\":\"WUT?\"},\"testFieldInt\":10}}}"_ba);
}

void QtProtobufJsonMapTypesSerializationTest::SimpleSFixed64ComplexMapSerializeTest()
{
    qtprotobufnamespace::tests::SimpleStringMessage stringMsg;

    stringMsg.setTestFieldString({ "ten sixteen" });
    ComplexMessage expected1;
    expected1.setTestFieldInt(16);
    expected1.setTestComplexField(stringMsg);

    stringMsg.setTestFieldString({ "minus fourty two ten sixteen" });
    ComplexMessage expected2;
    expected2.setTestFieldInt(10);
    expected2.setTestComplexField(stringMsg);

    stringMsg.setTestFieldString({ "WUT?" });
    ComplexMessage expected3;
    expected3.setTestFieldInt(10);
    expected3.setTestComplexField(stringMsg);

    SimpleSFixed64ComplexMessageMapMessage test;
    test.setMapField({ { 10, ComplexMessage(expected1) },
                       { -42, ComplexMessage(expected2) },
                       { 65555, ComplexMessage(expected3) } });
    QByteArray result = test.serialize(m_serializer.get());

    QCOMPARE(
        result,
        "{\"mapField\":{\"-42\":{\"testComplexField\":{\"testFieldString\":"
        "\"minus fourty two ten sixteen\"},\"testFieldInt\":10},\"10\":"
        "{\"testComplexField\":{\"testFieldString\":\"ten sixteen\"},\"testFieldInt\":16},"
        "\"65555\":{\"testComplexField\":{\"testFieldString\":\"WUT?\"},\"testFieldInt\":10}}}"_ba);
}

void QtProtobufJsonMapTypesSerializationTest::SimpleInt64ComplexMapSerializeTest()
{
    qtprotobufnamespace::tests::SimpleStringMessage stringMsg;

    stringMsg.setTestFieldString({ "ten sixteen" });
    ComplexMessage expected1;
    expected1.setTestFieldInt(16);
    expected1.setTestComplexField(stringMsg);

    stringMsg.setTestFieldString({ "minus fourty two ten sixteen" });
    ComplexMessage expected2;
    expected2.setTestFieldInt(10);
    expected2.setTestComplexField(stringMsg);

    stringMsg.setTestFieldString({ "WUT?" });
    ComplexMessage expected3;
    expected3.setTestFieldInt(10);
    expected3.setTestComplexField(stringMsg);

    SimpleInt64ComplexMessageMapMessage test;
    test.setMapField({ { 10, ComplexMessage(expected1) },
                       { -42, ComplexMessage(expected2) },
                       { 65555, ComplexMessage(expected3) } });
    QByteArray result = test.serialize(m_serializer.get());

    QCOMPARE(result,
             "{\"mapField\":{\"-42\":{\"testComplexField\":{\"testFieldString\":"
             "\"minus fourty two ten sixteen\"},\"testFieldInt\":10},\"10\":{\"testComplexField\":"
             "{\"testFieldString\":\"ten sixteen\"},\"testFieldInt\":16},\"65555\":"
             "{\"testComplexField\":{\"testFieldString\":\"WUT?\"},\"testFieldInt\":10}}}"_ba);
}

void QtProtobufJsonMapTypesSerializationTest::SimpleSInt64ComplexMapSerializeTest()
{
    qtprotobufnamespace::tests::SimpleStringMessage stringMsg;

    stringMsg.setTestFieldString({ "ten sixteen" });
    ComplexMessage expected1;
    expected1.setTestFieldInt(16);
    expected1.setTestComplexField(stringMsg);

    stringMsg.setTestFieldString({ "minus fourty two ten sixteen" });
    ComplexMessage expected2;
    expected2.setTestFieldInt(10);
    expected2.setTestComplexField(stringMsg);

    stringMsg.setTestFieldString({ "WUT?" });
    ComplexMessage expected3;
    expected3.setTestFieldInt(10);
    expected3.setTestComplexField(stringMsg);

    SimpleSInt64ComplexMessageMapMessage test;
    test.setMapField({ { 10, ComplexMessage(expected1) },
                       { -42, ComplexMessage(expected2) },
                       { 65555, ComplexMessage(expected3) } });
    QByteArray result = test.serialize(m_serializer.get());

    QCOMPARE(result,
             "{\"mapField\":{\"-42\":{\"testComplexField\":{\"testFieldString\":"
             "\"minus fourty two ten sixteen\"},\"testFieldInt\":10},\"10\":{\"testComplexField\":"
             "{\"testFieldString\":\"ten sixteen\"},\"testFieldInt\":16},\"65555\":"
             "{\"testComplexField\":{\"testFieldString\":\"WUT?\"},\"testFieldInt\":10}}}"_ba);
}

void QtProtobufJsonMapTypesSerializationTest::SimpleUInt64ComplexMapSerializeTest()
{
    qtprotobufnamespace::tests::SimpleStringMessage stringMsg;

    stringMsg.setTestFieldString({ "ten eleven" });
    ComplexMessage expected1;
    expected1.setTestFieldInt(11);
    expected1.setTestComplexField(stringMsg);

    stringMsg.setTestFieldString({ "fourty two ten sixteen" });
    ComplexMessage expected2;
    expected2.setTestFieldInt(10);
    expected2.setTestComplexField(stringMsg);

    stringMsg.setTestFieldString({ "WUT?" });
    ComplexMessage expected3;
    expected3.setTestFieldInt(10);
    expected3.setTestComplexField(stringMsg);

    SimpleUInt64ComplexMessageMapMessage test;
    test.setMapField({ { 10, ComplexMessage(expected1) },
                       { 42, ComplexMessage(expected2) },
                       { 65555, ComplexMessage(expected3) } });
    QByteArray result = test.serialize(m_serializer.get());

    QCOMPARE(result,
             "{\"mapField\":{\"10\":{\"testComplexField\":{\"testFieldString\":\"ten eleven\"},"
             "\"testFieldInt\":11},\"42\":{\"testComplexField\":{\"testFieldString\":"
             "\"fourty two ten sixteen\"},\"testFieldInt\":10},\"65555\":{\"testComplexField\""
             ":{\"testFieldString\":\"WUT?\"},\"testFieldInt\":10}}}"_ba);
}

void QtProtobufJsonMapTypesSerializationTest::SimpleStringComplexMapSerializeTest()
{
    qtprotobufnamespace::tests::SimpleStringMessage stringMsg;

    stringMsg.setTestFieldString({ "ten eleven" });
    ComplexMessage expected1;
    expected1.setTestFieldInt(11);
    expected1.setTestComplexField(stringMsg);

    stringMsg.setTestFieldString({ "fourty two ten sixteen" });
    ComplexMessage expected2;
    expected2.setTestFieldInt(10);
    expected2.setTestComplexField(stringMsg);

    stringMsg.setTestFieldString({ "?WUT?" });
    ComplexMessage expected3;
    expected3.setTestFieldInt(10);
    expected3.setTestComplexField(stringMsg);

    SimpleStringComplexMessageMapMessage test;
    test.setMapField({ { "ben", ComplexMessage(expected1) },
                       { "where is my car dude?", ComplexMessage(expected2) },
                       { "WUT??", ComplexMessage(expected3) } });
    QByteArray result = test.serialize(m_serializer.get());

    QCOMPARE(result,
             "{\"mapField\":{\"WUT??\":{\"testComplexField\":{\"testFieldString\":\"?WUT?\"},"
             "\"testFieldInt\":10},\"ben\":{\"testComplexField\":{\"testFieldString\":\"ten "
             "eleven\"},\"testFieldInt\":11},\"where is my car "
             "dude?\":{\"testComplexField\":{\"testFieldString\":\"fourty two ten "
             "sixteen\"},\"testFieldInt\":10}}}");
}

QTEST_MAIN(QtProtobufJsonMapTypesSerializationTest)
#include "tst_protobuf_serialization_json_maptypes.moc"
