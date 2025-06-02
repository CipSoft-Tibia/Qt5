// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGrpc/qgrpccalloptions.h>

#include <QtTest/qtest.h>

#include <cstring>

using namespace std::chrono_literals;

class QGrpcCallOptionsTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void hasSpecialMemberFunctions() const;
    void hasImplicitQVariant() const;
    void hasMemberSwap() const;
    void propertyMetadata() const;
    void propertyDeadline() const;
    void streamsToDebug() const;
};

void QGrpcCallOptionsTest::hasSpecialMemberFunctions() const
{
    QGrpcCallOptions o1;
    QVERIFY(!o1.deadlineTimeout());
    QVERIFY(o1.metadata().empty());

    o1.setDeadlineTimeout(100ms);

    QGrpcCallOptions o2(o1);
    QCOMPARE_EQ(o1.deadlineTimeout(), o2.deadlineTimeout());

    QGrpcCallOptions o3 = o1;
    QCOMPARE_EQ(o1.deadlineTimeout(), o3.deadlineTimeout());

    QGrpcCallOptions o4(std::move(o1));
    QCOMPARE_EQ(o4.deadlineTimeout(), o2.deadlineTimeout());

    o1 = std::move(o4);
    QCOMPARE_EQ(o1.deadlineTimeout(), o2.deadlineTimeout());
}

void QGrpcCallOptionsTest::hasImplicitQVariant() const
{
    QGrpcCallOptions o1;
    o1.setDeadlineTimeout(250ms);
    o1.setMetadata({
        { "keyA", "valA" },
        { "keyB", "valB" },
    });

    QVariant v = o1;
    QCOMPARE_EQ(v.metaType(), QMetaType::fromType<QGrpcCallOptions>());
    const auto o2 = v.value<QGrpcCallOptions>();
    QCOMPARE_EQ(o1.metadata(), o2.metadata());
    QCOMPARE_EQ(o1.deadlineTimeout(), o2.deadlineTimeout());
}

void QGrpcCallOptionsTest::hasMemberSwap() const
{
    constexpr std::chrono::milliseconds Dur = 50ms;

    QGrpcCallOptions o1;
    o1.setDeadlineTimeout(Dur);
    QGrpcCallOptions o2;

    QCOMPARE_EQ(o1.deadlineTimeout(), Dur);
    QVERIFY(!o2.deadlineTimeout());
    o2.swap(o1);
    QCOMPARE_EQ(o2.deadlineTimeout(), Dur);
    QVERIFY(!o1.deadlineTimeout());
    swap(o2, o1);
    QCOMPARE_EQ(o1.deadlineTimeout(), Dur);
    QVERIFY(!o2.deadlineTimeout());
}

void QGrpcCallOptionsTest::propertyMetadata() const
{
    QHash<QByteArray, QByteArray> md = {
        { "keyA", "valA" },
        { "keyB", "valB" },
    };

    QGrpcCallOptions o1;
    auto o1Detach = o1;
    o1.setMetadata(md);
    QCOMPARE_EQ(o1.metadata(), md);
    QCOMPARE_NE(o1.metadata(), o1Detach.metadata());

    QGrpcCallOptions o2;
    auto o2Detach = o2;
    o2.setMetadata(std::move(md));
    QCOMPARE_EQ(o2.metadata(), o1.metadata());
    QCOMPARE_NE(o2.metadata(), o2Detach.metadata());

    QCOMPARE_EQ(std::move(o1).metadata(), o2.metadata());
}

void QGrpcCallOptionsTest::propertyDeadline() const
{
    constexpr std::chrono::milliseconds Dur = 50ms;

    QGrpcCallOptions o1;
    auto o1Detach = o1;
    o1.setDeadlineTimeout(Dur);
    QCOMPARE_EQ(o1.deadlineTimeout(), Dur);
    QCOMPARE_NE(o1.deadlineTimeout(), o1Detach.deadlineTimeout());
}

void QGrpcCallOptionsTest::streamsToDebug() const
{
    QGrpcCallOptions o;
    QString storage;
    QDebug dbg(&storage);
    dbg.noquote().nospace();

    dbg << o;
    QVERIFY(!storage.isEmpty());

    std::unique_ptr<char[]> ustr(QTest::toString(o));
    QCOMPARE_EQ(storage, QString::fromUtf8(ustr.get()));
}

QTEST_MAIN(QGrpcCallOptionsTest)

#include "tst_qgrpccalloptions.moc"
