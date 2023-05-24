// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPROTOBUFONEOF_H
#define QPROTOBUFONEOF_H

#if 0
#  pragma qt_class(QProtobufOneof)
#endif

#include <QtCore/QExplicitlySharedDataPointer>
#include <QtCore/qmetaobject.h>
#include <QtCore/qvariant.h>
#include <QtProtobuf/qtprotobuftypes.h>
#include <QtProtobuf/qprotobufmessage.h>

#include <type_traits>

QT_BEGIN_NAMESPACE

class QAbstractProtobufSerializer;

namespace QtProtobufPrivate {

class QProtobufOneofPrivate;
class Q_PROTOBUF_EXPORT QProtobufOneof final
{
    template<typename T>
    using IsProtobufMessageType =
            typename std::enable_if_t<!std::is_pointer_v<T>
                                              && std::is_base_of_v<QProtobufMessage, T>
                                              && HasProtobufPropertyOrdering<T>,
                                      int>;

    template<typename T>
    using IsNonMessageProtobufType = typename std::enable_if_t<
            std::disjunction_v<
                    std::is_same<T, QtProtobuf::int32>, std::is_same<T, QtProtobuf::int64>,
                    std::is_same<T, QtProtobuf::sint32>, std::is_same<T, QtProtobuf::sint64>,
                    std::is_same<T, QtProtobuf::uint32>, std::is_same<T, QtProtobuf::uint64>,
                    std::is_same<T, QtProtobuf::fixed32>, std::is_same<T, QtProtobuf::fixed64>,
                    std::is_same<T, QtProtobuf::sfixed32>, std::is_same<T, QtProtobuf::sfixed64>,
                    std::is_same<T, float>, std::is_same<T, double>,
                    std::is_same<T, QtProtobuf::boolean>, std::is_enum<T>,
                    std::is_same<T, QString>, std::is_same<T, QByteArray>>,
            int>;

public:
    QProtobufOneof();
    ~QProtobufOneof();
    QProtobufOneof(const QProtobufOneof &other);
    QProtobufOneof &operator=(const QProtobufOneof &other);
    QProtobufOneof(QProtobufOneof &&other) noexcept : d_ptr(std::exchange(other.d_ptr, {})) { }
    QProtobufOneof &operator=(QProtobufOneof &&other) noexcept
    {
        qt_ptr_swap(d_ptr, other.d_ptr);
        return *this;
    }

    template<typename T>
    void setValue(const T &value, int fieldNumber)
    {
        setValue(QVariant::fromValue<T>(value), fieldNumber);
    }

    template<typename T, IsNonMessageProtobufType<T> = 0>
    T value() const
    {
        Q_ASSERT(QMetaType::fromType<T>() == rawValue().metaType());
        return rawValue().value<T>();
    }

    template<typename T, IsProtobufMessageType<T> = 0>
    T *value() const
    {
        Q_ASSERT(QMetaType::fromType<T>() == rawValue().metaType());
        return reinterpret_cast<T *>(rawValue().data());
    }

    template<typename T, IsNonMessageProtobufType<T> = 0>
    bool isEqual(const T &otherValue, int fieldNumber) const
    {
        return this->fieldNumber() == fieldNumber
                && QMetaType::fromType<T>() == rawValue().metaType() && value<T>() == otherValue;
    }

    template<typename T, IsProtobufMessageType<T> = 0>
    bool isEqual(const T &otherValue, int fieldNumber) const
    {
        return this->fieldNumber() == fieldNumber
                && QMetaType::fromType<T>() == rawValue().metaType() && value<T>()
                && *(value<T>()) == otherValue;
    }

    int fieldNumber() const;
    bool holdsField(int fieldNumber) const;

private:
    friend bool operator==(const QProtobufOneof &lhs, const QProtobufOneof &rhs)
    {
        return lhs.isEqual(rhs);
    }
    friend bool operator!=(const QProtobufOneof &lhs, const QProtobufOneof &rhs)
    {
        return !lhs.isEqual(rhs);
    }
    bool isEqual(const QProtobufOneof &other) const;

    friend class QT_PREPEND_NAMESPACE(QProtobufMessage);

    void setValue(const QVariant &value, int fieldNumber);
    QVariant &rawValue() const;

    QProtobufOneofPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QProtobufOneof)
};
} // namespace QtProtobufPrivate

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QtProtobufPrivate::QProtobufOneof)

#endif // QPROTOBUFONEOF_H
