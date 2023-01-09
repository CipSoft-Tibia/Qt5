/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "OSAllocator.h"

#if OS(UNIX)

#include <cstdlib>

#include "PageAllocation.h"
#include <dlfcn.h>
#include <errno.h>
#include <sys/mman.h>
#include <wtf/Assertions.h>
#include <wtf/UnusedParam.h>

#if OS(LINUX)
#include <sys/syscall.h>
#ifndef MFD_CLOEXEC
#define MFD_CLOEXEC         0x0001U
#endif
#endif

#if defined(__ANDROID__) && defined(SYS_memfd_create)
   // On Android it's been observed that permissions of memory mappings
   // backed by a memfd could not be changed via mprotect for no obvious
   // reason.
#  undef SYS_memfd_create
#endif

namespace WTF {

#ifdef SYS_memfd_create
static int memfdForUsage(size_t bytes, OSAllocator::Usage usage)
{
    const char *type = "unknown-usage:";
    switch (usage) {
    case OSAllocator::UnknownUsage:
        break;
    case OSAllocator::FastMallocPages:
        type = "fastmalloc:";
        break;
    case OSAllocator::JSGCHeapPages:
        type = "JSGCHeap:";
        break;
    case OSAllocator::JSVMStackPages:
        type = "JSVMStack:";
        break;
    case OSAllocator::JSJITCodePages:
        type = "JITCode:";
        break;
    }

    char buf[PATH_MAX];
    strcpy(buf, type);
    strcat(buf, "QtQml");

    int fd = syscall(SYS_memfd_create, buf, MFD_CLOEXEC);
    if (fd != -1) {
        if (ftruncate(fd, bytes) == 0)
            return fd;
    }
    close(fd);
    return -1;
}
#elif OS(LINUX)
static int memfdForUsage(size_t bytes, OSAllocator::Usage usage)
{
    UNUSED_PARAM(bytes);
    UNUSED_PARAM(usage);
    return -1;
}
#endif

void* OSAllocator::reserveUncommitted(size_t bytes, Usage usage, bool writable, bool executable)
{
#if OS(QNX)
    // Reserve memory with PROT_NONE and MAP_LAZY so it isn't committed now.
    void* result = mmap(0, bytes, PROT_NONE, MAP_LAZY | MAP_PRIVATE | MAP_ANON, -1, 0);
    if (result == MAP_FAILED)
        CRASH();
#elif OS(LINUX)
    UNUSED_PARAM(writable);
    UNUSED_PARAM(executable);
    int fd = memfdForUsage(bytes, usage);

    void* result = mmap(0, bytes, PROT_NONE, MAP_NORESERVE | MAP_PRIVATE |
                        (fd == -1 ? MAP_ANON : 0), fd, 0);
    if (result == MAP_FAILED)
        CRASH();
    madvise(result, bytes, MADV_DONTNEED);

    if (fd != -1)
        close(fd);
#else
    void* result = reserveAndCommit(bytes, usage, writable, executable);
#if HAVE(MADV_FREE_REUSE)
    // To support the "reserve then commit" model, we have to initially decommit.
    while (madvise(result, bytes, MADV_FREE_REUSABLE) == -1 && errno == EAGAIN) { }
#endif

#endif // OS(QNX)

    return result;
}

void* OSAllocator::reserveAndCommit(size_t bytes, Usage usage, bool writable, bool executable, bool includesGuardPages)
{
    // All POSIX reservations start out logically committed.
    int protection = PROT_READ;
    if (writable)
        protection |= PROT_WRITE;
    if (executable)
        protection |= PROT_EXEC;

    int flags = MAP_PRIVATE | MAP_ANON;
#if PLATFORM(IOS)
    if (executable)
        flags |= MAP_JIT;
#endif

#if OS(DARWIN)
    int fd = usage;
#elif OS(LINUX)
    int fd = memfdForUsage(bytes, usage);
    if (fd != -1)
        flags &= ~MAP_ANON;
#else
    UNUSED_PARAM(usage);
    int fd = -1;
#endif

    void* result = 0;
#if (OS(DARWIN) && CPU(X86_64))
    if (executable) {
        ASSERT(includesGuardPages);
        // Cook up an address to allocate at, using the following recipe:
        //   17 bits of zero, stay in userspace kids.
        //   26 bits of randomness for ASLR.
        //   21 bits of zero, at least stay aligned within one level of the pagetables.
        //
        // But! - as a temporary workaround for some plugin problems (rdar://problem/6812854),
        // for now instead of 2^26 bits of ASLR lets stick with 25 bits of randomization plus
        // 2^24, which should put up somewhere in the middle of userspace (in the address range
        // 0x200000000000 .. 0x5fffffffffff).
        intptr_t randomLocation = 0;
        randomLocation = arc4random() & ((1 << 25) - 1);
        randomLocation += (1 << 24);
        randomLocation <<= 21;
        result = reinterpret_cast<void*>(randomLocation);
    }
#endif

    result = mmap(result, bytes, protection, flags, fd, 0);
    if (result == MAP_FAILED) {
#if ENABLE(LLINT)
        if (executable)
            result = 0;
        else
#endif
            CRASH();
    }
    if (result && includesGuardPages) {
        // We use mmap to remap the guardpages rather than using mprotect as
        // mprotect results in multiple references to the code region.  This
        // breaks the madvise based mechanism we use to return physical memory
        // to the OS.
        mmap(result, pageSize(), PROT_NONE, MAP_FIXED | MAP_PRIVATE | MAP_ANON, fd, 0);
        mmap(static_cast<char*>(result) + bytes - pageSize(), pageSize(), PROT_NONE, MAP_FIXED | MAP_PRIVATE | MAP_ANON, fd, 0);
    }

#if OS(LINUX)
    if (fd != -1)
        close(fd);
#endif

    return result;
}

void OSAllocator::commit(void* address, size_t bytes, bool writable, bool executable)
{
#if OS(QNX)
    int protection = PROT_READ;
    if (writable)
        protection |= PROT_WRITE;
    if (executable)
        protection |= PROT_EXEC;
    if (MAP_FAILED == mmap(address, bytes, protection, MAP_FIXED | MAP_PRIVATE | MAP_ANON, -1, 0))
        CRASH();
#elif OS(LINUX)
    int protection = PROT_READ;
    if (writable)
        protection |= PROT_WRITE;
    if (executable)
        protection |= PROT_EXEC;
    if (mprotect(address, bytes, protection))
        CRASH();
    madvise(address, bytes, MADV_WILLNEED);
#elif HAVE(MADV_FREE_REUSE)
    UNUSED_PARAM(writable);
    UNUSED_PARAM(executable);
    while (madvise(address, bytes, MADV_FREE_REUSE) == -1 && errno == EAGAIN) { }
#else
    // Non-MADV_FREE_REUSE reservations automatically commit on demand.
    UNUSED_PARAM(address);
    UNUSED_PARAM(bytes);
    UNUSED_PARAM(writable);
    UNUSED_PARAM(executable);
#endif
}

void OSAllocator::decommit(void* address, size_t bytes)
{
#if OS(QNX)
    // Use PROT_NONE and MAP_LAZY to decommit the pages.
    mmap(address, bytes, PROT_NONE, MAP_FIXED | MAP_LAZY | MAP_PRIVATE | MAP_ANON, -1, 0);
#elif OS(LINUX)
    madvise(address, bytes, MADV_DONTNEED);
    if (mprotect(address, bytes, PROT_NONE))
        CRASH();
#elif HAVE(MADV_FREE_REUSE)
    while (madvise(address, bytes, MADV_FREE_REUSABLE) == -1 && errno == EAGAIN) { }
#elif HAVE(MADV_FREE)
    while (madvise(address, bytes, MADV_FREE) == -1 && errno == EAGAIN) { }
#elif HAVE(MADV_DONTNEED)
    while (madvise(address, bytes, MADV_DONTNEED) == -1 && errno == EAGAIN) { }
#else
    UNUSED_PARAM(address);
    UNUSED_PARAM(bytes);
#endif
}

void OSAllocator::releaseDecommitted(void* address, size_t bytes)
{
    int result = munmap(address, bytes);
    if (result == -1)
        CRASH();
}

bool OSAllocator::canAllocateExecutableMemory()
{
    int flags = MAP_PRIVATE | MAP_ANON;
#if PLATFORM(IOS)
    if (executable)
        flags |= MAP_JIT;
#endif
    const auto size = pageSize();
    void *testPage = mmap(nullptr, size, PROT_READ | PROT_WRITE | PROT_EXEC, flags, /*fd*/-1, /*offset*/0);
    if (testPage == MAP_FAILED)
        return false;
    munmap(testPage, size);
    return true;
}

} // namespace WTF

#endif // OS(UNIX)
