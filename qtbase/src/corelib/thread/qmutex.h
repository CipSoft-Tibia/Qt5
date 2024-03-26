// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMUTEX_H
#define QMUTEX_H

#include <QtCore/qglobal.h>
#include <QtCore/qatomic.h>
#include <QtCore/qdeadlinetimer.h>
#include <QtCore/qtsan_impl.h>

#include <chrono>

QT_BEGIN_NAMESPACE

#if QT_CONFIG(thread) || defined(Q_QDOC)

#if defined(Q_OS_LINUX) || defined(Q_OS_WIN) // these platforms use futex
# define QT_MUTEX_LOCK_NOEXCEPT noexcept
#else
# define QT_MUTEX_LOCK_NOEXCEPT
#endif

class QMutex;
class QRecursiveMutex;
class QMutexPrivate;

class Q_CORE_EXPORT QBasicMutex
{
    Q_DISABLE_COPY_MOVE(QBasicMutex)
public:
    constexpr QBasicMutex()
        : d_ptr(nullptr)
    {}

    // BasicLockable concept
    inline void lock() QT_MUTEX_LOCK_NOEXCEPT {
        QtTsan::mutexPreLock(this, 0u);

        if (!fastTryLock())
            lockInternal();

        QtTsan::mutexPostLock(this, 0u, 0);
    }

    // BasicLockable concept
    inline void unlock() noexcept {
        Q_ASSERT(d_ptr.loadRelaxed()); //mutex must be locked

        QtTsan::mutexPreUnlock(this, 0u);

        if (!fastTryUnlock())
            unlockInternal();

        QtTsan::mutexPostUnlock(this, 0u);
    }

    bool tryLock() noexcept {
        unsigned tsanFlags = QtTsan::TryLock;
        QtTsan::mutexPreLock(this, tsanFlags);

        const bool success = fastTryLock();

        if (!success)
            tsanFlags |= QtTsan::TryLockFailed;
        QtTsan::mutexPostLock(this, tsanFlags, 0);

        return success;
    }

    // Lockable concept
    bool try_lock() noexcept { return tryLock(); }

private:
    inline bool fastTryLock() noexcept
    {
        if (d_ptr.loadRelaxed() != nullptr)
            return false;
        return d_ptr.testAndSetAcquire(nullptr, dummyLocked());
    }
    inline bool fastTryUnlock() noexcept {
        return d_ptr.testAndSetRelease(dummyLocked(), nullptr);
    }

    void lockInternal() QT_MUTEX_LOCK_NOEXCEPT;
    bool lockInternal(QDeadlineTimer timeout) QT_MUTEX_LOCK_NOEXCEPT;
#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
    bool lockInternal(int timeout) QT_MUTEX_LOCK_NOEXCEPT;
#endif
    void unlockInternal() noexcept;
    void destroyInternal(QMutexPrivate *d);

    QBasicAtomicPointer<QMutexPrivate> d_ptr;
    static inline QMutexPrivate *dummyLocked() {
        return reinterpret_cast<QMutexPrivate *>(quintptr(1));
    }

    friend class QMutex;
    friend class QMutexPrivate;
};

class Q_CORE_EXPORT QMutex : public QBasicMutex
{
public:
    constexpr QMutex() = default;
    ~QMutex()
    {
        QMutexPrivate *d = d_ptr.loadRelaxed();
        if (d)
            destroyInternal(d);
    }

#ifdef Q_QDOC
    inline void lock() QT_MUTEX_LOCK_NOEXCEPT;
    inline void unlock() noexcept;
    bool tryLock() noexcept;
#endif

    // Lockable concept
    bool try_lock() noexcept { return tryLock(); }


    using QBasicMutex::tryLock;
    bool tryLock(int timeout) QT_MUTEX_LOCK_NOEXCEPT
    {
        return tryLock(QDeadlineTimer(timeout));
    }

    bool tryLock(QDeadlineTimer timeout) QT_MUTEX_LOCK_NOEXCEPT
    {
        unsigned tsanFlags = QtTsan::TryLock;
        QtTsan::mutexPreLock(this, tsanFlags);

        bool success = fastTryLock();

        if (success) {
            QtTsan::mutexPostLock(this, tsanFlags, 0);
            return success;
        }

        success = lockInternal(timeout);

        if (!success)
            tsanFlags |= QtTsan::TryLockFailed;
        QtTsan::mutexPostLock(this, tsanFlags, 0);

        return success;
    }

    // TimedLockable concept
    template <class Rep, class Period>
    bool try_lock_for(std::chrono::duration<Rep, Period> duration)
    {
        return tryLock(QDeadlineTimer(duration));
    }

    // TimedLockable concept
    template<class Clock, class Duration>
    bool try_lock_until(std::chrono::time_point<Clock, Duration> timePoint)
    {
        return tryLock(QDeadlineTimer(timePoint));
    }
};

class Q_CORE_EXPORT QRecursiveMutex
{
    Q_DISABLE_COPY_MOVE(QRecursiveMutex)
    // written to by the thread that first owns 'mutex';
    // read during attempts to acquire ownership of 'mutex' from any other thread:
    QAtomicPointer<void> owner = nullptr;
    // only ever accessed from the thread that owns 'mutex':
    uint count = 0;
    QMutex mutex;

public:
    constexpr QRecursiveMutex() = default;
    ~QRecursiveMutex();


    // BasicLockable concept
    void lock() QT_MUTEX_LOCK_NOEXCEPT
    { tryLock(QDeadlineTimer(QDeadlineTimer::Forever)); }
    QT_CORE_INLINE_SINCE(6, 6)
    bool tryLock(int timeout) QT_MUTEX_LOCK_NOEXCEPT;
    bool tryLock(QDeadlineTimer timer = {}) QT_MUTEX_LOCK_NOEXCEPT;
    // BasicLockable concept
    void unlock() noexcept;

    // Lockable concept
    bool try_lock() QT_MUTEX_LOCK_NOEXCEPT { return tryLock(); }

    // TimedLockable concept
    template <class Rep, class Period>
    bool try_lock_for(std::chrono::duration<Rep, Period> duration)
    {
        return tryLock(QDeadlineTimer(duration));
    }

    // TimedLockable concept
    template<class Clock, class Duration>
    bool try_lock_until(std::chrono::time_point<Clock, Duration> timePoint)
    {
        return tryLock(QDeadlineTimer(timePoint));
    }
};

#if QT_CORE_INLINE_IMPL_SINCE(6, 6)
bool QRecursiveMutex::tryLock(int timeout) QT_MUTEX_LOCK_NOEXCEPT
{
    return tryLock(QDeadlineTimer(timeout));
}
#endif

template <typename Mutex>
class QMutexLocker
{
public:
    Q_NODISCARD_CTOR
    inline explicit QMutexLocker(Mutex *mutex) QT_MUTEX_LOCK_NOEXCEPT
    {
        m_mutex = mutex;
        if (Q_LIKELY(mutex)) {
            mutex->lock();
            m_isLocked = true;
        }
    }

    Q_NODISCARD_CTOR
    inline QMutexLocker(QMutexLocker &&other) noexcept
        : m_mutex(std::exchange(other.m_mutex, nullptr)),
          m_isLocked(std::exchange(other.m_isLocked, false))
    {}

    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(QMutexLocker)

    inline ~QMutexLocker()
    {
        if (m_isLocked)
            unlock();
    }

    inline bool isLocked() const noexcept
    {
        return m_isLocked;
    }

    inline void unlock() noexcept
    {
        Q_ASSERT(m_isLocked);
        m_mutex->unlock();
        m_isLocked = false;
    }

    inline void relock() QT_MUTEX_LOCK_NOEXCEPT
    {
        Q_ASSERT(!m_isLocked);
        m_mutex->lock();
        m_isLocked = true;
    }

    inline void swap(QMutexLocker &other) noexcept
    {
        qt_ptr_swap(m_mutex, other.m_mutex);
        std::swap(m_isLocked, other.m_isLocked);
    }

    Mutex *mutex() const
    {
        return m_mutex;
    }
private:
    Q_DISABLE_COPY(QMutexLocker)

    Mutex *m_mutex;
    bool m_isLocked = false;
};

#else // !QT_CONFIG(thread) && !Q_QDOC

class QMutex
{
public:

    constexpr QMutex() noexcept { }

    inline void lock() noexcept {}
    inline bool tryLock(int timeout = 0) noexcept { Q_UNUSED(timeout); return true; }
    inline bool try_lock() noexcept { return true; }
    inline void unlock() noexcept {}

    template <class Rep, class Period>
    inline bool try_lock_for(std::chrono::duration<Rep, Period> duration) noexcept
    {
        Q_UNUSED(duration);
        return true;
    }

    template<class Clock, class Duration>
    inline bool try_lock_until(std::chrono::time_point<Clock, Duration> timePoint) noexcept
    {
        Q_UNUSED(timePoint);
        return true;
    }

private:
    Q_DISABLE_COPY(QMutex)
};

class QRecursiveMutex : public QMutex {};

template <typename Mutex>
class QMutexLocker
{
public:
    Q_NODISCARD_CTOR
    inline explicit QMutexLocker(Mutex *) noexcept {}
    inline ~QMutexLocker() noexcept {}

    inline void unlock() noexcept {}
    void relock() noexcept {}
    inline Mutex *mutex() const noexcept { return nullptr; }

private:
    Q_DISABLE_COPY(QMutexLocker)
};

typedef QMutex QBasicMutex;

#endif // !QT_CONFIG(thread) && !Q_QDOC

QT_END_NAMESPACE

#endif // QMUTEX_H
