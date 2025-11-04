// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtHttpServer/private/qhttpserverrequestfilter_p.h>
#include <QtHttpServer/qhttpserverconfiguration.h>

#include <QtTest/qtest.h>

QT_BEGIN_NAMESPACE

class tst_QHttpServerRequestFilter : public QObject
{
    Q_OBJECT

private slots:
    void testRateLimit();
};

void tst_QHttpServerRequestFilter::testRateLimit()
{
    using namespace QHttpServerRequestFilterPrivate;

    QHttpServerRequestFilter filter1;
    QHostAddress address("127.0.0.1");
    QCOMPARE(filter1.isRequestWithinRate(address), true);

    QHttpServerConfiguration config;
    config.setRateLimitPerSecond(0);
    filter1.setConfiguration(config);
    QCOMPARE(filter1.isRequestWithinRate(address), true);

    QHttpServerRequestFilter filter2;
    config.setRateLimitPerSecond(1);
    filter2.setConfiguration(config);
    qint64 currTimeMSec = QDateTime::currentMSecsSinceEpoch();
    QCOMPARE(filter2.isRequestWithinRate(address, currTimeMSec), true);
    QCOMPARE(filter2.isRequestWithinRate(address, currTimeMSec), false);
    // check next period
    currTimeMSec += cPeriodDurationMSec + 1;
    QCOMPARE(filter2.isRequestWithinRate(address, currTimeMSec), true);
    QCOMPARE(filter2.isRequestWithinRate(address, currTimeMSec), false);
    // check after 2 periods
    currTimeMSec += 2 * cPeriodDurationMSec + 1;
    QCOMPARE(filter2.isRequestWithinRate(address, currTimeMSec), true);
    QCOMPARE(filter2.isRequestWithinRate(address, currTimeMSec), false);
    // check after previous ip info becomes garbage
    QHostAddress address2("127.0.0.2");
    currTimeMSec += 3 * cPeriodDurationMSec;
    QCOMPARE(filter2.isRequestWithinRate(address2, currTimeMSec), true);
    QCOMPARE(filter2.isRequestWithinRate(address2, currTimeMSec), false);
    // check after this ip becomes garbage
    currTimeMSec += 3 * cPeriodDurationMSec;
    QCOMPARE(filter2.isRequestWithinRate(address2, currTimeMSec), true);
    QCOMPARE(filter2.isRequestWithinRate(address2, currTimeMSec), false);

    // higher rate limit
    QHttpServerRequestFilter filter3;
    const int nRequests = 10;
    config.setRateLimitPerSecond(nRequests);
    filter3.setConfiguration(config);
    currTimeMSec = QDateTime::currentMSecsSinceEpoch();
    QCOMPARE(filter3.isRequestWithinRate(address2, currTimeMSec), true);
    currTimeMSec += cPeriodDurationMSec;
    for (int i = 0; i < nRequests - 1; ++i)
        QCOMPARE(filter3.isRequestWithinRate(address2, currTimeMSec), true);
    QCOMPARE(filter3.isRequestWithinRate(address2, currTimeMSec), false);
    // check next period
    currTimeMSec += 1;
    QCOMPARE(filter3.isRequestWithinRate(address2, currTimeMSec), true);
    for (int i = 0; i < nRequests - 1; ++i)
        QCOMPARE(filter3.isRequestWithinRate(address2, currTimeMSec), true);
    QCOMPARE(filter3.isRequestWithinRate(address2, currTimeMSec), false);

    // more hosts
    QHttpServerRequestFilter filter4;
    config.setRateLimitPerSecond(1);
    filter4.setConfiguration(config);
    currTimeMSec = QDateTime::currentMSecsSinceEpoch();
    for (int i = 0; i < 10; ++i) {
        const auto hostAddress = QHostAddress(QString("127.0.0.%1").arg(i));
        QCOMPARE(filter4.isRequestWithinRate(hostAddress, currTimeMSec), true);
        QCOMPARE(filter4.isRequestWithinRate(hostAddress, currTimeMSec), false);
    }
    // check after all the hosts become garbage
    currTimeMSec += 3 * cPeriodDurationMSec;
    QCOMPARE(filter4.isRequestWithinRate(QHostAddress("168.0.0.1"), currTimeMSec), true);
}

QT_END_NAMESPACE

QTEST_MAIN(tst_QHttpServerRequestFilter)

#include "tst_qhttpserverrequestfilter.moc"
