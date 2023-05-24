// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2019 Alexey Edelev <semlanik@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QABSTRACTPROTOBUFSERIALIZER_H
#define QABSTRACTPROTOBUFSERIALIZER_H

#include <QtProtobuf/qtprotobufglobal.h>
#include <QtProtobuf/qtprotobuftypes.h>
#include <QtProtobuf/qprotobufmessage.h>

#include <type_traits>

QT_BEGIN_NAMESPACE

class Q_PROTOBUF_EXPORT QAbstractProtobufSerializer
{

public:
    enum DeserializationError {
        NoError,
        InvalidHeaderError,
        NoDeserializerError,
        UnexpectedEndOfStreamError,
    };

    template<typename T>
    QByteArray serialize(const QProtobufMessage *message) const
    {
        static_assert(QtProtobufPrivate::HasProtobufPropertyOrdering<T>,
                      "T must have the Q_PROTOBUF_OBJECT macro");
        return doSerialize(message, T::propertyOrdering);
    }

    template<typename T>
    bool deserialize(T *object, QByteArrayView data) const
    {
        static_assert(QtProtobufPrivate::HasProtobufPropertyOrdering<T>,
                      "T must have the Q_PROTOBUF_OBJECT macro");
        static_assert(std::is_base_of_v<QProtobufMessage, T>,
                      "T must be derived from QProtobufMessage");
        // Initialize default object first and make copy afterwards, it's necessary to set default
        // values of properties that was not stored in data.
        T newValue;
        bool success = doDeserialize(&newValue, T::propertyOrdering, data);
        *object = newValue;
        return success;
    }

    virtual ~QAbstractProtobufSerializer();

    virtual QAbstractProtobufSerializer::DeserializationError deserializationError() const = 0;
    virtual QString deserializationErrorString() const = 0;

    QByteArray serializeRawMessage(const QProtobufMessage *message) const;
    bool deserializeRawMessage(QProtobufMessage *message, QByteArrayView data) const;

protected:
    virtual QByteArray
    serializeMessage(const QProtobufMessage *message,
                     const QtProtobufPrivate::QProtobufPropertyOrdering &ordering) const = 0;
    virtual bool deserializeMessage(QProtobufMessage *message,
                                    const QtProtobufPrivate::QProtobufPropertyOrdering &ordering,
                                    QByteArrayView data) const = 0;

private:
    QByteArray doSerialize(const QProtobufMessage *message,
                           const QtProtobufPrivate::QProtobufPropertyOrdering &ordering) const;
    bool doDeserialize(QProtobufMessage *message,
                       const QtProtobufPrivate::QProtobufPropertyOrdering &ordering,
                       QByteArrayView data) const;
};

QT_END_NAMESPACE
#endif // QABSTRACTPROTOBUFSERIALIZER_H
