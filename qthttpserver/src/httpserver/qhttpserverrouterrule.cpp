// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtHttpServer/qhttpserverrouterrule.h>
#include <QtHttpServer/qhttpserverresponder.h>

#include <private/qhttpserverrouterrule_p.h>
#include <private/qhttpserverrequest_p.h>

#include <QtCore/qmetaobject.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qregularexpression.h>
#include <QtCore/qstringbuilder.h>
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcRouterRule, "qt.httpserver.router.rule")

/*!
    \class QHttpServerRouterRule
    \since 6.4
    \brief The QHttpServerRouterRule is the base class for QHttpServerRouter rules.
    \inmodule QtHttpServer

    Use QHttpServerRouterRule to specify expected request parameters:

    \value path                 QUrl::path()
    \value HTTP methods         QHttpServerRequest::Methods
    \value callback             User-defined response callback

    \note This is a low level API, see QHttpServer for higher level alternatives.

    Example of QHttpServerRouterRule and QHttpServerRouter usage:

    \code
    template<typename ViewHandler>
    void route(const char *path, const QHttpServerRequest::Methods methods, ViewHandler &&viewHandler)
    {
        auto rule = std::make_unique<QHttpServerRouterRule>(
                path, methods, [this, viewHandler = std::forward<ViewHandler>(viewHandler)]
                                                   (QRegularExpressionMatch &match,
                                                    const QHttpServerRequest &request,
                                                    QHttpServerResponder &&responder) mutable {
            auto boundViewHandler = router.bindCaptured<ViewHandler>(
                    std::move(viewHandler), match);
            // call viewHandler
            boundViewHandler();
        });

        // QHttpServerRouter
        router.addRule<ViewHandler>(std::move(rule));
    }

    // Valid:
    route("/user/", [] (qint64 id) { } );                            // "/user/1"
                                                                     // "/user/3"
                                                                     //
    route("/user/<arg>/history", [] (qint64 id) { } );               // "/user/1/history"
                                                                     // "/user/2/history"
                                                                     //
    route("/user/<arg>/history/", [] (qint64 id, qint64 page) { } ); // "/user/1/history/1"
                                                                     // "/user/2/history/2"

    // Invalid:
    route("/user/<arg>", [] () { } );  // ERROR: path pattern has <arg>, but ViewHandler does not have any arguments
    route("/user/\\d+", [] () { } );   // ERROR: path pattern does not support manual regexp
    \endcode

    \note Regular expressions in the path pattern are not supported, but
    can be registered (to match a use of "<val>" to a specific type) using
    QHttpServerRouter::addConverter().
*/

/*!
   \typealias QHttpServerRouterRule::RouterHandler

   Type alias for
    std::function<void(const QRegularExpressionMatch &,const QHttpServerRequest &, QHttpServerResponder &&)>
 */

/*!
    Constructs a rule with pathPattern \a pathPattern, and routerHandler \a routerHandler.

    The rule accepts all HTTP methods by default.

    \sa QHttpServerRequest::Methods
*/
QHttpServerRouterRule::QHttpServerRouterRule(const QString &pathPattern,
                                             RouterHandler routerHandler)
    : QHttpServerRouterRule(pathPattern,
                            QHttpServerRequest::Method::AnyKnown,
                            std::move(routerHandler))
{
}

/*!
    Constructs a rule with pathPattern \a pathPattern, methods \a methods
    and routerHandler \a routerHandler.

    The rule accepts any combinations of available HTTP methods.

    \sa QHttpServerRequest::Methods
*/
QHttpServerRouterRule::QHttpServerRouterRule(const QString &pathPattern,
                                             const QHttpServerRequest::Methods methods,
                                             RouterHandler routerHandler)
    : QHttpServerRouterRule(
        new QHttpServerRouterRulePrivate{pathPattern,
                                         methods,
                                         std::move(routerHandler), {}})
{
}

/*!
    \internal
 */
QHttpServerRouterRule::QHttpServerRouterRule(QHttpServerRouterRulePrivate *d)
    : d_ptr(d)
{
}

/*!
    Destroys a QHttpServerRouterRule.
*/
QHttpServerRouterRule::~QHttpServerRouterRule()
{
}

/*!
    Returns \c true if the methods is valid
*/
bool QHttpServerRouterRule::hasValidMethods() const
{
    Q_D(const QHttpServerRouterRule);
    return d->methods & QHttpServerRequest::Method::AnyKnown;
}

/*!
    Executes this rule for the given \a request, if it matches.

    This function is called by QHttpServerRouter when it receives a new
    request. If the given \a request matches this rule, this function handles
    the request by delivering a response to the given \a responder, then returns
    \c true. Otherwise, it returns \c false.
*/
bool QHttpServerRouterRule::exec(const QHttpServerRequest &request,
                                 QHttpServerResponder &responder) const
{
    Q_D(const QHttpServerRouterRule);

    QRegularExpressionMatch match;
    if (!matches(request, &match))
        return false;

    // Ensure that original responder object is destroyed even if the route
    // handler does not explicitly move out of it. The websocket handler code
    // assumes this is the case (see also QTBUG-120746).
    auto r = std::move(responder);
    d->routerHandler(match, request, std::move(r));
    return true;
}

/*!
    Determines whether a given \a request matches this rule.

    This virtual function is called by exec() to check if \a request matches
    this rule. If a match is found, it is stored in the object pointed to by
    \a match (which \e{must not} be \nullptr) and this function returns
    \c true. Otherwise, it returns \c false.
*/
bool QHttpServerRouterRule::matches(const QHttpServerRequest &request,
                                    QRegularExpressionMatch *match) const
{
    Q_D(const QHttpServerRouterRule);

    if (d->methods && !(d->methods & request.method()))
        return false;

    *match = d->pathRegexp.match(request.url().path());
    return (match->hasMatch() && d->pathRegexp.captureCount() == match->lastCapturedIndex());
}

/*!
    \internal
*/
bool QHttpServerRouterRule::createPathRegexp(std::initializer_list<QMetaType> metaTypes,
                                             const QHash<QMetaType, QString> &converters)
{
    Q_D(QHttpServerRouterRule);

    QString pathRegexp = d->pathPattern;
    const QLatin1StringView arg("<arg>");
    for (auto metaType : metaTypes) {
        if (metaType.id() >= QMetaType::User
            && !QMetaType::hasRegisteredConverterFunction(QMetaType::fromType<QString>(), metaType)) {
            qCWarning(lcRouterRule,
                      "%s has not registered a converter to QString. "
                      "Use QHttpServerRouter::addConveter<Type>(converter).",
                      metaType.name());
            return false;
        }

        auto it = converters.constFind(metaType);
        if (it == converters.end()) {
            qCWarning(lcRouterRule, "Can not find converter for type: %s", metaType.name());
            return false;
        }

        if (it->isEmpty())
            continue;

        const auto index = pathRegexp.indexOf(arg);
        const QString &regexp = QLatin1Char('(') % *it % QLatin1Char(')');
        if (index == -1)
            pathRegexp.append(regexp);
        else
            pathRegexp.replace(index, arg.size(), regexp);
    }

    if (pathRegexp.indexOf(arg) != -1) {
        qCWarning(lcRouterRule) << "not enough types or one of the types is not supported, regexp:"
                                << pathRegexp
                                << ", pattern:" << d->pathPattern
                                << ", types:" << metaTypes;
        return false;
    }

    if (!pathRegexp.startsWith(QLatin1Char('^')))
        pathRegexp = QLatin1Char('^') % pathRegexp;
    if (!pathRegexp.endsWith(QLatin1Char('$')))
        pathRegexp += u'$';

    qCDebug(lcRouterRule) << "url pathRegexp:" << pathRegexp;

    d->pathRegexp.setPattern(pathRegexp);
    d->pathRegexp.optimize();
    return true;
}

QT_END_NAMESPACE
