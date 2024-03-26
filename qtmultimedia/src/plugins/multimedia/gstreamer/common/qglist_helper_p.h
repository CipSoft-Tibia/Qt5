// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGLIST_HELPER_P_H
#define QGLIST_HELPER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qtconfigmacros.h>

#include <glib.h>
#include <iterator>

QT_BEGIN_NAMESPACE

namespace QGstUtils {

template <typename ListType>
struct GListIterator
{
    explicit GListIterator(const GList *element = nullptr) : element(element) { }

    const ListType &operator*() const noexcept { return *operator->(); }
    const ListType *operator->() const noexcept
    {
        return reinterpret_cast<const ListType *>(&element->data);
    }

    GListIterator &operator++() noexcept
    {
        if (element)
            element = element->next;

        return *this;
    }
    GListIterator operator++(int n) noexcept
    {
        for (int i = 0; i != n; ++i)
            operator++();

        return *this;
    }

    bool operator==(const GListIterator &r) const noexcept { return element == r.element; }
    bool operator!=(const GListIterator &r) const noexcept { return element != r.element; }

    using difference_type = std::ptrdiff_t;
    using value_type = ListType;
    using pointer = value_type *;
    using reference = value_type &;
    using iterator_category = std::input_iterator_tag;

    const GList *element = nullptr;
};

template <typename ListType>
struct GListRangeAdaptor
{
    static_assert(std::is_pointer_v<ListType>);

    explicit GListRangeAdaptor(const GList *list) : head(list) { }

    auto begin() { return GListIterator<ListType>(head); }
    auto end() { return GListIterator<ListType>(nullptr); }

    const GList *head;
};

} // namespace QGstUtils

QT_END_NAMESPACE

#endif
