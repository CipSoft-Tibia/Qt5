// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QSet>
#include <QTest>

class tst_QSet : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void intersect_int_data();
    void intersect_int();
    void intersect_complexType_data();
    void intersect_complexType();
    void unite_int_data();
    void unite_int();
    void contains_then_insert_int_data();
    void contains_then_insert_int();
};

void tst_QSet::initTestCase()
{
    QHashSeed::setDeterministicGlobalSeed();
}

void tst_QSet::intersect_int_data()
{
    QTest::addColumn<int>("lhsSize");
    QTest::addColumn<int>("rhsSize");
    QTest::addColumn<int>("intersectSize");

    QTest::newRow("1000000.intersect(1000) = empty") << 1000000 << 1000 << 0;
    QTest::newRow("1000.intersect(1000000) = empty") << 1000 << 1000000 << 0;
    QTest::newRow("1000000.intersect(1000) = 500") << 1000000 << 1000 << 500;
    QTest::newRow("1000.intersect(1000000) = 500") << 1000 << 1000000 << 500;
    QTest::newRow("1000000.intersect(1000) = 1000") << 1000000 << 1000 << 1000;
    QTest::newRow("1000.intersect(1000000) = 1000") << 1000 << 1000000 << 1000;
}

void tst_QSet::intersect_int()
{
    QFETCH(int, lhsSize);
    QFETCH(int, rhsSize);
    QFETCH(int, intersectSize);

    // E.g. when lhsSize = 1000, rhsSize = 1000000 and intersectSize = 500:
    // lhsSize = { 0, 1, ... 1000 }
    // rhsSize = { 500, 501, ... 1000500 }

    QSet<int> lhs;
    for (int i = 0; i < lhsSize; ++i)
        lhs.insert(i);

    QSet<int> rhs;
    const int start = lhsSize - intersectSize;
    for (int i = start; i < start + rhsSize; ++i)
        rhs.insert(i);

    QBENCHMARK {
        lhs.intersect(rhs);
    }

    QVERIFY(lhs.size() == intersectSize);
}

struct ComplexType
{
    ComplexType(int a) : a(a) {}
    int a;
    int b;
    int c;
};

inline uint qHash(const ComplexType &key, uint seed = 0)
{
    return uint(key.a) ^ seed;
}

inline bool operator==(const ComplexType &lhs, const ComplexType &rhs)
{
    return lhs.a == rhs.a;
}

void tst_QSet::intersect_complexType_data()
{
    intersect_int_data();
}

void tst_QSet::intersect_complexType()
{
    QFETCH(int, lhsSize);
    QFETCH(int, rhsSize);
    QFETCH(int, intersectSize);

    QSet<ComplexType> lhs;
    for (int i = 0; i < lhsSize; ++i)
        lhs.insert(ComplexType(i));

    QSet<ComplexType> rhs;
    const int start = lhsSize - intersectSize;
    for (int i = start; i < start + rhsSize; ++i)
        rhs.insert(ComplexType(i));

    QBENCHMARK {
        lhs.intersect(rhs);
    }
}

void tst_QSet::unite_int_data()
{
    QTest::addColumn<int>("lhsSize");
    QTest::addColumn<int>("rhsSize");
    QTest::addColumn<int>("overlap");

    QTest::newRow("1000000.unite(1000) - 0 overlap") << 1000000 << 1000 << 0;
    QTest::newRow("1000000.unite(1000) - 100 overlap") << 1000000 << 1000 << 100;
    QTest::newRow("1000000.unite(1000) - 1000 overlap") << 1000000 << 1000 << 1000;
    QTest::newRow("1000.unite(1000000) - 0 overlap") << 1000 << 1000000 << 0;
    QTest::newRow("1000.unite(1000000) - 100 overlap") << 1000 << 1000000 << 100;
    QTest::newRow("1000.unite(1000000) - 1000 overlap") << 1000 << 1000000 << 1000;
}

auto build_sets(int lhsSize, int rhsSize, int overlap)
{
    QSet<int> lhs;
    lhs.reserve(lhsSize);
    for (int i = 0; i < lhsSize; ++i)
        lhs.insert(i);

    QSet<int> rhs;
    rhs.reserve(rhsSize);
    for (int i = lhsSize - overlap; i < rhsSize + lhsSize - overlap; ++i)
        rhs.insert(i);

    return std::make_pair(lhs, rhs);
}

void tst_QSet::unite_int()
{
    QFETCH(int, lhsSize);
    QFETCH(int, rhsSize);
    QFETCH(int, overlap);

    auto [lhs, rhs] = build_sets(lhsSize, rhsSize, overlap);

    QBENCHMARK {
        QSet united = QSet(lhs).unite(rhs);
        QCOMPARE(united.size(), lhsSize + rhsSize - overlap);
    }
}

void tst_QSet::contains_then_insert_int_data()
{
    unite_int_data();
}

void tst_QSet::contains_then_insert_int()
{
    QFETCH(int, lhsSize);
    QFETCH(int, rhsSize);
    QFETCH(int, overlap);

    auto [lhs, rhs] = build_sets(lhsSize, rhsSize, overlap);

    QBENCHMARK {
        QSet copy(lhs);
        for (auto i : rhs) {
            if (!copy.contains(i))
                copy.insert(i);
        }
        QCOMPARE(copy.size(), lhsSize + rhsSize - overlap);
    }
}

QTEST_MAIN(tst_QSet)

#include "tst_bench_qset.moc"
