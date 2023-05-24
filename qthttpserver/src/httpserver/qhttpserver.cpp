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

void QHttpServerPrivate::callMissingHandler(const QHttpServerRequest &request,
                                            QHttpServerResponder &&responder)
{
    Q_Q(QHttpServer);

    if (missingHandler) {
        missingHandler(request, std::move(responder));
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

    \code

    QHttpServer server;

    server.route("/", [] () {
        return "hello world";
    });
    server.listen();

    \endcode
*/

/*!
    Creates an instance of QHttpServer with parent \a parent.
*/
QHttpServer::QHttpServer(QObject *parent)
    : QAbstractHttpServer(*new QHttpServerPrivate, parent)
{
}

/*! \fn template<typename Rule = QHttpServerRouterRule, typename ... Args> bool QHttpServer::route(Args && ... args)

    This function is just a wrapper to simplify the router API.

    This function takes variadic arguments \a args. The last argument is a
    callback (\c{ViewHandler}). The remaining arguments are used to create a
    new \c Rule (the default is QHttpServerRouterRule). This is in turn added
    to the QHttpServerRouter. It returns \c true if a new rule is created,
    otherwise it returns \c false.

    \c ViewHandler can be a function pointer, non-mutable lambda, or any
    other copiable callable with const call operator. The callable can take two
    optional special arguments: \c {const QHttpServerRequest&} and
    \c {QHttpServerResponder&&}. These special arguments must be the last in
    the parameter list, but in any order, and there can be none, one, or both
    of them present.

    Examples:

    \code

    QHttpServer server;

    // Valid:
    server.route("test", [] (const int page) { return ""; });
    server.route("test", [] (const int page, const QHttpServerRequest &request) { return ""; });
    server.route("test", [] (QHttpServerResponder &&responder) { return ""; });

    // Invalid (compile time error):
    server.route("test", [] (const QHttpServerRequest &request, const int page) { return ""; }); // request must be last
    server.route("test", [] (QHttpServerRequest &request) { return ""; });      // request must be passed by const reference
    server.route("test", [] (QHttpServerResponder &responder) { return ""; });  // responder must be passed by universal reference

    \endcode

    The request handler may return \c {QFuture<QHttpServerResponse>} if
    asynchronous processing is desired:

    \code
    server.route("/feature/", [] (int id) {
        return QtConcurrent::run([] () {
            return QHttpServerResponse("the future is coming");
        });
    });
    \endcode


    \sa QHttpServerRouter::addRule
*/

/*! \fn template<typename ViewHandler> void QHttpServer::afterRequest(ViewHandler &&viewHandler)
    Register a function to be run after each request.

    The \a viewHandler argument can be a function pointer, non-mutable lambda,
    or any other copiable callable with const call operator. The callable
    can take one or two optional arguments: \c {QHttpServerResponse &&} and
    \c {const QHttpServerRequest &}. If both are given, they can be in either
    order.

    Examples:

    \code

    QHttpServer server;

    // Valid:
    server.afterRequest([] (QHttpServerResponse &&resp, const QHttpServerRequest &request) {
        return std::move(resp);
    }
    server.afterRequest([] (const QHttpServerRequest &request, QHttpServerResponse &&resp) {
        return std::move(resp);
    }
    server.afterRequest([] (QHttpServerResponse &&resp) { return std::move(resp); }

    // Invalid (compile time error):
    // resp must be passed by universal reference
    server.afterRequest([] (QHttpServerResponse &resp, const QHttpServerRequest &request) {
        return std::move(resp);
    }
    // request must be passed by const reference
    server.afterRequest([] (QHttpServerResponse &&resp, QHttpServerRequest &request) {
        return std::move(resp);
    }

    \endcode
*/

/*!
    Destroys a QHttpServer.
*/
QHttpServer::~QHttpServer()
{
}

/*!
    Returns the router object.
*/
QHttpServerRouter *QHttpServer::router()
{
    Q_D(QHttpServer);
    return &d->router;
}

/*!
    \typealias QHttpServer::MissingHandler

    Type alias for std::function<void(const QHttpServerRequest &request,
                                      QHttpServerResponder &&responder)>.
*/

/*!
    Set a handler to call for unhandled paths.

    The invocable passed as \a handler will be invoked for each request
    that cannot be handled by any of registered route handlers. Passing a
    default-constructed std::function resets the handler to the default one
    that produces replies with status 404 Not Found.
*/
void QHttpServer::setMissingHandler(QHttpServer::MissingHandler handler)
{
    Q_D(QHttpServer);
    d->missingHandler = handler;
}

/*!
    \internal
*/
void QHttpServer::afterRequestImpl(AfterRequestHandler afterRequestHandler)
{
    Q_D(QHttpServer);
    d->afterRequestHandlers.push_back(std::move(afterRequestHandler));
}

/*!
    \internal
*/
void QHttpServer::sendResponse(QHttpServerResponse &&response, const QHttpServerRequest &request,
                               QHttpServerResponder &&responder)
{
    Q_D(QHttpServer);
    for (auto afterRequestHandler : d->afterRequestHandlers)
        response = afterRequestHandler(std::move(response), request);
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
void QHttpServer::missingHandler(const QHttpServerRequest &request,
                                 QHttpServerResponder &&responder)
{
    Q_D(QHttpServer);
    return d->callMissingHandler(request, std::move(responder));
}

QT_END_NAMESPACE

#include "moc_qhttpserver.cpp"
