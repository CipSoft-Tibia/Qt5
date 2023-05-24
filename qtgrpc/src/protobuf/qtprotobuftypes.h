// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2019 Alexey Edelev <semlanik@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTPROTOBUFTYPES_H
#define QTPROTOBUFTYPES_H

#include <QtProtobuf/qtprotobufglobal.h>

#include <QtCore/QList>
#include <QtCore/QHash>
#include <QtCore/QMetaType>
#include <QtCore/QtEndian>
#include <QtProtobuf/QProtobufMessage>

#include <memory>
#include <functional>
#include <list>
#include <type_traits>
#include <utility>

QT_BEGIN_NAMESPACE

namespace QtProtobufPrivate {

enum FieldFlag : uint { NoFlags = 0x0, NonPacked = 0x1, Oneof = 0x02 };

struct QProtobufPropertyOrdering
{
    const struct Data
    {
        uint version;
        uint numFields;
        uint fieldNumberOffset;
        uint propertyIndexOffset;
        uint flagsOffset;
        uint fullPackageNameSize;
    } *data;

    Q_PROTOBUF_EXPORT QUtf8StringView getMessageFullName() const;
    Q_PROTOBUF_EXPORT QUtf8StringView getJsonName(int index) const;
    Q_PROTOBUF_EXPORT int getFieldNumber(int index) const;
    Q_PROTOBUF_EXPORT int getPropertyIndex(int index) const;
    Q_PROTOBUF_EXPORT uint getFieldFlags(int index) const;
    Q_PROTOBUF_EXPORT int indexOfFieldNumber(int fieldNumber) const;
    int fieldCount() const { return int(data->numFields); }

private:
    const uint *uint_data() const;
    const char *char_data() const;
    const uint &uint_dataForIndex(int index, uint offset) const;
};
static_assert(std::is_trivially_destructible_v<QProtobufPropertyOrdering>);

extern Q_PROTOBUF_EXPORT void registerOrdering(QMetaType type, QProtobufPropertyOrdering ordering);

// Convenience structure to hold a reference to a single entry
struct QProtobufPropertyOrderingInfo
{
    QProtobufPropertyOrderingInfo(QProtobufPropertyOrdering ord, int ind)
        : ordering(ord), index(ind)
    {
        Q_ASSERT(index >= 0);
    }

    QUtf8StringView getJsonName() const { return ordering.getJsonName(index); }
    int getFieldNumber() const
    {
        return (overrideFieldNumber >= 0) ? overrideFieldNumber : ordering.getFieldNumber(index);
    }
    int getPropertyIndex() const { return ordering.getPropertyIndex(index); }
    uint getFieldFlags() const { return ordering.getFieldFlags(index); }

    // Needed for maps, which uses field number 1 and 2 for key and value respectively
    QProtobufPropertyOrderingInfo infoForMapKey() const { return { ordering, index, 1 }; }
    QProtobufPropertyOrderingInfo infoForMapValue() const { return { ordering, index, 2 }; }

private:
    QProtobufPropertyOrderingInfo(QProtobufPropertyOrdering ord, int ind, int fieldNumber)
        : ordering(ord), index(ind), overrideFieldNumber(fieldNumber)
    {
    }
    const QProtobufPropertyOrdering ordering;
    const int index;
    const int overrideFieldNumber = -1; // special case for maps
};

template<typename>
using sfinae = void;
template<typename T, typename = void>
[[maybe_unused]] static constexpr bool HasProtobufPropertyOrdering = false;
template<typename T>
[[maybe_unused]] static constexpr bool
        HasProtobufPropertyOrdering<T, sfinae<decltype(T::propertyOrdering)>> = true;
} // namespace QtProtobufPrivate

namespace QtProtobuf {
Q_NAMESPACE_EXPORT(Q_PROTOBUF_EXPORT)

[[maybe_unused]] constexpr int InvalidFieldNumber = 0;

enum class WireTypes
{
    Unknown = -1,
    Varint = 0,
    Fixed64 = 1,
    LengthDelimited = 2,
    StartGroup = 3,
    EndGroup = 4,
    Fixed32 = 5
};
Q_ENUM_NS(WireTypes)

// The 'tag' template param exists only create a unique type
template<typename T, typename tag>
struct TransparentWrapper
{
    TransparentWrapper(T t = T()) : _t(t) { }
    T _t;
    operator T &() { return _t; }
    operator T() const { return _t; }
    TransparentWrapper &operator=(const T &t)
    {
        _t = t;
        return *this;
    }

    static T toType(TransparentWrapper t) { return t._t; }
    static TransparentWrapper fromType(T _t) { return TransparentWrapper(_t); }

    static QString toString(TransparentWrapper t) { return QString::number(t._t); }
};

template<typename T, typename tag>
constexpr TransparentWrapper<T, tag> qbswap(TransparentWrapper<T, tag> source)
{
    return { QT_PREPEND_NAMESPACE(qbswap)(source._t) };
}

using int32 = TransparentWrapper<int32_t, struct int_tag>;
using int64 = TransparentWrapper<int64_t, struct int_tag>;
using uint32 = uint32_t;
using uint64 = uint64_t;
using sint32 = int32_t;
using sint64 = int64_t;
using fixed32 = TransparentWrapper<uint32_t, struct fixed_tag>;
using fixed64 = TransparentWrapper<uint64_t, struct fixed_tag>;
using sfixed32 = TransparentWrapper<int32_t, struct fixed_tag>;
using sfixed64 = TransparentWrapper<int64_t, struct fixed_tag>;
using boolean = bool;
using int32List = QList<int32>;
using int64List = QList<int64>;
using uint32List = QList<uint32>;
using uint64List = QList<uint64>;
using sint32List = QList<sint32>;
using sint64List = QList<sint64>;
using fixed32List = QList<fixed32>;
using fixed64List = QList<fixed64>;
using sfixed32List = QList<sfixed32>;
using sfixed64List = QList<sfixed64>;
using floatList = QList<float>;
using doubleList = QList<double>;
using boolList = QList<bool>;

using RegisterFunction = void (*)();
// This struct is used for type registrations in generated code
struct ProtoTypeRegistrar
{
    Q_PROTOBUF_EXPORT explicit ProtoTypeRegistrar(QtProtobuf::RegisterFunction initializer);
};

template<typename T>
bool repeatedValueCompare(const QList<T> &a, const QList<T> &b)
{
    return std::equal(a.begin(), a.end(), b.begin(), b.end());
}

template<typename K, typename V>
bool repeatedValueCompare(const QHash<K, V> &a, const QHash<K, V> &b)
{
    return a == b;
}

template<typename T>
struct qMakeUnsignedImpl
{
    using type = std::make_unsigned_t<T>;
};
template<typename T>
struct qMakeUnsignedImpl<TransparentWrapper<T, struct fixed_tag>>
{
    using type = TransparentWrapper<std::make_unsigned_t<T>, fixed_tag>;
};
template<>
struct qMakeUnsignedImpl<int32>
{
    using type = uint32_t;
};
template<>
struct qMakeUnsignedImpl<int64>
{
    using type = uint64_t;
};
template<typename T>
using qMakeUnsigned = typename qMakeUnsignedImpl<T>::type;

} // namespace QtProtobuf

Q_PROTOBUF_EXPORT void qRegisterProtobufTypes();

QT_END_NAMESPACE

QT_DECL_METATYPE_EXTERN_TAGGED(QtProtobuf::int32, QtProtobuf_int32, Q_PROTOBUF_EXPORT)
QT_DECL_METATYPE_EXTERN_TAGGED(QtProtobuf::int64, QtProtobuf_int64, Q_PROTOBUF_EXPORT)
QT_DECL_METATYPE_EXTERN_TAGGED(QtProtobuf::fixed32, QtProtobuf_fixed32, Q_PROTOBUF_EXPORT)
QT_DECL_METATYPE_EXTERN_TAGGED(QtProtobuf::fixed64, QtProtobuf_fixed64, Q_PROTOBUF_EXPORT)
QT_DECL_METATYPE_EXTERN_TAGGED(QtProtobuf::sfixed32, QtProtobuf_sfixed32, Q_PROTOBUF_EXPORT)
QT_DECL_METATYPE_EXTERN_TAGGED(QtProtobuf::sfixed64, QtProtobuf_sfixed64, Q_PROTOBUF_EXPORT)

#endif // QTPROTOBUFTYPES_H
