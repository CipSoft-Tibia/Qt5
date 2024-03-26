// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QtTest>
#include <QLinkedList>

struct Movable
{
    Movable(char input = 'j') : i(input), state(Constructed) { ++liveCount; }
    Movable(const Movable &other) : i(other.i), state(Constructed)
    {
        check(other.state, Constructed);
        ++liveCount;
    }

    ~Movable()
    {
        check(state, Constructed);
        i = 0;
        --liveCount;
        state = Destructed;
    }

    bool operator==(const Movable &other) const
    {
        check(state, Constructed);
        check(other.state, Constructed);
        return i == other.i;
    }

    Movable &operator=(const Movable &other)
    {
        check(state, Constructed);
        check(other.state, Constructed);
        i = other.i;
        return *this;
    }
    char i;

    static int getLiveCount() { return liveCount; }

private:
    static int liveCount;

    enum State { Constructed = 106, Destructed = 110 };
    State state;

    static void check(const State state1, const State state2)
    {
        QCOMPARE(int(state1), int(state2));
    }
};

int Movable::liveCount = 0;

QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(Movable, Q_RELOCATABLE_TYPE);
QT_END_NAMESPACE

Q_DECLARE_METATYPE(Movable);

Q_DECLARE_METATYPE(QLinkedList<int>);

size_t qHash(const Movable &movable)
{
    return qHash(movable.i);
}

struct Complex
{
    Complex(int val = 0) : value(val), checkSum(this) { ++liveCount; }

    Complex(Complex const &other) : value(other.value), checkSum(this) { ++liveCount; }

    Complex &operator=(Complex const &other)
    {
        check();
        other.check();

        value = other.value;
        return *this;
    }

    ~Complex()
    {
        --liveCount;
        check();
    }

    operator int() const { return value; }

    bool operator==(Complex const &other) const
    {
        check();
        other.check();
        return value == other.value;
    }

    void check() const { QVERIFY(this == checkSum); }

    static int getLiveCount() { return liveCount; }

private:
    static int liveCount;

    int value;
    void *checkSum;
};

int Complex::liveCount = 0;

Q_DECLARE_METATYPE(Complex);

// Tests depend on the fact that:
Q_STATIC_ASSERT(QTypeInfo<int>::isRelocatable);
Q_STATIC_ASSERT(!QTypeInfo<int>::isComplex);
Q_STATIC_ASSERT(QTypeInfo<Movable>::isRelocatable);
Q_STATIC_ASSERT(QTypeInfo<Movable>::isComplex);
Q_STATIC_ASSERT(!QTypeInfo<Complex>::isRelocatable);
Q_STATIC_ASSERT(QTypeInfo<Complex>::isComplex);

QT_BEGIN_NAMESPACE

static_assert(QTypeTraits::has_ostream_operator_v<QDataStream, QLinkedList<QString>>);
static_assert(QTypeTraits::has_istream_operator_v<QDataStream, QLinkedList<QString>>);
static_assert(!QTypeTraits::has_ostream_operator_v<QDataStream, QLinkedList<Movable>>);
static_assert(!QTypeTraits::has_istream_operator_v<QDataStream, QLinkedList<Movable>>);

class tst_QLinkedList : public QObject
{
    Q_OBJECT
private slots:
    void eraseValidIteratorsOnSharedList() const;
    void insertWithIteratorsOnSharedList() const;
    void lengthInt() const;
    void lengthMovable() const;
    void lengthComplex() const;
    void lengthSignature() const;
    void firstInt() const;
    void firstMovable() const;
    void firstComplex() const;
    void lastInt() const;
    void lastMovable() const;
    void lastComplex() const;
    void beginInt() const;
    void beginMovable() const;
    void beginComplex() const;
    void endInt() const;
    void endMovable() const;
    void endComplex() const;
    void containsInt() const;
    void containsMovable() const;
    void containsComplex() const;
    void countInt() const;
    void countMovable() const;
    void countComplex() const;
    void cpp17ctad() const;
    void emptyInt() const;
    void emptyMovable() const;
    void emptyComplex() const;
    void endsWithInt() const;
    void endsWithMovable() const;
    void endsWithComplex() const;
    void removeAllInt() const;
    void removeAllMovable() const;
    void removeAllComplex() const;
    void removeOneInt() const;
    void removeOneMovable() const;
    void removeOneComplex() const;
    void reverseIterators() const;
    void startsWithInt() const;
    void startsWithMovable() const;
    void startsWithComplex() const;
    void takeFirstInt() const;
    void takeFirstMovable() const;
    void takeFirstComplex() const;
    void takeLastInt() const;
    void takeLastMovable() const;
    void takeLastComplex() const;
    void toStdListInt() const;
    void toStdListMovable() const;
    void toStdListComplex() const;
    void testOperatorsInt() const;
    void testOperatorsMovable() const;
    void testOperatorsComplex() const;
    void testSTLIteratorsInt() const;
    void testSTLIteratorsMovable() const;
    void testSTLIteratorsComplex() const;

    void initializeList() const;

    void constSharedNullInt() const;
    void constSharedNullMovable() const;
    void constSharedNullComplex() const;

    void iterators() const;

private:
    template<typename T>
    void length() const;
    template<typename T>
    void first() const;
    template<typename T>
    void last() const;
    template<typename T>
    void begin() const;
    template<typename T>
    void end() const;
    template<typename T>
    void contains() const;
    template<typename T>
    void count() const;
    template<typename T>
    void empty() const;
    template<typename T>
    void endsWith() const;
    template<typename T>
    void move() const;
    template<typename T>
    void removeAll() const;
    template<typename T>
    void removeOne() const;
    template<typename T>
    void startsWith() const;
    template<typename T>
    void swap() const;
    template<typename T>
    void takeFirst() const;
    template<typename T>
    void takeLast() const;
    template<typename T>
    void toStdList() const;
    template<typename T>
    void value() const;

    template<typename T>
    void testOperators() const;
    template<typename T>
    void testSTLIterators() const;

    template<typename T>
    void constSharedNull() const;

    int dummyForGuard;
};

template<typename T>
struct SimpleValue
{
    static T at(int index) { return values[index % maxSize]; }
    static const uint maxSize = 7;
    static const T values[maxSize];
};

template<>
const int SimpleValue<int>::values[] = { 10, 20, 30, 40, 100, 101, 102 };
template<>
const Movable SimpleValue<Movable>::values[] = { 10, 20, 30, 40, 100, 101, 102 };
template<>
const Complex SimpleValue<Complex>::values[] = { 10, 20, 30, 40, 100, 101, 102 };

// Make some macros for the tests to use in order to be slightly more readable...
#define T_FOO SimpleValue<T>::at(0)
#define T_BAR SimpleValue<T>::at(1)
#define T_BAZ SimpleValue<T>::at(2)
#define T_CAT SimpleValue<T>::at(3)
#define T_DOG SimpleValue<T>::at(4)
#define T_BLAH SimpleValue<T>::at(5)
#define T_WEEE SimpleValue<T>::at(6)

template<typename T>
void tst_QLinkedList::length() const
{
    /* Empty list. */
    {
        const QLinkedList<T> list;
        QCOMPARE(list.size(), 0);
    }

    /* One entry. */
    {
        QLinkedList<T> list;
        list.append(T_FOO);
        QCOMPARE(list.size(), 1);
    }

    /* Two entries. */
    {
        QLinkedList<T> list;
        list.append(T_FOO);
        list.append(T_BAR);
        QCOMPARE(list.size(), 2);
    }

    /* Three entries. */
    {
        QLinkedList<T> list;
        list.append(T_FOO);
        list.append(T_BAR);
        list.append(T_BAZ);
        QCOMPARE(list.size(), 3);
    }
}

void tst_QLinkedList::eraseValidIteratorsOnSharedList() const
{
    QLinkedList<int> a, b;
    a.append(5);
    a.append(10);
    a.append(20);
    a.append(20);
    a.append(20);
    a.append(20);
    a.append(30);

    QLinkedList<int>::iterator i = a.begin();
    ++i;
    ++i;
    ++i;
    b = a;
    QLinkedList<int>::iterator r = a.erase(i);
    QCOMPARE(b.size(), 7);
    QCOMPARE(a.size(), 6);
    --r;
    --r;
    QCOMPARE(*r, 10); // Ensure that number 2 instance was removed;
}

void tst_QLinkedList::insertWithIteratorsOnSharedList() const
{
    QLinkedList<int> a, b;
    a.append(5);
    a.append(10);
    a.append(20);
    QLinkedList<int>::iterator i = a.begin();
    ++i;
    ++i;
    b = a;

    QLinkedList<int>::iterator i2 = a.insert(i, 15);
    QCOMPARE(b.size(), 3);
    QCOMPARE(a.size(), 4);
    --i2;
    QCOMPARE(*i2, 10);
}

void tst_QLinkedList::lengthInt() const
{
    length<int>();
}

void tst_QLinkedList::lengthMovable() const
{
    const int liveCount = Movable::getLiveCount();
    length<Movable>();
    QCOMPARE(liveCount, Movable::getLiveCount());
}

void tst_QLinkedList::lengthComplex() const
{
    const int liveCount = Complex::getLiveCount();
    length<Complex>();
    QCOMPARE(liveCount, Complex::getLiveCount());
}

void tst_QLinkedList::lengthSignature() const
{
    /* Constness. */
    {
        const QLinkedList<int> list;
        /* The function should be const. */
        list.size();
    }
}

template<typename T>
void tst_QLinkedList::first() const
{
    QLinkedList<T> list;
    list << T_FOO << T_BAR;

    QCOMPARE(list.first(), T_FOO);

    // remove an item, make sure it still works
    list.pop_front();
    QVERIFY(list.size() == 1);
    QCOMPARE(list.first(), T_BAR);
}

void tst_QLinkedList::firstInt() const
{
    first<int>();
}

void tst_QLinkedList::firstMovable() const
{
    const int liveCount = Movable::getLiveCount();
    first<Movable>();
    QCOMPARE(liveCount, Movable::getLiveCount());
}

void tst_QLinkedList::firstComplex() const
{
    const int liveCount = Complex::getLiveCount();
    first<Complex>();
    QCOMPARE(liveCount, Complex::getLiveCount());
}

template<typename T>
void tst_QLinkedList::last() const
{
    QLinkedList<T> list;
    list << T_FOO << T_BAR;

    QCOMPARE(list.last(), T_BAR);

    // remove an item, make sure it still works
    list.pop_back();
    QVERIFY(list.size() == 1);
    QCOMPARE(list.last(), T_FOO);
}

void tst_QLinkedList::lastInt() const
{
    last<int>();
}

void tst_QLinkedList::lastMovable() const
{
    const int liveCount = Movable::getLiveCount();
    last<Movable>();
    QCOMPARE(liveCount, Movable::getLiveCount());
}

void tst_QLinkedList::lastComplex() const
{
    const int liveCount = Complex::getLiveCount();
    last<Complex>();
    QCOMPARE(liveCount, Complex::getLiveCount());
}

template<typename T>
void tst_QLinkedList::begin() const
{
    QLinkedList<T> list;
    list << T_FOO << T_BAR;

    QCOMPARE(*list.begin(), T_FOO);

    // remove an item, make sure it still works
    list.pop_front();
    QVERIFY(list.size() == 1);
    QCOMPARE(*list.begin(), T_BAR);
}

void tst_QLinkedList::beginInt() const
{
    begin<int>();
}

void tst_QLinkedList::beginMovable() const
{
    const int liveCount = Movable::getLiveCount();
    begin<Movable>();
    QCOMPARE(liveCount, Movable::getLiveCount());
}

void tst_QLinkedList::beginComplex() const
{
    const int liveCount = Complex::getLiveCount();
    begin<Complex>();
    QCOMPARE(liveCount, Complex::getLiveCount());
}

template<typename T>
void tst_QLinkedList::end() const
{
    QLinkedList<T> list;
    list << T_FOO << T_BAR;

    QCOMPARE(*--list.end(), T_BAR);

    // remove an item, make sure it still works
    list.pop_back();
    QVERIFY(list.size() == 1);
    QCOMPARE(*--list.end(), T_FOO);
}

void tst_QLinkedList::endInt() const
{
    end<int>();
}

void tst_QLinkedList::endMovable() const
{
    const int liveCount = Movable::getLiveCount();
    end<Movable>();
    QCOMPARE(liveCount, Movable::getLiveCount());
}

void tst_QLinkedList::endComplex() const
{
    const int liveCount = Complex::getLiveCount();
    end<Complex>();
    QCOMPARE(liveCount, Complex::getLiveCount());
}

template<typename T>
void tst_QLinkedList::contains() const
{
    QLinkedList<T> list;
    list << T_FOO << T_BAR << T_BAZ;

    QVERIFY(list.contains(T_FOO));
    QVERIFY(list.contains(T_BLAH) != true);

    // add it and make sure it matches
    list.append(T_BLAH);
    QVERIFY(list.contains(T_BLAH));
}

void tst_QLinkedList::containsInt() const
{
    contains<int>();
}

void tst_QLinkedList::containsMovable() const
{
    const int liveCount = Movable::getLiveCount();
    contains<Movable>();
    QCOMPARE(liveCount, Movable::getLiveCount());
}

void tst_QLinkedList::containsComplex() const
{
    const int liveCount = Complex::getLiveCount();
    contains<Complex>();
    QCOMPARE(liveCount, Complex::getLiveCount());
}

template<typename T>
void tst_QLinkedList::count() const
{
    QLinkedList<T> list;

    // starts empty
    QVERIFY(list.count() == 0);

    // goes up
    list.append(T_FOO);
    QVERIFY(list.count() == 1);

    // and up
    list.append(T_BAR);
    QVERIFY(list.count() == 2);

    // and down
    list.pop_back();
    QVERIFY(list.count() == 1);

    // and empty. :)
    list.pop_back();
    QVERIFY(list.count() == 0);
}

void tst_QLinkedList::countInt() const
{
    count<int>();
}

void tst_QLinkedList::countMovable() const
{
    const int liveCount = Movable::getLiveCount();
    count<Movable>();
    QCOMPARE(liveCount, Movable::getLiveCount());
}

void tst_QLinkedList::countComplex() const
{
    const int liveCount = Complex::getLiveCount();
    count<Complex>();
    QCOMPARE(liveCount, Complex::getLiveCount());
}

void tst_QLinkedList::cpp17ctad() const
{
#ifdef __cpp_deduction_guides
#    define QVERIFY_IS_LIST_OF(obj, Type)                                                          \
        QVERIFY2((std::is_same<decltype(obj), QLinkedList<Type>>::value),                          \
                 QMetaType(qMetaTypeId<decltype(obj)::value_type>()).name())
#    define CHECK(Type, One, Two, Three)                                                           \
        do {                                                                                       \
            const Type v[] = { One, Two, Three };                                                  \
            QLinkedList v1 = { One, Two, Three };                                                  \
            QVERIFY_IS_LIST_OF(v1, Type);                                                          \
            QLinkedList v2(v1.begin(), v1.end());                                                  \
            QVERIFY_IS_LIST_OF(v2, Type);                                                          \
            QLinkedList v3(std::begin(v), std::end(v));                                            \
            QVERIFY_IS_LIST_OF(v3, Type);                                                          \
        } while (false) /*end*/
    CHECK(int, 1, 2, 3);
    CHECK(double, 1.0, 2.0, 3.0);
    CHECK(QString, QStringLiteral("one"), QStringLiteral("two"), QStringLiteral("three"));
#    undef QVERIFY_IS_LIST_OF
#    undef CHECK
#else
    QSKIP("This test requires C++17 Constructor Template Argument Deduction support enabled in the "
          "compiler.");
#endif
}

template<typename T>
void tst_QLinkedList::empty() const
{
    QLinkedList<T> list;

    // make sure it starts empty
    QVERIFY(list.empty());

    // and doesn't stay empty
    list.append(T_FOO);
    QVERIFY(!list.empty());

    // and goes back to being empty
    list.pop_back();
    QVERIFY(list.empty());
}

void tst_QLinkedList::emptyInt() const
{
    empty<int>();
}

void tst_QLinkedList::emptyMovable() const
{
    const int liveCount = Movable::getLiveCount();
    empty<Movable>();
    QCOMPARE(liveCount, Movable::getLiveCount());
}

void tst_QLinkedList::emptyComplex() const
{
    const int liveCount = Complex::getLiveCount();
    empty<Complex>();
    QCOMPARE(liveCount, Complex::getLiveCount());
}

template<typename T>
void tst_QLinkedList::endsWith() const
{
    QLinkedList<T> list;
    list << T_FOO << T_BAR << T_BAZ;

    // test it returns correctly in both cases
    QVERIFY(list.endsWith(T_BAZ));
    QVERIFY(!list.endsWith(T_BAR));

    // remove an item and make sure the end item changes
    list.pop_back();
    QVERIFY(list.endsWith(T_BAR));
}

void tst_QLinkedList::endsWithInt() const
{
    endsWith<int>();
}

void tst_QLinkedList::endsWithMovable() const
{
    const int liveCount = Movable::getLiveCount();
    endsWith<Movable>();
    QCOMPARE(liveCount, Movable::getLiveCount());
}

void tst_QLinkedList::endsWithComplex() const
{
    const int liveCount = Complex::getLiveCount();
    endsWith<Complex>();
    QCOMPARE(liveCount, Complex::getLiveCount());
}

template<typename T>
void tst_QLinkedList::removeAll() const
{
    QLinkedList<T> list;
    list << T_FOO << T_BAR << T_BAZ;

    // remove one instance
    list.removeAll(T_BAR);
    QCOMPARE(list, QLinkedList<T>() << T_FOO << T_BAZ);

    // many instances
    list << T_FOO << T_BAR << T_BAZ << T_FOO << T_BAR << T_BAZ << T_FOO << T_BAR << T_BAZ;
    list.removeAll(T_BAR);
    QCOMPARE(list,
             QLinkedList<T>() << T_FOO << T_BAZ << T_FOO << T_BAZ << T_FOO << T_BAZ << T_FOO
                              << T_BAZ);

    // try remove something that doesn't exist
    list.removeAll(T_WEEE);
    QCOMPARE(list,
             QLinkedList<T>() << T_FOO << T_BAZ << T_FOO << T_BAZ << T_FOO << T_BAZ << T_FOO
                              << T_BAZ);
}

void tst_QLinkedList::removeAllInt() const
{
    removeAll<int>();
}

void tst_QLinkedList::removeAllMovable() const
{
    const int liveCount = Movable::getLiveCount();
    removeAll<Movable>();
    QCOMPARE(liveCount, Movable::getLiveCount());
}

void tst_QLinkedList::removeAllComplex() const
{
    const int liveCount = Complex::getLiveCount();
    removeAll<Complex>();
    QCOMPARE(liveCount, Complex::getLiveCount());
}

template<typename T>
void tst_QLinkedList::removeOne() const
{
    QLinkedList<T> list;
    list << T_FOO << T_BAR << T_BAZ;

    // middle
    list.removeOne(T_BAR);
    QCOMPARE(list, QLinkedList<T>() << T_FOO << T_BAZ);

    // start
    list.removeOne(T_FOO);
    QCOMPARE(list, QLinkedList<T>() << T_BAZ);

    // last
    list.removeOne(T_BAZ);
    QCOMPARE(list, QLinkedList<T>());

    // make sure it really only removes one :)
    list << T_FOO << T_FOO;
    list.removeOne(T_FOO);
    QCOMPARE(list, QLinkedList<T>() << T_FOO);

    // try remove something that doesn't exist
    list.removeOne(T_WEEE);
    QCOMPARE(list, QLinkedList<T>() << T_FOO);
}

void tst_QLinkedList::removeOneInt() const
{
    removeOne<int>();
}

void tst_QLinkedList::removeOneMovable() const
{
    const int liveCount = Movable::getLiveCount();
    removeOne<Movable>();
    QCOMPARE(liveCount, Movable::getLiveCount());
}

void tst_QLinkedList::removeOneComplex() const
{
    const int liveCount = Complex::getLiveCount();
    removeOne<Complex>();
    QCOMPARE(liveCount, Complex::getLiveCount());
}

void tst_QLinkedList::reverseIterators() const
{
    QLinkedList<int> l;
    l << 1 << 2 << 3 << 4;
    QLinkedList<int> lr = l;
    std::reverse(lr.begin(), lr.end());
    const QLinkedList<int> &clr = lr;
    QVERIFY(std::equal(l.begin(), l.end(), lr.rbegin()));
    QVERIFY(std::equal(l.begin(), l.end(), lr.crbegin()));
    QVERIFY(std::equal(l.begin(), l.end(), clr.rbegin()));
    QVERIFY(std::equal(lr.rbegin(), lr.rend(), l.begin()));
    QVERIFY(std::equal(lr.crbegin(), lr.crend(), l.begin()));
    QVERIFY(std::equal(clr.rbegin(), clr.rend(), l.begin()));
}

template<typename T>
void tst_QLinkedList::startsWith() const
{
    QLinkedList<T> list;
    list << T_FOO << T_BAR << T_BAZ;

    // make sure it starts ok
    QVERIFY(list.startsWith(T_FOO));

    // remove an item
    list.removeFirst();
    QVERIFY(list.startsWith(T_BAR));
}

void tst_QLinkedList::startsWithInt() const
{
    startsWith<int>();
}

void tst_QLinkedList::startsWithMovable() const
{
    const int liveCount = Movable::getLiveCount();
    startsWith<Movable>();
    QCOMPARE(liveCount, Movable::getLiveCount());
}

void tst_QLinkedList::startsWithComplex() const
{
    const int liveCount = Complex::getLiveCount();
    startsWith<Complex>();
    QCOMPARE(liveCount, Complex::getLiveCount());
}

template<typename T>
void tst_QLinkedList::takeFirst() const
{
    QLinkedList<T> list;
    list << T_FOO << T_BAR << T_BAZ;

    QCOMPARE(list.takeFirst(), T_FOO);
    QVERIFY(list.size() == 2);
    QCOMPARE(list.takeFirst(), T_BAR);
    QVERIFY(list.size() == 1);
    QCOMPARE(list.takeFirst(), T_BAZ);
    QVERIFY(list.size() == 0);
}

void tst_QLinkedList::takeFirstInt() const
{
    takeFirst<int>();
}

void tst_QLinkedList::takeFirstMovable() const
{
    const int liveCount = Movable::getLiveCount();
    takeFirst<Movable>();
    QCOMPARE(liveCount, Movable::getLiveCount());
}

void tst_QLinkedList::takeFirstComplex() const
{
    const int liveCount = Complex::getLiveCount();
    takeFirst<Complex>();
    QCOMPARE(liveCount, Complex::getLiveCount());
}

template<typename T>
void tst_QLinkedList::takeLast() const
{
    QLinkedList<T> list;
    list << T_FOO << T_BAR << T_BAZ;

    QCOMPARE(list.takeLast(), T_BAZ);
    QCOMPARE(list.takeLast(), T_BAR);
    QCOMPARE(list.takeLast(), T_FOO);
}

void tst_QLinkedList::takeLastInt() const
{
    takeLast<int>();
}

void tst_QLinkedList::takeLastMovable() const
{
    const int liveCount = Movable::getLiveCount();
    takeLast<Movable>();
    QCOMPARE(liveCount, Movable::getLiveCount());
}

void tst_QLinkedList::takeLastComplex() const
{
    const int liveCount = Complex::getLiveCount();
    takeLast<Complex>();
    QCOMPARE(liveCount, Complex::getLiveCount());
}

template<typename T>
void tst_QLinkedList::toStdList() const
{
    QLinkedList<T> list;
    list << T_FOO << T_BAR << T_BAZ;

    // yuck.
    std::list<T> slist;
    slist.push_back(T_FOO);
    slist.push_back(T_BAR);
    slist.push_back(T_BAZ);

    QCOMPARE(list.toStdList(), slist);
    QCOMPARE(list, QLinkedList<T>() << T_FOO << T_BAR << T_BAZ);
}

void tst_QLinkedList::toStdListInt() const
{
    toStdList<int>();
}

void tst_QLinkedList::toStdListMovable() const
{
    const int liveCount = Movable::getLiveCount();
    toStdList<Movable>();
    QCOMPARE(liveCount, Movable::getLiveCount());
}

void tst_QLinkedList::toStdListComplex() const
{
    const int liveCount = Complex::getLiveCount();
    toStdList<Complex>();
    QCOMPARE(liveCount, Complex::getLiveCount());
}

template<typename T>
void tst_QLinkedList::testOperators() const
{
    QLinkedList<T> list;
    list << T_FOO << T_BAR << T_BAZ;

    QLinkedList<T> listtwo;
    listtwo << T_FOO << T_BAR << T_BAZ;

    // test equal
    QVERIFY(list == listtwo);

    // not equal
    listtwo.append(T_CAT);
    QVERIFY(list != listtwo);

    // +=
    list += listtwo;
    QVERIFY(list.size() == 7);
    QVERIFY(listtwo.size() == 4);
    QCOMPARE(list, QLinkedList<T>() << T_FOO << T_BAR << T_BAZ << T_FOO << T_BAR << T_BAZ << T_CAT);

    // =
    list = listtwo;
    QCOMPARE(list, listtwo);
    QCOMPARE(list, QLinkedList<T>() << T_FOO << T_BAR << T_BAZ << T_CAT);
}

void tst_QLinkedList::testOperatorsInt() const
{
    testOperators<int>();
}

void tst_QLinkedList::testOperatorsMovable() const
{
    const int liveCount = Movable::getLiveCount();
    testOperators<Movable>();
    QCOMPARE(liveCount, Movable::getLiveCount());
}

void tst_QLinkedList::testOperatorsComplex() const
{
    const int liveCount = Complex::getLiveCount();
    testOperators<Complex>();
    QCOMPARE(liveCount, Complex::getLiveCount());
}

template<typename T>
void tst_QLinkedList::testSTLIterators() const
{
    QLinkedList<T> list;

    // create a list
    list << T_FOO << T_BAR << T_BAZ;
    typename QLinkedList<T>::iterator it = list.begin();
    QCOMPARE(*it, T_FOO);
    it++;
    QCOMPARE(*it, T_BAR);
    it++;
    QCOMPARE(*it, T_BAZ);
    it++;
    QCOMPARE(it, list.end());
    it--;

    // walk backwards
    QCOMPARE(*it, T_BAZ);
    it--;
    QCOMPARE(*it, T_BAR);
    it--;
    QCOMPARE(*it, T_FOO);

    // test erase
    it = list.erase(it);
    QVERIFY(list.size() == 2);
    QCOMPARE(*it, T_BAR);

    // test multiple erase
    it = list.erase(it, it + 2);
    QVERIFY(list.size() == 0);
    QCOMPARE(it, list.end());

    // insert again
    it = list.insert(it, T_FOO);
    QVERIFY(list.size() == 1);
    QCOMPARE(*it, T_FOO);

    // insert again
    it = list.insert(it, T_BAR);
    QVERIFY(list.size() == 2);
    QCOMPARE(*it++, T_BAR);
    QCOMPARE(*it, T_FOO);
}

void tst_QLinkedList::testSTLIteratorsInt() const
{
    testSTLIterators<int>();
}

void tst_QLinkedList::testSTLIteratorsMovable() const
{
    const int liveCount = Movable::getLiveCount();
    testSTLIterators<Movable>();
    QCOMPARE(liveCount, Movable::getLiveCount());
}

void tst_QLinkedList::testSTLIteratorsComplex() const
{
    const int liveCount = Complex::getLiveCount();
    testSTLIterators<Complex>();
    QCOMPARE(liveCount, Complex::getLiveCount());
}

void tst_QLinkedList::initializeList() const
{
    QLinkedList<int> v1 { 2, 3, 4 };
    QCOMPARE(v1, QLinkedList<int>() << 2 << 3 << 4);
    QCOMPARE(v1, (QLinkedList<int> { 2, 3, 4 }));

    QLinkedList<QLinkedList<int>> v2 { v1, { 1 }, QLinkedList<int>(), { 2, 3, 4 } };
    QLinkedList<QLinkedList<int>> v3;
    v3 << v1 << (QLinkedList<int>() << 1) << QLinkedList<int>() << v1;
    QCOMPARE(v3, v2);
}

template<typename T>
void tst_QLinkedList::constSharedNull() const
{
    QLinkedList<T> list2;
    QVERIFY(!list2.isDetached());
}

void tst_QLinkedList::constSharedNullInt() const
{
    constSharedNull<int>();
}

void tst_QLinkedList::constSharedNullMovable() const
{
    const int liveCount = Movable::getLiveCount();
    constSharedNull<Movable>();
    QCOMPARE(liveCount, Movable::getLiveCount());
}

void tst_QLinkedList::constSharedNullComplex() const
{
    const int liveCount = Complex::getLiveCount();
    constSharedNull<Complex>();
    QCOMPARE(liveCount, Complex::getLiveCount());
}

void tst_QLinkedList::iterators() const
{
    QLinkedList<int> list;
    QVERIFY(list.isEmpty());
    list.append(1);
    list.push_back(2);
    list += (3);
    list << 4 << 5 << 6;
    QVERIFY(!list.isEmpty());
    QVERIFY(list.size() == 6);
    {
        int sum = 0;
        QLinkedListIterator<int> i = list;
        while (i.hasNext()) {
            sum += i.next();
        }
        QVERIFY(sum == 21);
    }
    {
        int sum = 0;
        QLinkedList<int>::const_iterator i = list.begin();
        while (i != list.end())
            sum += *i++;
        QVERIFY(sum == 21);
    }
    {
        QMutableLinkedListIterator<int> i = list;
        while (i.hasNext())
            i.setValue(2 * i.next());
    }
    {
        int sum = 0;
        QLinkedListIterator<int> i = list;
        i.toBack();
        while (i.hasPrevious())
            sum += i.previous();
        QVERIFY(sum == 2 * 21);
    }
    {
        QMutableLinkedListIterator<int> i = list;
        i.toBack();
        while (i.hasPrevious())
            i.setValue(2 * i.previous());
    }
    {
        int sum = 0;
        QLinkedListIterator<int> i = list;
        i.toBack();
        while (i.hasPrevious())
            sum += i.previous();
        QVERIFY(sum == 2 * 2 * 21);
    }
    {
        QMutableLinkedListIterator<int> i = list;
        while (i.hasNext()) {
            int a = i.next();
            i.insert(a);
        }
    }
    {
        int sum = 0;
        QLinkedList<int>::iterator i = list.begin();
        while (i != list.end())
            sum += *i++;
        QVERIFY(sum == 2 * 2 * 2 * 21);
    }
    {
        int duplicates = 0;
        QLinkedListIterator<int> i = list;
        while (i.hasNext()) {
            int a = i.next();
            if (i.hasNext() && a == i.peekNext())
                duplicates++;
        }
        QVERIFY(duplicates == 6);
    }
    {
        int duplicates = 0;
        QLinkedListIterator<int> i = list;
        i.toBack();
        while (i.hasPrevious()) {
            int a = i.previous();
            if (i.hasPrevious() && a == i.peekPrevious())
                duplicates++;
        }
        QVERIFY(duplicates == 6);
    }
    {
        QMutableLinkedListIterator<int> i = list;
        while (i.hasNext()) {
            int a = i.next();
            if (i.hasNext() && i.peekNext() == a)
                i.remove();
        }
    }
    {
        int duplicates = 0;
        QMutableLinkedListIterator<int> i = list;
        i.toBack();
        while (i.hasPrevious()) {
            int a = i.previous();
            if (i.hasPrevious() && a == i.peekPrevious())
                duplicates++;
        }
        QVERIFY(duplicates == 0);
    }
    {
        QVERIFY(list.size() == 6);
        QMutableLinkedListIterator<int> i = list;
        while (i.hasNext()) {
            int a = i.peekNext();
            i.insert(42);
            QVERIFY(i.peekPrevious() == 42 && i.peekNext() == a);
            i.next();
        }
        QVERIFY(list.size() == 12);
        i.toFront();
        while (i.findNext(42))
            i.remove();
    }
}

QT_END_NAMESPACE

QTEST_APPLESS_MAIN(tst_QLinkedList)
#include "tst_qlinkedlist.moc"
