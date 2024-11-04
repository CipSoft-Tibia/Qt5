// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QtCore/QBuffer>
#include <QtCore/QDataStream>

#include "qbitarray.h"

#include <QtCore/qelapsedtimer.h>
#include <QtCore/qscopeguard.h>

/**
 * Helper function to initialize a bitarray from a string
 */
static QBitArray QStringToQBitArray(const QString &str)
{
    QBitArray ba;
    ba.resize(str.size());
    int i;
    QChar tru('1');
    for (i = 0; i < str.size(); i++)
    {
        if (str.at(i) == tru)
        {
            ba.setBit(i, true);
        }
    }
    return ba;
}

static QBitArray detached(QBitArray a)
{
    a.detach();
    return a;
}

class tst_QBitArray : public QObject
{
    Q_OBJECT
private slots:
    void canHandleIntMaxBits();
    void size_data();
    void size();
    void countBits_data();
    void countBits();
    void countBits2();
    void isEmpty();
    void swap();
    void fill();
    void toggleBit_data();
    void toggleBit();
    // operator &=
    void operator_andeq_data();
    void operator_andeq();
    // operator &
    void operator_and_data() { operator_andeq_data(); }
    void operator_and();
    // operator |=
    void operator_oreq_data();
    void operator_oreq();
    // operator |
    void operator_or_data() { operator_oreq_data(); }
    void operator_or();
    // operator ^=
    void operator_xoreq_data();
    void operator_xoreq();
    // operator ^
    void operator_xor_data() { operator_xoreq_data(); }
    void operator_xor();
    // operator ~
    void operator_neg_data();
    void operator_neg();
    void datastream_data();
    void datastream();
    void invertOnNull() const;
    void operator_noteq_data();
    void operator_noteq();

    void resize();
    void fromBits_data();
    void fromBits();

    void toUInt32_data();
    void toUInt32();
};

void tst_QBitArray::canHandleIntMaxBits()
{
    QElapsedTimer timer;
    timer.start();
    const auto print = qScopeGuard([&] {
        qDebug("Function took %lldms", qlonglong(timer.elapsed()));
    });

    try {
        constexpr qsizetype Size1 = sizeof(void*) > sizeof(int) ? qsizetype(INT_MAX) + 2 :
                                                                  INT_MAX - 2;
        constexpr qsizetype Size2 = Size1 + 2;

        QBitArray ba(Size1, true);
        QCOMPARE(ba.size(), Size1);
        QCOMPARE(ba.at(Size1 - 1), true);

        ba.resize(Size2);
        QCOMPARE(ba.size(), Size2);
        QCOMPARE(ba.at(Size1 - 1), true);
        QCOMPARE(ba.at(Size1),     false);
        QCOMPARE(ba.at(Size2 - 1), false);

        QByteArray serialized;
        if constexpr (sizeof(void*) > sizeof(int)) {
            QDataStream ds(&serialized, QIODevice::WriteOnly);
            ds.setVersion(QDataStream::Qt_5_15);
            ds << ba;
            QCOMPARE(ds.status(), QDataStream::Status::SizeLimitExceeded);
            serialized.clear();
        }
        {
            QDataStream ds(&serialized, QIODevice::WriteOnly);
            ds << ba;
            QCOMPARE(ds.status(), QDataStream::Status::Ok);
        }
        {
            QDataStream ds(serialized);
            QBitArray ba2;
            ds >> ba2;
            QCOMPARE(ds.status(), QDataStream::Status::Ok);
            QCOMPARE(ba, ba2);
        }
    } catch (const std::bad_alloc &) {
        QSKIP("Failed to allocate sufficient memory");
    }
}

void tst_QBitArray::size_data()
{
    //create the testtable instance and define the elements
    QTest::addColumn<int>("count");
    QTest::addColumn<QString>("res");

    //next we fill it with data
    QTest::newRow( "data0" )  << 1 << QString("1");
    QTest::newRow( "data1" )  << 2 << QString("11");
    QTest::newRow( "data2" )  << 3 << QString("111");
    QTest::newRow( "data3" )  << 9 << QString("111111111");
    QTest::newRow( "data4" )  << 10 << QString("1111111111");
    QTest::newRow( "data5" )  << 17 << QString("11111111111111111");
    QTest::newRow( "data6" )  << 18 << QString("111111111111111111");
    QTest::newRow( "data7" )  << 19 << QString("1111111111111111111");
    QTest::newRow( "data8" )  << 20 << QString("11111111111111111111");
    QTest::newRow( "data9" )  << 21 << QString("111111111111111111111");
    QTest::newRow( "data10" )  << 22 << QString("1111111111111111111111");
    QTest::newRow( "data11" )  << 23 << QString("11111111111111111111111");
    QTest::newRow( "data12" )  << 24 << QString("111111111111111111111111");
    QTest::newRow( "data13" )  << 25 << QString("1111111111111111111111111");
    QTest::newRow( "data14" )  << 32 << QString("11111111111111111111111111111111");
}

void tst_QBitArray::size()
{
    QFETCH(int,count);

    QString S;
    QBitArray a(count);
    a.fill(1);
    int len = a.size();
    for (int j=0; j<len; j++) {
        bool b = a[j];
        if (b)
            S+= QLatin1Char('1');
        else
            S+= QLatin1Char('0');
    }
    QTEST(S,"res");
}

void tst_QBitArray::countBits_data()
{
    QTest::addColumn<QString>("bitField");
    QTest::addColumn<int>("numBits");
    QTest::addColumn<int>("onBits");

    QTest::newRow("empty") << QString() << 0 << 0;
    QTest::newRow("1") << QString("1") << 1 << 1;
    QTest::newRow("101") << QString("101") << 3 << 2;
    QTest::newRow("101100001") << QString("101100001") << 9 << 4;
    QTest::newRow("101100001101100001") << QString("101100001101100001") << 18 << 8;
    QTest::newRow("101100001101100001101100001101100001") << QString("101100001101100001101100001101100001") << 36 << 16;
    QTest::newRow("00000000000000000000000000000000000") << QString("00000000000000000000000000000000000") << 35 << 0;
    QTest::newRow("11111111111111111111111111111111111") << QString("11111111111111111111111111111111111") << 35 << 35;
    QTest::newRow("11111111111111111111111111111111") << QString("11111111111111111111111111111111") << 32 << 32;
    QTest::newRow("11111111111111111111111111111111111111111111111111111111")
        << QString("11111111111111111111111111111111111111111111111111111111") << 56 << 56;
    QTest::newRow("00000000000000000000000000000000") << QString("00000000000000000000000000000000") << 32 << 0;
    QTest::newRow("00000000000000000000000000000000000000000000000000000000")
        << QString("00000000000000000000000000000000000000000000000000000000") << 56 << 0;
}

void tst_QBitArray::countBits()
{
    QFETCH(QString, bitField);
    QFETCH(int, numBits);
    QFETCH(int, onBits);

    QBitArray bits(bitField.size());
    for (int i = 0; i < bitField.size(); ++i) {
        if (bitField.at(i) == QLatin1Char('1'))
            bits.setBit(i);
    }

    QCOMPARE(bits.size(), numBits);
    // NOLINTNEXTLINE(qt-port-to-std-compatible-api): We want to test count() and size()
    QCOMPARE(bits.count(), numBits);
    QCOMPARE(bits.count(true), onBits);
    QCOMPARE(bits.count(false), numBits - onBits);
}

void tst_QBitArray::countBits2()
{
    QBitArray bitArray;
    for (int i = 0; i < 4017; ++i) {
        bitArray.resize(i);
        bitArray.fill(true);
        QCOMPARE(bitArray.count(true), i);
        QCOMPARE(bitArray.count(false), 0);
        bitArray.fill(false);
        QCOMPARE(bitArray.count(true), 0);
        QCOMPARE(bitArray.count(false), i);
    }
}

void tst_QBitArray::isEmpty()
{
    QBitArray a1;
    QVERIFY(a1.isEmpty());
    QVERIFY(a1.isNull());
    QVERIFY(a1.size() == 0);

    QBitArray a2(0, true);
    QVERIFY(a2.isEmpty());
    QVERIFY(!a2.isNull());
    QVERIFY(a2.size() == 0);

    QBitArray a3(1, true);
    QVERIFY(!a3.isEmpty());
    QVERIFY(!a3.isNull());
    QVERIFY(a3.size() == 1);

    a1.resize(0);
    QVERIFY(a1.isEmpty());
    QVERIFY(!a1.isNull());
    QVERIFY(a1.size() == 0);

    a2.resize(0);
    QVERIFY(a2.isEmpty());
    QVERIFY(!a2.isNull());
    QVERIFY(a2.size() == 0);

    a1.resize(1);
    QVERIFY(!a1.isEmpty());
    QVERIFY(!a1.isNull());
    QVERIFY(a1.size() == 1);

    a1.resize(2);
    QVERIFY(!a1.isEmpty());
    QVERIFY(!a1.isNull());
    QVERIFY(a1.size() == 2);
}

void tst_QBitArray::swap()
{
    QBitArray b1 = QStringToQBitArray("1"), b2 = QStringToQBitArray("10");
    b1.swap(b2);
    QCOMPARE(b1,QStringToQBitArray("10"));
    QCOMPARE(b2,QStringToQBitArray("1"));
}

void tst_QBitArray::fill()
{
    int N = 64;
    int M = 17;
    QBitArray a(N, false);
    int i, j;

    for (i = 0; i < N-M; ++i) {
        a.fill(true, i, i + M);
        for (j = 0; j < N; ++j) {
            if (j >= i && j < i + M) {
                QVERIFY(a.at(j));
            } else {
                QVERIFY(!a.at(j));
            }
        }
        a.fill(false, i, i + M);
    }
    for (i = 0; i < N; ++i)
        a.fill(i % 2 == 0, i, i + 1);
    for (i = 0; i < N; ++i) {
        QVERIFY(a.at(i) == (i % 2 == 0));
    }
}

void tst_QBitArray::toggleBit_data()
{
    QTest::addColumn<int>("index");
    QTest::addColumn<QBitArray>("input");
    QTest::addColumn<QBitArray>("res");
    // 8 bits, toggle first bit
    QTest::newRow( "data0" )  << 0 << QStringToQBitArray(QString("11111111")) << QStringToQBitArray(QString("01111111"));
    // 8 bits
    QTest::newRow( "data1" )  << 1 << QStringToQBitArray(QString("11111111")) << QStringToQBitArray(QString("10111111"));
    // 11 bits, toggle last bit
    QTest::newRow( "data2" )  << 10 << QStringToQBitArray(QString("11111111111")) << QStringToQBitArray(QString("11111111110"));

}

void tst_QBitArray::toggleBit()
{
    QFETCH(int,index);
    QFETCH(QBitArray, input);
    QFETCH(QBitArray, res);

    input.toggleBit(index);

    QCOMPARE(input, res);
}

void tst_QBitArray::operator_andeq_data()
{
    QTest::addColumn<QBitArray>("input1");
    QTest::addColumn<QBitArray>("input2");
    QTest::addColumn<QBitArray>("res");

    QTest::newRow( "data0" )   << QStringToQBitArray(QString("11111111"))
                            << QStringToQBitArray(QString("00101100"))
                            << QStringToQBitArray(QString("00101100"));


    QTest::newRow( "data1" )   << QStringToQBitArray(QString("11011011"))
                            << QStringToQBitArray(QString("00101100"))
                            << QStringToQBitArray(QString("00001000"));

    QTest::newRow( "data2" )   << QStringToQBitArray(QString("11011011111"))
                            << QStringToQBitArray(QString("00101100"))
                            << QStringToQBitArray(QString("00001000000"));

    QTest::newRow( "data3" )   << QStringToQBitArray(QString("11011011"))
                            << QStringToQBitArray(QString("00101100111"))
                            << QStringToQBitArray(QString("00001000000"));

    QTest::newRow( "data4" )   << QStringToQBitArray(QString())
                            << QStringToQBitArray(QString("00101100111"))
                            << QStringToQBitArray(QString("00000000000"));

    QTest::newRow( "data5" ) << QStringToQBitArray(QString("00101100111"))
                             << QStringToQBitArray(QString())
                             << QStringToQBitArray(QString("00000000000"));

    QTest::newRow( "data6" ) << QStringToQBitArray(QString())
                             << QStringToQBitArray(QString())
                             << QStringToQBitArray(QString());
}

void tst_QBitArray::operator_andeq()
{
    QFETCH(QBitArray, input1);
    QFETCH(QBitArray, input2);
    QFETCH(QBitArray, res);

    QBitArray result = input1;
    result &= input2;
    QCOMPARE(result, res);
    result = input1;
    result &= std::move(input2);
    QCOMPARE(result, res);
    result = input1;
    result &= detached(input2);
    QCOMPARE(result, res);

    // operation is commutative
    result = input2;
    result &= input1;
    QCOMPARE(result, res);
    result = input2;
    result &= std::move(input1);
    QCOMPARE(result, res);
    result = input2;
    result &= detached(input1);
    QCOMPARE(result, res);

    // operation is idempotent
    result &= result;
    QCOMPARE(result, res);
    result &= std::move(result);
    QCOMPARE(result, res);
    result &= detached(result);
    QCOMPARE(result, res);
}

void tst_QBitArray::operator_and()
{
    QFETCH(QBitArray, input1);
    QFETCH(QBitArray, input2);
    QFETCH(QBitArray, res);

    QBitArray result = input1 & input2;
    QCOMPARE(result, res);
    result = input1 & QBitArray(input2);
    QCOMPARE(result, res);
    result = input1 & detached(input2);
    QCOMPARE(result, res);

    // operation is commutative
    result = input2 & input1;
    QCOMPARE(result, res);
    result = input2 & QBitArray(input1);
    QCOMPARE(result, res);
    result = input2 & detached(input1);
    QCOMPARE(result, res);

    // operation is idempotent
    result = result & result;
    QCOMPARE(result, res);
    result = result & QBitArray(result);
    QCOMPARE(result, res);
    result = result & detached(result);
    QCOMPARE(result, res);
}

void tst_QBitArray::operator_oreq_data()
{
    QTest::addColumn<QBitArray>("input1");
    QTest::addColumn<QBitArray>("input2");
    QTest::addColumn<QBitArray>("res");

    QTest::newRow( "data0" )   << QStringToQBitArray(QString("11111111"))
                            << QStringToQBitArray(QString("00101100"))
                            << QStringToQBitArray(QString("11111111"));


    QTest::newRow( "data1" )   << QStringToQBitArray(QString("11011011"))
                            << QStringToQBitArray(QString("00101100"))
                            << QStringToQBitArray(QString("11111111"));

    QTest::newRow( "data2" )   << QStringToQBitArray(QString("01000010"))
                            << QStringToQBitArray(QString("10100001"))
                            << QStringToQBitArray(QString("11100011"));

    QTest::newRow( "data3" )   << QStringToQBitArray(QString("11011011"))
                            << QStringToQBitArray(QString("00101100000"))
                            << QStringToQBitArray(QString("11111111000"));

    QTest::newRow( "data4" )   << QStringToQBitArray(QString("11011011111"))
                            << QStringToQBitArray(QString("00101100"))
                            << QStringToQBitArray(QString("11111111111"));

    QTest::newRow( "data5" )   << QStringToQBitArray(QString())
                            << QStringToQBitArray(QString("00101100111"))
                            << QStringToQBitArray(QString("00101100111"));

    QTest::newRow( "data6" ) << QStringToQBitArray(QString("00101100111"))
                             << QStringToQBitArray(QString())
                             << QStringToQBitArray(QString("00101100111"));

    QTest::newRow( "data7" ) << QStringToQBitArray(QString())
                             << QStringToQBitArray(QString())
                             << QStringToQBitArray(QString());
}

void tst_QBitArray::operator_oreq()
{
    QFETCH(QBitArray, input1);
    QFETCH(QBitArray, input2);
    QFETCH(QBitArray, res);

    QBitArray result = input1;
    result |= input2;
    QCOMPARE(result, res);
    result = input1;
    result |= QBitArray(input2);
    QCOMPARE(result, res);
    result = input1;
    result |= detached(input2);
    QCOMPARE(result, res);

    // operation is commutative
    result = input2;
    result |= input1;
    QCOMPARE(result, res);
    result = input2;
    result |= QBitArray(input1);
    QCOMPARE(result, res);
    result = input2;
    result |= detached(input1);
    QCOMPARE(result, res);

    // operation is idempotent
    result |= result;
    QCOMPARE(result, res);
    result |= QBitArray(result);
    QCOMPARE(result, res);
    result |= detached(result);
    QCOMPARE(result, res);
}

void tst_QBitArray::operator_or()
{
    QFETCH(QBitArray, input1);
    QFETCH(QBitArray, input2);
    QFETCH(QBitArray, res);

    QBitArray result = input1 | input2;
    QCOMPARE(result, res);
    result = input1 | QBitArray(input2);
    QCOMPARE(result, res);
    result = input1 | detached(input2);
    QCOMPARE(result, res);

    // operation is commutative
    result = input2 | input1;
    QCOMPARE(result, res);
    result = input2 | QBitArray(input1);
    QCOMPARE(result, res);
    result = input2 | detached(input1);
    QCOMPARE(result, res);

    // operation is idempotent
    result = result | result;
    QCOMPARE(result, res);
    result = result | QBitArray(result);
    QCOMPARE(result, res);
    result = result | detached(result);
    QCOMPARE(result, res);
}

void tst_QBitArray::operator_xoreq_data()
{
    QTest::addColumn<QBitArray>("input1");
    QTest::addColumn<QBitArray>("input2");
    QTest::addColumn<QBitArray>("res");
    QTest::newRow( "data0" )   << QStringToQBitArray(QString("11111111"))
                            << QStringToQBitArray(QString("00101100"))
                            << QStringToQBitArray(QString("11010011"));

    QTest::newRow( "data1" )   << QStringToQBitArray(QString("11011011"))
                            << QStringToQBitArray(QString("00101100"))
                            << QStringToQBitArray(QString("11110111"));

    QTest::newRow( "data2" )   << QStringToQBitArray(QString("01000010"))
                            << QStringToQBitArray(QString("10100001"))
                            << QStringToQBitArray(QString("11100011"));

    QTest::newRow( "data3" )   << QStringToQBitArray(QString("01000010"))
                            << QStringToQBitArray(QString("10100001101"))
                            << QStringToQBitArray(QString("11100011101"));

    QTest::newRow( "data4" )   << QStringToQBitArray(QString("01000010111"))
                            << QStringToQBitArray(QString("101000011"))
                            << QStringToQBitArray(QString("11100011011"));

    QTest::newRow( "data5" )   << QStringToQBitArray(QString())
                            << QStringToQBitArray(QString("00101100111"))
                            << QStringToQBitArray(QString("00101100111"));

    QTest::newRow( "data6" ) << QStringToQBitArray(QString("00101100111"))
                             << QStringToQBitArray(QString())
                             << QStringToQBitArray(QString("00101100111"));

    QTest::newRow( "data7" ) << QStringToQBitArray(QString())
                             << QStringToQBitArray(QString())
                             << QStringToQBitArray(QString());
}

void tst_QBitArray::operator_xoreq()
{
    QFETCH(QBitArray, input1);
    QFETCH(QBitArray, input2);
    QFETCH(QBitArray, res);

    QBitArray result = input1;
    result ^= input2;
    QCOMPARE(result, res);
    result = input1;
    result ^= QBitArray(input2);
    QCOMPARE(result, res);
    result = input1;
    result ^= detached(input2);
    QCOMPARE(result, res);

    // operation is commutative
    result = input2;
    result ^= input1;
    QCOMPARE(result, res);
    result = input2;
    result ^= QBitArray(input1);
    QCOMPARE(result, res);
    result = input2;
    result ^= detached(input1);
    QCOMPARE(result, res);

    // XORing with oneself is nilpotent
    result = input1;
    result ^= input1;
    QCOMPARE(result, QBitArray(input1.size()));
    result = input1;
    result ^= QBitArray(result);
    QCOMPARE(result, QBitArray(input1.size()));
    result = input1;
    result ^= detached(result);
    QCOMPARE(result, QBitArray(input1.size()));

    result = input2;
    result ^= input2;
    QCOMPARE(result, QBitArray(input2.size()));
    result = input2;
    result ^= QBitArray(input2);
    QCOMPARE(result, QBitArray(input2.size()));
    result = input2;
    result ^= detached(input2);
    QCOMPARE(result, QBitArray(input2.size()));

    result = res;
    result ^= res;
    QCOMPARE(result, QBitArray(res.size()));
    result = res;
    result ^= QBitArray(res);
    QCOMPARE(result, QBitArray(res.size()));
    result = res;
    result ^= detached(res);
    QCOMPARE(result, QBitArray(res.size()));
}

void tst_QBitArray::operator_xor()
{
    QFETCH(QBitArray, input1);
    QFETCH(QBitArray, input2);
    QFETCH(QBitArray, res);

    QBitArray result = input1 ^ input2;
    QCOMPARE(result, res);
    result = input1 ^ QBitArray(input2);
    QCOMPARE(result, res);
    result = input1 ^ detached(input2);
    QCOMPARE(result, res);

    // operation is commutative
    result = input2 ^ input1;
    QCOMPARE(result, res);
    result = input2 ^ QBitArray(input1);
    QCOMPARE(result, res);
    result = input2 ^ detached(input1);
    QCOMPARE(result, res);

    // XORing with oneself is nilpotent
    result = input1 ^ input1;
    QCOMPARE(result, QBitArray(input1.size()));
    result = input1 ^ QBitArray(input1);
    QCOMPARE(result, QBitArray(input1.size()));
    result = input1 ^ detached(input1);
    QCOMPARE(result, QBitArray(input1.size()));

    result = input2 ^ input2;
    QCOMPARE(result, QBitArray(input2.size()));
    result = input2 ^ QBitArray(input2);
    QCOMPARE(result, QBitArray(input2.size()));
    result = input2 ^ detached(input2);
    QCOMPARE(result, QBitArray(input2.size()));

    result = res ^ res;
    QCOMPARE(result, QBitArray(res.size()));
    result = res ^ QBitArray(res);
    QCOMPARE(result, QBitArray(res.size()));
    result = res ^ detached(res);
    QCOMPARE(result, QBitArray(res.size()));
}

void tst_QBitArray::operator_neg_data()
{
    QTest::addColumn<QBitArray>("input");
    QTest::addColumn<QBitArray>("res");

    QTest::newRow( "data0" )   << QStringToQBitArray(QString("11111111"))
                               << QStringToQBitArray(QString("00000000"));

    QTest::newRow( "data1" )   << QStringToQBitArray(QString("11011011"))
                               << QStringToQBitArray(QString("00100100"));

    QTest::newRow( "data2" )   << QStringToQBitArray(QString("00000000"))
                               << QStringToQBitArray(QString("11111111"));

    QTest::newRow( "data3" )   << QStringToQBitArray(QString())
                               << QStringToQBitArray(QString());

    QTest::newRow( "data4" )   << QStringToQBitArray("1")
                               << QStringToQBitArray("0");

    QTest::newRow( "data5" )   << QStringToQBitArray("0")
                               << QStringToQBitArray("1");

    QTest::newRow( "data6" )   << QStringToQBitArray("01")
                               << QStringToQBitArray("10");

    QTest::newRow( "data7" )   << QStringToQBitArray("1110101")
                               << QStringToQBitArray("0001010");

    QTest::newRow( "data8" )   << QStringToQBitArray("01110101")
                               << QStringToQBitArray("10001010");

    QTest::newRow( "data9" )   << QStringToQBitArray("011101010")
                               << QStringToQBitArray("100010101");

    QTest::newRow( "data10" )   << QStringToQBitArray("0111010101111010")
                                << QStringToQBitArray("1000101010000101");
}

void tst_QBitArray::operator_neg()
{
    QFETCH(QBitArray, input);
    QFETCH(QBitArray, res);

    input = ~input;

    QCOMPARE(input, res);
    QCOMPARE(~~input, res);     // performs two in-place negations
}

void tst_QBitArray::datastream_data()
{
    QTest::addColumn<QString>("bitField");
    QTest::addColumn<int>("numBits");
    QTest::addColumn<int>("onBits");

    QTest::newRow("empty") << QString() << 0 << 0;
    QTest::newRow("1") << QString("1") << 1 << 1;
    QTest::newRow("101") << QString("101") << 3 << 2;
    QTest::newRow("101100001") << QString("101100001") << 9 << 4;
    QTest::newRow("101100001101100001") << QString("101100001101100001") << 18 << 8;
    QTest::newRow("101100001101100001101100001101100001") << QString("101100001101100001101100001101100001") << 36 << 16;
    QTest::newRow("00000000000000000000000000000000000") << QString("00000000000000000000000000000000000") << 35 << 0;
    QTest::newRow("11111111111111111111111111111111111") << QString("11111111111111111111111111111111111") << 35 << 35;
    QTest::newRow("11111111111111111111111111111111") << QString("11111111111111111111111111111111") << 32 << 32;
    QTest::newRow("11111111111111111111111111111111111111111111111111111111")
        << QString("11111111111111111111111111111111111111111111111111111111") << 56 << 56;
    QTest::newRow("00000000000000000000000000000000") << QString("00000000000000000000000000000000") << 32 << 0;
    QTest::newRow("00000000000000000000000000000000000000000000000000000000")
        << QString("00000000000000000000000000000000000000000000000000000000") << 56 << 0;
}

void tst_QBitArray::datastream()
{
    QFETCH(QString, bitField);
    QFETCH(int, numBits);
    QFETCH(int, onBits);

    QBuffer buffer;
    QVERIFY(buffer.open(QBuffer::ReadWrite));
    QDataStream stream(&buffer);

    QBitArray bits(bitField.size());
    for (int i = 0; i < bitField.size(); ++i) {
        if (bitField.at(i) == QLatin1Char('1'))
            bits.setBit(i);
    }

    QCOMPARE(bits.size(), numBits);
    QCOMPARE(bits.count(true), onBits);
    QCOMPARE(bits.count(false), numBits - onBits);

    stream << bits << bits << bits;
    buffer.close();

    QCOMPARE(stream.status(), QDataStream::Ok);

    QVERIFY(buffer.open(QBuffer::ReadWrite));
    QDataStream stream2(&buffer);

    QBitArray array1, array2, array3;
    stream2 >> array1 >> array2 >> array3;

    QCOMPARE(array1.size(), numBits);
    QCOMPARE(array1.count(true), onBits);
    QCOMPARE(array1.count(false), numBits - onBits);

    QCOMPARE(array1, bits);
    QCOMPARE(array2, bits);
    QCOMPARE(array3, bits);
}

void tst_QBitArray::invertOnNull() const
{
    QBitArray a;
    QCOMPARE(a = ~a, QBitArray());
}

void tst_QBitArray::operator_noteq_data()
{
    QTest::addColumn<QBitArray>("input1");
    QTest::addColumn<QBitArray>("input2");
    QTest::addColumn<bool>("res");

    QTest::newRow("data0") << QStringToQBitArray(QString("11111111"))
                           << QStringToQBitArray(QString("00101100"))
                           << true;

    QTest::newRow("data1") << QStringToQBitArray(QString("11011011"))
                           << QStringToQBitArray(QString("11011011"))
                           << false;

    QTest::newRow("data2") << QStringToQBitArray(QString())
                           << QStringToQBitArray(QString("00101100111"))
                           << true;

    QTest::newRow("data3") << QStringToQBitArray(QString())
                           << QStringToQBitArray(QString())
                           << false;

    QTest::newRow("data4") << QStringToQBitArray(QString("00101100"))
                           << QStringToQBitArray(QString("11111111"))
                           << true;

    QTest::newRow("data5") << QStringToQBitArray(QString("00101100111"))
                           << QStringToQBitArray(QString())
                           << true;
}

void tst_QBitArray::operator_noteq()
{
    QFETCH(QBitArray, input1);
    QFETCH(QBitArray, input2);
    QFETCH(bool, res);

    bool b = input1 != input2;
    QCOMPARE(b, res);
}

void tst_QBitArray::resize()
{
    // -- check that a resize handles the bits correctly
    QBitArray a = QStringToQBitArray(QString("11"));
    a.resize(10);
    QVERIFY(a.size() == 10);
    QCOMPARE( a, QStringToQBitArray(QString("1100000000")) );

    a.setBit(9);
    a.resize(9);
    // now the bit in a should have been gone:
    QCOMPARE( a, QStringToQBitArray(QString("110000000")) );

    // grow the array back and check the new bit
    a.resize(10);
    QCOMPARE( a, QStringToQBitArray(QString("1100000000")) );

    // other test with and
    a.resize(9);
    QBitArray b = QStringToQBitArray(QString("1111111111"));
    b &= a;
    QCOMPARE( b, QStringToQBitArray(QString("1100000000")) );

}

void tst_QBitArray::fromBits_data()
{
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<int>("size");
    QTest::addColumn<QBitArray>("expected");

    QTest::newRow("empty") << QByteArray() << 0 << QBitArray();

    auto add = [](const QByteArray &tag, const char *data) {
        QTest::newRow(tag) << QByteArray(data, (tag.size() + 7) / 8) << tag.size()
                           << QStringToQBitArray(tag);
    };

    // "0" to "0000000000000000"
    for (int i = 1; i < 16; ++i) {
        char zero[2] = { 0, 0 };
        QByteArray pattern(i, '0');
        add(pattern, zero);
    }

    // "1" to "1111111111111111"
    for (int i = 1; i < 16; ++i) {
        char one[2] = { '\xff', '\xff' };
        QByteArray pattern(i, '1');
        add(pattern, one);
    }

    // trailing 0 and 1
    char zero = 1;
    char one = 0;
    QByteArray pzero = "1";
    QByteArray pone = "0";
    for (int i = 2; i < 8; ++i) {
        zero <<= 1;
        pzero.prepend('0');
        add(pzero, &zero);

        one = (one << 1) | 1;
        pone.prepend('1');
        add(pone, &one);
    }
}

void tst_QBitArray::fromBits()
{
    QFETCH(QByteArray, data);
    QFETCH(int, size);
    QFETCH(QBitArray, expected);

    QBitArray fromBits = QBitArray::fromBits(data, size);
    QCOMPARE(fromBits, expected);

    QCOMPARE(QBitArray::fromBits(fromBits.bits(), fromBits.size()), expected);
}

void tst_QBitArray::toUInt32_data()
{
    QTest::addColumn<QBitArray>("data");
    QTest::addColumn<int>("endianness");
    QTest::addColumn<bool>("check");
    QTest::addColumn<quint32>("result");

    QTest::newRow("ctor")           << QBitArray()
                                    << static_cast<int>(QSysInfo::Endian::LittleEndian)
                                    << true
                                    << quint32(0);

    QTest::newRow("empty")          << QBitArray(0)
                                    << static_cast<int>(QSysInfo::Endian::LittleEndian)
                                    << true
                                    << quint32(0);

    QTest::newRow("LittleEndian4")  << QStringToQBitArray(QString("0111"))
                                    << static_cast<int>(QSysInfo::Endian::LittleEndian)
                                    << true
                                    << quint32(14);

    QTest::newRow("BigEndian4")     << QStringToQBitArray(QString("0111"))
                                    << static_cast<int>(QSysInfo::Endian::BigEndian)
                                    << true
                                    << quint32(7);

    QTest::newRow("LittleEndian8")  << QStringToQBitArray(QString("01111111"))
                                    << static_cast<int>(QSysInfo::Endian::LittleEndian)
                                    << true
                                    << quint32(254);

    QTest::newRow("BigEndian8")     << QStringToQBitArray(QString("01111111"))
                                    << static_cast<int>(QSysInfo::Endian::BigEndian)
                                    << true
                                    << quint32(127);

    QTest::newRow("LittleEndian16") << QStringToQBitArray(QString("0111111111111111"))
                                    << static_cast<int>(QSysInfo::Endian::LittleEndian)
                                    << true
                                    << quint32(65534);

    QTest::newRow("BigEndian16")    << QStringToQBitArray(QString("0111111111111111"))
                                    << static_cast<int>(QSysInfo::Endian::BigEndian)
                                    << true
                                    << quint32(32767);

    QTest::newRow("LittleEndian31") << QBitArray(31, true)
                                    << static_cast<int>(QSysInfo::Endian::LittleEndian)
                                    << true
                                    << quint32(2147483647);

    QTest::newRow("BigEndian31")    << QBitArray(31, true)
                                    << static_cast<int>(QSysInfo::Endian::BigEndian)
                                    << true
                                    << quint32(2147483647);

    QTest::newRow("LittleEndian32") << QBitArray(32, true)
                                    << static_cast<int>(QSysInfo::Endian::LittleEndian)
                                    << true
                                    << quint32(4294967295);

    QTest::newRow("BigEndian32")    << QBitArray(32, true)
                                    << static_cast<int>(QSysInfo::Endian::BigEndian)
                                    << true
                                    << quint32(4294967295);

    QTest::newRow("LittleEndian33") << QBitArray(33, true)
                                    << static_cast<int>(QSysInfo::Endian::LittleEndian)
                                    << false
                                    << quint32(0);

    QTest::newRow("BigEndian33")    << QBitArray(33, true)
                                    << static_cast<int>(QSysInfo::Endian::BigEndian)
                                    << false
                                    << quint32(0);
}

void tst_QBitArray::toUInt32()
{
    QFETCH(QBitArray, data);
    QFETCH(int, endianness);
    QFETCH(bool, check);
    QFETCH(quint32, result);
    bool ok = false;

    QCOMPARE(data.toUInt32(static_cast<QSysInfo::Endian>(endianness), &ok), result);
    QCOMPARE(ok, check);
}

QTEST_APPLESS_MAIN(tst_QBitArray)
#include "tst_qbitarray.moc"
