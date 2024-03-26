// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QANYSTRINGVIEW_H
#define QANYSTRINGVIEW_H

#include <QtCore/qlatin1stringview.h>
#include <QtCore/qstringview.h>
#include <QtCore/qutf8stringview.h>

#ifdef __cpp_impl_three_way_comparison
#include <compare>
#endif
#include <QtCore/q20type_traits.h>
#include <limits>

class tst_QAnyStringView;

QT_BEGIN_NAMESPACE

namespace QtPrivate {

template <typename Tag, typename Result>
struct wrapped { using type = Result; };

template <typename Tag, typename Result>
using wrapped_t = typename wrapped<Tag, Result>::type;

} // namespace QtPrivate

class QAnyStringView
{
public:
    typedef qptrdiff difference_type;
    typedef qsizetype size_type;
private:
    static constexpr size_t SizeMask = (std::numeric_limits<size_t>::max)() / 4;
#if QT_VERSION >= QT_VERSION_CHECK(7, 0, 0) || defined(QT_BOOTSTRAPPED)
    static constexpr int SizeShift = 2;
    static constexpr size_t Latin1Flag = 1;
#else
    static constexpr int SizeShift = 0;
    static constexpr size_t Latin1Flag = SizeMask + 1;
#endif
    static constexpr size_t TwoByteCodePointFlag = Latin1Flag << 1;
    static constexpr size_t TypeMask = ~(SizeMask << SizeShift);
    static_assert(TypeMask == (Latin1Flag|TwoByteCodePointFlag));

    // Tag bits
    //  0  0   Utf8
    //  0  1   Latin1
    //  1  0   Utf16
    //  1  1   Unused
    //  ^  ^ latin1
    //  | sizeof code-point == 2
    enum Tag : size_t {
        Utf8     = 0,
        Latin1   = Latin1Flag,
        Utf16    = TwoByteCodePointFlag,
        Unused   = TypeMask,
    };

    template <typename Char>
    using if_compatible_char = std::enable_if_t<std::disjunction_v<
        QtPrivate::IsCompatibleCharType<Char>,
        QtPrivate::IsCompatibleChar8Type<Char>
    >, bool>;

    template <typename Pointer>
    using if_compatible_pointer = std::enable_if_t<std::disjunction_v<
        QtPrivate::IsCompatiblePointer<Pointer>,
        QtPrivate::IsCompatiblePointer8<Pointer>
    >, bool>;


    template <typename T>
    using if_compatible_container = std::enable_if_t<std::disjunction_v<
        QtPrivate::IsContainerCompatibleWithQStringView<T>,
        QtPrivate::IsContainerCompatibleWithQUtf8StringView<T>
    >, bool>;

    template <typename QStringOrQByteArray, typename T>
    using if_convertible_to = std::enable_if_t<std::conjunction_v<
        // need to exclude a bunch of stuff, because we take by universal reference:
        std::negation<std::disjunction<
            std::is_same<q20::remove_cvref_t<T>, QAnyStringView::Tag>,
            std::is_same<q20::remove_cvref_t<T>, QAnyStringView>, // don't make a copy/move ctor
            std::is_pointer<std::decay_t<T>>, // const char*, etc
            std::is_same<q20::remove_cvref_t<T>, QByteArray>,
            std::is_same<q20::remove_cvref_t<T>, QString>
        >>,
        // this is what we're really after:
        std::is_convertible<T, QStringOrQByteArray>
    >, bool>;

    // confirm we don't make an accidental copy constructor:
    static_assert(QtPrivate::IsContainerCompatibleWithQStringView<QAnyStringView>::value == false);
    static_assert(QtPrivate::IsContainerCompatibleWithQUtf8StringView<QAnyStringView>::value == false);

    template<typename Char>
    static constexpr bool isAsciiOnlyCharsAtCompileTime(Char *str, qsizetype sz) noexcept
    {
        // do not perform check if not at compile time
#if !(defined(__cpp_lib_is_constant_evaluated) || defined(Q_CC_GNU))
        Q_UNUSED(str);
        Q_UNUSED(sz);
        return false;
#else
#  if defined(__cpp_lib_is_constant_evaluated)
        if (!std::is_constant_evaluated())
            return false;
#  elif defined(Q_CC_GNU) && !defined(Q_CC_CLANG)
        if (!str || !__builtin_constant_p(*str))
            return false;
#  endif
        if constexpr (sizeof(Char) != sizeof(char)) {
            Q_UNUSED(str);
            Q_UNUSED(sz);
            return false;
        } else {
            for (qsizetype i = 0; i < sz; ++i) {
                if (uchar(str[i]) > 0x7f)
                    return false;
            }
            return true;
        }
#endif
    }

    template<typename Char>
    static constexpr std::size_t encodeType(const Char *str, qsizetype sz) noexcept
    {
        // Utf16 if 16 bit, Latin1 if ASCII, else Utf8
        Q_ASSERT(sz >= 0);
        Q_ASSERT(sz <= qsizetype(SizeMask));
        Q_ASSERT(str || !sz);
        return (std::size_t(sz) << SizeShift)
                | uint(sizeof(Char) == sizeof(char16_t)) * Tag::Utf16
                | uint(isAsciiOnlyCharsAtCompileTime(str, sz)) * Tag::Latin1;
    }

    template <typename Char>
    static qsizetype lengthHelperPointer(const Char *str) noexcept
    {
#if defined(Q_CC_GNU) && !defined(Q_CC_CLANG)
        if (__builtin_constant_p(*str)) {
            qsizetype result = 0;
            while (*str++ != u'\0')
                ++result;
            return result;
        }
#endif
        if constexpr (sizeof(Char) == sizeof(char16_t))
            return QtPrivate::qustrlen(reinterpret_cast<const char16_t*>(str));
        else
            return qsizetype(strlen(reinterpret_cast<const char*>(str)));
    }

    template <typename Container>
    static constexpr qsizetype lengthHelperContainer(const Container &c) noexcept
    {
        return qsizetype(std::size(c));
    }

    template <typename Char, size_t N>
    static constexpr qsizetype lengthHelperContainer(const Char (&str)[N]) noexcept
    {
        const auto it = std::char_traits<Char>::find(str, N, Char(0));
        const auto end = it ? it : std::next(str, N);
        return qsizetype(std::distance(str, end));
    }

    static QChar toQChar(char ch) noexcept { return toQChar(QLatin1Char{ch}); } // we don't handle UTF-8 multibytes
    static QChar toQChar(QChar ch) noexcept { return ch; }
    static QChar toQChar(QLatin1Char ch) noexcept { return ch; }

    explicit constexpr QAnyStringView(const void *d, qsizetype n, std::size_t sizeAndType) noexcept
        : m_data{d}, m_size{std::size_t(n) | (sizeAndType & TypeMask)} {}
public:
    constexpr QAnyStringView() noexcept
        : m_data{nullptr}, m_size{0} {}
    constexpr QAnyStringView(std::nullptr_t) noexcept
        : QAnyStringView() {}

    template <typename Char, if_compatible_char<Char> = true>
    constexpr QAnyStringView(const Char *str, qsizetype len)
        : m_data{str}, m_size{encodeType<Char>(str, len)}
    {
    }

    template <typename Char, if_compatible_char<Char> = true>
    constexpr QAnyStringView(const Char *f, const Char *l)
        : QAnyStringView(f, l - f) {}

#ifdef Q_QDOC
    template <typename Char, size_t N>
    constexpr QAnyStringView(const Char (&array)[N]) noexcept;

    template <typename Char>
    constexpr QAnyStringView(const Char *str) noexcept;
#else

    template <typename Pointer, if_compatible_pointer<Pointer> = true>
    constexpr QAnyStringView(const Pointer &str) noexcept
        : QAnyStringView{str, str ? lengthHelperPointer(str) : 0} {}
#endif

    // defined in qstring.h
    inline QAnyStringView(const QByteArray &str) noexcept; // TODO: Should we have this at all? Remove?
    inline QAnyStringView(const QString &str) noexcept;
    inline constexpr QAnyStringView(QLatin1StringView str) noexcept;

    template <typename Container, if_compatible_container<Container> = true>
    constexpr QAnyStringView(const Container &c) noexcept
        : QAnyStringView(std::data(c), lengthHelperContainer(c)) {}

    template <typename Container, if_convertible_to<QString, Container> = true>
    constexpr QAnyStringView(Container &&c, QtPrivate::wrapped_t<Container, QString> &&capacity = {})
            //noexcept(std::is_nothrow_constructible_v<QString, Container>)
        : QAnyStringView(capacity = std::forward<Container>(c)) {}

    template <typename Container, if_convertible_to<QByteArray, Container> = true>
    constexpr QAnyStringView(Container &&c, QtPrivate::wrapped_t<Container, QByteArray> &&capacity = {})
            //noexcept(std::is_nothrow_constructible_v<QByteArray, Container>)
        : QAnyStringView(capacity = std::forward<Container>(c)) {}

    template <typename Char, if_compatible_char<Char> = true>
    constexpr QAnyStringView(const Char &c) noexcept
        : QAnyStringView{&c, 1} {}
    constexpr QAnyStringView(const QChar &c) noexcept
        : QAnyStringView{&c, 1} {}

    template <typename Char, typename Container = decltype(QChar::fromUcs4(U'x')),
              std::enable_if_t<std::is_same_v<Char, char32_t>, bool> = true>
    constexpr QAnyStringView(Char c, Container &&capacity = {})
        : QAnyStringView(capacity = QChar::fromUcs4(c)) {}

    constexpr QAnyStringView(QStringView v) noexcept
        : QAnyStringView(std::data(v), lengthHelperContainer(v)) {}

    template <bool UseChar8T>
    constexpr QAnyStringView(QBasicUtf8StringView<UseChar8T> v) noexcept
        : QAnyStringView(std::data(v), lengthHelperContainer(v)) {}

    template <typename Char, size_t Size, if_compatible_char<Char> = true>
    [[nodiscard]] constexpr static QAnyStringView fromArray(const Char (&string)[Size]) noexcept
    { return QAnyStringView(string, Size); }

    // defined in qstring.h:
    template <typename Visitor>
    inline constexpr decltype(auto) visit(Visitor &&v) const;

    [[nodiscard]]
    constexpr QAnyStringView mid(qsizetype pos, qsizetype n = -1) const
    {
        using namespace QtPrivate;
        auto result = QContainerImplHelper::mid(size(), &pos, &n);
        return result == QContainerImplHelper::Null ? QAnyStringView() : sliced(pos, n);
    }
    [[nodiscard]]
    constexpr QAnyStringView left(qsizetype n) const
    {
        if (size_t(n) >= size_t(size()))
            n = size();
        return sliced(0, n);
    }
    [[nodiscard]]
    constexpr QAnyStringView right(qsizetype n) const
    {
        if (size_t(n) >= size_t(size()))
            n = size();
        return sliced(size() - n, n);
    }

    [[nodiscard]] constexpr QAnyStringView sliced(qsizetype pos) const
    { verify(pos); auto r = *this; r.advanceData(pos); r.setSize(size() - pos); return r; }
    [[nodiscard]] constexpr QAnyStringView sliced(qsizetype pos, qsizetype n) const
    { verify(pos, n); auto r = *this; r.advanceData(pos); r.setSize(n); return r; }
    [[nodiscard]] constexpr QAnyStringView first(qsizetype n) const
    { verify(n); return sliced(0, n); }
    [[nodiscard]] constexpr QAnyStringView last(qsizetype n) const
    { verify(n); return sliced(size() - n, n); }
    [[nodiscard]] constexpr QAnyStringView chopped(qsizetype n) const
    { verify(n); return sliced(0, size() - n); }

    constexpr void truncate(qsizetype n)
    { verify(n); setSize(n); }
    constexpr void chop(qsizetype n)
    { verify(n); setSize(size() - n); }


    [[nodiscard]] inline QString toString() const; // defined in qstring.h

    [[nodiscard]] constexpr qsizetype size() const noexcept
    { return qsizetype((m_size >> SizeShift) & SizeMask); }
    [[nodiscard]] constexpr const void *data() const noexcept { return m_data; }

    [[nodiscard]] Q_CORE_EXPORT static int compare(QAnyStringView lhs, QAnyStringView rhs, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
    [[nodiscard]] Q_CORE_EXPORT static bool equal(QAnyStringView lhs, QAnyStringView rhs) noexcept;

    static constexpr inline bool detects_US_ASCII_at_compile_time =
#ifdef __cpp_lib_is_constant_evaluated
            true
#else
            false
#endif
            ;
    //
    // STL compatibility API:
    //
    [[nodiscard]] constexpr QChar front() const; // NOT noexcept!
    [[nodiscard]] constexpr QChar back() const; // NOT noexcept!
    [[nodiscard]] constexpr bool empty() const noexcept { return size() == 0; }
    [[nodiscard]] constexpr qsizetype size_bytes() const noexcept
    { return size() * charSize(); }

    //
    // Qt compatibility API:
    //
    [[nodiscard]] constexpr bool isNull() const noexcept { return !m_data; }
    [[nodiscard]] constexpr bool isEmpty() const noexcept { return empty(); }
    [[nodiscard]] constexpr qsizetype length() const noexcept
    { return size(); }

private:
    [[nodiscard]] friend inline bool operator==(QAnyStringView lhs, QAnyStringView rhs) noexcept
    { return QAnyStringView::equal(lhs, rhs); }
    [[nodiscard]] friend inline bool operator!=(QAnyStringView lhs, QAnyStringView rhs) noexcept
    { return !QAnyStringView::equal(lhs, rhs); }

#if defined(__cpp_impl_three_way_comparison) && !defined(Q_QDOC)
    [[nodiscard]] friend inline auto operator<=>(QAnyStringView lhs, QAnyStringView rhs) noexcept
    { return QAnyStringView::compare(lhs, rhs) <=> 0; }
#else
    [[nodiscard]] friend inline bool operator<=(QAnyStringView lhs, QAnyStringView rhs) noexcept
    { return QAnyStringView::compare(lhs, rhs) <= 0; }
    [[nodiscard]] friend inline bool operator>=(QAnyStringView lhs, QAnyStringView rhs) noexcept
    { return QAnyStringView::compare(lhs, rhs) >= 0; }
    [[nodiscard]] friend inline bool operator<(QAnyStringView lhs, QAnyStringView rhs) noexcept
    { return QAnyStringView::compare(lhs, rhs) < 0; }
    [[nodiscard]] friend inline bool operator>(QAnyStringView lhs, QAnyStringView rhs) noexcept
    { return QAnyStringView::compare(lhs, rhs) > 0; }
#endif

    [[nodiscard]] constexpr Tag tag() const noexcept { return Tag{m_size & TypeMask}; }
    [[nodiscard]] constexpr bool isUtf16() const noexcept { return tag() == Tag::Utf16; }
    [[nodiscard]] constexpr bool isUtf8() const noexcept { return tag() == Tag::Utf8; }
    [[nodiscard]] constexpr bool isLatin1() const noexcept { return tag() == Tag::Latin1; }
    [[nodiscard]] constexpr QStringView asStringView() const
    { return Q_ASSERT(isUtf16()), QStringView{m_data_utf16, size()}; }
    [[nodiscard]] constexpr q_no_char8_t::QUtf8StringView asUtf8StringView() const
    { return Q_ASSERT(isUtf8()), q_no_char8_t::QUtf8StringView{m_data_utf8, size()}; }
    [[nodiscard]] inline constexpr QLatin1StringView asLatin1StringView() const;
    [[nodiscard]] constexpr size_t charSize() const noexcept { return isUtf16() ? 2 : 1; }
    constexpr void setSize(qsizetype sz) noexcept { m_size = size_t(sz) | tag(); }
    constexpr void advanceData(qsizetype delta) noexcept
    { m_data_utf8 += delta * charSize(); }
    Q_ALWAYS_INLINE constexpr void verify(qsizetype pos, qsizetype n = 0) const
    {
        Q_ASSERT(pos >= 0);
        Q_ASSERT(pos <= size());
        Q_ASSERT(n >= 0);
        Q_ASSERT(n <= size() - pos);
    }
    union {
        const void *m_data;
        const char *m_data_utf8;
        const char16_t *m_data_utf16;
    };
    size_t m_size;
    friend class ::tst_QAnyStringView;
};
Q_DECLARE_TYPEINFO(QAnyStringView, Q_PRIMITIVE_TYPE);

template <typename QStringLike, std::enable_if_t<std::disjunction_v<
        std::is_same<QStringLike, QString>,
        std::is_same<QStringLike, QByteArray>
    >, bool> = true>
[[nodiscard]] inline QAnyStringView qToAnyStringViewIgnoringNull(const QStringLike &s) noexcept
{ return QAnyStringView(s.data(), s.size()); }

QT_END_NAMESPACE

#endif /* QANYSTRINGVIEW_H */
