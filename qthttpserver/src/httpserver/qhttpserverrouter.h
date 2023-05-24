// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QHTTPSERVERROUTER_H
#define QHTTPSERVERROUTER_H

#include <QtHttpServer/qthttpserverglobal.h>
#include <QtHttpServer/qhttpserverrouterviewtraits.h>

#include <QtCore/qscopedpointer.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qregularexpression.h>

#include <functional>
#include <initializer_list>
#include <memory>

QT_BEGIN_NAMESPACE

class QHttpServerResponder;
class QHttpServerRequest;
class QHttpServerRouterRule;

class QHttpServerRouterPrivate;
class Q_HTTPSERVER_EXPORT QHttpServerRouter
{
    Q_DECLARE_PRIVATE(QHttpServerRouter)
    Q_DISABLE_COPY_MOVE(QHttpServerRouter)

public:
    QHttpServerRouter();
    ~QHttpServerRouter();

    template<typename Type>
    bool addConverter(QAnyStringView regexp) {
        if (!QMetaType::registerConverter<QString, Type>())
            return false;

        addConverter(QMetaType::fromType<Type>(), regexp);
        return true;
    }

    void addConverter(QMetaType metaType, QAnyStringView regexp);
    void removeConverter(QMetaType metaType);
    void clearConverters();
    const QHash<QMetaType, QString> &converters() const;

    template<typename ViewHandler, typename ViewTraits = QHttpServerRouterViewTraits<ViewHandler>>
    bool addRule(std::unique_ptr<QHttpServerRouterRule> rule)
    {
        return addRuleHelper<ViewTraits>(
                std::move(rule),
                typename ViewTraits::Arguments::Indexes{});
    }

    template<typename ViewHandler, typename ViewTraits = QHttpServerRouterViewTraits<ViewHandler>>
    typename ViewTraits::BindableType bindCaptured(ViewHandler &&handler,
                      const QRegularExpressionMatch &match) const
    {
        return bindCapturedImpl<ViewHandler, ViewTraits>(
                std::forward<ViewHandler>(handler), match,
                typename ViewTraits::Arguments::CapturableIndexes{});
    }

    bool handleRequest(const QHttpServerRequest &request, QHttpServerResponder &responder) const;

private:
    template<typename ViewTraits, int ... Idx>
    bool addRuleHelper(std::unique_ptr<QHttpServerRouterRule> rule,
                       QtPrivate::IndexesList<Idx...>)
    {
        return addRuleImpl(std::move(rule), {ViewTraits::Arguments::template metaType<Idx>()...});
    }

    bool addRuleImpl(std::unique_ptr<QHttpServerRouterRule> rule,
                     std::initializer_list<QMetaType> metaTypes);

    // Implementation of C++20 std::bind_front() in C++17
    template<typename F, typename... Args>
    auto bind_front(F &&f, Args &&...args) const
    {
        return [f = std::forward<F>(f),
                args = std::make_tuple(std::forward<Args>(args)...)](auto &&...callArgs) {
            return std::apply(f,
                              std::tuple_cat(args,
                                             std::forward_as_tuple(std::forward<decltype(callArgs)>(
                                                     callArgs)...)));
        };
    }

    template<typename ViewHandler, typename ViewTraits, int... Cx>
    typename ViewTraits::BindableType bindCapturedImpl(ViewHandler &&handler,
                                                       const QRegularExpressionMatch &match,
                                                       QtPrivate::IndexesList<Cx...>) const
    {
        return bind_front(
                std::forward<ViewHandler>(handler),
                QVariant(match.captured(Cx + 1))
                        .value<typename ViewTraits::Arguments::template Arg<Cx>::CleanType>()...);
    }

    std::unique_ptr<QHttpServerRouterPrivate> d_ptr;
};

QT_END_NAMESPACE

#endif // QHTTPSERVERROUTER_H
