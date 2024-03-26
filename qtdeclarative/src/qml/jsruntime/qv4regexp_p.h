// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QV4REGEXP_H
#define QV4REGEXP_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QString>
#include <QVector>

#include <wtf/RefPtr.h>
#include <wtf/FastAllocBase.h>
#include <wtf/BumpPointerAllocator.h>

#include <limits.h>

#include <yarr/Yarr.h>
#include <yarr/YarrInterpreter.h>
#include <yarr/YarrJIT.h>

#include "qv4managed_p.h"
#include "qv4engine_p.h"

QT_BEGIN_NAMESPACE

namespace QV4 {

struct ExecutionEngine;
struct RegExpCacheKey;

namespace Heap {

struct RegExp : Base {
    void init(ExecutionEngine *engine, const QString& pattern, uint flags);
    void destroy();

    QString *pattern;
    JSC::Yarr::BytecodePattern *byteCode;
#if ENABLE(YARR_JIT)
    JSC::Yarr::YarrCodeBlock *jitCode;
#endif
    bool hasValidJITCode() const {
#if ENABLE(YARR_JIT)
        return jitCode && !jitCode->failureReason().has_value() && jitCode->has16BitCode();
#else
        return false;
#endif
    }

    bool ignoreCase() const { return flags & CompiledData::RegExp::RegExp_IgnoreCase; }
    bool multiLine() const { return flags & CompiledData::RegExp::RegExp_Multiline; }
    bool global() const { return flags & CompiledData::RegExp::RegExp_Global; }
    bool unicode() const { return flags & CompiledData::RegExp::RegExp_Unicode; }
    bool sticky() const { return flags & CompiledData::RegExp::RegExp_Sticky; }

    RegExpCache *cache;
    int subPatternCount;
    uint flags;
    bool valid;

    QString flagsAsString() const;
    int captureCount() const { return subPatternCount + 1; }
};
Q_STATIC_ASSERT(std::is_trivial_v<RegExp>);

}

struct RegExp : public Managed
{
    V4_MANAGED(RegExp, Managed)
    Q_MANAGED_TYPE(RegExp)
    V4_NEEDS_DESTROY
    V4_INTERNALCLASS(RegExp)

    QString pattern() const { return *d()->pattern; }
    JSC::Yarr::BytecodePattern *byteCode() { return d()->byteCode; }
#if ENABLE(YARR_JIT)
    JSC::Yarr::YarrCodeBlock *jitCode() const { return d()->jitCode; }
#endif
    RegExpCache *cache() const { return d()->cache; }
    int subPatternCount() const { return d()->subPatternCount; }
    bool ignoreCase() const { return d()->ignoreCase(); }
    bool multiLine() const { return d()->multiLine(); }
    bool global() const { return d()->global(); }
    bool unicode() const { return d()->unicode(); }
    bool sticky() const { return d()->sticky(); }

    static Heap::RegExp *create(ExecutionEngine* engine, const QString& pattern, uint flags = CompiledData::RegExp::RegExp_NoFlags);

    bool isValid() const { return d()->valid; }

    uint match(const QString& string, int start, uint *matchOffsets);

    int captureCount() const { return subPatternCount() + 1; }

    static QString getSubstitution(const QString &matched, const QString &str, int position, const Value *captures, int nCaptures, const QString &replacement);

    friend class RegExpCache;
};

struct RegExpCacheKey
{
    RegExpCacheKey(const QString &pattern, uint flags)
        : pattern(pattern), flags(flags)
    { }
    explicit inline RegExpCacheKey(const RegExp::Data *re);

    bool operator==(const RegExpCacheKey &other) const
    { return pattern == other.pattern && flags == other.flags;; }
    bool operator!=(const RegExpCacheKey &other) const
    { return !operator==(other); }

    QString pattern;
    uint flags;
};

inline RegExpCacheKey::RegExpCacheKey(const RegExp::Data *re)
    : pattern(*re->pattern)
    , flags(re->flags)
{}

inline size_t qHash(const RegExpCacheKey& key, size_t seed = 0) noexcept
{ return qHash(key.pattern, seed); }

class RegExpCache : public QHash<RegExpCacheKey, WeakValue>
{
public:
    ~RegExpCache();
};



}

QT_END_NAMESPACE

#endif // QV4REGEXP_H
