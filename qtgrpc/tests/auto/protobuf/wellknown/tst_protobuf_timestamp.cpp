// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "timestampmessages.qpb.h"

#include <QtProtobuf/qprotobufserializer.h>

#include <ratio>

#include <QtTest/QtTest>

class tst_protobuf_timestamp : public QObject
{
    Q_OBJECT
private slots:
    void simpleMessage();
};

using namespace MyTopLevelNamespace::qtproto::tests;
using namespace Qt::StringLiterals;

void tst_protobuf_timestamp::simpleMessage()
{
    QProtobufSerializer serializer;

    TimestampMessage ts;
    ts.setField(google::protobuf::Timestamp::fromDateTime(
            { { 1970, 01, 01 }, { 0, 0, 1 }, QTimeZone(QTimeZone::UTC) }));

    QCOMPARE_EQ(ts.field().nanos(), 0);
    QCOMPARE_EQ(ts.field().seconds(), 1);

    ts.setField(google::protobuf::Timestamp::fromDateTime(
            { { 1984, 6, 8 }, { 10, 10, 10 }, QTimeZone(QTimeZone::UTC) }));

    QCOMPARE_EQ(ts.field().nanos(), 0);
    QCOMPARE_EQ(ts.field().seconds(), 455537410);

    QDateTime now = QDateTime::currentDateTime();
    ts.setField(google::protobuf::Timestamp::fromDateTime(now));
    QCOMPARE_EQ(ts.field().toDateTime(), now);

    constexpr auto milliToNano = std::ratio_divide<std::nano, std::milli>::den;
    QCOMPARE_EQ(ts.field().nanos(), (now.toMSecsSinceEpoch() % std::milli::den) * milliToNano);
    QCOMPARE_EQ(ts.field().seconds(), now.toMSecsSinceEpoch() / std::milli::den);

    ts.setField(google::protobuf::Timestamp::fromDateTime(
            QDateTime::fromMSecsSinceEpoch(1688124178703, QTimeZone(QTimeZone::UTC))));
    QCOMPARE_EQ(ts.serialize(&serializer).toHex(), "0a0c0892f6faa40610c0db9bcf02"_ba);

    QVERIFY(ts.deserialize(&serializer, QByteArray::fromHex("0a0c08c3f7faa4061080d594d603")));
    QCOMPARE_EQ(ts.field().toDateTime(),
                QDateTime::fromMSecsSinceEpoch(1688124355986, QTimeZone(QTimeZone::UTC)));
}

QTEST_MAIN(tst_protobuf_timestamp)
#include "tst_protobuf_timestamp.moc"
