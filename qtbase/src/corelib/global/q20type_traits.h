// Copyright (C) 2021 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef Q20TYPE_TRAITS_H
#define Q20TYPE_TRAITS_H

#include <QtCore/qtconfigmacros.h>

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

#include <type_traits>

QT_BEGIN_NAMESPACE

namespace q20 {
// like std::remove_cvref(_t)
#ifdef __cpp_lib_remove_cvref
using std::remove_cvref;
using std::remove_cvref_t;
#else
template <typename T>
using remove_cvref = std::remove_cv<std::remove_reference_t<T>>;
template <typename T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;
#endif // __cpp_lib_remove_cvref
}

namespace q20 {
// like std::type_identity(_t)
#ifdef __cpp_lib_type_identity
using std::type_identity;
using std::type_identity_t;
#else
template <typename T>
struct type_identity { using type = T; };
template <typename T>
using type_identity_t = typename type_identity<T>::type;
#endif // __cpp_lib_type_identity
}

QT_END_NAMESPACE

#endif /* Q20TYPE_TRAITS_H */
