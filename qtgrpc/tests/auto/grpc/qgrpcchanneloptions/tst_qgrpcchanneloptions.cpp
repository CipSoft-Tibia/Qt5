// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGrpc/qgrpcchanneloptions.h>
#include <QtGrpc/qgrpcserializationformat.h>

#include <QtTest/qtest.h>

#include <cstring>

using namespace std::chrono_literals;

class QGrpcChannelOptionsTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void hasSpecialMemberFunctions() const;
    void hasImplicitQVariant() const;
    void hasMemberSwap() const;
    void propertyMetadata() const;
    void propertyDeadline() const;
    void propertySerializationFormat() const;
#if QT_CONFIG(ssl)
    void propertySslConfiguration() const;
#endif
    void streamsToDebug() const;
};

void QGrpcChannelOptionsTest::hasSpecialMemberFunctions() const
{
    QGrpcChannelOptions o1;
    QVERIFY(!o1.deadlineTimeout());
    QVERIFY(o1.metadata().empty());

#if QT_CONFIG(ssl)
    QVERIFY(!o1.sslConfiguration());
#endif

    o1.setDeadlineTimeout(100ms);

    QGrpcChannelOptions o2(o1);
    QCOMPARE_EQ(o1.deadlineTimeout(), o2.deadlineTimeout());

    QGrpcChannelOptions o3 = o1;
    QCOMPARE_EQ(o1.deadlineTimeout(), o3.deadlineTimeout());

    QGrpcChannelOptions o4(std::move(o1));
    QCOMPARE_EQ(o4.deadlineTimeout(), o2.deadlineTimeout());

    o1 = std::move(o4);
    QCOMPARE_EQ(o1.deadlineTimeout(), o2.deadlineTimeout());
}

void QGrpcChannelOptionsTest::hasImplicitQVariant() const
{
    QGrpcChannelOptions o1;
    o1.setDeadlineTimeout(250ms);
    o1.setMetadata({
        { "keyA", "valA" },
        { "keyB", "valB" },
    });

    QVariant v = o1;
    QCOMPARE_EQ(v.metaType(), QMetaType::fromType<QGrpcChannelOptions>());
    const auto o2 = v.value<QGrpcChannelOptions>();
    QCOMPARE_EQ(o1.metadata(), o2.metadata());
    QCOMPARE_EQ(o1.deadlineTimeout(), o2.deadlineTimeout());
}

void QGrpcChannelOptionsTest::hasMemberSwap() const
{
    constexpr std::chrono::milliseconds Dur = 50ms;

    QGrpcChannelOptions o1;
    o1.setDeadlineTimeout(Dur);
    QGrpcChannelOptions o2;

    QCOMPARE_EQ(o1.deadlineTimeout(), Dur);
    QVERIFY(!o2.deadlineTimeout());
    o2.swap(o1);
    QCOMPARE_EQ(o2.deadlineTimeout(), Dur);
    swap(o2, o1);
    QCOMPARE_EQ(o1.deadlineTimeout(), Dur);
    QVERIFY(!o2.deadlineTimeout());
}

void QGrpcChannelOptionsTest::propertyMetadata() const
{
    QHash<QByteArray, QByteArray> md = {
        { "keyA", "valA" },
        { "keyB", "valB" },
    };

    QGrpcChannelOptions o1;
    auto o1Detach = o1;
    o1.setMetadata(md);
    QCOMPARE_EQ(o1.metadata(), md);
    QCOMPARE_NE(o1.metadata(), o1Detach.metadata());

    QGrpcChannelOptions o2;
    auto o2Detach = o2;
    o2.setMetadata(std::move(md));
    QCOMPARE_EQ(o2.metadata(), o1.metadata());
    QCOMPARE_NE(o1.metadata(), o2Detach.metadata());

    QCOMPARE_EQ(std::move(o1).metadata(), o2.metadata());
}

void QGrpcChannelOptionsTest::propertyDeadline() const
{
    constexpr std::chrono::milliseconds Dur = 50ms;

    QGrpcChannelOptions o1;
    auto o1Detach = o1;
    o1.setDeadlineTimeout(Dur);
    QCOMPARE_EQ(o1.deadlineTimeout(), Dur);
    QCOMPARE_NE(o1.deadlineTimeout(), o1Detach.deadlineTimeout());
}

void QGrpcChannelOptionsTest::propertySerializationFormat() const
{
    QGrpcSerializationFormat fmt(QtGrpc::SerializationFormat::Json);

    QGrpcChannelOptions o1;
    auto o1Detach = o1;
    QVERIFY(o1.serializationFormat().suffix().isEmpty());
    o1.setSerializationFormat(fmt);
    QVERIFY(!o1.serializationFormat().suffix().isEmpty());
    QCOMPARE_EQ(o1.serializationFormat().suffix(), fmt.suffix());
    QCOMPARE_NE(o1.serializationFormat().suffix(), o1Detach.serializationFormat().suffix());
}

#if QT_CONFIG(ssl)
void QGrpcChannelOptionsTest::propertySslConfiguration() const
{
    QSslConfiguration sslConfig;
    sslConfig.setSessionTicket("test");

    QGrpcChannelOptions o1;
    auto o1Detach = o1;
    QVERIFY(!o1.sslConfiguration());
    o1.setSslConfiguration(sslConfig);
    QVERIFY(o1.sslConfiguration());
    QCOMPARE_EQ(o1.sslConfiguration()->sessionTicket(), sslConfig.sessionTicket());
    QCOMPARE_NE(o1.sslConfiguration(), o1Detach.sslConfiguration());
}
#endif

void QGrpcChannelOptionsTest::streamsToDebug() const
{
    QGrpcChannelOptions o;
    QString storage;
    QDebug dbg(&storage);
    dbg.nospace().noquote();

    dbg << o;
    QVERIFY(!storage.isEmpty());

    std::unique_ptr<char[]> ustr(QTest::toString(o));
    QCOMPARE_EQ(storage, QString::fromUtf8(ustr.get()));
}

QTEST_MAIN(QGrpcChannelOptionsTest)

#include "tst_qgrpcchanneloptions.moc"
