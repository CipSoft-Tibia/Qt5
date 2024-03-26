// Copyright (C) 2023 The Qt Company Ltd.
// Copyright (C) 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef Q20MEMORY_H
#define Q20MEMORY_H

#include <QtCore/qtconfigmacros.h>

#include <memory>

#include <type_traits>
#include <utility>

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. Types and functions defined in this
// file can reliably be replaced by their std counterparts, once available.
// You may use these definitions in your own code, but be aware that we
// will remove them once Qt depends on the C++ version that supports
// them in namespace std. There will be NO deprecation warning, the
// definitions will JUST go away.
//
// If you can't agree to these terms, don't use these definitions!
//
// We mean it.
//

QT_BEGIN_NAMESPACE

// like std::construct_at (but not whitelisted for constexpr)
namespace q20 {
#ifdef __cpp_lib_constexpr_dynamic_alloc
using std::construct_at;
#else
template <typename T,
          typename... Args,
          typename Enable = std::void_t<decltype(::new (std::declval<void *>()) T(std::declval<Args>()...))> >
T *construct_at(T *ptr, Args && ... args)
{
    return ::new (const_cast<void *>(static_cast<const volatile void *>(ptr)))
                                                                T(std::forward<Args>(args)...);
}
#endif // __cpp_lib_constexpr_dynamic_alloc
} // namespace q20


namespace q20 {
// like std::to_address
#ifdef __cpp_lib_to_address
using std::to_address;
#else
// http://eel.is/c++draft/pointer.conversion
template <typename T>
constexpr T *to_address(T *p) noexcept {
    // http://eel.is/c++draft/pointer.conversion#1:
    //    Mandates: T is not a function type.
    static_assert(!std::is_function_v<T>, "to_address must not be used on function types");
    return p;
}

template <typename Ptr, typename std::enable_if_t<!std::is_pointer_v<Ptr>, bool> = true>
constexpr auto to_address(const Ptr &ptr) noexcept; // fwd declared

namespace detail {
    // http://eel.is/c++draft/pointer.conversion#3
    template <typename Ptr, typename = void>
    struct to_address_helper {
        static auto get(const Ptr &ptr) noexcept
        { return q20::to_address(ptr.operator->()); }
    };
    template <typename Ptr>
    struct to_address_helper<Ptr, std::void_t<
            decltype(std::pointer_traits<Ptr>::to_address(std::declval<const Ptr&>()))
        >>
    {
        static auto get(const Ptr &ptr) noexcept
        { return std::pointer_traits<Ptr>::to_address(ptr); }
    };
} // namespace detail

template <typename Ptr, typename std::enable_if_t<!std::is_pointer_v<Ptr>, bool>>
constexpr auto to_address(const Ptr &ptr) noexcept
{ return detail::to_address_helper<Ptr>::get(ptr); }

#endif // __cpp_lib_to_address
} // namespace q20

QT_END_NAMESPACE

#endif /* Q20MEMORY_H */
