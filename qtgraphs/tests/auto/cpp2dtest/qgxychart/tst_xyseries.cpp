// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGraphs/QScatterSeries>
#include <QtTest/QtTest>

class tst_xyseries : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void initialProperties();
    void initializeProperties();

    void selectDeselect();
    void appendInsertRemove();
    void replaceAtClear();

private:
    // QXYSeries is uncreatable, so testing is done through QScatterSeries
    QScatterSeries *m_series;
};

void tst_xyseries::initTestCase() {}

void tst_xyseries::cleanupTestCase() {}

void tst_xyseries::init()
{
    m_series = new QScatterSeries();
}

void tst_xyseries::cleanup()
{
    delete m_series;
}

void tst_xyseries::initialProperties()
{
    QVERIFY(m_series);

    // Properties from QXYSeries
    QCOMPARE(m_series->color(), "#ffffff");
    QCOMPARE(m_series->selectedColor(), QColor::Invalid);
    QCOMPARE(m_series->markerSize(), 15.0);
}

void tst_xyseries::initializeProperties()
{
    QVERIFY(m_series);

    m_series->setColor("#ff0000");
    m_series->setSelectedColor("#0000ff");
    m_series->setMarkerSize(5.0);

    QCOMPARE(m_series->color(), "#ff0000");
    QCOMPARE(m_series->selectedColor(), "#0000ff");
    QCOMPARE(m_series->markerSize(), 5.0);
}

void tst_xyseries::selectDeselect()
{
    QVERIFY(m_series);

    QList<QPointF> points = {{0, 0}, {1, 1}, {2, 2}};
    QList<int> allselected = {0, 1, 2};

    m_series->append(points);

    QCOMPARE(m_series->selectedPoints(), {});

    m_series->selectAllPoints();

    QCOMPARE(m_series->selectedPoints().size(), allselected.size());
    for (int i = 0; i < allselected.size(); i++) {
        QCOMPARE(m_series->selectedPoints().contains(allselected[i]), true);
    }

    m_series->deselectAllPoints();

    QCOMPARE(m_series->selectedPoints(), {});

    m_series->selectPoints(allselected);

    QCOMPARE(m_series->selectedPoints().size(), allselected.size());
    for (int i = 0; i < allselected.size(); i++) {
        QCOMPARE(m_series->selectedPoints().contains(allselected[i]), true);
    }

    m_series->toggleSelection(allselected);

    QCOMPARE(m_series->selectedPoints(), {});
}

void tst_xyseries::appendInsertRemove()
{
    QVERIFY(m_series);

    QList<QPointF> points = {{0, 0}, {1, 1}, {2, 2}};
    QList<QPointF> morepoints = {{3, 3}, {4, 4}, {5, 5}};
    QList<QPointF> allpoints = {{0, 0}, {1, 1}, {2, 2}, {3, 3}, {4, 4}, {5, 5}};
    QList<QPointF> mixedpoints = {{0, 0}, {3, 3}, {1, 1}, {4, 4}, {2, 2}, {5, 5}};

    // Append 3
    for (int i = 0; i < points.count(); ++i)
        m_series->append(points[i]);

    QCOMPARE(m_series->points(), points);

    // Append 3 more
    m_series->append(morepoints);

    QCOMPARE(m_series->points(), allpoints);

    // Remove the first 3 one by one
    for (int i = 2; i >= 0; --i)
        m_series->remove(i);

    QCOMPARE(m_series->points(), morepoints);

    // Insert them in between
    m_series->insert(0, points[0]);
    m_series->insert(2, points[1]);
    m_series->insert(4, points[2]);

    QCOMPARE(m_series->points(), mixedpoints);

    // Remove first 3
    m_series->removePoints(0, 3);

    QCOMPARE(m_series->count(), 3);

    // Append 3 by qreals
    for (int i = 10; i < 13; ++i)
        m_series->append(i, i);

    QCOMPARE(m_series->count(), 6);

    // Remove 3 by qreals
    for (int i = 10; i < 13; ++i)
        m_series->remove(i, i);

    QCOMPARE(m_series->count(), 3);
}

void tst_xyseries::replaceAtClear()
{
    QVERIFY(m_series);

    QList<QPointF> points = {{0, 0}, {1, 1}, {2, 2}};
    QList<QPointF> morepoints = {{3, 3}, {4, 4}, {5, 5}};

    m_series->append(points);

    for (int i = 0; i < m_series->count(); ++i)
        QCOMPARE(m_series->at(i), points[i]);

    for (int i = 0; i < m_series->count(); ++i)
        m_series->replace(i, morepoints[i]);

    for (int i = 0; i < m_series->count(); ++i)
        QCOMPARE(m_series->at(i), morepoints[i]);

    m_series->clear();

    QCOMPARE(m_series->count(), 0);
}

QTEST_MAIN(tst_xyseries)
#include "tst_xyseries.moc"
