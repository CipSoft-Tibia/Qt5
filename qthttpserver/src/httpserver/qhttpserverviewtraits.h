// Copyright (C) 2020 Mikhail Svetkin <mikhail.svetkin@gmail.com>
// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QHTTPSERVERVIEWTRAITS_H
#define QHTTPSERVERVIEWTRAITS_H

#include <QtHttpServer/qhttpserverviewtraits_impl.h>

QT_BEGIN_NAMESPACE

class QHttpServerRequest;
class QHttpServerResponse;

namespace QtPrivate {

template <typename ViewHandler, bool DisableStaticAssert>
struct AfterRequestViewTraitsHelper : ViewTraits<ViewHandler, DisableStaticAssert> {
    using VTraits = ViewTraits<ViewHandler, DisableStaticAssert>;
    using FunctionTraits = typename VTraits::FTraits;

    static_assert(DisableStaticAssert ||
                  FunctionTraits::ArgumentCount == 2 ||
                  FunctionTraits::ArgumentCount == 1,
                  "ViewHandler arguments error: "
                  "afterRequest can only accept QHttpServerResponse and QHttpServerRequest");

    static_assert(DisableStaticAssert ||
                  std::is_same<typename FunctionTraits::ReturnType,
                               QHttpServerResponse>::value,
                  "ViewHandler return type error: "
                  "Return type can only be QHttpServerResponse");

    template<int I>
    struct ArgumentChecker {
        using IsRequest = typename VTraits::template Special<I, const QHttpServerRequest &>;
        static_assert(IsRequest::AssertCondition,
                      "ViewHandler arguments error: "
                      "QHttpServerRequest can only be passed as a const reference");

        using IsResponse = typename VTraits::template Special<I, QHttpServerResponse &&>;
        static_assert(IsResponse::AssertCondition,
                      "ViewHandler arguments error: "
                      "QHttpServerResponse can only be passed as an rvalue reference");

        using IsSpecial = CheckAny<IsRequest, IsResponse>;

        static constexpr bool Valid = IsSpecial::Valid;
        static constexpr bool StaticAssert = IsSpecial::StaticAssert;
    };

    struct Arguments {
        template<int ... I>
        struct ArgumentsReturn {
            template<int Idx>
            using Arg = ArgumentChecker<Idx>;
            static constexpr bool Valid = (Arg<I>::Valid && ...);
            static constexpr bool StaticAssert = (Arg<I>::StaticAssert && ...);
            using Last = Arg<FunctionTraits::ArgumentIndexMax>;
            static constexpr std::size_t Count = FunctionTraits::ArgumentCount;
        };

        template<int ... I>
        static constexpr ArgumentsReturn<I...> eval(QtPrivate::IndexesList<I...>) noexcept
        {
            return ArgumentsReturn<I...>{};
        }
    };
};

} // namespace QtPrivate

template <typename ViewHandler, bool DisableStaticAssert = false>
struct QHttpServerAfterRequestViewTraits
{
    using Helpers = typename QtPrivate::AfterRequestViewTraitsHelper<ViewHandler, DisableStaticAssert>;
    using Arguments = decltype(Helpers::Arguments::eval(typename Helpers::ArgumentIndexes{}));
};

QT_END_NAMESPACE

#endif  // QHTTPSERVERVIEWTRAITS_H
