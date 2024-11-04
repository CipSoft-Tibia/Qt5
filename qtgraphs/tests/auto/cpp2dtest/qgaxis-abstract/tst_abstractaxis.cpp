// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qtestcase.h"
#include <QtGraphs/QBarCategoryAxis>
#include <QtTest/QtTest>

class tst_abstractaxis : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void initialProperties();
    void initializeProperties();

    void showHide();

private:
    // QAbstractAxis is uncreatable, so testing is done through QBarCategoryAxis
    QBarCategoryAxis *m_axis;
};

void tst_abstractaxis::initTestCase() {}

void tst_abstractaxis::cleanupTestCase() {}

void tst_abstractaxis::init()
{
    m_axis = new QBarCategoryAxis();
}

void tst_abstractaxis::cleanup()
{
    delete m_axis;
}

void tst_abstractaxis::initialProperties()
{
    QVERIFY(m_axis);

    QCOMPARE(m_axis->isVisible(), true);
    QCOMPARE(m_axis->isLineVisible(), true);
    QCOMPARE(m_axis->labelsVisible(), true);
    QCOMPARE(m_axis->labelsAngle(), 0);
    QCOMPARE(m_axis->isGridLineVisible(), true);
    QCOMPARE(m_axis->isMinorGridLineVisible(), true);
    QCOMPARE(m_axis->titleText(), QString());
    QCOMPARE(m_axis->titleColor(), QColor());
    QCOMPARE(m_axis->isTitleVisible(), true);
    QCOMPARE(m_axis->titleFont(), QFont());
    QCOMPARE(m_axis->orientation(), Qt::Orientation(0));
    QCOMPARE(m_axis->alignment(), 0);
}

void tst_abstractaxis::initializeProperties()
{
    QVERIFY(m_axis);

    auto font = QFont("Arial", 20, 2, true);

    m_axis->setVisible(false);
    m_axis->setLineVisible(false);
    m_axis->setLabelsVisible(false);
    m_axis->setLabelsAngle(90.0);
    m_axis->setGridLineVisible(false);
    m_axis->setMinorGridLineVisible(false);
    m_axis->setTitleText("Title");
    m_axis->setTitleColor("#ff0000");
    m_axis->setTitleVisible(false);
    m_axis->setTitleFont(font);
    m_axis->setOrientation(Qt::Vertical);
    // no setter for alignment?

    QCOMPARE(m_axis->isVisible(), false);
    QCOMPARE(m_axis->isLineVisible(), false);
    QCOMPARE(m_axis->labelsVisible(), false);
    QCOMPARE(m_axis->labelsAngle(), 90.0);
    QCOMPARE(m_axis->isGridLineVisible(), false);
    QCOMPARE(m_axis->isMinorGridLineVisible(), false);
    QCOMPARE(m_axis->titleText(), "Title");
    QCOMPARE(m_axis->titleColor(), "#ff0000");
    QCOMPARE(m_axis->isTitleVisible(), false);
    QCOMPARE(m_axis->titleFont(), font);
    QCOMPARE(m_axis->orientation(), Qt::Vertical);
    QCOMPARE(m_axis->alignment(), 0);
}

void tst_abstractaxis::showHide()
{
    QVERIFY(m_axis);

    m_axis->hide();

    QCOMPARE(m_axis->isVisible(), false);

    m_axis->show();

    QCOMPARE(m_axis->isVisible(), true);
}

QTEST_MAIN(tst_abstractaxis)
#include "tst_abstractaxis.moc"
