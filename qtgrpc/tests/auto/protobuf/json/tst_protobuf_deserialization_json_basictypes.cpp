// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "basicmessages.qpb.h"

#include <QObject>
#include <QProtobufJsonSerializer>
#include <QTest>

#include <cmath>

using namespace Qt::Literals::StringLiterals;

class QtProtobufTypesJsonDeserializationTest : public QObject
{
    Q_OBJECT
private slots:
    void init() { serializer.reset(new QProtobufJsonSerializer); }

    void BoolMessageDeserializeTest_data();
    void BoolMessageDeserializeTest();
    void IntMessageDeserializeTest_data();
    void IntMessageDeserializeTest();
    void UIntMessageDeserializeTest_data();
    void UIntMessageDeserializeTest();
    void SIntMessageDeserializeTest_data();
    void SIntMessageDeserializeTest();
    void Int64MessageDeserializeTest_data();
    void Int64MessageDeserializeTest();
    void UInt64MessageDeserializeTest_data();
    void UInt64MessageDeserializeTest();
    void SInt64MessageDeserializeTest_data();
    void SInt64MessageDeserializeTest();
    void FixedInt32MessageDeserializeTest_data();
    void FixedInt32MessageDeserializeTest();
    void FixedInt64MessageDeserializeTest_data();
    void FixedInt64MessageDeserializeTest();
    void SFixedInt32MessageDeserializeTest_data();
    void SFixedInt32MessageDeserializeTest();
    void SFixedInt64MessageDeserializeTest_data();
    void SFixedInt64MessageDeserializeTest();
    void FloatMessageDeserializeTest_data();
    void FloatMessageDeserializeTest();
    void DoubleMessageDeserializeTest_data();
    void DoubleMessageDeserializeTest();
    void StringMessageDeserializeTest();
    void ComplexTypeDeserializeTest_data();
    void ComplexTypeDeserializeTest();
    void ResetComplexTypeDeserializeTest_data();
    void ResetComplexTypeDeserializeTest();
    void DefaultConstructedComplexTypeDeserializeTest();
    void EmptyBytesMessageTest();
    void EmptyStringMessageTest();

    void MalformedJsonTest();
    void InvalidTypeTest();
private:
    std::unique_ptr<QProtobufJsonSerializer> serializer;
};

using namespace qtprotobufnamespace::tests;

void QtProtobufTypesJsonDeserializationTest::BoolMessageDeserializeTest_data()
{
    QTest::addColumn<bool>("value");
    QTest::addColumn<QByteArray>("serializeData");

    QTest::newRow("True") << true << "{\"testFieldBool\":true}"_ba;
    QTest::newRow("False") << false << "{\"testFieldBool\":false}"_ba;
}

void QtProtobufTypesJsonDeserializationTest::BoolMessageDeserializeTest()
{
    QFETCH(const bool, value);
    QFETCH(const QByteArray, serializeData);

    SimpleBoolMessage test;
    test.deserialize(serializer.get(), serializeData);
    QCOMPARE(test.testFieldBool(), value);
}

void QtProtobufTypesJsonDeserializationTest::IntMessageDeserializeTest_data()
{
    QTest::addColumn<int32_t>("value");
    QTest::addColumn<QByteArray>("serializeData");

    QTest::newRow("555") << 555 << "{\"testFieldInt\":555}"_ba;
    QTest::newRow("0") << 0 << "{\"testFieldInt\":0}"_ba;
}

void QtProtobufTypesJsonDeserializationTest::IntMessageDeserializeTest()
{
    QFETCH(const int32_t, value);
    QFETCH(const QByteArray, serializeData);

    SimpleIntMessage test;
    test.deserialize(serializer.get(), serializeData);
    QCOMPARE(test.testFieldInt(), value);
}

void QtProtobufTypesJsonDeserializationTest::UIntMessageDeserializeTest_data()
{
    QTest::addColumn<uint32_t>("value");
    QTest::addColumn<QByteArray>("serializeData");

    QTest::newRow("15") << (uint32_t)15 << "{\"testFieldInt\":15}"_ba;
    QTest::newRow("300") << (uint32_t)300 << "{\"testFieldInt\":300}"_ba;
    QTest::newRow("65545") << (uint32_t)65545 << "{\"testFieldInt\":65545}"_ba;
}

void QtProtobufTypesJsonDeserializationTest::UIntMessageDeserializeTest()
{
    QFETCH(const uint32_t, value);
    QFETCH(const QByteArray, serializeData);

    SimpleUIntMessage test;
    test.deserialize(serializer.get(), serializeData);
    QCOMPARE(test.testFieldInt(), value);
}

void QtProtobufTypesJsonDeserializationTest::SIntMessageDeserializeTest_data()
{
    QTest::addColumn<int32_t>("value");
    QTest::addColumn<QByteArray>("serializeData");

    QTest::newRow("15") << (int32_t)15 << "{\"testFieldInt\":15}"_ba;
    QTest::newRow("300") << (int32_t)300 << "{\"testFieldInt\":300}"_ba;
    QTest::newRow("65545") << (int32_t)65545 << "{\"testFieldInt\":65545}"_ba;

    QTest::newRow("-1") << (int32_t)-1 << "{\"testFieldInt\":-1}"_ba;
    QTest::newRow("-462") << (int32_t)-462 << "{\"testFieldInt\":-462}"_ba;
    QTest::newRow("-63585") << (int32_t)-63585 << "{\"testFieldInt\":-63585}"_ba;
}

void QtProtobufTypesJsonDeserializationTest::SIntMessageDeserializeTest()
{
    QFETCH(const int32_t, value);
    QFETCH(const QByteArray, serializeData);

    SimpleSIntMessage test;
    test.deserialize(serializer.get(), serializeData);
    QCOMPARE(test.testFieldInt(), value);
}

void QtProtobufTypesJsonDeserializationTest::Int64MessageDeserializeTest_data()
{
    QTest::addColumn<int64_t>("value");
    QTest::addColumn<QByteArray>("serializeData");

    QTest::newRow("15") << (int64_t)15 << "{\"testFieldInt\":15}"_ba;
    QTest::newRow("300") << (int64_t)300 << "{\"testFieldInt\":300}"_ba;
    QTest::newRow("65545") << (int64_t)65545 << "{\"testFieldInt\":65545}"_ba;

    QTest::newRow("-1") << (int64_t)-1 << "{\"testFieldInt\":-1}"_ba;
    QTest::newRow("-462") << (int64_t)-462 << "{\"testFieldInt\":-462}"_ba;
    QTest::newRow("-63585") << (int64_t)-63585 << "{\"testFieldInt\":-63585}"_ba;
}

void QtProtobufTypesJsonDeserializationTest::Int64MessageDeserializeTest()
{
    QFETCH(const int64_t, value);
    QFETCH(const QByteArray, serializeData);

    SimpleInt64Message test;
    test.deserialize(serializer.get(), serializeData);
    QCOMPARE(test.testFieldInt(), value);
}

void QtProtobufTypesJsonDeserializationTest::UInt64MessageDeserializeTest_data()
{
    QTest::addColumn<uint64_t>("value");
    QTest::addColumn<QByteArray>("serializeData");

    QTest::newRow("15") << (uint64_t)15 << "{\"testFieldInt\":15}"_ba;
    QTest::newRow("300") << (uint64_t)300 << "{\"testFieldInt\":300}"_ba;
    QTest::newRow("65545") << (uint64_t)65545 << "{\"testFieldInt\":65545}"_ba;
    QTest::newRow("18446744073709549568")
        << uint64_t(18446744073709549568u) << "{\"testFieldInt\":18446744073709549568}"_ba;
    QTest::newRow("123245324235425234")
        << uint64_t(123245324235425234u) << "{\"testFieldInt\":123245324235425234}"_ba;
    QTest::newRow("123245324235425234_string")
        << uint64_t(123245324235425234u) << "{\"testFieldInt\":\"123245324235425234\"}"_ba;
}

void QtProtobufTypesJsonDeserializationTest::UInt64MessageDeserializeTest()
{
    QFETCH(const QtProtobuf::uint64, value);
    QFETCH(const QByteArray, serializeData);

    SimpleUInt64Message test;
    test.deserialize(serializer.get(), serializeData);
    QCOMPARE(test.testFieldInt(), value);
}

void QtProtobufTypesJsonDeserializationTest::SInt64MessageDeserializeTest_data()
{
    QTest::addColumn<int64_t>("value");
    QTest::addColumn<QByteArray>("serializeData");

    QTest::newRow("15") << (int64_t)15 << "{\"testFieldInt\":15}"_ba;
    QTest::newRow("300") << (int64_t)300 << "{\"testFieldInt\":300}"_ba;
    QTest::newRow("65545") << (int64_t)65545 << "{\"testFieldInt\":65545}"_ba;

    QTest::newRow("-1") << (int64_t)-1 << "{\"testFieldInt\":-1}"_ba;
    QTest::newRow("-462") << (int64_t)-462 << "{\"testFieldInt\":-462}"_ba;
    QTest::newRow("-63585") << (int64_t)-63585 << "{\"testFieldInt\":-63585}"_ba;
}

void QtProtobufTypesJsonDeserializationTest::SInt64MessageDeserializeTest()
{
    QFETCH(const int64_t, value);
    QFETCH(const QByteArray, serializeData);

    SimpleSInt64Message test;
    test.deserialize(serializer.get(), serializeData);
    QCOMPARE(test.testFieldInt(), value);
}

void QtProtobufTypesJsonDeserializationTest::FixedInt32MessageDeserializeTest_data()
{
    QTest::addColumn<uint32_t>("value");
    QTest::addColumn<QByteArray>("serializeData");

    QTest::newRow("15") << (uint32_t)15 << "{\"testFieldFixedInt32\":15}"_ba;
    QTest::newRow("300") << (uint32_t)300 << "{\"testFieldFixedInt32\":300}"_ba;
    QTest::newRow("65545") << (uint32_t)65545 << "{\"testFieldFixedInt32\":65545}"_ba;

    QTest::newRow("5") << (uint32_t)5 << "{\"testFieldFixedInt32\": 5}"_ba;
    QTest::newRow("0") << (uint32_t)0 << "{\"testFieldFixedInt32\": 0}"_ba;
    QTest::newRow("555") << (uint32_t)555 << "{\"testFieldFixedInt32\":555}"_ba;
    QTest::newRow("nullptr") << (uint32_t)0 << "{\"testFieldFixedInt32\":null}"_ba;
}

void QtProtobufTypesJsonDeserializationTest::FixedInt32MessageDeserializeTest()
{
    QFETCH(const uint32_t, value);
    QFETCH(const QByteArray, serializeData);

    SimpleFixedInt32Message test;
    test.deserialize(serializer.get(), serializeData);
    QCOMPARE(test.testFieldFixedInt32(), value);
}

void QtProtobufTypesJsonDeserializationTest::FixedInt64MessageDeserializeTest_data()
{
    QTest::addColumn<uint64_t>("value");
    QTest::addColumn<QByteArray>("serializeData");

    QTest::newRow("15") << (uint64_t)15 << "{\"testFieldFixedInt64\":15}"_ba;
    QTest::newRow("300") << (uint64_t)300 << "{\"testFieldFixedInt64\":300}"_ba;
    QTest::newRow("65545") << (uint64_t)65545 << "{\"testFieldFixedInt64\":65545}"_ba;
}

void QtProtobufTypesJsonDeserializationTest::FixedInt64MessageDeserializeTest()
{
    QFETCH(const uint64_t, value);
    QFETCH(const QByteArray, serializeData);

    SimpleFixedInt64Message test;
    test.deserialize(serializer.get(), serializeData);
    QCOMPARE(test.testFieldFixedInt64(), value);
}

void QtProtobufTypesJsonDeserializationTest::SFixedInt32MessageDeserializeTest_data()
{
    QTest::addColumn<int32_t>("value");
    QTest::addColumn<QByteArray>("serializeData");

    QTest::newRow("15") << (int32_t)15 << "{\"testFieldFixedInt32\":15}"_ba;
    QTest::newRow("300") << (int32_t)300 << "{\"testFieldFixedInt32\":300}"_ba;
    QTest::newRow("65545") << (int32_t)65545 << "{\"testFieldFixedInt32\":65545}"_ba;

    QTest::newRow("-1") << (int32_t)-1 << "{\"testFieldFixedInt32\":-1}"_ba;
    QTest::newRow("-462") << (int32_t)-462 << "{\"testFieldFixedInt32\":-462}"_ba;
    QTest::newRow("-63585") << (int32_t)-63585 << "{\"testFieldFixedInt32\":-63585}"_ba;
}

void QtProtobufTypesJsonDeserializationTest::SFixedInt32MessageDeserializeTest()
{
    QFETCH(const int32_t, value);
    QFETCH(const QByteArray, serializeData);

    SimpleSFixedInt32Message test;
    test.deserialize(serializer.get(), serializeData);
    QCOMPARE(test.testFieldFixedInt32(), value);
}

void QtProtobufTypesJsonDeserializationTest::SFixedInt64MessageDeserializeTest_data()
{
    QTest::addColumn<int64_t>("value");
    QTest::addColumn<QByteArray>("serializeData");

    QTest::newRow("15") << (int64_t)15 << "{\"testFieldFixedInt64\":15}"_ba;
    QTest::newRow("300") << (int64_t)300 << "{\"testFieldFixedInt64\":300}"_ba;
    QTest::newRow("65545") << (int64_t)65545 << "{\"testFieldFixedInt64\":65545}"_ba;

    QTest::newRow("-1") << (int64_t)-1 << "{\"testFieldFixedInt64\":-1}"_ba;
    QTest::newRow("-462") << (int64_t)-462 << "{\"testFieldFixedInt64\":-462}"_ba;
    QTest::newRow("-63585") << (int64_t)-63585 << "{\"testFieldFixedInt64\":-63585}"_ba;
}

void QtProtobufTypesJsonDeserializationTest::SFixedInt64MessageDeserializeTest()
{
    QFETCH(const int64_t, value);
    QFETCH(const QByteArray, serializeData);

    SimpleSFixedInt64Message test;
    test.deserialize(serializer.get(), serializeData);
    QCOMPARE(test.testFieldFixedInt64(), value);
}

void QtProtobufTypesJsonDeserializationTest::FloatMessageDeserializeTest_data()
{
    QTest::addColumn<float>("value");
    QTest::addColumn<QByteArray>("serializeData");

    QTest::newRow("float_value_0_1") << 0.1f << "{\"testFieldFloat\":0.1}"_ba;
    QTest::newRow("float_value_min") << std::numeric_limits<float>::min()
                                     << "{\"testFieldFloat\":1.17549e-38}"_ba;
    QTest::newRow("float_value_max") << std::numeric_limits<float>::max()
                                     << "{\"testFieldFloat\":3.40282e+38}"_ba;
    QTest::newRow("float_neg_value_4_2") << -4.2f << "{\"testFieldFloat\":-4.2}"_ba;
    QTest::newRow("float_neg_value_0_0") << (float)-0.0f << "{\"testFieldFloat\":-0}"_ba;
    QTest::newRow("float_value_0_0") << (float)0.0f << "{\"testFieldFloat\":0}"_ba;
    QTest::newRow("float_nan") << NAN << "{\"testFieldFloat\":\"NaN\"}"_ba;
    QTest::newRow("float_infinity") << INFINITY << "{\"testFieldFloat\":\"Infinity\"}"_ba;
    QTest::newRow("float_neg_infinity") << -INFINITY << "{\"testFieldFloat\":\"-Infinity\"}"_ba;
}

void QtProtobufTypesJsonDeserializationTest::FloatMessageDeserializeTest()
{
    QFETCH(const float, value);
    QFETCH(const QByteArray, serializeData);

    SimpleFloatMessage test;
    test.setTestFieldFloat(value);
    test.deserialize(serializer.get(), serializeData);
    QCOMPARE(test.testFieldFloat(), value);
    // See QTBUG-120077
    QEXPECT_FAIL("float_neg_value_0_0", "QJsonValue ignores sign bit when deserializing floating"
                                        " point values", Continue);
    QCOMPARE(std::signbit(test.testFieldFloat()), std::signbit(value));
}

void QtProtobufTypesJsonDeserializationTest::DoubleMessageDeserializeTest_data()
{
    QTest::addColumn<double>("value");
    QTest::addColumn<QByteArray>("serializeData");

    QTest::newRow("double_value_0_1") << 0.1 << "{\"testFieldDouble\":0.1}"_ba;
    QTest::newRow("double_value_min")
        << std::numeric_limits<double>::min()
        << "{\"testFieldDouble\":2.2250738585072014e-308}"_ba;
    QTest::newRow("double_value_max")
        << std::numeric_limits<double>::max()
        << "{\"testFieldDouble\":1.7976931348623157e+308}"_ba;
    QTest::newRow("double_neg_value_4_2") << -4.2
                                          << "{\"testFieldDouble\":-4.2}"_ba;
    QTest::newRow("double_value_0_0") << 0.0 << "{\"testFieldDouble\":0}"_ba;
    QTest::newRow("double_neg_value_0_0") << -0.0 << "{\"testFieldDouble\":-0}"_ba;
    QTest::newRow("double_nan") << double(NAN) << "{\"testFieldDouble\":\"NaN\"}"_ba;
    QTest::newRow("double_infinity") << double(INFINITY) << "{\"testFieldDouble\":\"Infinity\"}"_ba;
    QTest::newRow("double_neg_infinity")
        << double(-INFINITY) << "{\"testFieldDouble\":\"-Infinity\"}"_ba;
}

void QtProtobufTypesJsonDeserializationTest::DoubleMessageDeserializeTest()
{
    QFETCH(const double, value);
    QFETCH(const QByteArray, serializeData);

    SimpleDoubleMessage test;
    test.setTestFieldDouble(value);
    test.deserialize(serializer.get(), serializeData);
    QCOMPARE(test.testFieldDouble(), value);
    // See QTBUG-120077
    QEXPECT_FAIL("double_neg_value_0_0", "QJsonValue ignores sign bit when deserializing floating"
                                        " point values", Continue);
    QCOMPARE(std::signbit(test.testFieldDouble()), std::signbit(value));
}

void QtProtobufTypesJsonDeserializationTest::StringMessageDeserializeTest()
{
    SimpleStringMessage test;
    test.deserialize(serializer.get(), "{\"testFieldString\":\"qwerty\"}"_ba);
    QCOMPARE(test.testFieldString(), "qwerty");

    test.deserialize(serializer.get(), "{\"testFieldString\":\"\"}"_ba);
    QCOMPARE(test.testFieldString(), "");

    test.deserialize(serializer.get(), "{\"testFieldString\":\"null\"}"_ba);
    QCOMPARE(test.testFieldString(), "null");

    test.deserialize(serializer.get(), "{\"testFieldString\":\"NaN\"}"_ba);
    QCOMPARE(test.testFieldString(), "NaN");

    test.deserialize(serializer.get(), "{\"testFieldString\":\"Infinity\"}"_ba);
    QCOMPARE(test.testFieldString(), "Infinity");

    test.deserialize(serializer.get(), "{\"testFieldString\":\"-Infinity\"}"_ba);
    QCOMPARE(test.testFieldString(), "-Infinity");
}

void QtProtobufTypesJsonDeserializationTest::ComplexTypeDeserializeTest_data()
{
    QTest::addColumn<int>("intField");
    QTest::addColumn<QString>("stringField");
    QTest::addColumn<QByteArray>("serializeData");

    QTest::newRow("empty_value")
        << 0 << ""
        << "{\"testFieldInt\":0,\"testComplexField\":{\"testFieldString\":\"\"}}"_ba;
    QTest::newRow("null_complex_field")
        << 0 << ""
        << "{\"testFieldInt\":0,\"testComplexField\":null}"_ba;
    QTest::newRow("value_only_int")
        << 42 << ""
        << "{\"testFieldInt\":42,\"testComplexField\":{\"testFieldString\":\"\"}}"_ba;
    QTest::newRow("value_only_string")
        << 0 << "qwerty"
        << "{\"testFieldInt\":0,\"testComplexField\":{\"testFieldString\":\"qwerty\"}}"_ba;
    QTest::newRow("int_and_string")
        << 42 << "qwerty"
        << "{\"testFieldInt\":42,\"testComplexField\":{\"testFieldString\":\"qwerty\"}}"_ba;
    QTest::newRow("int_and_big_string")
        << 42
        << "YVRfJvjxqbgvFwS1YvOZXgtj5ffGLS7AiNHz9oZIoKbm7z8H79xBuyPkpQXvGoO09OY9xRawx3eOAs9xjo"
           "TA1xJhrw28TAcq1CebYlC9WUfQC6hIantaNdyHiKToffi0Zt7la42SRxXZSP4GuxbcZIp53pJnyCwfCy1q"
           "dFczT0dmn7h8fpyAdemEavwFeda4d0PApGfSU2jLt39X8kYUBxNM2WgALRBgHdVde87q6Pi5U69TjhMd28"
           "W1SFD1DxyogCCrqOct2ZPICoLnrqdF3OdNzjRVLfeyvQ8LgLvRNFR9WfWAyAz79nKgBamd8Ntlvt4Mg35E"
           "5gVS2g7AQ7rkm72cBdnW9sCEyGabeXAuH5j4GRbuLT7qBZWDcFLF4SsCdS3WfFGdNHfwaijzykByo71PvF"
           "VlTXH2WJWoFvR5FALjBTn7bCdP0pAiSbLCY8Xz2Msc3dBb5Ff9GISPbUpNmUvBdMZMHQvqOmTNXEPpN0b7"
           "4MDOMQfWJShOo3NkAvMjs"
        << "{\"testFieldInt\":42,\"testComplexField\":{\"testFieldString\":\"YVRfJvjxqbgvFwS1Y"
           "vOZXgtj5ffGLS7AiNHz9oZIoKbm7z8H79xBuyPkpQXvGoO09OY9xRawx3eOAs9xjoTA1xJhrw28TAcq1Ce"
           "bYlC9WUfQC6hIantaNdyHiKToffi0Zt7la42SRxXZSP4GuxbcZIp53pJnyCwfCy1qdFczT0dmn7h8fpyAd"
           "emEavwFeda4d0PApGfSU2jLt39X8kYUBxNM2WgALRBgHdVde87q6Pi5U69TjhMd28W1SFD1DxyogCCrqOc"
           "t2ZPICoLnrqdF3OdNzjRVLfeyvQ8LgLvRNFR9WfWAyAz79nKgBamd8Ntlvt4Mg35E5gVS2g7AQ7rkm72cB"
           "dnW9sCEyGabeXAuH5j4GRbuLT7qBZWDcFLF4SsCdS3WfFGdNHfwaijzykByo71PvFVlTXH2WJWoFvR5FAL"
           "jBTn7bCdP0pAiSbLCY8Xz2Msc3dBb5Ff9GISPbUpNmUvBdMZMHQvqOmTNXEPpN0b74MDOMQfWJShOo3NkA"
           "vMjs\"}}"_ba;
    QTest::newRow("neg_int_and_string")
        << -45 << "qwerty"
        << "{\"testFieldInt\":-45,\"testComplexField\":{\"testFieldString\":\"qwerty\"}}"_ba;
}

void QtProtobufTypesJsonDeserializationTest::ComplexTypeDeserializeTest()
{
    QFETCH(const int, intField);
    QFETCH(const QString, stringField);
    QFETCH(const QByteArray, serializeData);

    SimpleStringMessage stringMessage;
    stringMessage.setTestFieldString(stringField);

    ComplexMessage test;
    test.deserialize(serializer.get(), serializeData);
    QCOMPARE(test.testFieldInt(), intField);
    QCOMPARE(test.testComplexField(), stringMessage);
}

void QtProtobufTypesJsonDeserializationTest::ResetComplexTypeDeserializeTest_data()
{
    QTest::addColumn<int>("intField");
    QTest::addColumn<QString>("stringField");
    QTest::addColumn<QByteArray>("serializeData");

    QTest::newRow("empty_value")
        << 0 << u""_s << "{\"testFieldInt\":0,\"testComplexField\":{}}"_ba;
    QTest::newRow("value_only_int")
        << 42 << u""_s << "{\"testFieldInt\":42,\"testComplexField\":{}}"_ba;
    QTest::newRow("value_only_string")
        << 0 << u""_s << "{\"testFieldInt\":0,\"testComplexField\":{}}"_ba;
    QTest::newRow("int_and_string")
        << 42 << u"qwerty"_s
        << "{\"testFieldInt\":42,\"testComplexField\":{\"testFieldString\":\"qwerty\"}}"_ba;
}

void QtProtobufTypesJsonDeserializationTest::ResetComplexTypeDeserializeTest()
{
    QFETCH(const int, intField);
    QFETCH(const QString, stringField);
    QFETCH(const QByteArray, serializeData);

    SimpleStringMessage stringMessage;
    stringMessage.setTestFieldString(stringField);
    ComplexMessage test;
    test.setTestFieldInt(intField);
    test.setTestComplexField(stringMessage);

    // Reset Complex field
    test.clearTestComplexField();
    test.deserialize(serializer.get(), serializeData);
    QCOMPARE(test.testFieldInt(), intField);
    QCOMPARE(test.testComplexField(), stringMessage);
}

void QtProtobufTypesJsonDeserializationTest::DefaultConstructedComplexTypeDeserializeTest()
{
    ComplexMessage test;
    test.deserialize(serializer.get(), "{\"testFieldInt\":0,\"testComplexField\":{}}"_ba);
    QCOMPARE(test.testFieldInt(), 0);
    QCOMPARE(test.testComplexField().testFieldString(), "");
}

void QtProtobufTypesJsonDeserializationTest::EmptyBytesMessageTest()
{
    SimpleBytesMessage test;
    test.deserialize(serializer.get(), "{\"testFieldBytes\":\"\"}"_ba);
    QCOMPARE(test.testFieldBytes(), "");
}

void QtProtobufTypesJsonDeserializationTest::EmptyStringMessageTest()
{
    SimpleStringMessage test;
    test.deserialize(serializer.get(), "{\"testFieldString\":\"\"}"_ba);
    QCOMPARE(test.testFieldString(), "");
}

void QtProtobufTypesJsonDeserializationTest::MalformedJsonTest()
{
    ComplexMessage msg;
    // Closing brace is missing
    msg.deserialize(serializer.get(),
                    "{\"testFieldInt\":-45,"
                    "\"testComplexField\":{\"testFieldString\":\"qwerty\"}"_ba);

    QCOMPARE(msg.testFieldInt(), 0);
    QCOMPARE(msg.testComplexField(), SimpleStringMessage());
    QCOMPARE(serializer->deserializationError(),
             QAbstractProtobufSerializer::UnexpectedEndOfStreamError);

    msg.deserialize(serializer.get(),
                    "[{\"testFieldInt\":-45,"
                    "\"testComplexField\":{\"testFieldString\":\"qwerty\"}}]"_ba);
    QCOMPARE(msg.testFieldInt(), 0);
    QCOMPARE(msg.testComplexField(), SimpleStringMessage());
    QCOMPARE(serializer->deserializationError(),
             QAbstractProtobufSerializer::InvalidFormatError);
}

void QtProtobufTypesJsonDeserializationTest::InvalidTypeTest()
{
    ComplexMessage msg;
    // Closing brace is missing
    msg.deserialize(serializer.get(),
                    "{\"testFieldInt\":-45,"
                    "\"testComplexField\":200}"_ba);

    QCOMPARE(msg.testFieldInt(), 0);
    QCOMPARE(msg.testComplexField(), SimpleStringMessage());
    QCOMPARE(serializer->deserializationError(),
             QAbstractProtobufSerializer::InvalidFormatError);

    // Expected integer but the value is an array
    msg.deserialize(serializer.get(),
                    "{\"testFieldInt\":[],"
                    "\"testComplexField\":{\"testFieldString\":\"qwerty\"}}"_ba);

    QCOMPARE(msg.testFieldInt(), 0);
    QCOMPARE(msg.testComplexField().testFieldString(), "qwerty"_L1);
    QCOMPARE(serializer->deserializationError(),
             QAbstractProtobufSerializer::InvalidFormatError);

    SimpleIntMessage intMsg;
    intMsg.deserialize(serializer.get(), "{\"testFieldInt\": 0.5}");
    QCOMPARE(serializer->deserializationError(), QAbstractProtobufSerializer::InvalidFormatError);
    QCOMPARE(intMsg.testFieldInt(), 0);

    intMsg.deserialize(serializer.get(), "{\"testFieldInt\":4294967296}");
    QCOMPARE(serializer->deserializationError(), QAbstractProtobufSerializer::InvalidFormatError);
    QCOMPARE(intMsg.testFieldInt(), 0);

    SimpleUIntMessage uintMsg;
    uintMsg.deserialize(serializer.get(), "{\"testFieldInt\":4294967296}");
    QCOMPARE(serializer->deserializationError(), QAbstractProtobufSerializer::InvalidFormatError);
    QCOMPARE(uintMsg.testFieldInt(), 0u);
}


QTEST_MAIN(QtProtobufTypesJsonDeserializationTest)
#include "tst_protobuf_deserialization_json_basictypes.moc"
