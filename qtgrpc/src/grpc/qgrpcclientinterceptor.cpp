// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include "qgrpcclientinterceptor.h"

QT_BEGIN_NAMESPACE

/*!
    \class QGrpcRpcInfo
    \inmodule QtGrpc
    \since 6.7
    \brief A class that defines response type for the specific QGrpcOperation.
    \internal

    The structure defines the \c ReplyType type alias, that serves as a mechanism
    to associate specific reply types with different QtGRPC call/stream types.
*/

/*!
    \typealias QGrpcRpcInfo::ReplyType
    \inmodule QtGrpc
    \since 6.7
    \internal
    \brief The alias for std::shared_ptr of the specific QGrpcOperation.

    The alias serves as a mechanism to associate specific reply types with
    different QtGRPC call/stream types.
*/

/*!
    \typealias QGrpcInterceptorContinuation::ReplyType
    \inmodule QtGrpc
    \since 6.7
    \internal
*/

/*!
    \typealias QGrpcInterceptorContinuation::ParamType
    \inmodule QtGrpc
    \since 6.7
    \internal
*/
/*!
    \class QGrpcInterceptorContinuation
    \inmodule QtGrpc
    \since 6.7
    \brief A template class for Qt GRPC interceptor continuation.

    The QGrpcInterceptorContinuation class is a helper class that wraps std::function,
    based on the QGrpcOperation type for the specific type of the call or stream. It is used
    for managing the continuation of Qt GRPC interceptor processing.
*/

/*!
    \fn template <typename T> QGrpcInterceptorContinuation<T>::QGrpcInterceptorContinuation(std::function<void(ReplyType, ParamType)> _func)

    Constructs a QGrpcInterceptorContinuation with the provided function \a _func.
*/
/*!
    \fn template <typename T> void QGrpcInterceptorContinuation<T>::operator()(ReplyType response,
        std::shared_ptr<QGrpcChannelOperation> param)

    Invokes the continuation function with the given \a response and \a param parameter.
*/
/*!
    \fn template <typename T> void QGrpcClientInterceptor::intercept(std::shared_ptr<QGrpcChannelOperation> operation,
        typename QGrpcInterceptorContinuation<T>::ReplyType response, QGrpcInterceptorContinuation<T> &continuation)

    Intercepts a Qt GRPC call or stream operation.

    This method provides a generic interface for intercepting Qt GRPC operations based on the
    specified QGrpcOperation.
    It delegates to specialized methods for different types of Qt GRPC operations
    and passes \a operation, \a response, and \a continuation parameters to specialized method.
*/

/*!
    \class QGrpcClientInterceptor
    \inmodule QtGrpc
    \since 6.7
    \brief Base class for Qt GRPC client interceptors.

    The QGrpcClientInterceptor class provides a base for creating custom Qt GRPC client interceptors.
    It defines methods for intercepting different types of Qt GRPC calls and streams. Users can it
    to implement specific interception behavior.

    \sa {Qt GRPC Client Interceptors}
*/

/*!
    Default destructor of QGrpcClientInterceptor object.
*/
QGrpcClientInterceptor::~QGrpcClientInterceptor() = default;

/*!
    Intercepts a Qt GRPC call operation.

    This method provides the default implementation of a virtual function. Users have the option
    to override this to offer specific functionality for QGrpcClientInterceptor.

    The \a operation carries the values associated with the call.
    The \a response carries a preallocated QGrpcCallReply of the call response.

    The \a continuation is a delegate that initiates the processing of the next interceptor or
    calls the underlying gRPC function. Implementations may call \a continuation zero or more times,
    depending on the desired outcome.
*/
void QGrpcClientInterceptor::interceptCall(std::shared_ptr<QGrpcChannelOperation> operation,
                                           std::shared_ptr<QGrpcCallReply> response,
                                           QGrpcInterceptorContinuation<QGrpcCallReply>
                                               &continuation)
{
    continuation(std::move(response), std::move(operation));
}

/*!
    Intercepts a Qt GRPC server streaming operation.

    This method provides the default implementation of a virtual function. Users have the option
    to override this to offer specific functionality for QGrpcClientInterceptor.

    The \a operation carries the values associated with the call.
    The \a response carries a preallocated QGrpcServerStream of the stream response.

    The \a continuation is a delegate that initiates the processing of the next interceptor or
    calls the underlying gRPC function. Implementations may call \a continuation zero or more times,
    depending on the desired outcome.
*/
void QGrpcClientInterceptor::interceptServerStream(std::shared_ptr<QGrpcChannelOperation> operation,
                                                   std::shared_ptr<QGrpcServerStream> response,
                                                   QGrpcInterceptorContinuation<QGrpcServerStream>
                                                       &continuation)
{
    continuation(std::move(response), std::move(operation));
}

/*!
    Intercepts a Qt GRPC client streaming operation.

    This method provides the default implementation of a virtual function. Users have the option
    to override this to offer specific functionality for QGrpcClientInterceptor.

    The \a operation carries the values associated with the call.
    The \a response carries a preallocated QGrpcClientStream of the stream response.

    The \a continuation is a delegate that initiates the processing of the next interceptor or
    calls the underlying gRPC function. Implementations may call \a continuation zero or more times,
    depending on the desired outcome.
*/
void QGrpcClientInterceptor::interceptClientStream(std::shared_ptr<QGrpcChannelOperation> operation,
                                                   std::shared_ptr<QGrpcClientStream> response,
                                                   QGrpcInterceptorContinuation<QGrpcClientStream>
                                                       &continuation)
{
    continuation(std::move(response), std::move(operation));
}

/*!
    Intercepts a Qt GRPC bidirectional streaming operation.

    This method provides the default implementation of a virtual function. Users have the option
    to override this to offer specific functionality for QGrpcClientInterceptor.

    The \a operation carries the values associated with the call.
    The \a response carries a preallocated QGrpcBidirStream of the stream response.

    The \a continuation is a delegate that initiates the processing of the next interceptor or
    calls the underlying gRPC function. Implementations may call \a continuation zero or more times,
    depending on the desired outcome.
*/
void QGrpcClientInterceptor::interceptBidirStream(std::shared_ptr<QGrpcChannelOperation> operation,
                                                  std::shared_ptr<QGrpcBidirStream> response,
                                                  QGrpcInterceptorContinuation<QGrpcBidirStream>
                                                      &continuation)
{
    continuation(std::move(response), std::move(operation));
}

QT_END_NAMESPACE
