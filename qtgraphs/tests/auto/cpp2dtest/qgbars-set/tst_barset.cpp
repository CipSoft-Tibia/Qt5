// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGraphs/QBarSet>
#include <QtTest/QtTest>

class tst_barset : public QObject
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

    void selectDeselectSum();
    void appendInsertRemove();
    void replaceAt();

private:
    QBarSet *m_set;
};

void tst_barset::initTestCase() {}

void tst_barset::cleanupTestCase() {}

void tst_barset::init()
{
    m_set = new QBarSet();
}

void tst_barset::cleanup()
{
    delete m_set;
}

void tst_barset::construct()
{
    QBarSet *set = new QBarSet();
    QVERIFY(set);
    delete set;
}

void tst_barset::initialProperties()
{
    QVERIFY(m_set);

    QCOMPARE(m_set->label(), "");
    QCOMPARE(m_set->color(), QColor(Qt::transparent));
    QCOMPARE(m_set->borderColor(), QColor(Qt::transparent));
    QCOMPARE(m_set->labelColor(), QColor(Qt::transparent));
    QCOMPARE(m_set->values(), {});
    QCOMPARE(m_set->borderWidth(), -1);
    QCOMPARE(m_set->count(), 0);
    QCOMPARE(m_set->selectedBars(), {});
}

void tst_barset::initializeProperties()
{
    QVERIFY(m_set);

    QVariantList vals = {1, 2, 3};

    m_set->setLabel("BarSet");
    m_set->setColor("#ff0000");
    m_set->setBorderColor("00ff00");
    m_set->setLabelColor("#0000ff");
    m_set->setValues(vals);
    m_set->setBorderWidth(2);

    QCOMPARE(m_set->label(), "BarSet");
    QCOMPARE(m_set->color(), "#ff0000");
    QCOMPARE(m_set->borderColor(), "00ff00");
    QCOMPARE(m_set->labelColor(), "#0000ff");
    QCOMPARE(m_set->values(), vals);
    QCOMPARE(m_set->borderWidth(), 2);
    QCOMPARE(m_set->count(), 3);
}

void tst_barset::selectDeselectSum()
{
    QVERIFY(m_set);

    QVariantList vals = {10, 20, 30};
    QList<int> selectedsome = {0, 2};
    QList<int> selectedall = {0, 1, 2};

    m_set->setValues(vals);

    QCOMPARE(m_set->sum(), 60);

    // Select one
    m_set->selectBar(1);

    QCOMPARE(m_set->selectedBars(), {1});
    QCOMPARE(m_set->isBarSelected(1), true);

    // Toggle selection of one
    m_set->setBarSelected(1, false);

    QCOMPARE(m_set->selectedBars(), {});
    QCOMPARE(m_set->isBarSelected(1), false);

    // Toggle selection of one again
    m_set->setBarSelected(1, true);

    QCOMPARE(m_set->selectedBars(), {1});

    m_set->deselectAllBars();

    // Select two
    m_set->selectBars(selectedsome);

    QCOMPARE(m_set->selectedBars().size(), selectedsome.size());
    for (int i = 0; i < selectedsome.size(); i++) {
        QCOMPARE(m_set->selectedBars().contains(selectedsome[i]), true);
    }

    // Select all
    m_set->selectAllBars();

    QCOMPARE(m_set->selectedBars().size(), selectedall.size());
    for (int i = 0; i < selectedsome.size(); i++) {
        QCOMPARE(m_set->selectedBars().contains(selectedall[i]), true);
    }

    // Deselect one
    m_set->deselectBar(1);

    QCOMPARE(m_set->selectedBars().size(), selectedsome.size());
    for (int i = 0; i < selectedsome.size(); i++) {
        QCOMPARE(m_set->selectedBars().contains(selectedsome[i]), true);
    }

    // Deselect all
    m_set->deselectAllBars();

    QCOMPARE(m_set->selectedBars(), {});

}

void tst_barset::appendInsertRemove()
{
    QVERIFY(m_set);

    QList<qreal> nums = {10, 20, 30};
    QList<qreal> morenums = {11, 21, 31};
    QVariantList vals = {10, 20, 30};
    QVariantList morevals = {11, 21, 31};
    QVariantList allvals = {10, 20, 30, 11, 21, 31};
    QVariantList mixedvals = {10, 11, 20, 21, 30, 31};

    // Append 3
    for (int i = 0; i < nums.count(); ++i)
        m_set->append(nums[i]);

    QCOMPARE(m_set->values(), vals);

    // Append 3 more
    for (int i = 0; i < morenums.count(); ++i)
        m_set->append(morenums[i]);

    QCOMPARE(m_set->values(), allvals);

    // Remove the first 3 one by one
    for (int i = 2; i >= 0; --i)
        m_set->remove(i);

    QCOMPARE(m_set->values(), morevals);

    // Insert them in between
    m_set->insert(0, nums[0]); // -> 10, 11, 21, 31
    m_set->insert(2, nums[1]); // -> 10, 11, 20, 21, 31
    m_set->insert(4, nums[2]); // -> 10, 11, 20, 21, 30, 31

    QCOMPARE(m_set->values(), mixedvals);
}

void tst_barset::replaceAt()
{
    QVERIFY(m_set);

    QList<qreal> nums = {10, 20, 30};
    QList<qreal> morenums = {11, 21, 31};

    m_set->append(nums);

    for (int i = 0; i < m_set->count(); ++i)
        QCOMPARE(m_set->at(i), nums[i]);

    for (int i = 0; i < m_set->count(); ++i)
        m_set->replace(i, morenums[i]);

    for (int i = 0; i < m_set->count(); ++i)
        QCOMPARE(m_set->at(i), morenums[i]);
}

QTEST_MAIN(tst_barset)
#include "tst_barset.moc"
