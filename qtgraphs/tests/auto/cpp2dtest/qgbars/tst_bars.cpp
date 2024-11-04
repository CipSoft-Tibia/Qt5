// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGraphs/QBarSeries>
#include <QtGraphs/QBarSet>
#include <QtGraphs/QValueAxis>
#include <QtGraphs/QBarCategoryAxis>
#include <QtTest/QtTest>

class tst_bars : public QObject
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
    QBarSeries *m_series;
};

void tst_bars::initTestCase() {}

void tst_bars::cleanupTestCase() {}

void tst_bars::init()
{
    m_series = new QBarSeries();
}

void tst_bars::cleanup()
{
    delete m_series;
}

void tst_bars::construct()
{
    QBarSeries *series = new QBarSeries();
    QVERIFY(series);
    delete series;
}

void tst_bars::initialProperties()
{
    QVERIFY(m_series);

    // Properties from QBarSeries
    QCOMPARE(m_series->axisX(), nullptr);
    QCOMPARE(m_series->axisY(), nullptr);

    // Properties from QAbstractBarSeries
    QCOMPARE(m_series->barWidth(), 0.5);
    QCOMPARE(m_series->count(), 0);
    QCOMPARE(m_series->isLabelsVisible(), false);
    QCOMPARE(m_series->labelsFormat(), "");
    QCOMPARE(m_series->labelsPosition(), QAbstractBarSeries::LabelsCenter);
    QCOMPARE(m_series->labelsAngle(), 0);
    QCOMPARE(m_series->labelsPrecision(), 6);

    // Properties from QAbstractSeries
    // TODO: QML API gives a theme, C++ API does not - investigate
    QCOMPARE(m_series->theme(), nullptr);
    QCOMPARE(m_series->name(), "");
    QCOMPARE(m_series->isVisible(), true);
    QCOMPARE(m_series->selectable(), false);
    QCOMPARE(m_series->hoverable(), false);
    QCOMPARE(m_series->opacity(), 1.0);
    QCOMPARE(m_series->valuesMultiplier(), 1.0);
}

void tst_bars::initializeProperties()
{
    QVERIFY(m_series);

    auto axisX = new QBarCategoryAxis(this);
    auto axisY = new QValueAxis(this);
    auto theme = new QSeriesTheme(this);
    auto set = new QBarSet(this);

    m_series->setAxisX(axisX);
    m_series->setAxisY(axisY);

    m_series->setBarWidth(0.75);
    m_series->setLabelsVisible(true);
    m_series->setLabelsFormat("i");
    m_series->setLabelsPosition(QAbstractBarSeries::LabelsInsideBase);
    m_series->setLabelsAngle(45.0);
    m_series->setLabelsPrecision(10);
    m_series->append(set);

    m_series->setTheme(theme);
    m_series->setName("BarSeries");
    m_series->setVisible(false);
    m_series->setSelectable(true);
    m_series->setHoverable(true);
    m_series->setOpacity(0.5);
    m_series->setValuesMultiplier(0.5);

    QCOMPARE(m_series->axisX(), axisX);
    QCOMPARE(m_series->axisY(), axisY);

    QCOMPARE(m_series->barWidth(), 0.75);
    QCOMPARE(m_series->count(), 1);
    QCOMPARE(m_series->isLabelsVisible(), true);
    QCOMPARE(m_series->labelsFormat(), "i");
    QCOMPARE(m_series->labelsPosition(), QAbstractBarSeries::LabelsInsideBase);
    QCOMPARE(m_series->labelsAngle(), 45.0);
    QCOMPARE(m_series->labelsPrecision(), 10);

    QCOMPARE(m_series->theme(), theme);
    QCOMPARE(m_series->name(), "BarSeries");
    QCOMPARE(m_series->isVisible(), false);
    QCOMPARE(m_series->selectable(), true);
    QCOMPARE(m_series->hoverable(), true);
    QCOMPARE(m_series->opacity(), 0.5);
    QCOMPARE(m_series->valuesMultiplier(), 0.5);
}

void tst_bars::invalidProperties()
{
    QVERIFY(m_series);

    auto axisX = new QBarCategoryAxis(this);
    auto axisY = new QValueAxis(this);

    m_series->setAxisX(axisY); // wrong axis type
    m_series->setAxisY(axisX); // wrong axis type
    m_series->setBarWidth(2.0); // range 0...1
    m_series->setValuesMultiplier(2.0); // range 0...1

    QCOMPARE(m_series->axisX(), nullptr);
    QCOMPARE(m_series->axisY(), nullptr);
    QCOMPARE(m_series->barWidth(), 1.0);
    QCOMPARE(m_series->valuesMultiplier(), 1.0);

    m_series->setBarWidth(-1.0); // range 0...1
    m_series->setValuesMultiplier(-1.0); // range 0...1

    QCOMPARE(m_series->barWidth(), 0.0);
    QCOMPARE(m_series->valuesMultiplier(), 0.0);
}

QTEST_MAIN(tst_bars)
#include "tst_bars.moc"
