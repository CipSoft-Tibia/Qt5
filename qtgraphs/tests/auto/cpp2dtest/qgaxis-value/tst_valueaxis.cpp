// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qtestcase.h"
#include <QtGraphs/QValueAxis>
#include <QtTest/QtTest>

class tst_valueaxis : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void construct();

    void initialProperties();
    void initializeProperties();
    void invalidProperties();

private:
    QValueAxis *m_axis;
};

void tst_valueaxis::initTestCase() {}

void tst_valueaxis::cleanupTestCase() {}

void tst_valueaxis::init()
{
    m_axis = new QValueAxis();
}

void tst_valueaxis::cleanup()
{
    delete m_axis;
}

void tst_valueaxis::construct()
{
    QValueAxis *axis = new QValueAxis();
    QVERIFY(axis);
    delete axis;
}

void tst_valueaxis::initialProperties()
{
    QVERIFY(m_axis);

    QCOMPARE(m_axis->min(), 0);
    QCOMPARE(m_axis->max(), 10);
    QCOMPARE(m_axis->labelFormat(), "");
    QCOMPARE(m_axis->labelDecimals(), -1);
    QCOMPARE(m_axis->minorTickCount(), 0);
    QCOMPARE(m_axis->tickAnchor(), 0.0);
    QCOMPARE(m_axis->tickInterval(), 0.0);
}

void tst_valueaxis::initializeProperties()
{
    QVERIFY(m_axis);

    m_axis->setMin(5);
    m_axis->setMax(100);
    m_axis->setLabelFormat("d");
    m_axis->setLabelDecimals(2);
    m_axis->setMinorTickCount(2);
    m_axis->setTickAnchor(0.5);
    m_axis->setTickInterval(0.5);

    QCOMPARE(m_axis->min(), 5);
    QCOMPARE(m_axis->max(), 100);
    QCOMPARE(m_axis->labelFormat(), "d");
    QCOMPARE(m_axis->labelDecimals(), 2);
    QCOMPARE(m_axis->minorTickCount(), 2);
    QCOMPARE(m_axis->tickAnchor(), 0.5);
    QCOMPARE(m_axis->tickInterval(), 0.5);
}

void tst_valueaxis::invalidProperties()
{
    QVERIFY(m_axis);

    m_axis->setMin(100);
    m_axis->setMax(0);
    m_axis->setMinorTickCount(-1);

    QCOMPARE(m_axis->min(), 0);
    QCOMPARE(m_axis->max(), 0);
    QCOMPARE(m_axis->minorTickCount(), 0);
}

QTEST_MAIN(tst_valueaxis)
#include "tst_valueaxis.moc"
