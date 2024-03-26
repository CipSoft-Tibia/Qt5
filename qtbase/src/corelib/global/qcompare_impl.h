// Copyright (C) 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCOMPARE_IMPL_H
#define QCOMPARE_IMPL_H

#if 0
#pragma qt_sync_skip_header_check
#pragma qt_sync_stop_processing
#endif

#include <QtCore/qtconfigmacros.h>
#include <QtCore/qcompilerdetection.h>

QT_BEGIN_NAMESPACE

namespace QtPrivate {

// [cmp.categories.pre] / 3, but using a safe bool trick
// and also rejecting std::nullptr_t (unlike the example)
class CompareAgainstLiteralZero {
public:
    using SafeZero = void (CompareAgainstLiteralZero::*)();
    Q_IMPLICIT constexpr CompareAgainstLiteralZero(SafeZero) noexcept {}

    template <typename T, std::enable_if_t<std::is_null_pointer_v<T>, bool> = true>
    CompareAgainstLiteralZero(T) = delete;
};

} // namespace QtPrivate

QT_END_NAMESPACE

#endif // QCOMPARE_IMPL_H
