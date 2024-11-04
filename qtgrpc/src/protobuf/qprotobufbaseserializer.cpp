// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qprotobufbaseserializer.h"

#include <QtProtobuf/qprotobufmessage.h>

QT_BEGIN_NAMESPACE

/*!
    \class QProtobufBaseSerializer
    \inmodule QtProtobuf
    \since 6.7
    \brief The QProtobufBaseSerializer class is an interface that represents
           basic functions for serializing/deserializing objects, lists, and enums.

    The QProtobufBaseSerializer class registers serializers/deserializers for
    classes implementing a protobuf message, inheriting \l QProtobufMessage. These
    classes are generated automatically, based on a \c{.proto} file, using the CMake
    function \l qt_add_protobuf or by running
    \l {The qtprotobufgen Tool} {qtprotobufgen} directly.

    This class should be used as a base for specific serializers. The handlers
    property contains all message-specific serializers and should be used while
    serialization/deserialization. Inherited classes should reimplement scope of
    virtual methods that used by registered message
    serialization/deserialization functions.
*/

/*!
    \fn void QProtobufBaseSerializer::serializeObject(const QProtobufMessage *message,
        const QtProtobufPrivate::QProtobufPropertyOrdering &ordering,
        const QtProtobufPrivate::QProtobufPropertyOrderingInfo &fieldInfo) const

    This function serializes a registered Protobuf message \a message
    with defined \a ordering and \a fieldInfo, that is recognized
    like an object, into a QByteArray. \a message must not be \nullptr.

    You should not call this function directly.

    \sa QProtobufBaseSerializer::deserializeObject()
*/

/*!
    \fn bool QProtobufBaseSerializer::deserializeObject(QProtobufMessage *message,
        const QtProtobufPrivate::QProtobufPropertyOrdering &ordering) const

    This function deserializes a registered Protobuf message \a message
    with defined \a ordering. \a message must not be \nullptr.
    Returns \c true if deserialization was successful, otherwise \c false.

    You should not call this function directly.

    \sa QProtobufBaseSerializer::serializeObject()
*/

/*!
    \fn void QProtobufBaseSerializer::serializeListObject(const QProtobufMessage *message,
        const QtProtobufPrivate::QProtobufPropertyOrdering &ordering,
        const QtProtobufPrivate::QProtobufPropertyOrderingInfo
        &fieldInfo) const

    This function serializes \a message as part of a list of messages one by one
    with \a ordering and \a fieldInfo.

    You should not call this function directly.

    \sa QProtobufBaseSerializer::deserializeListObject()
*/

/*!
    \fn bool QProtobufBaseSerializer::deserializeListObject(QProtobufMessage *message,
        const QtProtobufPrivate::QProtobufPropertyOrdering &ordering) const

    This function deserializes an \a message from byte stream as part of list property, with
    the associated message \a ordering from a wire.
    Returns \c true if deserialization was successful, otherwise \c false.

    You should not call this function directly.

    \sa QProtobufBaseSerializer::serializeListObject()
*/

/*!
    \fn void QProtobufBaseSerializer::serializeMapPair(const QVariant &key, const QVariant &value,
        const QtProtobufPrivate::QProtobufPropertyOrderingInfo &fieldInfo) const

    This function serializes pair of \a key and \a value, that belong as a protobuf map record,
    according to \a fieldInfo.

    You should not call this function directly.

    \sa QProtobufBaseSerializer::deserializeMapPair()
*/

/*!
    \fn bool QProtobufBaseSerializer::deserializeMapPair(QVariant &key, QVariant &value) const
    This function deserializes a pair of \a key and \a value from a wire.
    Returns \c true if deserialization was successful, otherwise \c false.

    You should not call this function directly.

    \sa QProtobufBaseSerializer::serializeMapPair()
*/

/*!
    \fn void QProtobufBaseSerializer::serializeEnum(QtProtobuf::int64 value,
        const QMetaEnum &metaEnum,
        const QtProtobufPrivate::QProtobufPropertyOrderingInfo &fieldInfo) const

    This function serializes \a value from enum associated with property \a fieldInfo.
    \a metaEnum helps to encode the enum value.

    You should not call this function directly.

    \sa QProtobufBaseSerializer::deserializeEnum()
*/

/*!
    \fn bool QProtobufBaseSerializer::deserializeEnum(QtProtobuf::int64 &value,
        const QMetaEnum &metaEnum) const

    This function deserializes an enum \a value from a wire. \a metaEnum helps to decode the enum
    value.
    Returns \c true if deserialization was successful, otherwise \c false.

    You should not call this function directly.

    \sa QProtobufBaseSerializer::serializeEnum()
*/

/*!
    \fn void QProtobufBaseSerializer::serializeEnumList(const QList<QtProtobuf::int64> &value,
        const QMetaEnum &metaEnum,
        const QtProtobufPrivate::QProtobufPropertyOrderingInfo &fieldInfo) const

    This function serializes a list, \a value, for enum list associated with property \a fieldInfo.
    \a metaEnum helps to encode the enum value.

    You should not call this function directly.

    \sa QProtobufBaseSerializer::deserializeEnumList()
*/

/*!
    \fn bool QProtobufBaseSerializer::deserializeEnumList(QList<QtProtobuf::int64> &value,
        const QMetaEnum &metaEnum) const

    This function deserializes a list of enum \a value from a wire. \a metaEnum helps to decode
    the enum value.
    Returns \c true if deserialization was successful, otherwise \c false.

    You should not call this function directly.

   \sa QProtobufBaseSerializer::serializeEnumList()
*/

/*!
    \relates QProtobufBaseSerializer
    \fn template<typename T> inline void qRegisterProtobufType()

    Registers a Protobuf type \e T.
    This function is normally called by generated code.
*/

/*!
    \relates QProtobufBaseSerializer
    \fn template<typename K, typename V> inline void qRegisterProtobufMapType();

    Registers a Protobuf map type \c K and \c V.
    \c V must be a QProtobufMessage.
    This function is normally called by generated code.
*/

/*!
    \relates QProtobufBaseSerializer
    \fn template<typename T> inline void qRegisterProtobufEnumType();

    Registers serializers for enumeration type \c T in QtProtobuf global
    serializers registry.

    This function is normally called by generated code.
*/

QT_END_NAMESPACE
