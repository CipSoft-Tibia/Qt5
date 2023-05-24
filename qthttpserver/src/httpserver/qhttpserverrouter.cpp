// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qhttpserverrouter_p.h"

#include <QtHttpServer/qhttpserverrouter.h>
#include <QtHttpServer/qhttpserverrouterrule.h>
#include <QtHttpServer/qhttpserverrequest.h>

#include <private/qhttpserverrouterrule_p.h>

#include <QtCore/qloggingcategory.h>
#include <QtCore/qmetatype.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcRouter, "qt.httpserver.router")

using namespace Qt::StringLiterals;

/*!
    \internal
*/
static const QHash<QMetaType, QString> defaultConverters = {
    { QMetaType::fromType<int>(), u"[+-]?\\d+"_s },
    { QMetaType::fromType<long>(), u"[+-]?\\d+"_s },
    { QMetaType::fromType<long long>(), u"[+-]?\\d+"_s },
    { QMetaType::fromType<short>(), u"[+-]?\\d+"_s },

    { QMetaType::fromType<unsigned int>(), u"[+]?\\d+"_s },
    { QMetaType::fromType<unsigned long>(), u"[+]?\\d+"_s },
    { QMetaType::fromType<unsigned long long>(), u"[+]?\\d+"_s },
    { QMetaType::fromType<unsigned short>(), u"[+]?\\d+"_s },

    { QMetaType::fromType<double>(), u"[+-]?(?:[0-9]+(?:[.][0-9]*)?|[.][0-9]+)"_s },
    { QMetaType::fromType<float>(), u"[+-]?(?:[0-9]+(?:[.][0-9]*)?|[.][0-9]+)"_s },

    { QMetaType::fromType<QString>(), u"[^/]+"_s },
    { QMetaType::fromType<QByteArray>(), u"[^/]+"_s },

    { QMetaType::fromType<QUrl>(), u".*"_s },

    { QMetaType::fromType<void>(), u""_s },
};

/*!
    \class QHttpServerRouter
    \since 6.4
    \brief Provides functions to bind a URL to a \c ViewHandler.
    \inmodule QtHttpServer

    You can register \c ViewHandler as a callback for requests to a specific URL.
    Variable parts in the route can be specified by the arguments in ViewHandler.

    \note This is a low-level routing API for an HTTP server.

    See the following example:

    \code
    auto pageView = [] (const quint64 page) {
        qDebug() << "page" << page;
    };
    using ViewHandler = decltype(pageView);

    QHttpServerRouter router;

    // register callback pageView on request "/page/<number>"
    // for example: "/page/10", "/page/15"
    router.addRoute<ViewHandler>(
        new QHttpServerRouterRule("/page/", [=] (QRegularExpressionMatch &match,
                                                 const QHttpServerRequest &,
                                                 QHttpServerResponder &&) {
        auto boundView = router.bindCaptured(pageView, match);

        // it calls pageView
        boundView();
    }));
    \endcode
*/

/*! \fn template <typename Type> bool QHttpServerRouter::addConverter(QAnyStringView regexp)

    Adds a new converter for type \e Type matching regular expression \a regexp,
    and returns \c true if this was successful, otherwise returns \c false.

    Automatically try to register an implicit converter from QString to \e Type.
    If there is already a converter of type \e Type, that converter's regexp
    is replaced with \a regexp.

    \code
    struct CustomArg {
        int data = 10;

        CustomArg() {} ;
        CustomArg(const QString &urlArg) : data(urlArg.toInt()) {}
    };
    Q_DECLARE_METATYPE(CustomArg);

    QHttpServerRouter router;
    router.addConverter<CustomArg>(u"[+-]?\\d+"));

    auto pageView = [] (const CustomArg &customArg) {
        qDebug("data: %d", customArg.data);
    };
    using ViewHandler = decltype(pageView);

    auto rule = std::make_unique<QHttpServerRouterRule>(
        "/<arg>/<arg>/log",
        [&router, &pageView] (QRegularExpressionMatch &match,
                              const QHttpServerRequest &request,
                              QHttpServerResponder &&responder) {
        // Bind and call viewHandler with match's captured string and quint32:
        router.bindCaptured(pageView, match)();
    });

    router.addRule<ViewHandler>(std::move(rule));
    \endcode
*/

/*! \fn template <typename ViewHandler, typename ViewTraits = QHttpServerRouterViewTraits<ViewHandler>> bool QHttpServerRouter::addRule(std::unique_ptr<QHttpServerRouterRule> rule)

    Adds a new \a rule and returns \c true if this was successful.

    Inside addRule, we determine ViewHandler arguments and generate a list of
    their QMetaType::Type ids. Then we parse the URL and replace each \c <arg>
    with a regexp for its type from the list.

    \code
    QHttpServerRouter router;

    using ViewHandler = decltype([] (const QString &page, const quint32 num) { });

    auto rule = std::make_unique<QHttpServerRouterRule>(
        "/<arg>/<arg>/log",
        [] (QRegularExpressionMatch &match,
            const QHttpServerRequest &request,
            QHttpServerResponder &&responder) {
    });

    router.addRule<ViewHandler>(std::move(rule));
    \endcode
*/

/*! \fn template<typename ViewHandler, typename ViewTraits = QHttpServerRouterViewTraits<ViewHandler>> typename ViewTraits::BindableType QHttpServerRouter::bindCaptured(ViewHandler &&handler, const QRegularExpressionMatch &match) const

    Supplies the \a handler with arguments derived from a URL.
    Returns the bound function that accepts whatever remaining arguments the handler may take,
    supplying them to the handler after the URL-derived values.
    Each match of the regex applied to the URL (as a string) is converted to
    the type of the handler's parameter at its position, so that it can be
    passed as \a match.

    \code
    QHttpServerRouter router;

    auto pageView = [] (const QString &page, const quint32 num) {
        qDebug("page: %s, num: %d", qPrintable(page), num);
    };
    using ViewHandler = decltype(pageView);

    auto rule = std::make_unique<QHttpServerRouterRule>(
        "/<arg>/<arg>/log",
        [&router, &pageView] (QRegularExpressionMatch &match,
                              const QHttpServerRequest &request,
                              QHttpServerResponder &&responder) {
        // Bind and call viewHandler with match's captured string and quint32:
        router.bindCaptured(pageView, match)();
    });

    router.addRule<ViewHandler>(std::move(rule));
    \endcode
*/

QHttpServerRouterPrivate::QHttpServerRouterPrivate()
    : converters(defaultConverters)
{}

/*!
    Creates a QHttpServerRouter object with default converters.

    \sa converters()
*/
QHttpServerRouter::QHttpServerRouter()
    : d_ptr(new QHttpServerRouterPrivate)
{}

/*!
    Destroys a QHttpServerRouter.
*/
QHttpServerRouter::~QHttpServerRouter()
{}

/*!
    Adds a new converter for type \a metaType matching regular expression \a regexp.

    If there is already a converter of type \a metaType, that converter's regexp
    is replaced with \a regexp.
*/
void QHttpServerRouter::addConverter(QMetaType metaType, QAnyStringView regexp)
{
    Q_D(QHttpServerRouter);
    d->converters[metaType] = regexp.toString();
}

/*!
    Removes the converter for type \a metaType.
*/
void QHttpServerRouter::removeConverter(QMetaType metaType)
{
    Q_D(QHttpServerRouter);
    d->converters.remove(metaType);
}

/*!
    Removes all converters.

    \note clearConverters() does not set up default converters.

    \sa converters()
*/
void QHttpServerRouter::clearConverters()
{
    Q_D(QHttpServerRouter);
    d->converters.clear();
}

/*!
    Returns a map of converter type and regexp.

    The following converters are available by default:

    \value QMetaType::Int
    \value QMetaType::Long
    \value QMetaType::LongLong
    \value QMetaType::Short
    \value QMetaType::UInt
    \value QMetaType::ULong
    \value QMetaType::ULongLong
    \value QMetaType::UShort
    \value QMetaType::Double
    \value QMetaType::Float
    \value QMetaType::QString
    \value QMetaType::QByteArray
    \value QMetaType::QUrl
    \value QMetaType::Void       An empty converter.
*/
const QHash<QMetaType, QString> &QHttpServerRouter::converters() const
{
    Q_D(const QHttpServerRouter);
    return d->converters;
}

bool QHttpServerRouter::addRuleImpl(std::unique_ptr<QHttpServerRouterRule> rule,
                                    std::initializer_list<QMetaType> metaTypes)
{
    Q_D(QHttpServerRouter);

    if (!rule->hasValidMethods() || !rule->createPathRegexp(metaTypes, d->converters)) {
        return false;
    }

    d->rules.push_back(std::move(rule));
    return true;
}

/*!
    Handles each new \a request for the HTTP server using \a responder.

    Iterates through the list of rules to find the first that matches,
    then executes this rule, returning \c true. Returns \c false if no rule
    matches the request.
*/
bool QHttpServerRouter::handleRequest(const QHttpServerRequest &request,
                                      QHttpServerResponder &responder) const
{
    Q_D(const QHttpServerRouter);
    for (const auto &rule : d->rules) {
        if (rule->exec(request, responder))
            return true;
    }

    return false;
}

QT_END_NAMESPACE
