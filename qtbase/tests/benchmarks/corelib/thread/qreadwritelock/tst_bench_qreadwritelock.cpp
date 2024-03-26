// Copyright (C) 2016 Olivier Goffart <ogoffart@woboq.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/QtCore>
#include <QTest>
#include <mutex>
#if __has_include(<shared_mutex>)
#include <shared_mutex>
#endif
#include <vector>

// Wrapers that take pointers instead of reference to have the same interface as Qt
template <typename T>
struct LockerWrapper : T
{
    LockerWrapper(typename T::mutex_type *mtx)
        : T(*mtx)
    {
    }
};

struct QRecursiveReadWriteLock : QReadWriteLock
{
    QRecursiveReadWriteLock() : QReadWriteLock(Recursive) {}
};

template <typename T, size_t N>
  // requires N = 2^M for some Integral M >= 0
struct Recursive
{
    Recursive<T, N/2> r1, r2;

    template <typename...Args>
    Q_ALWAYS_INLINE
    explicit Recursive(Args &&...args)
        : r1(args...),
          r2(args...)
    {}
};

template <typename T>
struct Recursive<T, 1>
{
    T t;
    template <typename...Args>
    Q_ALWAYS_INLINE
    explicit Recursive(Args &&...args)
        : t(args...) {}
};

template <typename T>
struct Recursive<T, 0>
{
    template <typename...Args>
    Q_ALWAYS_INLINE
    explicit Recursive(Args &&...) {}
};

template <size_t N>
using QRecursiveReadLocker = Recursive<QReadLocker, N>;

template <size_t N>
using QRecursiveWriteLocker = Recursive<QWriteLocker, N>;

int threadCount;

class tst_QReadWriteLock : public QObject
{
    Q_OBJECT
public:
    tst_QReadWriteLock()
    {
        // at least 2 threads, even on single cpu/core machines
        threadCount = qMax(2, QThread::idealThreadCount());
        qDebug("thread count: %d", threadCount);
    }

private slots:
    void uncontended_data();
    void uncontended();
    void readOnly_data();
    void readOnly();
    void writeOnly_data();
    void writeOnly();
    // void readWrite();
};

struct FunctionPtrHolder
{
    FunctionPtrHolder(QFunctionPointer value = nullptr)
        : value(value)
    {
    }
    QFunctionPointer value;
};
Q_DECLARE_METATYPE(FunctionPtrHolder)

struct FakeLock
{
    FakeLock(volatile int *i) { *i = 0; }
};

enum { Iterations = 1000000 };

template <typename Mutex, typename Locker>
void testUncontended()
{
    Mutex lock;
    QBENCHMARK {
        for (int i = 0; i < Iterations; ++i) {
            Locker locker(&lock);
        }
    }
}

void tst_QReadWriteLock::uncontended_data()
{
    QTest::addColumn<FunctionPtrHolder>("holder");

    QTest::newRow("nothing") << FunctionPtrHolder(testUncontended<int, FakeLock>);
    QTest::newRow("QMutex") << FunctionPtrHolder(testUncontended<QMutex, QMutexLocker<QMutex>>);
    QTest::newRow("QReadWriteLock, read")
        << FunctionPtrHolder(testUncontended<QReadWriteLock, QReadLocker>);
    QTest::newRow("QReadWriteLock, write")
        << FunctionPtrHolder(testUncontended<QReadWriteLock, QWriteLocker>);
#define ROW(n) \
    QTest::addRow("QReadWriteLock, %s, recursive: %d", "read", n) \
        << FunctionPtrHolder(testUncontended<QRecursiveReadWriteLock, QRecursiveReadLocker<n>>); \
    QTest::addRow("QReadWriteLock, %s, recursive: %d", "write", n) \
        << FunctionPtrHolder(testUncontended<QRecursiveReadWriteLock, QRecursiveWriteLocker<n>>)
    ROW(1);
    ROW(2);
    ROW(32);
#undef ROW
    QTest::newRow("std::mutex") << FunctionPtrHolder(
        testUncontended<std::mutex, LockerWrapper<std::unique_lock<std::mutex>>>);
#ifdef __cpp_lib_shared_mutex
    QTest::newRow("std::shared_mutex, read") << FunctionPtrHolder(
        testUncontended<std::shared_mutex,
                        LockerWrapper<std::shared_lock<std::shared_mutex>>>);
    QTest::newRow("std::shared_mutex, write") << FunctionPtrHolder(
        testUncontended<std::shared_mutex,
                        LockerWrapper<std::unique_lock<std::shared_mutex>>>);
#endif
#if defined __cpp_lib_shared_timed_mutex
    QTest::newRow("std::shared_timed_mutex, read") << FunctionPtrHolder(
        testUncontended<std::shared_timed_mutex,
                        LockerWrapper<std::shared_lock<std::shared_timed_mutex>>>);
    QTest::newRow("std::shared_timed_mutex, write") << FunctionPtrHolder(
        testUncontended<std::shared_timed_mutex,
                        LockerWrapper<std::unique_lock<std::shared_timed_mutex>>>);
#endif
}

void tst_QReadWriteLock::uncontended()
{
    QFETCH(FunctionPtrHolder, holder);
    holder.value();
}

static QHash<QString, QString> global_hash;

template <typename Mutex, typename Locker>
void testReadOnly()
{
    struct Thread : QThread
    {
        Mutex *lock;
        void run() override
        {
            for (int i = 0; i < Iterations; ++i) {
                QString s = QString::number(i); // Do something outside the lock
                Locker locker(lock);
                global_hash.contains(s);
            }
        }
    };
    Mutex lock;
    std::vector<std::unique_ptr<Thread>> threads;
    for (int i = 0; i < threadCount; ++i) {
        auto t = std::make_unique<Thread>();
        t->lock = &lock;
        threads.push_back(std::move(t));
    }
    QBENCHMARK {
        for (auto &t : threads) {
            t->start();
        }
        for (auto &t : threads) {
            t->wait();
        }
    }
}

void tst_QReadWriteLock::readOnly_data()
{
    QTest::addColumn<FunctionPtrHolder>("holder");

    QTest::newRow("nothing") << FunctionPtrHolder(testReadOnly<int, FakeLock>);
    QTest::newRow("QMutex") << FunctionPtrHolder(testReadOnly<QMutex, QMutexLocker<QMutex>>);
    QTest::newRow("QReadWriteLock") << FunctionPtrHolder(testReadOnly<QReadWriteLock, QReadLocker>);
#define ROW(n) \
    QTest::addRow("QReadWriteLock, recursive: %d", n) \
        << FunctionPtrHolder(testReadOnly<QRecursiveReadWriteLock, QRecursiveReadLocker<n>>)
    ROW(1);
    ROW(2);
    ROW(32);
#undef ROW
    QTest::newRow("std::mutex") << FunctionPtrHolder(
        testReadOnly<std::mutex, LockerWrapper<std::unique_lock<std::mutex>>>);
#ifdef __cpp_lib_shared_mutex
    QTest::newRow("std::shared_mutex") << FunctionPtrHolder(
        testReadOnly<std::shared_mutex,
                     LockerWrapper<std::shared_lock<std::shared_mutex>>>);
#endif
#if defined __cpp_lib_shared_timed_mutex
    QTest::newRow("std::shared_timed_mutex") << FunctionPtrHolder(
        testReadOnly<std::shared_timed_mutex,
                     LockerWrapper<std::shared_lock<std::shared_timed_mutex>>>);
#endif
}

void tst_QReadWriteLock::readOnly()
{
    QFETCH(FunctionPtrHolder, holder);
    holder.value();
}

static QString global_string;

template <typename Mutex, typename Locker>
void testWriteOnly()
{
    struct Thread : QThread
    {
        Mutex *lock;
        void run() override
        {
            for (int i = 0; i < Iterations; ++i) {
                QString s = QString::number(i); // Do something outside the lock
                Locker locker(lock);
                global_string = s;
            }
        }
    };
    Mutex lock;
    std::vector<std::unique_ptr<Thread>> threads;
    for (int i = 0; i < threadCount; ++i) {
        auto t = std::make_unique<Thread>();
        t->lock = &lock;
        threads.push_back(std::move(t));
    }
    QBENCHMARK {
        for (auto &t : threads) {
            t->start();
        }
        for (auto &t : threads) {
            t->wait();
        }
    }
}

void tst_QReadWriteLock::writeOnly_data()
{
    QTest::addColumn<FunctionPtrHolder>("holder");

    // QTest::newRow("nothing") << FunctionPtrHolder(testWriteOnly<int, FakeLock>);
    QTest::newRow("QMutex") << FunctionPtrHolder(testWriteOnly<QMutex, QMutexLocker<QMutex>>);
    QTest::newRow("QReadWriteLock") << FunctionPtrHolder(testWriteOnly<QReadWriteLock, QWriteLocker>);
#define ROW(n) \
    QTest::addRow("QReadWriteLock, recursive: %d", n) \
        << FunctionPtrHolder(testWriteOnly<QRecursiveReadWriteLock, QRecursiveWriteLocker<n>>)
    ROW(1);
    ROW(2);
    ROW(32);
#undef ROW
    QTest::newRow("std::mutex") << FunctionPtrHolder(
        testWriteOnly<std::mutex, LockerWrapper<std::unique_lock<std::mutex>>>);
#ifdef __cpp_lib_shared_mutex
    QTest::newRow("std::shared_mutex") << FunctionPtrHolder(
        testWriteOnly<std::shared_mutex,
                     LockerWrapper<std::unique_lock<std::shared_mutex>>>);
#endif
#if defined __cpp_lib_shared_timed_mutex
    QTest::newRow("std::shared_timed_mutex") << FunctionPtrHolder(
        testWriteOnly<std::shared_timed_mutex,
                     LockerWrapper<std::unique_lock<std::shared_timed_mutex>>>);
#endif
}

void tst_QReadWriteLock::writeOnly()
{
    QFETCH(FunctionPtrHolder, holder);
    holder.value();
}

QTEST_MAIN(tst_QReadWriteLock)
#include "tst_bench_qreadwritelock.moc"
