// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/private/qglobal_p.h>
#include "qcore_unix_p.h"
#include "qelapsedtimer.h"

#include <stdlib.h>

#ifdef __GLIBC__
#  include <sys/syscall.h>
#  include <pthread.h>
#  include <unistd.h>
#endif

#ifdef Q_OS_DARWIN
#include <mach/mach_time.h>
#endif

QT_BEGIN_NAMESPACE

QByteArray qt_readlink(const char *path)
{
#ifndef PATH_MAX
    // suitably large value that won't consume too much memory
#  define PATH_MAX  1024*1024
#endif

    QByteArray buf(256, Qt::Uninitialized);

    ssize_t len = ::readlink(path, buf.data(), buf.size());
    while (len == buf.size()) {
        // readlink(2) will fill our buffer and not necessarily terminate with NUL;
        if (buf.size() >= PATH_MAX) {
            errno = ENAMETOOLONG;
            return QByteArray();
        }

        // double the size and try again
        buf.resize(buf.size() * 2);
        len = ::readlink(path, buf.data(), buf.size());
    }

    if (len == -1)
        return QByteArray();

    buf.resize(len);
    return buf;
}

#if defined(Q_PROCESSOR_X86_32) && defined(__GLIBC__)
#  if !__GLIBC_PREREQ(2, 22)
// glibc prior to release 2.22 had a bug that suppresses the third argument to
// open() / open64() / openat(), causing file creation with O_TMPFILE to have
// the wrong permissions. So we bypass the glibc implementation and go straight
// for the syscall. See
// https://sourceware.org/git/?p=glibc.git;a=commit;h=65f6f938cd562a614a68e15d0581a34b177ec29d
int qt_open64(const char *pathname, int flags, mode_t mode)
{
    return syscall(SYS_open, pathname, flags | O_LARGEFILE, mode);
}
#  endif
#endif

#ifndef QT_BOOTSTRAPPED

static inline void do_gettime(qint64 *sec, qint64 *frac)
{
    timespec ts;
    clockid_t clk = CLOCK_REALTIME;
#if defined(CLOCK_MONOTONIC_RAW)
    clk = CLOCK_MONOTONIC_RAW;
#elif defined(CLOCK_MONOTONIC)
    clk = CLOCK_MONOTONIC;
#endif

    clock_gettime(clk, &ts);
    *sec = ts.tv_sec;
    *frac = ts.tv_nsec;
}

// also used in qeventdispatcher_unix.cpp
struct timespec qt_gettime() noexcept
{
    qint64 sec, frac;
    do_gettime(&sec, &frac);

    timespec tv;
    tv.tv_sec = sec;
    tv.tv_nsec = frac;

    return tv;
}

#if QT_CONFIG(poll_pollts)
#  define ppoll pollts
#endif

static inline bool time_update(struct timespec *tv, const struct timespec &start,
                               const struct timespec &timeout)
{
    // clock source is (hopefully) monotonic, so we can recalculate how much timeout is left;
    // if it isn't monotonic, we'll simply hope that it hasn't jumped, because we have no alternative
    struct timespec now = qt_gettime();
    *tv = timeout + start - now;
    return tv->tv_sec >= 0;
}

[[maybe_unused]]
static inline int timespecToMillisecs(const struct timespec *ts)
{
    using namespace std::chrono;
    if (!ts)
        return -1;
    auto ms = ceil<milliseconds>(timespecToChrono<nanoseconds>(*ts));
    return int(ms.count());
}

// defined in qpoll.cpp
int qt_poll(struct pollfd *fds, nfds_t nfds, const struct timespec *timeout_ts);

static inline int qt_ppoll(struct pollfd *fds, nfds_t nfds, const struct timespec *timeout_ts)
{
#if QT_CONFIG(poll_ppoll) || QT_CONFIG(poll_pollts)
    return ::ppoll(fds, nfds, timeout_ts, nullptr);
#elif QT_CONFIG(poll_poll)
    return ::poll(fds, nfds, timespecToMillisecs(timeout_ts));
#elif QT_CONFIG(poll_select)
    return qt_poll(fds, nfds, timeout_ts);
#else
    // configure.json reports an error when everything is not available
#endif
}


/*!
    \internal

    Behaves as close to POSIX poll(2) as practical but may be implemented
    using select(2) where necessary. In that case, returns -1 and sets errno
    to EINVAL if passed any descriptor greater than or equal to FD_SETSIZE.
*/
int qt_safe_poll(struct pollfd *fds, nfds_t nfds, const struct timespec *timeout_ts)
{
    if (!timeout_ts) {
        // no timeout -> block forever
        int ret;
        QT_EINTR_LOOP(ret, qt_ppoll(fds, nfds, nullptr));
        return ret;
    }

    timespec start = qt_gettime();
    timespec timeout = *timeout_ts;

    // loop and recalculate the timeout as needed
    forever {
        const int ret = qt_ppoll(fds, nfds, &timeout);
        if (ret != -1 || errno != EINTR)
            return ret;

        // recalculate the timeout
        if (!time_update(&timeout, start, *timeout_ts)) {
            // timeout during update
            // or clock reset, fake timeout error
            return 0;
        }
    }
}

#endif // QT_BOOTSTRAPPED

QT_END_NAMESPACE
