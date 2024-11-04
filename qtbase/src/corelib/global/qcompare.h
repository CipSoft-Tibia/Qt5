// Copyright (C) 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCOMPARE_H
#define QCOMPARE_H

#if 0
#pragma qt_class(QtCompare)
#endif

#include <QtCore/qglobal.h>
#include <QtCore/qcompare_impl.h>

#ifdef __cpp_lib_bit_cast
#include <bit>
#endif
#ifdef __cpp_lib_three_way_comparison
#include <compare>
#endif

QT_BEGIN_NAMESPACE

namespace QtPrivate {
using CompareUnderlyingType = qint8;

// [cmp.categories.pre] / 1
enum class Ordering : CompareUnderlyingType
{
    Equal = 0,
    Equivalent = Equal,
    Less = -1,
    Greater = 1
};

enum class Uncomparable : CompareUnderlyingType
{
    Unordered =
        #if defined(_LIBCPP_VERSION) // libc++
                -127
        #elif defined(__GLIBCXX__)   // libstdc++
                   2
        #else                        // assume MSSTL
                -128
        #endif
};

} // namespace QtPrivate

namespace QtOrderingPrivate {

template <typename O>
constexpr O reversed(O o) noexcept
{
    // https://eel.is/c++draft/cmp.partialord#5
    return is_lt(o) ? O::greater :
           is_gt(o) ? O::less :
           /*else*/ o ;
}

} // namespace QtOrderingPrivate

namespace Qt {

class weak_ordering;
class strong_ordering;

class partial_ordering
{
public:
    static const partial_ordering less;
    static const partial_ordering equivalent;
    static const partial_ordering greater;
    static const partial_ordering unordered;

    friend constexpr bool operator==(partial_ordering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.isOrdered() && lhs.m_order == 0; }

    friend constexpr bool operator!=(partial_ordering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return !lhs.isOrdered() || lhs.m_order != 0; }

    friend constexpr bool operator< (partial_ordering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.isOrdered() && lhs.m_order <  0; }

    friend constexpr bool operator<=(partial_ordering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.isOrdered() && lhs.m_order <= 0; }

    friend constexpr bool operator> (partial_ordering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.isOrdered() && lhs.m_order >  0; }

    friend constexpr bool operator>=(partial_ordering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.isOrdered() && lhs.m_order >= 0; }


    friend constexpr bool operator==(QtPrivate::CompareAgainstLiteralZero,
                                     partial_ordering rhs) noexcept
    { return rhs.isOrdered() && 0 == rhs.m_order; }

    friend constexpr bool operator!=(QtPrivate::CompareAgainstLiteralZero,
                                     partial_ordering rhs) noexcept
    { return !rhs.isOrdered() || 0 != rhs.m_order; }

    friend constexpr bool operator< (QtPrivate::CompareAgainstLiteralZero,
                                     partial_ordering rhs) noexcept
    { return rhs.isOrdered() && 0 <  rhs.m_order; }

    friend constexpr bool operator<=(QtPrivate::CompareAgainstLiteralZero,
                                     partial_ordering rhs) noexcept
    { return rhs.isOrdered() && 0 <= rhs.m_order; }

    friend constexpr bool operator> (QtPrivate::CompareAgainstLiteralZero,
                                     partial_ordering rhs) noexcept
    { return rhs.isOrdered() && 0 >  rhs.m_order; }

    friend constexpr bool operator>=(QtPrivate::CompareAgainstLiteralZero,
                                     partial_ordering rhs) noexcept
    { return rhs.isOrdered() && 0 >= rhs.m_order; }


#ifdef __cpp_lib_three_way_comparison
    friend constexpr std::partial_ordering
    operator<=>(partial_ordering lhs, QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs; } // https://eel.is/c++draft/cmp.partialord#4

    friend constexpr std::partial_ordering
    operator<=>(QtPrivate::CompareAgainstLiteralZero, partial_ordering rhs) noexcept
    { return QtOrderingPrivate::reversed(rhs); }
#endif // __cpp_lib_three_way_comparison


    friend constexpr bool operator==(partial_ordering lhs, partial_ordering rhs) noexcept
    { return lhs.m_order == rhs.m_order; }

    friend constexpr bool operator!=(partial_ordering lhs, partial_ordering rhs) noexcept
    { return lhs.m_order != rhs.m_order; }

#ifdef __cpp_lib_three_way_comparison
    constexpr Q_IMPLICIT partial_ordering(std::partial_ordering stdorder) noexcept
    {
        if (stdorder == std::partial_ordering::less)
            m_order = static_cast<QtPrivate::CompareUnderlyingType>(QtPrivate::Ordering::Less);
        else if (stdorder == std::partial_ordering::equivalent)
            m_order = static_cast<QtPrivate::CompareUnderlyingType>(QtPrivate::Ordering::Equivalent);
        else if (stdorder == std::partial_ordering::greater)
            m_order = static_cast<QtPrivate::CompareUnderlyingType>(QtPrivate::Ordering::Greater);
        else if (stdorder == std::partial_ordering::unordered)
            m_order = static_cast<QtPrivate::CompareUnderlyingType>(QtPrivate::Uncomparable::Unordered);
    }

    constexpr Q_IMPLICIT operator std::partial_ordering() const noexcept
    {
        static_assert(sizeof(*this) == sizeof(std::partial_ordering));
#ifdef __cpp_lib_bit_cast
        return std::bit_cast<std::partial_ordering>(*this);
#else
        using O = QtPrivate::Ordering;
        using U = QtPrivate::Uncomparable;
        using R = std::partial_ordering;
        switch (m_order) {
        case qToUnderlying(O::Less):        return R::less;
        case qToUnderlying(O::Greater):     return R::greater;
        case qToUnderlying(O::Equivalent):  return R::equivalent;
        case qToUnderlying(U::Unordered):   return R::unordered;
        }
        Q_UNREACHABLE_RETURN(R::unordered);
#endif // __cpp_lib_bit_cast
    }

    friend constexpr bool operator==(partial_ordering lhs, std::partial_ordering rhs) noexcept
    { return static_cast<std::partial_ordering>(lhs) == rhs; }

    friend constexpr bool operator!=(partial_ordering lhs, std::partial_ordering rhs) noexcept
    { return static_cast<std::partial_ordering>(lhs) != rhs; }

    friend constexpr bool operator==(std::partial_ordering lhs, partial_ordering rhs) noexcept
    { return lhs == static_cast<std::partial_ordering>(rhs); }

    friend constexpr bool operator!=(std::partial_ordering lhs, partial_ordering rhs) noexcept
    { return lhs != static_cast<std::partial_ordering>(rhs); }

    friend constexpr bool operator==(partial_ordering lhs, std::strong_ordering rhs) noexcept
    { return static_cast<std::partial_ordering>(lhs) == rhs; }

    friend constexpr bool operator!=(partial_ordering lhs, std::strong_ordering rhs) noexcept
    { return static_cast<std::partial_ordering>(lhs) != rhs; }

    friend constexpr bool operator==(std::strong_ordering lhs, partial_ordering rhs) noexcept
    { return lhs == static_cast<std::partial_ordering>(rhs); }

    friend constexpr bool operator!=(std::strong_ordering lhs, partial_ordering rhs) noexcept
    { return lhs != static_cast<std::partial_ordering>(rhs); }

    friend constexpr bool operator==(partial_ordering lhs, std::weak_ordering rhs) noexcept
    { return static_cast<std::partial_ordering>(lhs) == rhs; }

    friend constexpr bool operator!=(partial_ordering lhs, std::weak_ordering rhs) noexcept
    { return static_cast<std::partial_ordering>(lhs) != rhs; }

    friend constexpr bool operator==(std::weak_ordering lhs, partial_ordering rhs) noexcept
    { return lhs == static_cast<std::partial_ordering>(rhs); }

    friend constexpr bool operator!=(std::weak_ordering lhs, partial_ordering rhs) noexcept
    { return lhs != static_cast<std::partial_ordering>(rhs); }
#endif // __cpp_lib_three_way_comparison

private:
    friend class weak_ordering;
    friend class strong_ordering;

    constexpr explicit partial_ordering(QtPrivate::Ordering order) noexcept
        : m_order(static_cast<QtPrivate::CompareUnderlyingType>(order))
    {}
    constexpr explicit partial_ordering(QtPrivate::Uncomparable order) noexcept
        : m_order(static_cast<QtPrivate::CompareUnderlyingType>(order))
    {}

    QT_WARNING_PUSH
    // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=100903
    QT_WARNING_DISABLE_GCC("-Wzero-as-null-pointer-constant")
    friend constexpr bool is_eq  (partial_ordering o) noexcept { return o == 0; }
    friend constexpr bool is_neq (partial_ordering o) noexcept { return o != 0; }
    friend constexpr bool is_lt  (partial_ordering o) noexcept { return o <  0; }
    friend constexpr bool is_lteq(partial_ordering o) noexcept { return o <= 0; }
    friend constexpr bool is_gt  (partial_ordering o) noexcept { return o >  0; }
    friend constexpr bool is_gteq(partial_ordering o) noexcept { return o >= 0; }
    QT_WARNING_POP

    // instead of the exposition only is_ordered member in [cmp.partialord],
    // use a private function
    constexpr bool isOrdered() const noexcept
    { return m_order != static_cast<QtPrivate::CompareUnderlyingType>(QtPrivate::Uncomparable::Unordered); }

    QtPrivate::CompareUnderlyingType m_order;
};

inline constexpr partial_ordering partial_ordering::less(QtPrivate::Ordering::Less);
inline constexpr partial_ordering partial_ordering::equivalent(QtPrivate::Ordering::Equivalent);
inline constexpr partial_ordering partial_ordering::greater(QtPrivate::Ordering::Greater);
inline constexpr partial_ordering partial_ordering::unordered(QtPrivate::Uncomparable::Unordered);

class weak_ordering
{
public:
    static const weak_ordering less;
    static const weak_ordering equivalent;
    static const weak_ordering greater;

    constexpr Q_IMPLICIT operator partial_ordering() const noexcept
    { return partial_ordering(static_cast<QtPrivate::Ordering>(m_order)); }

    friend constexpr bool operator==(weak_ordering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.m_order == 0; }

    friend constexpr bool operator!=(weak_ordering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.m_order != 0; }

    friend constexpr bool operator< (weak_ordering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.m_order <  0; }

    friend constexpr bool operator<=(weak_ordering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.m_order <= 0; }

    friend constexpr bool operator> (weak_ordering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.m_order >  0; }

    friend constexpr bool operator>=(weak_ordering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.m_order >= 0; }


    friend constexpr bool operator==(QtPrivate::CompareAgainstLiteralZero,
                                     weak_ordering rhs) noexcept
    { return 0 == rhs.m_order; }

    friend constexpr bool operator!=(QtPrivate::CompareAgainstLiteralZero,
                                     weak_ordering rhs) noexcept
    { return 0 != rhs.m_order; }

    friend constexpr bool operator< (QtPrivate::CompareAgainstLiteralZero,
                                     weak_ordering rhs) noexcept
    { return 0 <  rhs.m_order; }

    friend constexpr bool operator<=(QtPrivate::CompareAgainstLiteralZero,
                                     weak_ordering rhs) noexcept
    { return 0 <= rhs.m_order; }

    friend constexpr bool operator> (QtPrivate::CompareAgainstLiteralZero,
                                     weak_ordering rhs) noexcept
    { return 0 > rhs.m_order; }

    friend constexpr bool operator>=(QtPrivate::CompareAgainstLiteralZero,
                                     weak_ordering rhs) noexcept
    { return 0 >= rhs.m_order; }


#ifdef __cpp_lib_three_way_comparison
    friend constexpr std::weak_ordering
    operator<=>(weak_ordering lhs, QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs; } // https://eel.is/c++draft/cmp.weakord#5

    friend constexpr std::weak_ordering
    operator<=>(QtPrivate::CompareAgainstLiteralZero, weak_ordering rhs) noexcept
    { return QtOrderingPrivate::reversed(rhs); }
#endif // __cpp_lib_three_way_comparison


    friend constexpr bool operator==(weak_ordering lhs, weak_ordering rhs) noexcept
    { return lhs.m_order == rhs.m_order; }

    friend constexpr bool operator!=(weak_ordering lhs, weak_ordering rhs) noexcept
    { return lhs.m_order != rhs.m_order; }

    friend constexpr bool operator==(weak_ordering lhs, partial_ordering rhs) noexcept
    { return static_cast<partial_ordering>(lhs) == rhs; }

    friend constexpr bool operator!=(weak_ordering lhs, partial_ordering rhs) noexcept
    { return static_cast<partial_ordering>(lhs) != rhs; }

    friend constexpr bool operator==(partial_ordering lhs, weak_ordering rhs) noexcept
    { return lhs == static_cast<partial_ordering>(rhs); }

    friend constexpr bool operator!=(partial_ordering lhs, weak_ordering rhs) noexcept
    { return lhs != static_cast<partial_ordering>(rhs); }

#ifdef __cpp_lib_three_way_comparison
    constexpr Q_IMPLICIT weak_ordering(std::weak_ordering stdorder) noexcept
    {
        if (stdorder == std::weak_ordering::less)
            m_order = static_cast<QtPrivate::CompareUnderlyingType>(QtPrivate::Ordering::Less);
        else if (stdorder == std::weak_ordering::equivalent)
            m_order = static_cast<QtPrivate::CompareUnderlyingType>(QtPrivate::Ordering::Equivalent);
        else if (stdorder == std::weak_ordering::greater)
            m_order = static_cast<QtPrivate::CompareUnderlyingType>(QtPrivate::Ordering::Greater);
    }

    constexpr Q_IMPLICIT operator std::weak_ordering() const noexcept
    {
        static_assert(sizeof(*this) == sizeof(std::weak_ordering));
#ifdef __cpp_lib_bit_cast
        return std::bit_cast<std::weak_ordering>(*this);
#else
        using O = QtPrivate::Ordering;
        using R = std::weak_ordering;
        switch (m_order) {
        case qToUnderlying(O::Less):          return R::less;
        case qToUnderlying(O::Greater):       return R::greater;
        case qToUnderlying(O::Equivalent):    return R::equivalent;
        }
        Q_UNREACHABLE_RETURN(R::equivalent);
#endif // __cpp_lib_bit_cast
    }

    friend constexpr bool operator==(weak_ordering lhs, std::weak_ordering rhs) noexcept
    { return static_cast<std::weak_ordering>(lhs) == rhs; }

    friend constexpr bool operator!=(weak_ordering lhs, std::weak_ordering rhs) noexcept
    { return static_cast<std::weak_ordering>(lhs) != rhs; }

    friend constexpr bool operator==(weak_ordering lhs, std::partial_ordering rhs) noexcept
    { return static_cast<std::weak_ordering>(lhs) == rhs; }

    friend constexpr bool operator!=(weak_ordering lhs, std::partial_ordering rhs) noexcept
    { return static_cast<std::weak_ordering>(lhs) != rhs; }

    friend constexpr bool operator==(weak_ordering lhs, std::strong_ordering rhs) noexcept
    { return static_cast<std::weak_ordering>(lhs) == rhs; }

    friend constexpr bool operator!=(weak_ordering lhs, std::strong_ordering rhs) noexcept
    { return static_cast<std::weak_ordering>(lhs) != rhs; }

    friend constexpr bool operator==(std::weak_ordering lhs, weak_ordering rhs) noexcept
    { return lhs == static_cast<std::weak_ordering>(rhs); }

    friend constexpr bool operator!=(std::weak_ordering lhs, weak_ordering rhs) noexcept
    { return lhs != static_cast<std::weak_ordering>(rhs); }

    friend constexpr bool operator==(std::partial_ordering lhs, weak_ordering rhs) noexcept
    { return lhs == static_cast<std::weak_ordering>(rhs); }

    friend constexpr bool operator!=(std::partial_ordering lhs, weak_ordering rhs) noexcept
    { return lhs != static_cast<std::weak_ordering>(rhs); }

    friend constexpr bool operator==(std::strong_ordering lhs, weak_ordering rhs) noexcept
    { return lhs == static_cast<std::weak_ordering>(rhs); }

    friend constexpr bool operator!=(std::strong_ordering lhs, weak_ordering rhs) noexcept
    { return lhs != static_cast<std::weak_ordering>(rhs); }
#endif // __cpp_lib_three_way_comparison

private:
    friend class strong_ordering;

    constexpr explicit weak_ordering(QtPrivate::Ordering order) noexcept
        : m_order(static_cast<QtPrivate::CompareUnderlyingType>(order))
    {}

    QT_WARNING_PUSH
    // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=100903
    QT_WARNING_DISABLE_GCC("-Wzero-as-null-pointer-constant")
    friend constexpr bool is_eq  (weak_ordering o) noexcept { return o == 0; }
    friend constexpr bool is_neq (weak_ordering o) noexcept { return o != 0; }
    friend constexpr bool is_lt  (weak_ordering o) noexcept { return o <  0; }
    friend constexpr bool is_lteq(weak_ordering o) noexcept { return o <= 0; }
    friend constexpr bool is_gt  (weak_ordering o) noexcept { return o >  0; }
    friend constexpr bool is_gteq(weak_ordering o) noexcept { return o >= 0; }
    QT_WARNING_POP

    QtPrivate::CompareUnderlyingType m_order;
};

inline constexpr weak_ordering weak_ordering::less(QtPrivate::Ordering::Less);
inline constexpr weak_ordering weak_ordering::equivalent(QtPrivate::Ordering::Equivalent);
inline constexpr weak_ordering weak_ordering::greater(QtPrivate::Ordering::Greater);

class strong_ordering
{
public:
    static const strong_ordering less;
    static const strong_ordering equivalent;
    static const strong_ordering equal;
    static const strong_ordering greater;

    constexpr Q_IMPLICIT operator partial_ordering() const noexcept
    { return partial_ordering(static_cast<QtPrivate::Ordering>(m_order)); }

    constexpr Q_IMPLICIT operator weak_ordering() const noexcept
    { return weak_ordering(static_cast<QtPrivate::Ordering>(m_order)); }

    friend constexpr bool operator==(strong_ordering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.m_order == 0; }

    friend constexpr bool operator!=(strong_ordering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.m_order != 0; }

    friend constexpr bool operator< (strong_ordering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.m_order <  0; }

    friend constexpr bool operator<=(strong_ordering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.m_order <= 0; }

    friend constexpr bool operator> (strong_ordering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.m_order >  0; }

    friend constexpr bool operator>=(strong_ordering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.m_order >= 0; }


    friend constexpr bool operator==(QtPrivate::CompareAgainstLiteralZero,
                                     strong_ordering rhs) noexcept
    { return 0 == rhs.m_order; }

    friend constexpr bool operator!=(QtPrivate::CompareAgainstLiteralZero,
                                     strong_ordering rhs) noexcept
    { return 0 != rhs.m_order; }

    friend constexpr bool operator< (QtPrivate::CompareAgainstLiteralZero,
                                    strong_ordering rhs) noexcept
    { return 0 <  rhs.m_order; }

    friend constexpr bool operator<=(QtPrivate::CompareAgainstLiteralZero,
                                     strong_ordering rhs) noexcept
    { return 0 <= rhs.m_order; }

    friend constexpr bool operator> (QtPrivate::CompareAgainstLiteralZero,
                                    strong_ordering rhs) noexcept
    { return 0 >  rhs.m_order; }

    friend constexpr bool operator>=(QtPrivate::CompareAgainstLiteralZero,
                                     strong_ordering rhs) noexcept
    { return 0 >= rhs.m_order; }


#ifdef __cpp_lib_three_way_comparison
    friend constexpr std::strong_ordering
    operator<=>(strong_ordering lhs, QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs; } // https://eel.is/c++draft/cmp.strongord#6

    friend constexpr std::strong_ordering
    operator<=>(QtPrivate::CompareAgainstLiteralZero, strong_ordering rhs) noexcept
    { return QtOrderingPrivate::reversed(rhs); }
#endif // __cpp_lib_three_way_comparison


    friend constexpr bool operator==(strong_ordering lhs, strong_ordering rhs) noexcept
    { return lhs.m_order == rhs.m_order; }

    friend constexpr bool operator!=(strong_ordering lhs, strong_ordering rhs) noexcept
    { return lhs.m_order != rhs.m_order; }

    friend constexpr bool operator==(strong_ordering lhs, partial_ordering rhs) noexcept
    { return static_cast<partial_ordering>(lhs) == rhs; }

    friend constexpr bool operator!=(strong_ordering lhs, partial_ordering rhs) noexcept
    { return static_cast<partial_ordering>(lhs) == rhs; }

    friend constexpr bool operator==(partial_ordering lhs, strong_ordering rhs) noexcept
    { return lhs == static_cast<partial_ordering>(rhs); }

    friend constexpr bool operator!=(partial_ordering lhs, strong_ordering rhs) noexcept
    { return lhs != static_cast<partial_ordering>(rhs); }

    friend constexpr bool operator==(strong_ordering lhs, weak_ordering rhs) noexcept
    { return static_cast<weak_ordering>(lhs) == rhs; }

    friend constexpr bool operator!=(strong_ordering lhs, weak_ordering rhs) noexcept
    { return static_cast<weak_ordering>(lhs) == rhs; }

    friend constexpr bool operator==(weak_ordering lhs, strong_ordering rhs) noexcept
    { return lhs == static_cast<weak_ordering>(rhs); }

    friend constexpr bool operator!=(weak_ordering lhs, strong_ordering rhs) noexcept
    { return lhs != static_cast<weak_ordering>(rhs); }

#ifdef __cpp_lib_three_way_comparison
    constexpr Q_IMPLICIT strong_ordering(std::strong_ordering stdorder) noexcept
    {
        if (stdorder == std::strong_ordering::less)
            m_order = static_cast<QtPrivate::CompareUnderlyingType>(QtPrivate::Ordering::Less);
        else if (stdorder == std::strong_ordering::equivalent)
            m_order = static_cast<QtPrivate::CompareUnderlyingType>(QtPrivate::Ordering::Equivalent);
        else if (stdorder == std::strong_ordering::equal)
            m_order = static_cast<QtPrivate::CompareUnderlyingType>(QtPrivate::Ordering::Equal);
        else if (stdorder == std::strong_ordering::greater)
            m_order = static_cast<QtPrivate::CompareUnderlyingType>(QtPrivate::Ordering::Greater);
    }

    constexpr Q_IMPLICIT operator std::strong_ordering() const noexcept
    {
        static_assert(sizeof(*this) == sizeof(std::strong_ordering));
#ifdef __cpp_lib_bit_cast
        return std::bit_cast<std::strong_ordering>(*this);
#else
        using O = QtPrivate::Ordering;
        using R = std::strong_ordering;
        switch (m_order) {
        case qToUnderlying(O::Less):    return R::less;
        case qToUnderlying(O::Greater): return R::greater;
        case qToUnderlying(O::Equal):   return R::equal;
        }
        Q_UNREACHABLE_RETURN(R::equal);
#endif // __cpp_lib_bit_cast
    }

    friend constexpr bool operator==(strong_ordering lhs, std::strong_ordering rhs) noexcept
    { return static_cast<std::strong_ordering>(lhs) == rhs; }

    friend constexpr bool operator!=(strong_ordering lhs, std::strong_ordering rhs) noexcept
    { return static_cast<std::strong_ordering>(lhs) != rhs; }

    friend constexpr bool operator==(strong_ordering lhs, std::partial_ordering rhs) noexcept
    { return static_cast<std::strong_ordering>(lhs) == rhs; }

    friend constexpr bool operator!=(strong_ordering lhs, std::partial_ordering rhs) noexcept
    { return static_cast<std::strong_ordering>(lhs) != rhs; }

    friend constexpr bool operator==(strong_ordering lhs, std::weak_ordering rhs) noexcept
    { return static_cast<std::strong_ordering>(lhs) == rhs; }

    friend constexpr bool operator!=(strong_ordering lhs, std::weak_ordering rhs) noexcept
    { return static_cast<std::strong_ordering>(lhs) != rhs; }

    friend constexpr bool operator==(std::strong_ordering lhs, strong_ordering rhs) noexcept
    { return lhs == static_cast<std::strong_ordering>(rhs); }

    friend constexpr bool operator!=(std::strong_ordering lhs, strong_ordering rhs) noexcept
    { return lhs != static_cast<std::strong_ordering>(rhs); }

    friend constexpr bool operator==(std::partial_ordering lhs, strong_ordering rhs) noexcept
    { return lhs == static_cast<std::strong_ordering>(rhs); }

    friend constexpr bool operator!=(std::partial_ordering lhs, strong_ordering rhs) noexcept
    { return lhs != static_cast<std::strong_ordering>(rhs); }

    friend constexpr bool operator==(std::weak_ordering lhs, strong_ordering rhs) noexcept
    { return lhs == static_cast<std::strong_ordering>(rhs); }

    friend constexpr bool operator!=(std::weak_ordering lhs, strong_ordering rhs) noexcept
    { return lhs != static_cast<std::strong_ordering>(rhs); }
#endif // __cpp_lib_three_way_comparison

    private:
    constexpr explicit strong_ordering(QtPrivate::Ordering order) noexcept
        : m_order(static_cast<QtPrivate::CompareUnderlyingType>(order))
    {}

    QT_WARNING_PUSH
    // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=100903
    QT_WARNING_DISABLE_GCC("-Wzero-as-null-pointer-constant")
    friend constexpr bool is_eq  (strong_ordering o) noexcept { return o == 0; }
    friend constexpr bool is_neq (strong_ordering o) noexcept { return o != 0; }
    friend constexpr bool is_lt  (strong_ordering o) noexcept { return o <  0; }
    friend constexpr bool is_lteq(strong_ordering o) noexcept { return o <= 0; }
    friend constexpr bool is_gt  (strong_ordering o) noexcept { return o >  0; }
    friend constexpr bool is_gteq(strong_ordering o) noexcept { return o >= 0; }
    QT_WARNING_POP

    QtPrivate::CompareUnderlyingType m_order;
};

inline constexpr strong_ordering strong_ordering::less(QtPrivate::Ordering::Less);
inline constexpr strong_ordering strong_ordering::equivalent(QtPrivate::Ordering::Equivalent);
inline constexpr strong_ordering strong_ordering::equal(QtPrivate::Ordering::Equal);
inline constexpr strong_ordering strong_ordering::greater(QtPrivate::Ordering::Greater);

} // namespace Qt

QT_BEGIN_INCLUDE_NAMESPACE

// This is intentionally included after Qt::*_ordering types and before
// qCompareThreeWay. Do not change!
#include <QtCore/qcomparehelpers.h>

QT_END_INCLUDE_NAMESPACE

namespace QtPrivate {

namespace CompareThreeWayTester {

    using Qt::compareThreeWay;

    // Check if compareThreeWay is implemented for the (LT, RT) argument
    // pair.
    template <typename LT, typename RT, typename = void>
    constexpr bool hasCompareThreeWay = false;

    template <typename LT, typename RT>
    constexpr bool hasCompareThreeWay<
            LT, RT, std::void_t<decltype(compareThreeWay(std::declval<LT>(), std::declval<RT>()))>
    > = true;

    // Check if the operation is noexcept. We have two different overloads,
    // depending on the available compareThreeWay() implementation.
    // Both are declared, but not implemented. To be used only in unevaluated
    // context.

    template <typename LT, typename RT,
             std::enable_if_t<hasCompareThreeWay<LT, RT>, bool> = true>
    constexpr bool compareThreeWayNoexcept() noexcept
    { return noexcept(compareThreeWay(std::declval<LT>(), std::declval<RT>())); }

    template <typename LT, typename RT,
             std::enable_if_t<!hasCompareThreeWay<LT, RT> && hasCompareThreeWay<RT, LT>,
                              bool> = true>
    constexpr bool compareThreeWayNoexcept() noexcept
    { return noexcept(compareThreeWay(std::declval<RT>(), std::declval<LT>())); }

} // namespace CompareThreeWayTester

} // namespace QtPrivate

#if defined(Q_QDOC)

template <typename LeftType, typename RightType>
auto qCompareThreeWay(const LeftType &lhs, const RightType &rhs);

#else

template <typename LT, typename RT,
          std::enable_if_t<QtPrivate::CompareThreeWayTester::hasCompareThreeWay<LT, RT>
                            || QtPrivate::CompareThreeWayTester::hasCompareThreeWay<RT, LT>,
                           bool> = true>
auto qCompareThreeWay(const LT &lhs, const RT &rhs)
        noexcept(QtPrivate::CompareThreeWayTester::compareThreeWayNoexcept<LT, RT>())
{
    using Qt::compareThreeWay;
    if constexpr (QtPrivate::CompareThreeWayTester::hasCompareThreeWay<LT, RT>) {
        return compareThreeWay(lhs, rhs);
    } else {
        const auto retval = compareThreeWay(rhs, lhs);
        return QtOrderingPrivate::reversed(retval);
    }
}

#endif // defined(Q_QDOC)

//
// Legacy QPartialOrdering
//

namespace QtPrivate {
enum class LegacyUncomparable : CompareUnderlyingType
{
    Unordered = -127
};
}

// [cmp.partialord]
class QPartialOrdering
{
public:
    static const QPartialOrdering Less;
    static const QPartialOrdering Equivalent;
    static const QPartialOrdering Greater;
    static const QPartialOrdering Unordered;

    static const QPartialOrdering less;
    static const QPartialOrdering equivalent;
    static const QPartialOrdering greater;
    static const QPartialOrdering unordered;

    friend constexpr bool operator==(QPartialOrdering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.isOrdered() && lhs.m_order == 0; }

    friend constexpr bool operator!=(QPartialOrdering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return !lhs.isOrdered() || lhs.m_order != 0; }

    friend constexpr bool operator< (QPartialOrdering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.isOrdered() && lhs.m_order <  0; }

    friend constexpr bool operator<=(QPartialOrdering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.isOrdered() && lhs.m_order <= 0; }

    friend constexpr bool operator> (QPartialOrdering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.isOrdered() && lhs.m_order >  0; }

    friend constexpr bool operator>=(QPartialOrdering lhs,
                                     QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs.isOrdered() && lhs.m_order >= 0; }


    friend constexpr bool operator==(QtPrivate::CompareAgainstLiteralZero,
                                     QPartialOrdering rhs) noexcept
    { return rhs.isOrdered() && 0 == rhs.m_order; }

    friend constexpr bool operator!=(QtPrivate::CompareAgainstLiteralZero,
                                     QPartialOrdering rhs) noexcept
    { return !rhs.isOrdered() || 0 != rhs.m_order; }

    friend constexpr bool operator< (QtPrivate::CompareAgainstLiteralZero,
                                     QPartialOrdering rhs) noexcept
    { return rhs.isOrdered() && 0 <  rhs.m_order; }

    friend constexpr bool operator<=(QtPrivate::CompareAgainstLiteralZero,
                                     QPartialOrdering rhs) noexcept
    { return rhs.isOrdered() && 0 <= rhs.m_order; }

    friend constexpr bool operator> (QtPrivate::CompareAgainstLiteralZero,
                                     QPartialOrdering rhs) noexcept
    { return rhs.isOrdered() && 0 >  rhs.m_order; }

    friend constexpr bool operator>=(QtPrivate::CompareAgainstLiteralZero,
                                     QPartialOrdering rhs) noexcept
    { return rhs.isOrdered() && 0 >= rhs.m_order; }


#ifdef __cpp_lib_three_way_comparison
    friend constexpr std::partial_ordering
    operator<=>(QPartialOrdering lhs, QtPrivate::CompareAgainstLiteralZero) noexcept
    { return lhs; } // https://eel.is/c++draft/cmp.partialord#4

    friend constexpr std::partial_ordering
    operator<=>(QtPrivate::CompareAgainstLiteralZero, QPartialOrdering rhs) noexcept
    { return QtOrderingPrivate::reversed(rhs); }
#endif // __cpp_lib_three_way_comparison


    friend constexpr bool operator==(QPartialOrdering lhs, QPartialOrdering rhs) noexcept
    { return lhs.m_order == rhs.m_order; }

    friend constexpr bool operator!=(QPartialOrdering lhs, QPartialOrdering rhs) noexcept
    { return lhs.m_order != rhs.m_order; }

    constexpr Q_IMPLICIT QPartialOrdering(Qt::partial_ordering order) noexcept
        : m_order{} // == equivalent
    {
        if (order == Qt::partial_ordering::less)
            m_order = static_cast<QtPrivate::CompareUnderlyingType>(QtPrivate::Ordering::Less);
        else if (order == Qt::partial_ordering::greater)
            m_order = static_cast<QtPrivate::CompareUnderlyingType>(QtPrivate::Ordering::Greater);
        else if (order == Qt::partial_ordering::unordered)
            m_order = static_cast<QtPrivate::CompareUnderlyingType>(QtPrivate::LegacyUncomparable::Unordered);
    }

    constexpr Q_IMPLICIT QPartialOrdering(Qt::weak_ordering stdorder) noexcept
        : QPartialOrdering(Qt::partial_ordering{stdorder}) {}

    constexpr Q_IMPLICIT QPartialOrdering(Qt::strong_ordering stdorder) noexcept
        : QPartialOrdering(Qt::partial_ordering{stdorder}) {}

    constexpr Q_IMPLICIT operator Qt::partial_ordering() const noexcept
    {
        using O = QtPrivate::Ordering;
        using U = QtPrivate::LegacyUncomparable;
        using R = Qt::partial_ordering;
        switch (m_order) {
        case qToUnderlying(O::Less):       return R::less;
        case qToUnderlying(O::Greater):    return R::greater;
        case qToUnderlying(O::Equivalent): return R::equivalent;
        case qToUnderlying(U::Unordered):  return R::unordered;
        }
        // GCC 8.x does not treat __builtin_unreachable() as constexpr
#if !defined(Q_CC_GNU_ONLY) || (Q_CC_GNU >= 900)
        // NOLINTNEXTLINE(qt-use-unreachable-return): Triggers on Clang, breaking GCC 8
        Q_UNREACHABLE();
#endif
        return R::unordered;
    }

    friend constexpr bool operator==(QPartialOrdering lhs, Qt::partial_ordering rhs) noexcept
    { Qt::partial_ordering qt = lhs; return qt == rhs; }

    friend constexpr bool operator!=(QPartialOrdering lhs, Qt::partial_ordering rhs) noexcept
    { Qt::partial_ordering qt = lhs; return qt != rhs; }

    friend constexpr bool operator==(Qt::partial_ordering lhs, QPartialOrdering rhs) noexcept
    { Qt::partial_ordering qt = rhs; return lhs == qt; }

    friend constexpr bool operator!=(Qt::partial_ordering lhs, QPartialOrdering rhs) noexcept
    { Qt::partial_ordering qt = rhs; return lhs != qt; }

#ifdef __cpp_lib_three_way_comparison
    constexpr Q_IMPLICIT QPartialOrdering(std::partial_ordering stdorder) noexcept
    {
        if (stdorder == std::partial_ordering::less)
            m_order = static_cast<QtPrivate::CompareUnderlyingType>(QtPrivate::Ordering::Less);
        else if (stdorder == std::partial_ordering::equivalent)
            m_order = static_cast<QtPrivate::CompareUnderlyingType>(QtPrivate::Ordering::Equivalent);
        else if (stdorder == std::partial_ordering::greater)
            m_order = static_cast<QtPrivate::CompareUnderlyingType>(QtPrivate::Ordering::Greater);
        else if (stdorder == std::partial_ordering::unordered)
            m_order = static_cast<QtPrivate::CompareUnderlyingType>(QtPrivate::LegacyUncomparable::Unordered);
    }

    constexpr Q_IMPLICIT QPartialOrdering(std::weak_ordering stdorder) noexcept
        : QPartialOrdering(std::partial_ordering(stdorder)) {}

    constexpr Q_IMPLICIT QPartialOrdering(std::strong_ordering stdorder) noexcept
        : QPartialOrdering(std::partial_ordering(stdorder)) {}

    constexpr Q_IMPLICIT operator std::partial_ordering() const noexcept
    {
        using O = QtPrivate::Ordering;
        using U = QtPrivate::LegacyUncomparable;
        using R = std::partial_ordering;
        switch (m_order) {
        case qToUnderlying(O::Less):       return R::less;
        case qToUnderlying(O::Greater):    return R::greater;
        case qToUnderlying(O::Equivalent): return R::equivalent;
        case qToUnderlying(U::Unordered):  return R::unordered;
        }
        Q_UNREACHABLE_RETURN(R::unordered);
    }

    friend constexpr bool operator==(QPartialOrdering lhs, std::partial_ordering rhs) noexcept
    { return static_cast<std::partial_ordering>(lhs) == rhs; }

    friend constexpr bool operator!=(QPartialOrdering lhs, std::partial_ordering rhs) noexcept
    { return static_cast<std::partial_ordering>(lhs) != rhs; }

    friend constexpr bool operator==(std::partial_ordering lhs, QPartialOrdering rhs) noexcept
    { return lhs == static_cast<std::partial_ordering>(rhs); }

    friend constexpr bool operator!=(std::partial_ordering lhs, QPartialOrdering rhs) noexcept
    { return lhs != static_cast<std::partial_ordering>(rhs); }
#endif // __cpp_lib_three_way_comparison

private:
    constexpr explicit QPartialOrdering(QtPrivate::Ordering order) noexcept
        : m_order(static_cast<QtPrivate::CompareUnderlyingType>(order))
    {}
    constexpr explicit QPartialOrdering(QtPrivate::LegacyUncomparable order) noexcept
        : m_order(static_cast<QtPrivate::CompareUnderlyingType>(order))
    {}

    QT_WARNING_PUSH
    // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=100903
    QT_WARNING_DISABLE_GCC("-Wzero-as-null-pointer-constant")
    friend constexpr bool is_eq  (QPartialOrdering o) noexcept { return o == 0; }
    friend constexpr bool is_neq (QPartialOrdering o) noexcept { return o != 0; }
    friend constexpr bool is_lt  (QPartialOrdering o) noexcept { return o <  0; }
    friend constexpr bool is_lteq(QPartialOrdering o) noexcept { return o <= 0; }
    friend constexpr bool is_gt  (QPartialOrdering o) noexcept { return o >  0; }
    friend constexpr bool is_gteq(QPartialOrdering o) noexcept { return o >= 0; }
    QT_WARNING_POP

    // instead of the exposition only is_ordered member in [cmp.partialord],
    // use a private function
    constexpr bool isOrdered() const noexcept
    { return m_order != static_cast<QtPrivate::CompareUnderlyingType>(QtPrivate::LegacyUncomparable::Unordered); }

    QtPrivate::CompareUnderlyingType m_order;
};

inline constexpr QPartialOrdering QPartialOrdering::Less(QtPrivate::Ordering::Less);
inline constexpr QPartialOrdering QPartialOrdering::Equivalent(QtPrivate::Ordering::Equivalent);
inline constexpr QPartialOrdering QPartialOrdering::Greater(QtPrivate::Ordering::Greater);
inline constexpr QPartialOrdering QPartialOrdering::Unordered(QtPrivate::LegacyUncomparable::Unordered);

inline constexpr QPartialOrdering QPartialOrdering::less(QtPrivate::Ordering::Less);
inline constexpr QPartialOrdering QPartialOrdering::equivalent(QtPrivate::Ordering::Equivalent);
inline constexpr QPartialOrdering QPartialOrdering::greater(QtPrivate::Ordering::Greater);
inline constexpr QPartialOrdering QPartialOrdering::unordered(QtPrivate::LegacyUncomparable::Unordered);

QT_END_NAMESPACE

#endif // QCOMPARE_H
