// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include "qgrpcclientinterceptormanager.h"
#include "qgrpcclientinterceptormanager_p.h"

#include "qgrpccallreply.h"
#include "qgrpcstream.h"

QT_BEGIN_NAMESPACE

/*!
    \class QGrpcClientInterceptorManager
    \inmodule QtGrpc
    \since 6.7
    \brief Manages QGrpcClientInterceptor interceptors.

    The QGrpcClientInterceptorManager class provides methods for registering and executing
    QGrpcClientInterceptor interceptors.

    \sa {Qt GRPC Client Interceptors}
*/

/*!
    The default destructor, destroyes QGrpcClientInterceptorManager object.
*/
QGrpcClientInterceptorManager::~QGrpcClientInterceptorManager()
{
    delete d_ptr;
}

/*!
    The default constructor, creates QGrpcClientInterceptorManager object.
*/
QGrpcClientInterceptorManager::QGrpcClientInterceptorManager()
    : d_ptr(new QGrpcClientInterceptorManagerPrivate())
{
}

QGrpcClientInterceptorManager::QGrpcClientInterceptorManager(const QGrpcClientInterceptorManager
                                                                 &other)
    : d_ptr(new QGrpcClientInterceptorManagerPrivate(*other.d_ptr))
{
}

QGrpcClientInterceptorManager::QGrpcClientInterceptorManager(QGrpcClientInterceptorManager &&other)
    : d_ptr(std::exchange(other.d_ptr, nullptr))
{
}

QGrpcClientInterceptorManager &
QGrpcClientInterceptorManager::operator=(const QGrpcClientInterceptorManager &other)
{
    if (&other != this)
        *d_ptr = *other.d_ptr;
    return *this;
}

QGrpcClientInterceptorManager &
QGrpcClientInterceptorManager::operator=(QGrpcClientInterceptorManager &&other)
{
    if (&other != this)
        d_ptr = std::exchange(other.d_ptr, nullptr);
    return *this;
}

/*!
    Registers a QGrpcClientInterceptor interceptor.

    Places the \a next interceptor at the beginning of the interceptor chain.
    Interceptors are executed in reverse order of registration.
    For instance,
    \code
        manager.registerInterceptor(myInterceptor1);
        manager.registerInterceptor(myInterceptor2);
    \endcode
    will result in:
    \badcode
        myInterceptor2 -> myInterceptor1 -> Qt GRPC operation
    \endcode
    order of execution.
*/
void QGrpcClientInterceptorManager::registerInterceptor(std::shared_ptr<QGrpcClientInterceptor>
                                                            next)
{
    Q_D(QGrpcClientInterceptorManager);
    d->interceptors.push_back(next);
}

/*!
    \fn void QGrpcClientInterceptorManager::registerInterceptors(std::vector<std::shared_ptr<QGrpcClientInterceptor>> nextInterceptors)
    Registers multiple QGrpcClientInterceptor interceptors.

    Adds the given interceptors to the start of the interceptor chain.
    While execution occurs in the reverse order of registration,
    the original order in the \a nextInterceptors vector is maintained,
    for example:
    \code
        manager.registerInterceptor(myInterceptor1);
        manager.registerInterceptors({myInterceptor2, myInterceptor3});
    \endcode
    will result in:
    \badcode
        myInterceptor2 -> myInterceptor3 -> myInterceptor1 -> Qt GRPC operation
    \endcode
    order of execution.
 */
void QGrpcClientInterceptorManager::
    registerInterceptors(std::vector<std::shared_ptr<QGrpcClientInterceptor>> nextInterceptors)
{
    Q_D(QGrpcClientInterceptorManager);
    d->interceptors.insert(d_ptr->interceptors.end(), nextInterceptors.rbegin(),
                           nextInterceptors.rend());
}

/*!
    \fn template <typename T> void QGrpcClientInterceptorManager::run(QGrpcInterceptorContinuation<T> &finalCall,
                                        typename QGrpcInterceptorContinuation<T>::ReplyType response,
                                        std::shared_ptr<QGrpcChannelOperation> operation,
                                        size_t pos)
    Executes the Qt GRPC interceptors in the chain for a specific QGrpcOperation type.

    The process initiates with the interceptor located at position
    \a pos in the QGrpcClientInterceptor chain.
    Both \a response and \a operation parameters are relayed to
    the QGrpcClientInterceptor::intercept() method.
    Upon reaching the end of the interceptor chain, the \a finalCall is invoked.
 */
template <typename T>
void QGrpcClientInterceptorManager::run(QGrpcInterceptorContinuation<T> &finalCall,
                                        typename QGrpcInterceptorContinuation<T>::ReplyType
                                            response,
                                        std::shared_ptr<QGrpcChannelOperation> operation,
                                        size_t pos)
{
    if (response->isFinished())
        return;

    Q_D(QGrpcClientInterceptorManager);

    if (pos < d_ptr->interceptors.size()) {
        auto nextCall =
            [this, pos, &finalCall](typename QGrpcInterceptorContinuation<T>::ReplyType response,
                                    typename QGrpcInterceptorContinuation<T>::ParamType operation) {
                this->run(finalCall, response, operation, pos + 1);
            };
        auto nextInterceptor = QGrpcInterceptorContinuation<T>(nextCall);
        // Execute interceptors in reversed order
        const auto rpos = d_ptr->interceptors.size() - 1 - pos;
        d->interceptors[rpos]->intercept<T>(operation, response, nextInterceptor);
        return;
    }
    // It's the time to call actuall call
    finalCall(response, operation);
}

template Q_GRPC_EXPORT void
QGrpcClientInterceptorManager::run(QGrpcInterceptorContinuation<QGrpcCallReply> &finalCall,
                                   typename QGrpcInterceptorContinuation<QGrpcCallReply>::ReplyType
                                       response,
                                   std::shared_ptr<QGrpcChannelOperation> operation, size_t pos);
template Q_GRPC_EXPORT void
QGrpcClientInterceptorManager::run(QGrpcInterceptorContinuation<QGrpcServerStream> &finalCall,
                                   typename QGrpcInterceptorContinuation<
                                       QGrpcServerStream>::ReplyType response,
                                   std::shared_ptr<QGrpcChannelOperation> operation, size_t pos);
template Q_GRPC_EXPORT void
QGrpcClientInterceptorManager::run(QGrpcInterceptorContinuation<QGrpcClientStream> &finalCall,
                                   typename QGrpcInterceptorContinuation<
                                       QGrpcClientStream>::ReplyType response,
                                   std::shared_ptr<QGrpcChannelOperation> operation, size_t pos);
template Q_GRPC_EXPORT void
QGrpcClientInterceptorManager::run(QGrpcInterceptorContinuation<QGrpcBidirStream> &finalCall,
                                   typename QGrpcInterceptorContinuation<
                                       QGrpcBidirStream>::ReplyType response,
                                   std::shared_ptr<QGrpcChannelOperation> operation, size_t pos);

QT_END_NAMESPACE
