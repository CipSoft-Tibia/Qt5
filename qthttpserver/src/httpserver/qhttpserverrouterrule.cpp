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

    QHttpServerRouterRule expresses the connection between a request path, an
    HTTP request method, and the respective handler callback. The \l
    QHttpServerRouter is a collection of such rules from which the handlers are
    called if the path and request method match the request. The handler
    callback must provide the response to the request.

    \section1 Path and Patterns

    Every QHttpServerRouterRule contains a path or path pattern which defines
    the paths for which it can provide a response through its handler. The path
    can contain placeholders that are forwarded to the rule's handler. The
    following examples of path patterns are shown with the \l
    QHttpServer::route convenience method, but can also be provided to the
    QHttpServerRouterRule constructor.

    In the simplest case the path is a string with a leading "/":
    \code
    QHttpServer server;
    server.route("/user", [] () { return "hello user"; } );
    \endcode
    This path pattern creates a rule that forwards all requests with "/user"
    to the provided hanlder, which in this case is a simple lambda (Note that
    the handler syntax would look different when using QHttpServerRouterRule
    directly, see below).

    The path pattern can further contain a trailing "/" to create a rule that
    addresses a collection of paths with arguments after the trailing "/". The
    argument will be forwarded to the Rule as a \l QRegularExpressionMatch.
    Using the QHttpServer::route convenience method the argument is directly
    forwarded to the lambda:
    \code
    server.route("/user/", [] ( qint64 id ) { return "hello user"; } );
    \endcode
    This would match the request urls "/user/1", "/user/2" and so on.

    The argument can be posititioned freely with the path pattern by using the
    "<arg>" placeholder. This keyword further allows multiple placeholder.
    \code
    server.route("/user/<arg>/history", [] (qint64 id){ return "hello user"; } );
    server.route("/user/<arg>/history/", [] (qint64 id, qint64 page){ return "hello user"; } );
    \endcode
    This would, for example, match the request url "/user/1/history/2".
    All types which are registered in \l QHttpServerRouter::converters() can be
    used in the callback and the respective placeholder.

    \section1 Request Method

    Request method is simply one of \l QHttpServerRequest::Method. If no
    method is provided to any overload of the Rule construction, the rule will
    match any request method.

    \section1 Handler Signature

    The handler is a callback with the signature
    \code
    void (*)(const QRegularExpressionMatch &, const QHttpServerRequest &, QHttpServerResponder &);
    \endcode

    The handler callback receives any matched placeholders as its first argument.
    The second argument contains details about the request and the response has
    to be written on the last argument by the handler.

    The following code example shows how new rules with the respective handler can be created and
    added to a \l QHttpServerRouter:
    \code
    template<typename ViewHandler>
    void route(const char *path, const QHttpServerRequest::Methods methods, ViewHandler &&viewHandler)
    {
        auto rule = std::make_unique<QHttpServerRouterRule>(
                path, methods, [this, viewHandler = std::forward<ViewHandler>(viewHandler)]
                                                (QRegularExpressionMatch &match,
                                                 const QHttpServerRequest &request,
                                                 QHttpServerResponder &responder) mutable {
            auto boundViewHandler = QHttpServerRouterRule::bindCaptured<ViewHandler>(
                    this, std::move(viewHandler), match);
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
    \endcode

    \note This is a low level API, see \l QHttpServer for higher level alternatives.

    \note Regular expressions in the path pattern are not supported, but
    can be registered (to match a use of "<arg>" to a specific type) using
    \l QHttpServerRouter::addConverter().
*/

/*! \fn template <typename Functor, typename ViewTraits = QHttpServerRouterViewTraits<Functor>> static typename ViewTraits::BindableType QHttpServerRouterRule::bindCaptured(QObject *receiver, Functor &&slot, const QRegularExpressionMatch &match) const

    Supplies the \a receiver and \a slot with arguments derived from a URL.
    Returns the bound function that accepts whatever remaining arguments the
    handler may take, supplying them to the slot after the URL-derived values.
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
        QHttpServerRouterRule::bindCaptured(pageView, match)();
    });

    router.addRule<ViewHandler>(std::move(rule));
    \endcode
*/

/*! \fn template <typename Functor> QHttpServerRouterRule::QHttpServerRouterRule(const QString &pathPattern, const QHttpServerRequest::Methods methods, const QObject *receiver, Functor &&slot)

    Constructs a rule for \a pathPattern, \a methods and connects it to \a
    receiver and \a slot. The \a slot can also be a function pointer,
    non-mutable lambda, or any other copiable callable with const call
    operator. In that case the \a receiver will be a context object. The
    handler will be valid until the receiver object is destroyed.

    The rule accepts any combinations of available HTTP methods.

    \sa QHttpServerRequest::Methods
*/

/*! \fn template <typename Functor> QHttpServerRouterRule::QHttpServerRouterRule(const QString &pathPattern, const QObject *receiver, Functor &&slot)

    \overload

    Constructs a rule for \a pathPattern, \l
    QHttpServerRequest::Method::AnyKnown and connects it to \a receiver and \a
    slot. The \a slot can also be a function pointer, non-mutable lambda, or
    any other copiable callable with const call operator. In that case the \a
    receiver will be a context object. The handler will be valid until the
    receiver object is destroyed.
*/
QHttpServerRouterRule::QHttpServerRouterRule(const QString &pathPattern,
                                             const QHttpServerRequest::Methods methods,
                                             const QObject *context,
                                             QtPrivate::QSlotObjectBase *slotObjRaw)
    : QHttpServerRouterRule(new QHttpServerRouterRulePrivate{
              pathPattern, methods, QtPrivate::SlotObjUniquePtr(slotObjRaw), QPointer(context), {}})
{
    Q_ASSERT(slotObjRaw);
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
    Returns the context object of this rule. This is the receiver that has to
    handle the request.
*/
const QObject *QHttpServerRouterRule::contextObject() const
{
    Q_D(const QHttpServerRouterRule);
    return d->context;
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
    Executes this rule for the given \a request.

    This function is called by \l QHttpServerRouter when it receives a new
    request. If the given \a request matches this rule, this function handles
    the request by delivering a response to the given \a responder, then returns
    \c true. Otherwise, it returns \c false.
*/
bool QHttpServerRouterRule::exec(const QHttpServerRequest &request,
                                 QHttpServerResponder &responder) const
{
    Q_D(const QHttpServerRouterRule);
    if (!d->routerHandler)
        return false;

    QRegularExpressionMatch match;
    if (!matches(request, &match))
        return false;

    void *args[] = { nullptr, &match, const_cast<QHttpServerRequest *>(&request), &responder };
    Q_ASSERT(d->routerHandler);
    d->routerHandler->call(nullptr, args);
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
