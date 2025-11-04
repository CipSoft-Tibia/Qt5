// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include <QtHttpServer/qhttpserver.h>

#include <QtHttpServer/qhttpserverrequest.h>
#include <QtHttpServer/qhttpserverresponder.h>
#include <QtHttpServer/qhttpserverresponse.h>

#include <private/qhttpserver_p.h>
#include <private/qhttpserverstream_p.h>

#include <QtCore/qloggingcategory.h>

#include <QtNetwork/qtcpsocket.h>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(lcHS, "qt.httpserver");

QHttpServerPrivate::QHttpServerPrivate(QHttpServer *p) :
    router(p)
{
}

void QHttpServerPrivate::callMissingHandler(const QHttpServerRequest &request,
                                            QHttpServerResponder &responder)
{
    Q_Q(QHttpServer);

    if (missingHandler.context && missingHandler.slotObject &&
        verifyThreadAffinity(missingHandler.context)) {
        void *args[] = { nullptr, const_cast<QHttpServerRequest *>(&request), &responder };
        missingHandler.slotObject->call(const_cast<QObject *>(missingHandler.context.data()), args);
    } else {
        qCDebug(lcHS) << "missing handler:" << request.url().path();
        q->sendResponse(QHttpServerResponder::StatusCode::NotFound, request, std::move(responder));
    }
}

/*!
    \class QHttpServer
    \since 6.4
    \inmodule QtHttpServer
    \brief QHttpServer is a simplified API for QAbstractHttpServer and QHttpServerRouter.

    QHttpServer is used to create a simple HTTP server by registering a range
    of request handlers.

    The \l route function can be used to conveniently add rules to the
    server's \l QHttpServerRouter. To register a handler that is called after
    every request to further process the response use \l
    addAfterRequestHandler, but this mechanism only works for routes returning
    \l QHttpServerResponse or \c {QFuture<QHttpServerResponse>}. To register a
    handler for all unhandled requests use \l setMissingHandler.

    Minimal example:

    \code

    QHttpServer server;

    server.route("/", [] () {
        return "hello world";
    });

    auto tcpserver = new QTcpServer();
    if (!tcpserver->listen() || !server.bind(tcpserver)) {
        delete tcpserver;
        return -1;
    }
    qDebug() << "Listening on port" << tcpserver->serverPort();

    \endcode
*/

/*!
    Creates an instance of QHttpServer with parent \a parent.
*/
QHttpServer::QHttpServer(QObject *parent)
    : QAbstractHttpServer(*new QHttpServerPrivate(this), parent)
{
}

/*! \fn template <typename Rule = QHttpServerRouterRule, typename Functor> Rule *QHttpServer::route(const QString &pathPattern, QHttpServerRequest::Methods method, const QObject *context, Functor &&slot)

    This method is used to add a new \c Rule to the server's
    \l{QHttpServerRouter} member. The \c Rule template parameter can be any
    custom class derived from \l{QHttpServerRouterRule}. The parameters are
    passed to \c Rule. When handling the incoming HTTP requests, the
    \l{QHttpServerRouter} matches the \c Rule to the incoming HTTP request
    using the URL and HTTP method, and the first match of both is executed.
    The \a pathPattern parameter is compared with the \l{QUrl::}{path()} of
    the URL of the incoming request. The \a method parameter is compared with
    the HTTP method of the incoming request.

    The \a slot parameter can be a member function pointer of \a context.
    It can also be a function pointer, a non-mutable lambda, or any other
    copyable callable with const call operator. The rule will be valid for
    the lifetime duration of \a context. The \a context must share the same
    thread affinity as the QHttpServer for the registration to be successful
    and for the rule to be executed.

    The slot can express its response with a return statement. In that case
    the function has to return QHttpServerResponse or any type that can be
    converted to QHttpServerResponse. A large range of conversion
    constructors are available, see \l{QHttpServerResponse}.

    \code
    QHttpServer server;
    server.route("/test/", this, [] () { return ""; });
    \endcode

    Alternatively, an optional \l{QHttpServerResponder}& argument can be
    provided, in which case the response has to be written using it and
    the function must return \c void.

    \code
    server.route("/test2", this, [] (QHttpServerResponder &responder) {
                                    responder.write(QHttpServerResponder::StatusCode::Forbidden); });
    \endcode

    The \a slot can also have \c const \l{QHttpServerRequest}& as the last
    parameter, or as the second to last if the \l{QHttpServerResponder}&
    is the last parameter. It contains detailed information on the request.

    \code
    server.route("/test3", QHttpServerRequest::Method::Post, this,
                 [] (const QHttpServerRequest &request, QHttpServerResponder &responder) {
                     responder.write(request.body(), "text/plain"_ba);
                 });
    \endcode

    The \a slot can also take any amount of copyable parameters that can have
    the types available from \l{QHttpServerRouter::converters()}. By default,
    these are most integer types, float, double, QString, QByteArray, and
    QUrl. Converters for additional types can be added by calling
    \l{QHttpServerRouter::addConverter()}.

    The \a pathPattern can contain a number of \c "<arg>" substrings that are
    matched with the parameters of \a slot from left to right. The converters
    are chosen based on the types of these parameters.

    Each registered type has an associated regex that is used to match and
    convert occurrences of \c "<arg>" in the \a pathPattern. These regex
    patterns are combined to construct a parser for the entire path. The
    resulting parser is then used to verify if the path matches the pattern.
    If parsing succeeds, the corresponding function is called with the
    converted parameters. If parsing fails, the next registered callback is
    attempted.

    In the example below, the value in the request path replacing \c "<arg>"
    is converted to an \c int because the lambda expects an \c int parameter.
    When an HTTP request matches the route, the converted value is passed to
    the lambda's \c page argument:
    \code
    QHttpServer server;
    server.route("/showpage/<arg>", this, [] (const int page) { return getPage(page); });
    \endcode

    This function returns, if successful, a pointer to the newly created Rule,
    otherwise a \c nullptr. The pointer can be used to set parameters on any
    custom \l{QHttpServerRouter} class:

    \code
    auto rule = server.route<MyRule>("/test4", this, [] () {return "";});
    rule->setParameter("test");
    \endcode

    \note This function, \l route, must not be called from \a slot, so no
    route handlers can register other route handlers.

    \note If a request was processed by a \a slot accepting \l
    {QHttpServerResponder}& as an argument, none of the after request handlers
    (see \l addAfterRequestHandler) will be called.

    Requests are processed sequentially inside the \l {QHttpServer}'s thread
    by default. The request handler may return \c {QFuture<QHttpServerResponse>}
    if concurrent processing is desired:

    \code
    server.route("/feature/<arg>", [] (int ms) {
        return QtConcurrent::run([ms] () {
            QThread::msleep(ms);
            return QHttpServerResponse("the future is coming");
        });
    });
    \endcode

    The lambda of the QtConcurrent::run() is executed concurrently,
    but all the network communication is executed sequentially in the
    thread the \c {QHttpServer} belongs to after the QFuture is done.
    Be aware that any QHttpServerRequest object is passed by reference to
    the callback. Extract all needed content before QtConcurrent::run()
    is called.

    The \l{QHttpServerResponder}& special argument is only available for
    routes returning \c void. When using a responder object the response
    is returned using it.

    \sa QHttpServerRouter::addRule, addAfterRequestHandler
*/

/*! \fn template <typename Rule = QHttpServerRouterRule, typename Functor> Rule *QHttpServer::route(const QString &pathPattern, const QObject *context, Functor &&slot)

    \overload

    Overload of \l QHttpServer::route to create a Rule for \a pathPattern and
    the method \l{QHttpServerRequest::Method::AnyKnown}. All requests are
    forwarded to \a context and \a slot.
*/

/*! \fn template <typename Rule = QHttpServerRouterRule, typename Functor> Rule *QHttpServer::route(const QString &pathPattern, QHttpServerRequest::Methods method, Functor &&handler)

    \overload

    Overload of \l QHttpServer::route to create a Rule for \a pathPattern and
    \a method. All requests are forwarded to \a handler, which can be a
    function pointer, a non-mutable lambda, or any other copyable callable with
    const call operator. The rule will be valid until the QHttpServer is
    destroyed.
*/

/*! \fn template <typename Rule = QHttpServerRouterRule, typename Functor> Rule *QHttpServer::route(const QString &pathPattern, Functor &&handler)

    \overload

    Overload of \l QHttpServer::route to create a Rule for \a pathPattern and
    \l QHttpServerRequest::Method::AnyKnown. All requests are forwarded to \a
    handler, which can be a function pointer, a non-mutable lambda, or any
    other copyable callable with const call operator. The rule will be valid
    until the QHttpServer is destroyed.
*/

/*!
    Destroys a QHttpServer.
*/
QHttpServer::~QHttpServer()
{
}

/*!
    Returns a pointer to the router object.
*/
QHttpServerRouter *QHttpServer::router()
{
    Q_D(QHttpServer);
    return &d->router;
}

/*!
    Returns a pointer to the constant router object.
*/
const QHttpServerRouter *QHttpServer::router() const
{
    Q_D(const QHttpServer);
    return &d->router;
}

/*! \fn template <typename Functor> void QHttpServer::setMissingHandler(const QObject *context, Functor &&slot)
    Set a handler for unhandled requests.

    All unhandled requests will be forwarded to the \a{context}'s \a slot.

    The \a slot has to implement the signature \c{void (*)(const
    QHttpServerRequest &, QHttpServerResponder &)}. The \a slot can also be a
    function pointer, non-mutable lambda, or any other copyable callable with
    const call operator. In that case the \a context will be a context object.
    The handler will be valid until the context object is destroyed.

    The default handler replies with status \c{404 Not Found}.

    \sa clearMissingHandler
*/

/*!
    \internal
*/
void QHttpServer::setMissingHandlerImpl(const QObject *context, QtPrivate::QSlotObjectBase *handler)
{
    Q_D(QHttpServer);
    auto slot = QtPrivate::SlotObjUniquePtr(handler);
    if (!d->verifyThreadAffinity(context))
        return;
    d->missingHandler = {context, std::move(slot)};
}

/*!
    Resets the handler to the default one that produces replies with
    status \c{404 Not Found}.

    \sa setMissingHandler
*/
void QHttpServer::clearMissingHandler()
{
    Q_D(QHttpServer);
    d->missingHandler.slotObject.reset();
}


/*! \fn template <typename Functor> void QHttpServer::addAfterRequestHandler(const QObject *context, Functor &&slot)
    Register a \a context and \a slot to be called after each request is
    handled.

    The \a slot has to implement the signature \c{void (*)(const QHttpServerRequest &,
    QHttpServerResponse &)}.

    The \a slot can also be a function pointer, non-mutable lambda, or any other
    copyable callable with const call operator. In that case the \a context will
    be a context object and the handler will be valid until the context
    object is destroyed.

    Example:

    \code
    server.addAfterRequestHandler(&server, [] (const QHttpServerRequest &req, QHttpServerResponse &resp) {
        auto h = resp.headers();
        h.append(QHttpHeaders::WellKnownHeader::Cookie, "PollyWants=Cracker");
        resp.setHeaders(std::move(h));
    }
    \endcode

    \note These handlers will only be called for requests that are processed
    by route handlers that either return QHttpServerResponse or
    QFuture<QHttpServerResponse>, and therefore do not take a
    QHttpServerResponder argument.
*/

/*!
    \internal
*/
void QHttpServer::addAfterRequestHandlerImpl(const QObject *context, QtPrivate::QSlotObjectBase *handler)
{
    Q_D(QHttpServer);
    auto slot = QtPrivate::SlotObjUniquePtr(handler);
    if (!d->verifyThreadAffinity(context))
        return;
    d->afterRequestHandlers.push_back({context, std::move(slot)});
}

/*!
    \internal
*/
void QHttpServer::sendResponse(QHttpServerResponse &&response, const QHttpServerRequest &request,
                               QHttpServerResponder &&responder)
{
    Q_D(QHttpServer);
    for (auto &afterRequestHandler : d->afterRequestHandlers) {
        if (afterRequestHandler.context && afterRequestHandler.slotObject &&
            d->verifyThreadAffinity(afterRequestHandler.context)) {
            void *args[] = { nullptr, const_cast<QHttpServerRequest *>(&request), &response };
            afterRequestHandler.slotObject->call(const_cast<QObject *>(afterRequestHandler.context.data()), args);
        }
    }
    responder.sendResponse(response);
}

#if QT_CONFIG(future)
/*!
    \internal
*/
void QHttpServer::sendResponse(QFuture<QHttpServerResponse> &&response,
                               const QHttpServerRequest &request, QHttpServerResponder &&responder)
{
    response.then(this,
                  [this, &request,
                   responder = std::move(responder)](QHttpServerResponse &&response) mutable {
                      sendResponse(std::move(response), request, std::move(responder));
                  });
}
#endif // QT_CONFIG(future)

/*!
    \internal
*/
bool QHttpServer::handleRequest(const QHttpServerRequest &request, QHttpServerResponder &responder)
{
    Q_D(QHttpServer);
    return d->router.handleRequest(request, responder);
}

/*!
    \internal
*/
void QHttpServer::missingHandler(const QHttpServerRequest &request, QHttpServerResponder &responder)
{
    Q_D(QHttpServer);
    return d->callMissingHandler(request, responder);
}

QT_END_NAMESPACE

#include "moc_qhttpserver.cpp"
