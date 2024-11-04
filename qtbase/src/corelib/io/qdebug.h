// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QDEBUG_H
#define QDEBUG_H

#if 0
#pragma qt_class(QtDebug)
#endif

#include <QtCore/qcontainerfwd.h>
#include <QtCore/qtextstream.h>
#include <QtCore/qtypes.h>
#include <QtCore/qstring.h>
#include <QtCore/qcontiguouscache.h>
#include <QtCore/qsharedpointer.h>

// all these have already been included by various headers above, but don't rely on indirect includes:
#include <chrono>
#include <list>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#if !defined(QT_LEAN_HEADERS) || QT_LEAN_HEADERS < 1
#  include <QtCore/qlist.h>
#  include <QtCore/qmap.h>
#  include <QtCore/qset.h>
#  include <QtCore/qvarlengtharray.h>
#endif

QT_BEGIN_NAMESPACE

class QT6_ONLY(Q_CORE_EXPORT) QDebug : public QIODeviceBase
{
    friend class QMessageLogger;
    friend class QDebugStateSaver;
    friend class QDebugStateSaverPrivate;
    struct Stream {
        enum { VerbosityShift = 29, VerbosityMask = 0x7 };

        Stream(QIODevice *device)
            : ts(device)
        {}
        Stream(QString *string)
            : ts(string, WriteOnly)
        {}
        Stream(QtMsgType t)
            : ts(&buffer, WriteOnly),
              type(t),
              message_output(true)
        {}
        QTextStream ts;
        QString buffer;
        int ref = 1;
        QtMsgType type = QtDebugMsg;
        bool space = true;
        bool noQuotes = false;
        bool message_output = false;
        int verbosity = DefaultVerbosity;
        QMessageLogContext context;
    } *stream;

    enum Latin1Content { ContainsBinary = 0, ContainsLatin1 };

    QT7_ONLY(Q_CORE_EXPORT) void putUcs4(uint ucs4);
    QT7_ONLY(Q_CORE_EXPORT) void putString(const QChar *begin, size_t length);
    QT7_ONLY(Q_CORE_EXPORT) void putByteArray(const char *begin, size_t length, Latin1Content content);
    QT7_ONLY(Q_CORE_EXPORT) void putTimeUnit(qint64 num, qint64 den);
    QT7_ONLY(Q_CORE_EXPORT) void putInt128(const void *i);
    QT7_ONLY(Q_CORE_EXPORT) void putUInt128(const void *i);
public:
    explicit QDebug(QIODevice *device) : stream(new Stream(device)) {}
    explicit QDebug(QString *string) : stream(new Stream(string)) {}
    explicit QDebug(QtMsgType t) : stream(new Stream(t)) {}
    QDebug(const QDebug &o) : stream(o.stream) { ++stream->ref; }
    QDebug(QDebug &&other) noexcept : stream{std::exchange(other.stream, nullptr)} {}
    inline QDebug &operator=(const QDebug &other);
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(QDebug)
    ~QDebug();
    void swap(QDebug &other) noexcept { qt_ptr_swap(stream, other.stream); }

    QT7_ONLY(Q_CORE_EXPORT) QDebug &resetFormat();

    inline QDebug &space() { stream->space = true; stream->ts << ' '; return *this; }
    inline QDebug &nospace() { stream->space = false; return *this; }
    inline QDebug &maybeSpace() { if (stream->space) stream->ts << ' '; return *this; }
    inline QDebug &verbosity(int verbosityLevel) { stream->verbosity = verbosityLevel; return *this; }
    int verbosity() const { return stream->verbosity; }
    void setVerbosity(int verbosityLevel) { stream->verbosity = verbosityLevel; }
    enum VerbosityLevel { MinimumVerbosity = 0, DefaultVerbosity = 2, MaximumVerbosity = 7 };

    bool autoInsertSpaces() const { return stream->space; }
    void setAutoInsertSpaces(bool b) { stream->space = b; }

    [[nodiscard]] bool quoteStrings() const noexcept { return !stream->noQuotes; }
    void setQuoteStrings(bool b) { stream->noQuotes = !b; }

    inline QDebug &quote() { stream->noQuotes = false; return *this; }
    inline QDebug &noquote() { stream->noQuotes = true; return *this; }
    inline QDebug &maybeQuote(char c = '"') { if (!stream->noQuotes) stream->ts << c; return *this; }

    inline QDebug &operator<<(QChar t) { putUcs4(t.unicode()); return maybeSpace(); }
    inline QDebug &operator<<(bool t) { stream->ts << (t ? "true" : "false"); return maybeSpace(); }
    inline QDebug &operator<<(char t) { stream->ts << t; return maybeSpace(); }
    inline QDebug &operator<<(signed short t) { stream->ts << t; return maybeSpace(); }
    inline QDebug &operator<<(unsigned short t) { stream->ts << t; return maybeSpace(); }
    inline QDebug &operator<<(char16_t t) { return *this << QChar(t); }
    inline QDebug &operator<<(char32_t t) { putUcs4(t); return maybeSpace(); }
    inline QDebug &operator<<(signed int t) { stream->ts << t; return maybeSpace(); }
    inline QDebug &operator<<(unsigned int t) { stream->ts << t; return maybeSpace(); }
    inline QDebug &operator<<(signed long t) { stream->ts << t; return maybeSpace(); }
    inline QDebug &operator<<(unsigned long t) { stream->ts << t; return maybeSpace(); }
    inline QDebug &operator<<(qint64 t) { stream->ts << t; return maybeSpace(); }
    inline QDebug &operator<<(quint64 t) { stream->ts << t; return maybeSpace(); }
    inline QDebug &operator<<(qfloat16 t) { stream->ts << t; return maybeSpace(); }
    inline QDebug &operator<<(float t) { stream->ts << t; return maybeSpace(); }
    inline QDebug &operator<<(double t) { stream->ts << t; return maybeSpace(); }
    inline QDebug &operator<<(const char* t) { stream->ts << QString::fromUtf8(t); return maybeSpace(); }
    inline QDebug &operator<<(const char16_t *t)  { stream->ts << QStringView(t); return maybeSpace(); }
    inline QDebug &operator<<(const QString & t) { putString(t.constData(), size_t(t.size())); return maybeSpace(); }
    inline QDebug &operator<<(QStringView s) { putString(s.data(), size_t(s.size())); return maybeSpace(); }
    inline QDebug &operator<<(QUtf8StringView s) { putByteArray(reinterpret_cast<const char*>(s.data()), s.size(), ContainsBinary); return maybeSpace(); }
    inline QDebug &operator<<(QLatin1StringView t) { putByteArray(t.latin1(), t.size(), ContainsLatin1); return maybeSpace(); }
    inline QDebug &operator<<(const QByteArray & t) { putByteArray(t.constData(), t.size(), ContainsBinary); return maybeSpace(); }
    inline QDebug &operator<<(QByteArrayView t) { putByteArray(t.constData(), t.size(), ContainsBinary); return maybeSpace(); }
    inline QDebug &operator<<(const void * t) { stream->ts << t; return maybeSpace(); }
    inline QDebug &operator<<(std::nullptr_t) { stream->ts << "(nullptr)"; return maybeSpace(); }
    inline QDebug &operator<<(std::nullopt_t) { stream->ts << "nullopt"; return maybeSpace(); }
    inline QDebug &operator<<(QTextStreamFunction f) {
        stream->ts << f;
        return *this;
    }

    inline QDebug &operator<<(QTextStreamManipulator m)
    { stream->ts << m; return *this; }

#ifdef Q_QDOC
    template <typename Char, typename...Args>
    QDebug &operator<<(const std::basic_string<Char, Args...> &s);

    template <typename Char, typename...Args>
    QDebug &operator<<(std::basic_string_view<Char, Args...> s);
#else
    template <typename...Args>
    QDebug &operator<<(const std::basic_string<char, Args...> &s)
    { return *this << QUtf8StringView(s); }

    template <typename...Args>
    QDebug &operator<<(std::basic_string_view<char, Args...> s)
    { return *this << QUtf8StringView(s); }

#ifdef __cpp_char8_t
    template <typename...Args>
    QDebug &operator<<(const std::basic_string<char8_t, Args...> &s)
    { return *this << QUtf8StringView(s); }

    template <typename...Args>
    QDebug &operator<<(std::basic_string_view<char8_t, Args...> s)
    { return *this << QUtf8StringView(s); }
#endif // __cpp_char8_t

    template <typename...Args>
    QDebug &operator<<(const std::basic_string<char16_t, Args...> &s)
    { return *this << QStringView(s); }

    template <typename...Args>
    QDebug &operator<<(std::basic_string_view<char16_t, Args...> s)
    { return *this << QStringView(s); }

    template <typename...Args>
    QDebug &operator<<(const std::basic_string<wchar_t, Args...> &s)
    {
        if constexpr (sizeof(wchar_t) == 2)
            return *this << QStringView(s);
        else
            return *this << QString::fromWCharArray(s.data(), s.size()); // ### optimize
    }

    template <typename...Args>
    QDebug &operator<<(std::basic_string_view<wchar_t, Args...> s)
    {
        if constexpr (sizeof(wchar_t) == 2)
            return *this << QStringView(s);
        else
            return *this << QString::fromWCharArray(s.data(), s.size()); // ### optimize
    }

    template <typename...Args>
    QDebug &operator<<(const std::basic_string<char32_t, Args...> &s)
    { return *this << QString::fromUcs4(s.data(), s.size()); }

    template <typename...Args>
    QDebug &operator<<(std::basic_string_view<char32_t, Args...> s)
    { return *this << QString::fromUcs4(s.data(), s.size()); }
#endif // !Q_QDOC

    template <typename Rep, typename Period>
    QDebug &operator<<(std::chrono::duration<Rep, Period> duration)
    {
        stream->ts << duration.count();
        putTimeUnit(Period::num, Period::den);
        return maybeSpace();
    }

#ifdef QT_SUPPORTS_INT128
private:
    // Constrained templates so they only match q(u)int128 without conversions.
    // Also keeps these operators out of the ABI.
    template <typename T>
    using if_qint128 = std::enable_if_t<std::is_same_v<T, qint128>, bool>;
    template <typename T>
    using if_quint128 = std::enable_if_t<std::is_same_v<T, quint128>, bool>;
public:
    template <typename T, if_qint128<T> = true>
    QDebug &operator<<(T i128) { putInt128(&i128); return maybeSpace(); }
    template <typename T, if_quint128<T> = true>
    QDebug &operator<<(T u128) { putUInt128(&u128); return maybeSpace(); }
#endif // QT_SUPPORTS_INT128

    template <typename T>
    static QString toString(T &&object)
    {
        QString buffer;
        QDebug stream(&buffer);
        stream.nospace() << std::forward<T>(object);
        return buffer;
    }
};

Q_DECLARE_SHARED(QDebug)

class QDebugStateSaverPrivate;
class QDebugStateSaver
{
public:
    Q_NODISCARD_CTOR Q_CORE_EXPORT
    QDebugStateSaver(QDebug &dbg);
    Q_CORE_EXPORT
    ~QDebugStateSaver();
private:
    Q_DISABLE_COPY(QDebugStateSaver)
    QScopedPointer<QDebugStateSaverPrivate> d;
};

class QNoDebug
{
public:
    inline QNoDebug &operator<<(QTextStreamFunction) { return *this; }
    inline QNoDebug &operator<<(QTextStreamManipulator) { return *this; }
    inline QNoDebug &space() { return *this; }
    inline QNoDebug &nospace() { return *this; }
    inline QNoDebug &maybeSpace() { return *this; }
    inline QNoDebug &quote() { return *this; }
    inline QNoDebug &noquote() { return *this; }
    inline QNoDebug &maybeQuote(const char = '"') { return *this; }
    inline QNoDebug &verbosity(int) { return *this; }

    template<typename T>
    inline QNoDebug &operator<<(const T &) { return *this; }
};

inline QDebug &QDebug::operator=(const QDebug &other)
{
    QDebug{other}.swap(*this);
    return *this;
}

namespace QtPrivate {

template <typename SequentialContainer>
inline QDebug printSequentialContainer(QDebug debug, const char *which, const SequentialContainer &c)
{
    const QDebugStateSaver saver(debug);
    debug.nospace() << which << '(';
    typename SequentialContainer::const_iterator it = c.begin(), end = c.end();
    if (it != end) {
        debug << *it;
        ++it;
    }
    while (it != end) {
        debug << ", " << *it;
        ++it;
    }
    debug << ')';
    return debug;
}

template <typename AssociativeContainer>
inline QDebug printAssociativeContainer(QDebug debug, const char *which, const AssociativeContainer &c)
{
    const QDebugStateSaver saver(debug);
    debug.nospace() << which << "(";
    for (typename AssociativeContainer::const_iterator it = c.constBegin();
         it != c.constEnd(); ++it) {
        debug << '(' << it.key() << ", " << it.value() << ')';
    }
    debug << ')';
    return debug;
}

} // namespace QtPrivate

template<typename ...T>
using QDebugIfHasDebugStream =
    std::enable_if_t<std::conjunction_v<QTypeTraits::has_ostream_operator<QDebug, T>...>, QDebug>;

template<typename Container, typename ...T>
using QDebugIfHasDebugStreamContainer =
    std::enable_if_t<std::conjunction_v<QTypeTraits::has_ostream_operator_container<QDebug, Container, T>...>, QDebug>;

#ifndef Q_QDOC

template<typename T>
inline QDebugIfHasDebugStreamContainer<QList<T>, T> operator<<(QDebug debug, const QList<T> &vec)
{
    return QtPrivate::printSequentialContainer(std::move(debug), "QList", vec);
}

template<typename T, qsizetype P>
inline QDebugIfHasDebugStream<T> operator<<(QDebug debug, const QVarLengthArray<T, P> &vec)
{
    return QtPrivate::printSequentialContainer(std::move(debug), "QVarLengthArray", vec);
}

template <typename T, typename Alloc>
inline QDebugIfHasDebugStream<T> operator<<(QDebug debug, const std::vector<T, Alloc> &vec)
{
    return QtPrivate::printSequentialContainer(std::move(debug), "std::vector", vec);
}

template <typename T, typename Alloc>
inline QDebugIfHasDebugStream<T> operator<<(QDebug debug, const std::list<T, Alloc> &vec)
{
    return QtPrivate::printSequentialContainer(std::move(debug), "std::list", vec);
}

template <typename T>
inline QDebugIfHasDebugStream<T> operator<<(QDebug debug, std::initializer_list<T> list)
{
    return QtPrivate::printSequentialContainer(std::move(debug), "std::initializer_list", list);
}

template <typename Key, typename T, typename Compare, typename Alloc>
inline QDebugIfHasDebugStream<Key, T> operator<<(QDebug debug, const std::map<Key, T, Compare, Alloc> &map)
{
    return QtPrivate::printSequentialContainer(std::move(debug), "std::map", map); // yes, sequential: *it is std::pair
}

template <typename Key, typename T, typename Compare, typename Alloc>
inline QDebugIfHasDebugStream<Key, T> operator<<(QDebug debug, const std::multimap<Key, T, Compare, Alloc> &map)
{
    return QtPrivate::printSequentialContainer(std::move(debug), "std::multimap", map); // yes, sequential: *it is std::pair
}

template <class Key, class T>
inline QDebugIfHasDebugStreamContainer<QMap<Key, T>, Key, T> operator<<(QDebug debug, const QMap<Key, T> &map)
{
    return QtPrivate::printAssociativeContainer(std::move(debug), "QMap", map);
}

template <class Key, class T>
inline QDebugIfHasDebugStreamContainer<QMultiMap<Key, T>, Key, T> operator<<(QDebug debug, const QMultiMap<Key, T> &map)
{
    return QtPrivate::printAssociativeContainer(std::move(debug), "QMultiMap", map);
}

template <class Key, class T>
inline QDebugIfHasDebugStreamContainer<QHash<Key, T>, Key, T> operator<<(QDebug debug, const QHash<Key, T> &hash)
{
    return QtPrivate::printAssociativeContainer(std::move(debug), "QHash", hash);
}

template <class Key, class T>
inline QDebugIfHasDebugStreamContainer<QMultiHash<Key, T>, Key, T> operator<<(QDebug debug, const QMultiHash<Key, T> &hash)
{
    return QtPrivate::printAssociativeContainer(std::move(debug), "QMultiHash", hash);
}

template <class T>
inline QDebugIfHasDebugStream<T> operator<<(QDebug debug, const std::optional<T> &opt)
{
    const QDebugStateSaver saver(debug);
    if (!opt)
        debug.nospace() << std::nullopt;
    else
        debug.nospace() << "std::optional(" << *opt << ')';
    return debug;
}

template <class T1, class T2>
inline QDebugIfHasDebugStream<T1, T2> operator<<(QDebug debug, const std::pair<T1, T2> &pair)
{
    const QDebugStateSaver saver(debug);
    debug.nospace() << "std::pair(" << pair.first << ',' << pair.second << ')';
    return debug;
}

template <typename T>
inline QDebugIfHasDebugStreamContainer<QSet<T>, T> operator<<(QDebug debug, const QSet<T> &set)
{
    return QtPrivate::printSequentialContainer(std::move(debug), "QSet", set);
}

template <class T>
inline QDebugIfHasDebugStream<T> operator<<(QDebug debug, const QContiguousCache<T> &cache)
{
    const QDebugStateSaver saver(debug);
    debug.nospace() << "QContiguousCache(";
    for (qsizetype i = cache.firstIndex(); i <= cache.lastIndex(); ++i) {
        debug << cache[i];
        if (i != cache.lastIndex())
            debug << ", ";
    }
    debug << ')';
    return debug;
}

#else
template <class T>
QDebug operator<<(QDebug debug, const QList<T> &list);

template <class T, qsizetype P>
QDebug operator<<(QDebug debug, const QVarLengthArray<T, P> &array);

template <typename T, typename Alloc>
QDebug operator<<(QDebug debug, const std::vector<T, Alloc> &vec);

template <typename T, typename Alloc>
QDebug operator<<(QDebug debug, const std::list<T, Alloc> &vec);

template <typename Key, typename T, typename Compare, typename Alloc>
QDebug operator<<(QDebug debug, const std::map<Key, T, Compare, Alloc> &map);

template <typename Key, typename T, typename Compare, typename Alloc>
QDebug operator<<(QDebug debug, const std::multimap<Key, T, Compare, Alloc> &map);

template <class Key, class T>
QDebug operator<<(QDebug debug, const QMap<Key, T> &map);

template <class Key, class T>
QDebug operator<<(QDebug debug, const QMultiMap<Key, T> &map);

template <class Key, class T>
QDebug operator<<(QDebug debug, const QHash<Key, T> &hash);

template <class Key, class T>
QDebug operator<<(QDebug debug, const QMultiHash<Key, T> &hash);

template <typename T>
QDebug operator<<(QDebug debug, const QSet<T> &set);

template <class T1, class T2>
QDebug operator<<(QDebug debug, const std::pair<T1, T2> &pair);

template <typename T>
QDebug operator<<(QDebug debug, const QContiguousCache<T> &cache);

#endif // Q_QDOC

template <class T>
inline QDebug operator<<(QDebug debug, const QSharedPointer<T> &ptr)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "QSharedPointer(" << ptr.data() << ")";
    return debug;
}

template <typename T, typename Tag> class QTaggedPointer;

template <typename T, typename Tag>
inline QDebug operator<<(QDebug debug, const QTaggedPointer<T, Tag> &ptr)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "QTaggedPointer(" << ptr.pointer() << ", " << ptr.tag() << ")";
    return debug;
}

Q_CORE_EXPORT void qt_QMetaEnum_flagDebugOperator(QDebug &debug, size_t sizeofT, int value);

template <typename Int>
void qt_QMetaEnum_flagDebugOperator(QDebug &debug, size_t sizeofT, Int value)
{
    const QDebugStateSaver saver(debug);
    debug.resetFormat();
    debug.nospace() << "QFlags(" << Qt::hex << Qt::showbase;
    bool needSeparator = false;
    for (size_t i = 0; i < sizeofT * 8; ++i) {
        if (value & (Int(1) << i)) {
            if (needSeparator)
                debug << '|';
            else
                needSeparator = true;
            debug << (Int(1) << i);
        }
    }
    debug << ')';
}

#if !defined(QT_NO_QOBJECT) && !defined(Q_QDOC)
Q_CORE_EXPORT QDebug qt_QMetaEnum_debugOperator(QDebug&, qint64 value, const QMetaObject *meta, const char *name);
Q_CORE_EXPORT QDebug qt_QMetaEnum_flagDebugOperator(QDebug &dbg, quint64 value, const QMetaObject *meta, const char *name);

template<typename T>
typename std::enable_if<QtPrivate::IsQEnumHelper<T>::Value, QDebug>::type
operator<<(QDebug dbg, T value)
{
    const QMetaObject *obj = qt_getEnumMetaObject(value);
    const char *name = qt_getEnumName(value);
    return qt_QMetaEnum_debugOperator(dbg, static_cast<typename std::underlying_type<T>::type>(value), obj, name);
}

template<typename T,
         typename A = typename std::enable_if<std::is_enum<T>::value, void>::type,
         typename B = typename std::enable_if<sizeof(T) <= sizeof(int), void>::type,
         typename C = typename std::enable_if<!QtPrivate::IsQEnumHelper<T>::Value, void>::type,
         typename D = typename std::enable_if<QtPrivate::IsQEnumHelper<QFlags<T>>::Value, void>::type>
inline QDebug operator<<(QDebug dbg, T value)
{
    typedef QFlags<T> FlagsT;
    const QMetaObject *obj = qt_getEnumMetaObject(FlagsT());
    const char *name = qt_getEnumName(FlagsT());
    return qt_QMetaEnum_debugOperator(dbg, typename FlagsT::Int(value), obj, name);
}

template <class T>
inline typename std::enable_if<
    QtPrivate::IsQEnumHelper<T>::Value || QtPrivate::IsQEnumHelper<QFlags<T> >::Value,
    QDebug>::type
qt_QMetaEnum_flagDebugOperator_helper(QDebug debug, const QFlags<T> &flags)
{
    const QMetaObject *obj = qt_getEnumMetaObject(T());
    const char *name = qt_getEnumName(T());
    return qt_QMetaEnum_flagDebugOperator(debug, flags.toInt(), obj, name);
}

template <class T>
inline typename std::enable_if<
    !QtPrivate::IsQEnumHelper<T>::Value && !QtPrivate::IsQEnumHelper<QFlags<T> >::Value,
    QDebug>::type
qt_QMetaEnum_flagDebugOperator_helper(QDebug debug, const QFlags<T> &flags)
#else // !QT_NO_QOBJECT && !Q_QDOC
template <class T>
inline QDebug qt_QMetaEnum_flagDebugOperator_helper(QDebug debug, const QFlags<T> &flags)
#endif
{
    qt_QMetaEnum_flagDebugOperator(debug, sizeof(T), typename QFlags<T>::Int(flags));
    return debug;
}

template<typename T>
inline QDebug operator<<(QDebug debug, const QFlags<T> &flags)
{
    // We have to use an indirection otherwise specialisation of some other overload of the
    // operator<< the compiler would try to instantiate QFlags<T> for the std::enable_if
    return qt_QMetaEnum_flagDebugOperator_helper(debug, flags);
}

inline QDebug operator<<(QDebug debug, QKeyCombination combination)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "QKeyCombination("
                    << combination.keyboardModifiers()
                    << ", "
                    << combination.key()
                    << ")";
    return debug;
}

#ifdef Q_OS_DARWIN

// We provide QDebug stream operators for commonly used Core Foundation
// and Core Graphics types, as well as NSObject. Additional CF/CG types
// may be added by the user, using Q_DECLARE_QDEBUG_OPERATOR_FOR_CF_TYPE.

#define QT_FOR_EACH_CORE_FOUNDATION_TYPE(F) \
    F(CFArray) \
    F(CFURL) \
    F(CFData) \
    F(CFNumber) \
    F(CFDictionary) \
    F(CFLocale) \
    F(CFDate) \
    F(CFBoolean) \
    F(CFTimeZone) \

#define QT_FOR_EACH_MUTABLE_CORE_FOUNDATION_TYPE(F) \
    F(CFError) \
    F(CFBundle) \

#define QT_FOR_EACH_CORE_GRAPHICS_TYPE(F) \
    F(CGPath) \

#define QT_FOR_EACH_MUTABLE_CORE_GRAPHICS_TYPE(F) \
    F(CGColorSpace) \
    F(CGImage) \
    F(CGFont) \
    F(CGColor) \

#define QT_FORWARD_DECLARE_CF_TYPE(type) Q_FORWARD_DECLARE_CF_TYPE(type);
#define QT_FORWARD_DECLARE_MUTABLE_CF_TYPE(type) Q_FORWARD_DECLARE_MUTABLE_CF_TYPE(type);
#define QT_FORWARD_DECLARE_CG_TYPE(type) Q_FORWARD_DECLARE_CG_TYPE(type);
#define QT_FORWARD_DECLARE_MUTABLE_CG_TYPE(type) Q_FORWARD_DECLARE_MUTABLE_CG_TYPE(type);

QT_END_NAMESPACE
Q_FORWARD_DECLARE_CF_TYPE(CFString);
struct objc_object;
Q_FORWARD_DECLARE_OBJC_CLASS(NSObject);
QT_FOR_EACH_CORE_FOUNDATION_TYPE(QT_FORWARD_DECLARE_CF_TYPE)
QT_FOR_EACH_MUTABLE_CORE_FOUNDATION_TYPE(QT_FORWARD_DECLARE_MUTABLE_CF_TYPE)
QT_FOR_EACH_CORE_GRAPHICS_TYPE(QT_FORWARD_DECLARE_CG_TYPE)
QT_FOR_EACH_MUTABLE_CORE_GRAPHICS_TYPE(QT_FORWARD_DECLARE_MUTABLE_CG_TYPE)
QT_BEGIN_NAMESPACE

#define QT_FORWARD_DECLARE_QDEBUG_OPERATOR_FOR_CF_TYPE(CFType) \
    Q_CORE_EXPORT QDebug operator<<(QDebug, CFType##Ref);

#define Q_DECLARE_QDEBUG_OPERATOR_FOR_CF_TYPE(CFType) \
    QDebug operator<<(QDebug debug, CFType##Ref ref) \
    { \
        if (!ref) \
            return debug << QT_STRINGIFY(CFType) "Ref(0x0)"; \
        if (CFStringRef description = CFCopyDescription(ref)) { \
            QDebugStateSaver saver(debug); \
            debug.noquote() << description; \
            CFRelease(description); \
        } \
        return debug; \
    }

// Defined in qcore_mac_objc.mm
#if defined(__OBJC__)
Q_CORE_EXPORT QDebug operator<<(QDebug, id);
#endif
Q_CORE_EXPORT QDebug operator<<(QDebug, objc_object *);
Q_CORE_EXPORT QDebug operator<<(QDebug, const NSObject *);
Q_CORE_EXPORT QDebug operator<<(QDebug, CFStringRef);

QT_FOR_EACH_CORE_FOUNDATION_TYPE(QT_FORWARD_DECLARE_QDEBUG_OPERATOR_FOR_CF_TYPE)
QT_FOR_EACH_MUTABLE_CORE_FOUNDATION_TYPE(QT_FORWARD_DECLARE_QDEBUG_OPERATOR_FOR_CF_TYPE)
QT_FOR_EACH_CORE_GRAPHICS_TYPE(QT_FORWARD_DECLARE_QDEBUG_OPERATOR_FOR_CF_TYPE)
QT_FOR_EACH_MUTABLE_CORE_GRAPHICS_TYPE(QT_FORWARD_DECLARE_QDEBUG_OPERATOR_FOR_CF_TYPE)

#undef QT_FORWARD_DECLARE_CF_TYPE
#undef QT_FORWARD_DECLARE_MUTABLE_CF_TYPE
#undef QT_FORWARD_DECLARE_CG_TYPE
#undef QT_FORWARD_DECLARE_MUTABLE_CG_TYPE

#endif // Q_OS_DARWIN

QT_END_NAMESPACE

#endif // QDEBUG_H
