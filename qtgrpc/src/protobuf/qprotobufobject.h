// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2019 Alexey Edelev <semlanik@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPROTOBUFOBJECT_H
#define QPROTOBUFOBJECT_H

#if 0
#  pragma qt_sync_skip_header_check
#  pragma qt_sync_stop_processing
#endif

#include <QtProtobuf/qabstractprotobufserializer.h>

#define Q_DECLARE_PROTOBUF_SERIALIZERS(Type)\
    public:\
        QByteArray serialize(QAbstractProtobufSerializer *serializer) const {\
            qRegisterProtobufTypes();\
            Q_ASSERT_X(serializer != nullptr, "QProtobufObject", "Serializer is null");\
            return serializer->serialize<Type>(this);\
        }\
        bool deserialize(QAbstractProtobufSerializer *serializer, QByteArrayView array) {\
            qRegisterProtobufTypes();\
            Q_ASSERT_X(serializer != nullptr, "QProtobufObject", "Serializer is null");\
            return serializer->deserialize<Type>(this, array);\
        }\
    private:

#define Q_PROTOBUF_OBJECT\
    public:\
        static const QtProtobufPrivate::QProtobufPropertyOrdering propertyOrdering;\
    private:

#endif // QPROTOBUFOBJECT_H
