// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "basicmessages.qpb.h"

#include <QProtobufJsonSerializer>
#include <QTest>
#include <QJsonDocument>

using namespace Qt::Literals::StringLiterals;

class QtProtobufTypesJsonSerializationTest : public QObject
{
    Q_OBJECT
private slots:
    void init() { m_serializer.reset(new QProtobufJsonSerializer); }

    void BoolMessageSerializeTest_data();
    void BoolMessageSerializeTest();
    void IntMessageSerializeTest_data();
    void IntMessageSerializeTest();
    void UIntMessageSerializeTest_data();
    void UIntMessageSerializeTest();
    void SIntMessageSerializeTest_data();
    void SIntMessageSerializeTest();
    void Int64MessageSerializeTest_data();
    void Int64MessageSerializeTest();
    void UInt64MessageSerializeTest_data();
    void UInt64MessageSerializeTest();
    void SInt64MessageSerializeTest_data();
    void SInt64MessageSerializeTest();
    void FixedInt32MessageSerializeTest_data();
    void FixedInt32MessageSerializeTest();
    void FixedInt64MessageSerializeTest_data();
    void FixedInt64MessageSerializeTest();
    void SFixedInt32MessageSerializeTest_data();
    void SFixedInt32MessageSerializeTest();
    void SFixedInt64MessageSerializeTest_data();
    void SFixedInt64MessageSerializeTest();
    void FloatMessageSerializeTest_data();
    void FloatMessageSerializeTest();
    void DoubleMessageSerializeTest_data();
    void DoubleMessageSerializeTest();
    void StringMessageSerializeTest();
    void ComplexTypeSerializeTest_data();
    void ComplexTypeSerializeTest();
    void ResetComplexTypeSerializeTest_data();
    void ResetComplexTypeSerializeTest();
    void DefaultConstructedComplexTypeSerializeTest();
    void EmptyBytesMessageTest();
    void EmptyStringMessageTest();
    void OneofMessageEmptyTest();
    void OneofMessageClearTest();
    void OneofMessageIntTest();
    void OneofMessageComplexTest();

private:
    std::unique_ptr<QProtobufJsonSerializer> m_serializer;
};

using namespace qtprotobufnamespace::tests;

void QtProtobufTypesJsonSerializationTest::BoolMessageSerializeTest_data()
{
    QTest::addColumn<bool>("value");
    QTest::addColumn<QByteArray>("expectedData");

    QTest::newRow("True") << true << "{\"testFieldBool\":true}"_ba;
    QTest::newRow("False") << false << "{}"_ba;
}

void QtProtobufTypesJsonSerializationTest::BoolMessageSerializeTest()
{
    QFETCH(const bool, value);
    QFETCH(const QByteArray, expectedData);

    SimpleBoolMessage test;
    test.setTestFieldBool(value);
    QByteArray result = test.serialize(m_serializer.get());
    QCOMPARE(result, expectedData);
    QVERIFY(!QJsonDocument::fromJson(result).isNull());
}

void QtProtobufTypesJsonSerializationTest::IntMessageSerializeTest_data()
{
    QTest::addColumn<int32_t>("value");
    QTest::addColumn<QByteArray>("expectedData");

    QTest::newRow("555") << 555 << "{\"testFieldInt\":555}"_ba;
    QTest::newRow("0") << 0 << "{}"_ba;
}

void QtProtobufTypesJsonSerializationTest::IntMessageSerializeTest()
{
    QFETCH(const int32_t, value);
    QFETCH(const QByteArray, expectedData);

    SimpleIntMessage test;
    test.setTestFieldInt(value);
    QByteArray result = test.serialize(m_serializer.get());
    QCOMPARE(result, expectedData);
    QVERIFY(!QJsonDocument::fromJson(result).isNull());
}

void QtProtobufTypesJsonSerializationTest::UIntMessageSerializeTest_data()
{
    QTest::addColumn<uint32_t>("value");
    QTest::addColumn<QByteArray>("expectedData");

    QTest::newRow("15") << (uint32_t)15 << "{\"testFieldInt\":15}"_ba;
    QTest::newRow("300") << (uint32_t)300 << "{\"testFieldInt\":300}"_ba;
    QTest::newRow("65545") << (uint32_t)65545 << "{\"testFieldInt\":65545}"_ba;
}

void QtProtobufTypesJsonSerializationTest::UIntMessageSerializeTest()
{
    QFETCH(const uint32_t, value);
    QFETCH(const QByteArray, expectedData);

    SimpleUIntMessage test;
    test.setTestFieldInt(value);
    QByteArray result = test.serialize(m_serializer.get());
    QCOMPARE(result, expectedData);
    QVERIFY(!QJsonDocument::fromJson(result).isNull());
}

void QtProtobufTypesJsonSerializationTest::SIntMessageSerializeTest_data()
{
    QTest::addColumn<int32_t>("value");
    QTest::addColumn<QByteArray>("expectedData");

    QTest::newRow("15") << (int32_t)15 << "{\"testFieldInt\":15}"_ba;
    QTest::newRow("300") << (int32_t)300 << "{\"testFieldInt\":300}"_ba;
    QTest::newRow("65545") << (int32_t)65545 << "{\"testFieldInt\":65545}"_ba;

    QTest::newRow("-1") << (int32_t)-1 << "{\"testFieldInt\":-1}"_ba;
    QTest::newRow("-462") << (int32_t)-462 << "{\"testFieldInt\":-462}"_ba;
    QTest::newRow("-63585") << (int32_t)-63585 << "{\"testFieldInt\":-63585}"_ba;
}

void QtProtobufTypesJsonSerializationTest::SIntMessageSerializeTest()
{
    QFETCH(const int32_t, value);
    QFETCH(const QByteArray, expectedData);

    SimpleSIntMessage test;
    test.setTestFieldInt(value);
    QByteArray result = test.serialize(m_serializer.get());
    QCOMPARE(result, expectedData);
    QVERIFY(!QJsonDocument::fromJson(result).isNull());
}

void QtProtobufTypesJsonSerializationTest::Int64MessageSerializeTest_data()
{
    QTest::addColumn<int64_t>("value");
    QTest::addColumn<QByteArray>("expectedData");

    QTest::newRow("15") << (int64_t)15 << "{\"testFieldInt\":15}"_ba;
    QTest::newRow("300") << (int64_t)300 << "{\"testFieldInt\":300}"_ba;
    QTest::newRow("65545") << (int64_t)65545 << "{\"testFieldInt\":65545}"_ba;

    QTest::newRow("-1") << (int64_t)-1 << "{\"testFieldInt\":-1}"_ba;
    QTest::newRow("-462") << (int64_t)-462 << "{\"testFieldInt\":-462}"_ba;
    QTest::newRow("-63585") << (int64_t)-63585 << "{\"testFieldInt\":-63585}"_ba;
}

void QtProtobufTypesJsonSerializationTest::Int64MessageSerializeTest()
{
    QFETCH(const int64_t, value);
    QFETCH(const QByteArray, expectedData);

    SimpleInt64Message test;
    test.setTestFieldInt(value);
    QByteArray result = test.serialize(m_serializer.get());
    QCOMPARE(result, expectedData);
    QVERIFY(!QJsonDocument::fromJson(result).isNull());
}

void QtProtobufTypesJsonSerializationTest::UInt64MessageSerializeTest_data()
{
    QTest::addColumn<uint64_t>("value");
    QTest::addColumn<QByteArray>("expectedData");

    QTest::newRow("15") << (uint64_t)15 << "{\"testFieldInt\":\"15\"}"_ba;
    QTest::newRow("300") << (uint64_t)300 << "{\"testFieldInt\":\"300\"}"_ba;
    QTest::newRow("65545") << (uint64_t)65545 << "{\"testFieldInt\":\"65545\"}"_ba;
    QTest::newRow("18446744073709549568")
        << uint64_t(18446744073709549568u) << "{\"testFieldInt\":\"18446744073709549568\"}"_ba;
    QTest::newRow("123245324235425234")
        << uint64_t(123245324235425234u) << "{\"testFieldInt\":\"123245324235425234\"}"_ba;
}

void QtProtobufTypesJsonSerializationTest::UInt64MessageSerializeTest()
{
    QFETCH(const QtProtobuf::uint64, value);
    QFETCH(const QByteArray, expectedData);

    SimpleUInt64Message test;
    test.setTestFieldInt(value);
    QByteArray result = test.serialize(m_serializer.get());
    QCOMPARE(result, expectedData);
    QVERIFY(!QJsonDocument::fromJson(result).isNull());
}

void QtProtobufTypesJsonSerializationTest::SInt64MessageSerializeTest_data()
{
    QTest::addColumn<int64_t>("value");
    QTest::addColumn<QByteArray>("expectedData");

    QTest::newRow("15") << (int64_t)15 << "{\"testFieldInt\":15}"_ba;
    QTest::newRow("300") << (int64_t)300 << "{\"testFieldInt\":300}"_ba;
    QTest::newRow("65545") << (int64_t)65545 << "{\"testFieldInt\":65545}"_ba;

    QTest::newRow("-1") << (int64_t)-1 << "{\"testFieldInt\":-1}"_ba;
    QTest::newRow("-462") << (int64_t)-462 << "{\"testFieldInt\":-462}"_ba;
    QTest::newRow("-63585") << (int64_t)-63585 << "{\"testFieldInt\":-63585}"_ba;
}

void QtProtobufTypesJsonSerializationTest::SInt64MessageSerializeTest()
{
    QFETCH(const int64_t, value);
    QFETCH(const QByteArray, expectedData);

    SimpleSInt64Message test;
    test.setTestFieldInt(value);
    QByteArray result = test.serialize(m_serializer.get());
    QCOMPARE(result, expectedData);
    QVERIFY(!QJsonDocument::fromJson(result).isNull());
}

void QtProtobufTypesJsonSerializationTest::FixedInt32MessageSerializeTest_data()
{
    QTest::addColumn<uint32_t>("value");
    QTest::addColumn<QByteArray>("expectedData");

    QTest::newRow("15") << (uint32_t)15 << "{\"testFieldFixedInt32\":15}"_ba;
    QTest::newRow("300") << (uint32_t)300 << "{\"testFieldFixedInt32\":300}"_ba;
    QTest::newRow("65545") << (uint32_t)65545 << "{\"testFieldFixedInt32\":65545}"_ba;
}

void QtProtobufTypesJsonSerializationTest::FixedInt32MessageSerializeTest()
{
    QFETCH(const uint32_t, value);
    QFETCH(const QByteArray, expectedData);

    SimpleFixedInt32Message test;
    test.setTestFieldFixedInt32(value);
    QByteArray result = test.serialize(m_serializer.get());
    QCOMPARE(result, expectedData);
    QVERIFY(!QJsonDocument::fromJson(result).isNull());
}

void QtProtobufTypesJsonSerializationTest::FixedInt64MessageSerializeTest_data()
{
    QTest::addColumn<uint64_t>("value");
    QTest::addColumn<QByteArray>("expectedData");

    QTest::newRow("15") << (uint64_t)15 << "{\"testFieldFixedInt64\":\"15\"}"_ba;
    QTest::newRow("300") << (uint64_t)300 << "{\"testFieldFixedInt64\":\"300\"}"_ba;
    QTest::newRow("65545") << (uint64_t)65545 << "{\"testFieldFixedInt64\":\"65545\"}"_ba;
}

void QtProtobufTypesJsonSerializationTest::FixedInt64MessageSerializeTest()
{
    QFETCH(const uint64_t, value);
    QFETCH(const QByteArray, expectedData);

    SimpleFixedInt64Message test;
    test.setTestFieldFixedInt64(value);
    QByteArray result = test.serialize(m_serializer.get());
    QCOMPARE(result, expectedData);
    QVERIFY(!QJsonDocument::fromJson(result).isNull());
}

void QtProtobufTypesJsonSerializationTest::SFixedInt32MessageSerializeTest_data()
{
    QTest::addColumn<int32_t>("value");
    QTest::addColumn<QByteArray>("expectedData");

    QTest::newRow("15") << (int32_t)15 << "{\"testFieldFixedInt32\":15}"_ba;
    QTest::newRow("300") << (int32_t)300 << "{\"testFieldFixedInt32\":300}"_ba;
    QTest::newRow("65545") << (int32_t)65545 << "{\"testFieldFixedInt32\":65545}"_ba;

    QTest::newRow("-1") << (int32_t)-1 << "{\"testFieldFixedInt32\":-1}"_ba;
    QTest::newRow("-462") << (int32_t)-462 << "{\"testFieldFixedInt32\":-462}"_ba;
    QTest::newRow("-63585") << (int32_t)-63585 << "{\"testFieldFixedInt32\":-63585}"_ba;
}
void QtProtobufTypesJsonSerializationTest::SFixedInt32MessageSerializeTest()
{
    QFETCH(const int32_t, value);
    QFETCH(const QByteArray, expectedData);

    SimpleSFixedInt32Message test;
    test.setTestFieldFixedInt32(value);
    QByteArray result = test.serialize(m_serializer.get());
    QCOMPARE(result, expectedData);
    QVERIFY(!QJsonDocument::fromJson(result).isNull());
}

void QtProtobufTypesJsonSerializationTest::SFixedInt64MessageSerializeTest_data()
{
    QTest::addColumn<int64_t>("value");
    QTest::addColumn<QByteArray>("expectedData");

    QTest::newRow("15") << (int64_t)15 << "{\"testFieldFixedInt64\":15}"_ba;
    QTest::newRow("300") << (int64_t)300 << "{\"testFieldFixedInt64\":300}"_ba;
    QTest::newRow("65545") << (int64_t)65545 << "{\"testFieldFixedInt64\":65545}"_ba;

    QTest::newRow("-1") << (int64_t)-1 << "{\"testFieldFixedInt64\":-1}"_ba;
    QTest::newRow("-462") << (int64_t)-462 << "{\"testFieldFixedInt64\":-462}"_ba;
    QTest::newRow("-63585") << (int64_t)-63585 << "{\"testFieldFixedInt64\":-63585}"_ba;
}

void QtProtobufTypesJsonSerializationTest::SFixedInt64MessageSerializeTest()
{
    QFETCH(const int64_t, value);
    QFETCH(const QByteArray, expectedData);

    SimpleSFixedInt64Message test;
    test.setTestFieldFixedInt64(value);
    QByteArray result = test.serialize(m_serializer.get());
    QCOMPARE(result, expectedData);
    QVERIFY(!QJsonDocument::fromJson(result).isNull());
}

void QtProtobufTypesJsonSerializationTest::FloatMessageSerializeTest_data()
{
    QTest::addColumn<float>("value");
    QTest::addColumn<QByteArray>("expectedData");

    QTest::newRow("float_value_0_1") << 0.1f << "{\"testFieldFloat\":0.10000000149011612}"_ba;
    QTest::newRow("float_value_min")
        << std::numeric_limits<float>::min() << "{\"testFieldFloat\":1.1754943508222875e-38}"_ba;
    QTest::newRow("float_value_max")
        << std::numeric_limits<float>::max() << "{\"testFieldFloat\":3.4028234663852886e+38}"_ba;
    QTest::newRow("float_neg_value_4_2") << -4.2f << "{\"testFieldFloat\":-4.199999809265137}"_ba;
    QTest::newRow("float_neg_value_0_0") << (float)-0.0f << "{\"testFieldFloat\":-0}"_ba;
    QTest::newRow("float_value_0_0") << (float)0.0f << "{}"_ba;
    QTest::newRow("float_nan") << NAN << "{\"testFieldFloat\":\"nan\"}"_ba;
    QTest::newRow("float_infinity") << INFINITY << "{\"testFieldFloat\":\"infinity\"}"_ba;
    QTest::newRow("float_neg_infinity") << -INFINITY << "{\"testFieldFloat\":\"-infinity\"}"_ba;
}

void QtProtobufTypesJsonSerializationTest::FloatMessageSerializeTest()
{
    QFETCH(const float, value);
    QFETCH(const QByteArray, expectedData);

    SimpleFloatMessage test;
    test.setTestFieldFloat(value);
    QByteArray result = test.serialize(m_serializer.get());
    QEXPECT_FAIL("float_neg_value_0_0", "QJsonValue ignores sign bit when serializing floating"
                                        " point values", Continue);
    QCOMPARE(result, expectedData);
    QVERIFY(!QJsonDocument::fromJson(result).isNull());
}

void QtProtobufTypesJsonSerializationTest::DoubleMessageSerializeTest_data()
{
    QTest::addColumn<double>("value");
    QTest::addColumn<QByteArray>("expectedData");

    QTest::newRow("double_value_0_1") << 0.1
                                      << "{\"testFieldDouble\":0.1}"_ba;
    QTest::newRow("double_value_min")
        << std::numeric_limits<double>::min()
        << "{\"testFieldDouble\":2.2250738585072014e-308}"_ba;
    QTest::newRow("double_value_max")
        << std::numeric_limits<double>::max()
        << "{\"testFieldDouble\":1.7976931348623157e+308}"_ba;
    QTest::newRow("double_neg_value_4_2") << -4.2
                                          << "{\"testFieldDouble\":-4.2}"_ba;
    QTest::newRow("double_neg_value_0_0") << -0.0 << "{\"testFieldDouble\":-0}"_ba;
    QTest::newRow("double_value_0_0") << 0.0 << "{}"_ba;
    QTest::newRow("double_nan") << double(NAN) << "{\"testFieldDouble\":\"nan\"}"_ba;
    QTest::newRow("double_infinity") << double(INFINITY) << "{\"testFieldDouble\":\"infinity\"}"_ba;
    QTest::newRow("double_neg_infinity")
        << double(-INFINITY) << "{\"testFieldDouble\":\"-infinity\"}"_ba;
}

void QtProtobufTypesJsonSerializationTest::DoubleMessageSerializeTest()
{
    QFETCH(const double, value);
    QFETCH(const QByteArray, expectedData);

    SimpleDoubleMessage test;
    test.setTestFieldDouble(value);
    QByteArray result = test.serialize(m_serializer.get());
    QEXPECT_FAIL("double_neg_value_0_0", "QJsonValue ignores sign bit when serializing floating"
                                        " point values", Continue);
    QCOMPARE(result, expectedData);
    QVERIFY(!QJsonDocument::fromJson(result).isNull());
}

void QtProtobufTypesJsonSerializationTest::StringMessageSerializeTest()
{
    SimpleStringMessage test;
    test.setTestFieldString("qwerty");
    QByteArray result = test.serialize(m_serializer.get());

    QCOMPARE(result, "{\"testFieldString\":\"qwerty\"}"_ba);
    QVERIFY(!QJsonDocument::fromJson(result).isNull());

    test.setTestFieldString("");
    result = test.serialize(m_serializer.get());
    QCOMPARE(result, "{}"_ba);
    QVERIFY(!QJsonDocument::fromJson(result).isNull());

    test.setTestFieldString("null");
    result = test.serialize(m_serializer.get());
    QCOMPARE(result, "{\"testFieldString\":\"null\"}"_ba);
    QVERIFY(!QJsonDocument::fromJson(result).isNull());

    test.setTestFieldString("NaN");
    result = test.serialize(m_serializer.get());
    QCOMPARE(result, "{\"testFieldString\":\"NaN\"}"_ba);
    QVERIFY(!QJsonDocument::fromJson(result).isNull());

    test.setTestFieldString("Infinity");
    result = test.serialize(m_serializer.get());
    QCOMPARE(result, "{\"testFieldString\":\"Infinity\"}"_ba);
    QVERIFY(!QJsonDocument::fromJson(result).isNull());

    test.setTestFieldString("-Infinity");
    result = test.serialize(m_serializer.get());
    QCOMPARE(result, "{\"testFieldString\":\"-Infinity\"}"_ba);
    QVERIFY(!QJsonDocument::fromJson(result).isNull());
}

void QtProtobufTypesJsonSerializationTest::ComplexTypeSerializeTest_data()
{
    QTest::addColumn<int>("intField");
    QTest::addColumn<QString>("stringField");
    QTest::addColumn<QByteArray>("expectedData");

    QTest::newRow("empty_value") << 0 << ""
                                 << "{\"testComplexField\":{}}"_ba;
    QTest::newRow("value_only_int") << 42 << ""
                                    << "{\"testComplexField\":{},\"testFieldInt\":42}"_ba;
    QTest::newRow("value_only_string")
        << 0 << "qwerty"
        << "{\"testComplexField\":{\"testFieldString\":\"qwerty\"}}"_ba;
    QTest::newRow("int_and_string")
        << 42 << "qwerty"
        << "{\"testComplexField\":{\"testFieldString\":\"qwerty\"},\"testFieldInt\":42}"_ba;
    QTest::newRow("int_and_big_string")
        << 42
        << "YVRfJvjxqbgvFwS1YvOZXgtj5ffGLS7AiNHz9oZIoKbm7z8H79xBuyPkpQXvGoO09OY9xRawx3eOAs9xjo"
           "TA1xJhrw28TAcq1CebYlC9WUfQC6hIantaNdyHiKToffi0Zt7la42SRxXZSP4GuxbcZIp53pJnyCwfCy1q"
           "dFczT0dmn7h8fpyAdemEavwFeda4d0PApGfSU2jLt39X8kYUBxNM2WgALRBgHdVde87q6Pi5U69TjhMd28"
           "W1SFD1DxyogCCrqOct2ZPICoLnrqdF3OdNzjRVLfeyvQ8LgLvRNFR9WfWAyAz79nKgBamd8Ntlvt4Mg35E"
           "5gVS2g7AQ7rkm72cBdnW9sCEyGabeXAuH5j4GRbuLT7qBZWDcFLF4SsCdS3WfFGdNHfwaijzykByo71PvF"
           "VlTXH2WJWoFvR5FALjBTn7bCdP0pAiSbLCY8Xz2Msc3dBb5Ff9GISPbUpNmUvBdMZMHQvqOmTNXEPpN0b7"
           "4MDOMQfWJShOo3NkAvMjs"
        << "{\"testComplexField\":{\"testFieldString\":\"YVRfJvjxqbgvFwS1Y"
           "vOZXgtj5ffGLS7AiNHz9oZIoKbm7z8H79xBuyPkpQXvGoO09OY9xRawx3eOAs9xjoTA1xJhrw28TAcq1Ce"
           "bYlC9WUfQC6hIantaNdyHiKToffi0Zt7la42SRxXZSP4GuxbcZIp53pJnyCwfCy1qdFczT0dmn7h8fpyAd"
           "emEavwFeda4d0PApGfSU2jLt39X8kYUBxNM2WgALRBgHdVde87q6Pi5U69TjhMd28W1SFD1DxyogCCrqOc"
           "t2ZPICoLnrqdF3OdNzjRVLfeyvQ8LgLvRNFR9WfWAyAz79nKgBamd8Ntlvt4Mg35E5gVS2g7AQ7rkm72cB"
           "dnW9sCEyGabeXAuH5j4GRbuLT7qBZWDcFLF4SsCdS3WfFGdNHfwaijzykByo71PvFVlTXH2WJWoFvR5FAL"
           "jBTn7bCdP0pAiSbLCY8Xz2Msc3dBb5Ff9GISPbUpNmUvBdMZMHQvqOmTNXEPpN0b74MDOMQfWJShOo3NkA"
           "vMjs\"},\"testFieldInt\":42}"_ba;
    QTest::newRow("neg_int_and_string")
        << -45 << "qwerty"
        << "{\"testComplexField\":{\"testFieldString\":\"qwerty\"},\"testFieldInt\":-45}"_ba;
}

void QtProtobufTypesJsonSerializationTest::ComplexTypeSerializeTest()
{
    QFETCH(const int, intField);
    QFETCH(const QString, stringField);
    QFETCH(const QByteArray, expectedData);

    SimpleStringMessage stringMessage;
    stringMessage.setTestFieldString(stringField);
    ComplexMessage test, testStd;

    test.setTestFieldInt(intField);
    test.setTestComplexField(stringMessage);

    testStd.setTestFieldInt(intField);
    testStd.setTestComplexField(stringMessage);

    QByteArray result = test.serialize(m_serializer.get());
    QCOMPARE(result, expectedData);
    QVERIFY(!QJsonDocument::fromJson(result).isNull());
}

void QtProtobufTypesJsonSerializationTest::ResetComplexTypeSerializeTest_data()
{
    QTest::addColumn<int>("intField");
    QTest::addColumn<QString>("stringField");
    QTest::addColumn<QByteArray>("expectedData");

    QTest::newRow("empty_value") << 0 << u""_s << "{}"_ba;
    QTest::newRow("value_only_int") << 42 << u""_s << "{\"testFieldInt\":42}"_ba;
    QTest::newRow("value_only_string") << 0 << u"qwerty"_s << "{}"_ba;
    QTest::newRow("int_and_string") << 42 << u"qwerty"_s << "{\"testFieldInt\":42}"_ba;
}

void QtProtobufTypesJsonSerializationTest::ResetComplexTypeSerializeTest()
{
    QFETCH(const int, intField);
    QFETCH(const QString, stringField);
    QFETCH(const QByteArray, expectedData);

    SimpleStringMessage stringMessage;
    stringMessage.setTestFieldString(stringField);
    ComplexMessage test;
    test.setTestFieldInt(intField);
    test.setTestComplexField(stringMessage);

    // Reset Complex field
    test.clearTestComplexField();
    QByteArray result = test.serialize(m_serializer.get());
    QCOMPARE(result, expectedData);
    QVERIFY(!QJsonDocument::fromJson(result).isNull());
}

void QtProtobufTypesJsonSerializationTest::DefaultConstructedComplexTypeSerializeTest()
{
    ComplexMessage test;
    QByteArray result = test.serialize(m_serializer.get());
    QCOMPARE(result, "{}"_ba);
    QVERIFY(!QJsonDocument::fromJson(result).isNull());
}

void QtProtobufTypesJsonSerializationTest::EmptyBytesMessageTest()
{
    SimpleBytesMessage msg;
    QByteArray result = msg.serialize(m_serializer.get());
    QCOMPARE(result, "{}"_ba);
    QVERIFY(!QJsonDocument::fromJson(result).isNull());
}

void QtProtobufTypesJsonSerializationTest::EmptyStringMessageTest()
{
    SimpleStringMessage msg;
    QByteArray result = msg.serialize(m_serializer.get());
    QCOMPARE(result, "{}"_ba);
    QVERIFY(!QJsonDocument::fromJson(result).isNull());
}

void QtProtobufTypesJsonSerializationTest::OneofMessageEmptyTest()
{
    OneofMessage test;
    QByteArray result = test.serialize(m_serializer.get());
    QCOMPARE(result, "{}");
}

void QtProtobufTypesJsonSerializationTest::OneofMessageIntTest()
{
    OneofMessage test;
    test.setTestFieldInt(-45);
    test.setTestOneofFieldInt(-45);
    QByteArray result = test.serialize(m_serializer.get());
    QCOMPARE(result, "{\"testFieldInt\":-45,\"testOneofFieldInt\":-45}"_ba);

    test.setTestFieldInt(0);
    test.setTestOneofFieldInt(0);
    result = test.serialize(m_serializer.get());
    QCOMPARE(result, "{\"testOneofFieldInt\":0}");
}

void QtProtobufTypesJsonSerializationTest::OneofMessageClearTest()
{
    OneofMessage test;
    test.setTestFieldInt(-45);
    test.setTestOneofFieldInt(-45);

    test.clearTestOneof();
    QByteArray result = test.serialize(m_serializer.get());
    QCOMPARE(result, "{\"testFieldInt\":-45}");
}

void QtProtobufTypesJsonSerializationTest::OneofMessageComplexTest()
{
    ComplexMessage complexField;
    SimpleStringMessage stringField;
    stringField.setTestFieldString("qwerty");
    complexField.setTestFieldInt(42);
    complexField.setTestComplexField(stringField);

    OneofMessage test;
    test.setTestOneofComplexField(complexField);
    QByteArray result = test.serialize(m_serializer.get());
    QCOMPARE(result,
             "{\"testOneofComplexField\":{\"testComplexField\":{\"testFieldString\":\"qwerty\"},"
             "\"testFieldInt\":42}}");

    test.clearTestOneof();

    test.setTestOneofComplexField({});
    result = test.serialize(m_serializer.get());
    QCOMPARE(result, "{\"testOneofComplexField\":{}}");

    test.setSecondComplexField({});
    result = test.serialize(m_serializer.get());
    QCOMPARE(result, "{\"secondComplexField\":{},\"testOneofComplexField\":{}}");

    // Check that partially intialized complex field doesn't apply its 'oneof'
    // peculiarity to the child fields.
    complexField.setTestComplexField({});
    complexField.setTestFieldInt(42);
    test.setTestOneofComplexField(complexField);
    test.setSecondComplexField({});
    result = test.serialize(m_serializer.get());
    QCOMPARE(result,
             "{\"secondComplexField\":{},\"testOneofComplexField\":{\"testComplexField\":{},"
             "\"testFieldInt\":42}}");
}

QTEST_MAIN(QtProtobufTypesJsonSerializationTest)
#include "tst_protobuf_serialization_json_basictypes.moc"
