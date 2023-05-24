// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef Q_PROTOBUF_MESSAGE_H
#define Q_PROTOBUF_MESSAGE_H

#include <QtProtobuf/qtprotobufglobal.h>
#include <QtCore/qtconfigmacros.h>
#include <QtCore/qtmetamacros.h>
#include <QtCore/qmetatype.h>

QT_BEGIN_NAMESPACE

class QProtobufMessage;
struct QProtobufMessageDeleter {
    Q_PROTOBUF_EXPORT void operator()(QProtobufMessage *ptr) noexcept;
};
using QProtobufMessagePointer = std::unique_ptr<QProtobufMessage, QProtobufMessageDeleter>;

namespace QtProtobufPrivate {
struct QProtobufPropertyOrderingInfo;
}

class QProtobufMessagePrivate;
class QProtobufMessage
{
    Q_GADGET_EXPORT(Q_PROTOBUF_EXPORT)
public:
    Q_PROTOBUF_EXPORT QVariant property(QAnyStringView propertyName) const;
    Q_PROTOBUF_EXPORT bool setProperty(QAnyStringView propertyName, const QVariant &value);
    Q_PROTOBUF_EXPORT bool setProperty(QAnyStringView propertyName, QVariant &&value);

    Q_REQUIRED_RESULT
    Q_PROTOBUF_EXPORT static QProtobufMessagePointer constructByName(const QString &messageType);

protected:
    Q_PROTOBUF_EXPORT explicit QProtobufMessage(const QMetaObject *metaObject);
    Q_PROTOBUF_EXPORT ~QProtobufMessage();
    Q_PROTOBUF_EXPORT QProtobufMessage(const QProtobufMessage &other);
    Q_PROTOBUF_EXPORT QProtobufMessage &operator=(const QProtobufMessage &other);
    QProtobufMessage(QProtobufMessage &&other) noexcept : d_ptr(std::exchange(other.d_ptr, {})) { }
    QProtobufMessage &operator=(QProtobufMessage &&other) noexcept
    {
        qt_ptr_swap(d_ptr, other.d_ptr);
        return *this;
    }

    Q_PROTOBUF_EXPORT
    static bool isEqual(const QProtobufMessage &lhs, const QProtobufMessage &rhs) noexcept;

    QVariant property(const QtProtobufPrivate::QProtobufPropertyOrderingInfo &fieldInfo) const;
    bool setProperty(const QtProtobufPrivate::QProtobufPropertyOrderingInfo &fieldInfo,
                     const QVariant &value);
    bool setProperty(const QtProtobufPrivate::QProtobufPropertyOrderingInfo &fieldInfo,
                     QVariant &&value);

private:
    const QMetaObject *metaObject() const;

    friend class QProtobufSerializer;
    friend class QAbstractProtobufSerializer;
    friend class QProtobufSerializerPrivate;
    friend class QAbstractProtobufSerializer;
    friend struct QProtobufMessageDeleter;

    QProtobufMessagePrivate *d_ptr;
    Q_DECLARE_PRIVATE(QProtobufMessage)
};

QT_END_NAMESPACE

#endif // Q_PROTOBUF_MESSAGE_H
