// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtHttpServer/qhttpserver.h>

#include <QtHttpServer/qhttpserverrequest.h>
#include <QtHttpServer/qhttpserverresponder.h>
#include <QtHttpServer/qhttpserverresponse.h>

#include <private/qhttpserver_p.h>
#include <private/qhttpserverstream_p.h>

#include <QtCore/qloggingcategory.h>

#include <QtNetwork/qtcpsocket.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcHS, "qt.httpserver");

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

    QHttpServer allows to create a simple Http server by setting a range of
    request handlers.

    The \l route function can be used to conveniently add rules to
    the servers \l QHttpServerRouter. To register a handler to be called after
    every request use \l addAfterRequestHandler and to register a handler for
    all unhandled requests use \l setMissingHandler.

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

/*! \fn template <typename Rule = QHttpServerRouterRule, typename Functor> Rule *QHttpServer::route(const QString &pathPattern, QHttpServerRequest::Methods method, const QObject *receiver, Functor &&slot)

    This is a convenience method to add a new \c Rule to the server's
    \l{QHttpServerRouter}. The Rule template parameter can be any custom class
    derived from QHttpServerRouterRule.

    This function takes a \a pathPattern and a \a method that represent a set
    of requests and creates a new \l{QHttpServerRouterRule} (or custom Rule if
    specified in the template parameters) that forwards all respective requests
    to the provided \a receiver and \a slot. The rule is added to the
    \l{router}. For details on valid patterns in \a pathPattern, see the
    \l{QHttpServerRouterRule} documentation.

    \a slot can be a member function pointer of \a receiver. It can also be
    a function pointer, a non-mutable lambda, or any other copiable callable
    with const call operator. In that case \a receiver has to be a \l QObject
    pointer. The rule will be valid for the lifetime duration of the \a
    receiver. The receiver must share the same thread affinity as the
    QHttpServer for the registration to be successful and for the rule to be
    executed.

    The slot can express its response with a return statement. The function has
    to return QHttpServerResponse or any type that can be converted to
    QHttpServerResponse. A large range of conversion constructors are available,
    see \l{QHttpServerResponse}.

    \code
    QHttpServer server;
    server.route("/test", this, [] () { return ""; });
    \endcode

    Alternatively, an optional last function argument \c {QHttpServerResponder&}
    can be provided on which the response has to be written. If the response is
    written to a \c {QHttpServerResponder&} the function must return \c void.

    \code
    server.route("/test2", this, [] (QHttpServerResponder &responder) {
                                    responder.write(QHttpServerResponder::StatusCode::Forbidden); });
    \endcode

    The slot can further have a \c {const QHttpServerRequest&} as a
    second to last parameter to get detailed information on the request

    \code
    server.route("/test3", this, [] (const QHttpServerRequest &request,
                                    QHttpServerResponder &responder) {
                                    responder.write(request.body(), "text/plain"_ba);});
    \endcode

    Finally, the callback can contain an arbitrary amount of copiable
    parameters that are registered with the QHttpServerRouter::converters. By
    default, these are most integer types, float, double, QString, QByteArray,
    and QUrl. Additional converters can be registered, see
    \l{QHttpServerRouter::addConverter}. These parameters must have a
    corresponding placeholder in the \a pathPattern. For details on
    placeholders and pathPattern see \l{QHttpServerRouterRule}.

    \code
    QHttpServer server;
    server.route("/test/<arg>", this, [] (const int page) { return ""; });
    \endcode

    This function returns, if successful, a pointer to the newly created Rule,
    otherwise a \c nullptr. The pointer can be used to set parameters on any
    custom \l{QHttpServerRouter} class:

    \code
    auto rule = server.route<MyRule>("/test", this, [] () {return "";});
    rule->setParameter("test");
    \endcode

.   This function must not be called from any \l route callback.

    \note If a request was processed by a handler accepting \c
    {QHttpServerResponder&} as an argument, none of the after request handlers
    (see \l addAfterRequestHandler) will be called.

    Requests are processed sequentially inside the \c {QHttpServer}'s thread
    by default. The request handler may return \c {QFuture<QHttpServerResponse>}
    if asynchronous processing is desired:

    \code
    server.route("/feature/", [] (int id) {
        return QtConcurrent::run([] () {
            return QHttpServerResponse("the future is coming");
        });
    });
    \endcode

    The body of \c QFuture is executed asynchronously, but all the network
    communication is executed sequentially in the thread the \c {QHttpServer}
    belongs to. The \c {QHttpServerResponder&} special argument is not
    available for routes returning a \c {QFuture}.

    \sa QHttpServerRouter::addRule, addAfterRequestHandler
*/

/*! \fn template <typename Rule = QHttpServerRouterRule, typename Functor> Rule *QHttpServer::route(const QString &pathPattern, const QObject *receiver, Functor &&slot)

    \overload

    Overload of \l QHttpServer::route to create a Rule for \a pathPattern and
    \l QHttpServerRequest::Method::AnyKnown. All requests are forwarded
    to \a receiver and \a slot.
*/

/*! \fn template <typename Rule = QHttpServerRouterRule, typename Functor> Rule *QHttpServer::route(const QString &pathPattern, QHttpServerRequest::Methods method, Functor &&handler)

    \overload

    Overload of \l QHttpServer::route to create a Rule for \a pathPattern and
    \a method. All requests are forwarded to \a handler, which can be a
    function pointer, a non-mutable lambda, or any other copiable callable with
    const call operator. The rule will be valid until the QHttpServer is
    destroyed.
*/

/*! \fn template <typename Rule = QHttpServerRouterRule, typename Functor> Rule *QHttpServer::route(const QString &pathPattern, Functor &&handler)

    \overload

    Overload of \l QHttpServer::route to create a Rule for \a pathPattern and
    \l QHttpServerRequest::Method::AnyKnown. All requests are forwarded to \a
    handler, which can be a function pointer, a non-mutable lambda, or any
    other copiable callable with const call operator. The rule will be valid
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

/*! \fn template <typename Functor> void QHttpServer::setMissingHandler(const QObject *receiver, Functor &&slot)
    Set a handler for unhandled requests.

    All unhandled requests will be forwarded to the \a{receiver}'s \a slot.

    The \a slot has to implement the signature \c{void (*)(const
    QHttpServerRequest &, QHttpServerResponder &)}. The \a slot can also be a
    function pointer, non-mutable lambda, or any other copiable callable with
    const call operator. In that case the \a receiver will be a context object.
    The handler will be valid until the receiver object is destroyed.

    The default handler replies with status 404: Not Found.
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
    status 404 Not Found.
*/
void QHttpServer::clearMissingHandler()
{
    Q_D(QHttpServer);
    d->missingHandler.slotObject.reset();
}


/*! \fn template <typename Functor> void QHttpServer::addAfterRequestHandler(const QObject *receiver, Functor &&slot)
    Register a \a receiver and \a slot to be called after every request is
    handled.

    The \a slot has to implement the signature \c{void (*)(const QHttpServerRequest &,
    QHttpServerResponse &)}.

    The \a slot can also be a function pointer, non-mutable lambda, or any other
    copiable callable with const call operator. In that case the \a receiver will
    be a context object and the handler will be valid until the context
    object is destroyed.

    Example:

    \code
    QHttpServer server;
    server.addAfterRequestHandler(&server, [] (const QHttpServerRequest &req, QHttpServerResponse &resp) {
        resp.write(req.body(), "text/plain"_ba);
    }
    \endcode

    \note These handlers won't be called for requests, processed by handlers
    with \c {QHttpServerResponder&} argument.
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
