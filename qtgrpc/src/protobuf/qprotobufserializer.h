// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2019 Alexey Edelev <semlanik@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPROTOBUFSERIALIZER_H
#define QPROTOBUFSERIALIZER_H

#include <QtProtobuf/qtprotobufglobal.h>

#include <QtProtobuf/qabstractprotobufserializer.h>

#include <QtProtobuf/qtprotobuftypes.h>

#include <QtProtobuf/QProtobufMessage>
#include <QtCore/QVariant>
#include <QtCore/QMetaObject>

#include <functional>
#include <memory>

QT_BEGIN_NAMESPACE

namespace QtProtobufPrivate {
class QProtobufSelfcheckIterator;
} // namespace QtProtobufPrivate

class QProtobufSerializerPrivate;
class Q_PROTOBUF_EXPORT QProtobufSerializer : public QAbstractProtobufSerializer
{
    Q_DISABLE_COPY_MOVE(QProtobufSerializer)
public:
    QProtobufSerializer();
    ~QProtobufSerializer() override;

    QProtobufSerializer::DeserializationError deserializationError() const override;
    QString deserializationErrorString() const override;

    QByteArray
    serializeMessage(const QProtobufMessage *message,
                     const QtProtobufPrivate::QProtobufPropertyOrdering &ordering) const override;
    bool deserializeMessage(QProtobufMessage *message,
                            const QtProtobufPrivate::QProtobufPropertyOrdering &ordering,
                            QByteArrayView data) const override;

    QByteArray
    serializeObject(const QProtobufMessage *message,
                    const QtProtobufPrivate::QProtobufPropertyOrdering &ordering,
                    const QtProtobufPrivate::QProtobufPropertyOrderingInfo &fieldInfo) const;
    bool deserializeObject(QProtobufMessage *message,
                           const QtProtobufPrivate::QProtobufPropertyOrdering &ordering,
                           QtProtobufPrivate::QProtobufSelfcheckIterator &it) const;

    QByteArray
    serializeListObject(const QProtobufMessage *message,
                        const QtProtobufPrivate::QProtobufPropertyOrdering &ordering,
                        const QtProtobufPrivate::QProtobufPropertyOrderingInfo &fieldInfo) const;
    bool deserializeListObject(QProtobufMessage *message,
                               const QtProtobufPrivate::QProtobufPropertyOrdering &ordering,
                               QtProtobufPrivate::QProtobufSelfcheckIterator &it) const;

    QByteArray
    serializeMapPair(const QVariant &key, const QVariant &value,
                     const QtProtobufPrivate::QProtobufPropertyOrderingInfo &fieldInfo) const;
    bool deserializeMapPair(QVariant &key, QVariant &value,
                            QtProtobufPrivate::QProtobufSelfcheckIterator &it) const;

    QByteArray
    serializeEnum(QtProtobuf::int64 value,
                  const QtProtobufPrivate::QProtobufPropertyOrderingInfo &fieldInfo) const;
    QByteArray
    serializeEnumList(const QList<QtProtobuf::int64> &value,
                      const QtProtobufPrivate::QProtobufPropertyOrderingInfo &fieldInfo) const;

    Q_REQUIRED_RESULT
    bool deserializeEnum(QtProtobuf::int64 &value,
                         QtProtobufPrivate::QProtobufSelfcheckIterator &it) const;
    Q_REQUIRED_RESULT
    bool deserializeEnumList(QList<QtProtobuf::int64> &value,
                             QtProtobufPrivate::QProtobufSelfcheckIterator &it) const;

private:
    std::unique_ptr<QProtobufSerializerPrivate> d_ptr;
};

namespace QtProtobufPrivate {

using Serializer = void (*)(const QProtobufSerializer *, const QVariant &,
                            const QProtobufPropertyOrderingInfo &, QByteArray &);
using Deserializer = void (*)(const QProtobufSerializer *, QProtobufSelfcheckIterator &,
                              QVariant &);

/*!
 \private
 \brief SerializationHandlers contains set of objects that required for class
 serialization/deserialization
 */
struct SerializationHandler
{
    Serializer serializer = nullptr; /*!< serializer assigned to class */
    Deserializer deserializer = nullptr; /*!< deserializer assigned to class */
};

extern Q_PROTOBUF_EXPORT SerializationHandler findHandler(QMetaType type);
extern Q_PROTOBUF_EXPORT void registerHandler(QMetaType type, const SerializationHandler &handlers);

/*!
 \private
 \brief default serializer template for type T that inherits from QProtobufMessage
 */
template<typename T,
         typename std::enable_if_t<std::is_base_of<QProtobufMessage, T>::value, int> = 0>
void serializeObject(const QProtobufSerializer *serializer, const QVariant &value,
                     const QProtobufPropertyOrderingInfo &fieldInfo, QByteArray &buffer)
{
    Q_ASSERT_X(serializer != nullptr, "QProtobufSerializer", "Serializer is null");
    buffer.append(serializer->serializeObject(value.value<T *>(), T::propertyOrdering, fieldInfo));
}

/*!
 \private
 \brief default serializer template for list of type T objects that inherits from QProtobufMessage
 */
template<typename V,
         typename std::enable_if_t<std::is_base_of<QProtobufMessage, V>::value, int> = 0>
void serializeList(const QProtobufSerializer *serializer, const QVariant &listValue,
                   const QProtobufPropertyOrderingInfo &fieldInfo, QByteArray &buffer)
{
    Q_ASSERT_X(serializer != nullptr, "QProtobufSerializer", "Serializer is null");
    for (const auto &value : listValue.value<QList<V>>()) {
        buffer.append(serializer->serializeListObject(&value, V::propertyOrdering, fieldInfo));
    }
}

/*!
 \private
 \brief default serializer template for map of key K, value V
 */
template<typename K, typename V,
         typename std::enable_if_t<!std::is_base_of<QProtobufMessage, V>::value, int> = 0>
void serializeMap(const QProtobufSerializer *serializer, const QVariant &value,
                  const QProtobufPropertyOrderingInfo &fieldInfo, QByteArray &buffer)
{
    Q_ASSERT_X(serializer != nullptr, "QProtobufSerializer", "Serializer is null");
    for (const auto &[k, v] : value.value<QHash<K, V>>().asKeyValueRange()) {
        buffer.append(serializer->serializeMapPair(QVariant::fromValue<K>(k),
                                                   QVariant::fromValue<V>(v), fieldInfo));
    }
}

/*!
 \private
 \brief default serializer template for map of type key K, value V. Specialization for V that
 inherits from QProtobufMessage
 */
template<typename K, typename V,
         typename std::enable_if_t<std::is_base_of<QProtobufMessage, V>::value, int> = 0>
void serializeMap(const QProtobufSerializer *serializer, const QVariant &value,
                  const QProtobufPropertyOrderingInfo &fieldInfo, QByteArray &buffer)
{
    Q_ASSERT_X(serializer != nullptr, "QProtobufSerializer", "Serializer is null");
    for (const auto &[k, v] : value.value<QHash<K, V>>().asKeyValueRange()) {
        buffer.append(serializer->serializeMapPair(QVariant::fromValue<K>(k),
                                                   QVariant::fromValue<V *>(&v), fieldInfo));
    }
}

/*!
 \private
 \brief default serializer template for enum types
 */
template<typename T, typename std::enable_if_t<std::is_enum<T>::value, int> = 0>
void serializeEnum(const QProtobufSerializer *serializer, const QVariant &value,
                   const QProtobufPropertyOrderingInfo &fieldInfo, QByteArray &buffer)
{
    Q_ASSERT_X(serializer != nullptr, "QProtobufSerializer", "Serializer is null");
    buffer.append(serializer->serializeEnum(QtProtobuf::int64(value.value<T>()), fieldInfo));
}

/*!
 \private
 \brief default serializer template for enum list types
 */
template<typename T, typename std::enable_if_t<std::is_enum<T>::value, int> = 0>
void serializeEnumList(const QProtobufSerializer *serializer, const QVariant &value,
                       const QProtobufPropertyOrderingInfo &fieldInfo, QByteArray &buffer)
{
    Q_ASSERT_X(serializer != nullptr, "QProtobufSerializer", "Serializer is null");
    QList<QtProtobuf::int64> intList;
    for (auto enumValue : value.value<QList<T>>()) {
        intList.append(QtProtobuf::int64(enumValue));
    }
    buffer.append(serializer->serializeEnumList(intList, fieldInfo));
}

/*!
 \private
 \brief default deserializer template for type T that inherits from QProtobufMessage
 */
template<typename T,
         typename std::enable_if_t<std::is_base_of<QProtobufMessage, T>::value, int> = 0>
void deserializeObject(const QProtobufSerializer *serializer, QProtobufSelfcheckIterator &it,
                       QVariant &to)
{
    Q_ASSERT_X(serializer != nullptr, "QProtobufSerializer", "Serializer is null");
    Q_ASSERT_X(to.isNull() || to.metaType() == QMetaType::fromType<T *>(), "QProtobufSerializer",
               "Property should be either uninitialized or contain a valid pointer");

    T *value = to.value<T *>();
    if (value == nullptr) {
        value = new T;
        to = QVariant::fromValue<T *>(value);
    }
    serializer->deserializeObject(value, T::propertyOrdering, it);
}

/*!
 \private
 \brief default deserializer template for list of type T objects that inherits from QProtobufMessage
 */
template<typename V,
         typename std::enable_if_t<std::is_base_of<QProtobufMessage, V>::value, int> = 0>
void deserializeList(const QProtobufSerializer *serializer, QProtobufSelfcheckIterator &it,
                     QVariant &previous)
{
    Q_ASSERT_X(serializer != nullptr, "QProtobufSerializer", "Serializer is null");

    V newValue;
    if (serializer->deserializeListObject(&newValue, V::propertyOrdering, it)) {
        QList<V> list = previous.value<QList<V>>();
        list.append(newValue);
        previous.setValue(list);
    }
}

/*!
 \private
 *
 \brief default deserializer template for map of key K, value V
 */
template<typename K, typename V,
         typename std::enable_if_t<!std::is_base_of<QProtobufMessage, V>::value, int> = 0>
void deserializeMap(const QProtobufSerializer *serializer, QProtobufSelfcheckIterator &it,
                    QVariant &previous)
{
    Q_ASSERT_X(serializer != nullptr, "QProtobufSerializer", "Serializer is null");

    QHash<K, V> out = previous.value<QHash<K, V>>();
    QVariant key = QVariant::fromValue<K>(K());
    QVariant value = QVariant::fromValue<V>(V());

    if (serializer->deserializeMapPair(key, value, it)) {
        out[key.value<K>()] = value.value<V>();
        previous = QVariant::fromValue<QHash<K, V>>(out);
    }
}

/*!
 \private
 *
 \brief default deserializer template for map of type key K, value V. Specialization for V
        that inherits from QProtobufMessage
 */
template<typename K, typename V,
         typename std::enable_if_t<std::is_base_of<QProtobufMessage, V>::value, int> = 0>
void deserializeMap(const QProtobufSerializer *serializer, QProtobufSelfcheckIterator &it,
                    QVariant &previous)
{
    Q_ASSERT_X(serializer != nullptr, "QProtobufSerializer", "Serializer is null");

    auto out = previous.value<QHash<K, V>>();
    QVariant key = QVariant::fromValue<K>(K());
    QVariant value = QVariant::fromValue<V *>(nullptr);
    bool ok = serializer->deserializeMapPair(key, value, it);
    V *valuePtr = value.value<V *>();
    if (ok) {
        out[key.value<K>()] = valuePtr ? *valuePtr : V();
        previous = QVariant::fromValue<QHash<K, V>>(out);
    }
    delete valuePtr;
}

/*!
 \private
 *
 \brief default deserializer template for enum type T
 */
template<typename T, typename std::enable_if_t<std::is_enum<T>::value, int> = 0>
void deserializeEnum(const QProtobufSerializer *serializer, QProtobufSelfcheckIterator &it,
                     QVariant &to)
{
    Q_ASSERT_X(serializer != nullptr, "QProtobufSerializer", "Serializer is null");
    QtProtobuf::int64 intValue;
    if (serializer->deserializeEnum(intValue, it))
        to = QVariant::fromValue<T>(static_cast<T>(intValue._t));
}

/*!
 \private
 *
 \brief default deserializer template for enumList type T
 */
template<typename T, typename std::enable_if_t<std::is_enum<T>::value, int> = 0>
void deserializeEnumList(const QProtobufSerializer *serializer, QProtobufSelfcheckIterator &it,
                         QVariant &previous)
{
    Q_ASSERT_X(serializer != nullptr, "QProtobufSerializer", "Serializer is null");
    QList<QtProtobuf::int64> intList;
    if (!serializer->deserializeEnumList(intList, it))
        return;
    QList<T> enumList = previous.value<QList<T>>();
    for (auto intValue : intList)
        enumList.append(static_cast<T>(intValue._t));
    previous = QVariant::fromValue<QList<T>>(enumList);
}
} // namespace QtProtobufPrivate

template<typename T>
inline void qRegisterProtobufType()
{
    T::registerTypes();
    QtProtobufPrivate::registerOrdering(QMetaType::fromType<T>(), T::propertyOrdering);
    QtProtobufPrivate::registerHandler(
            QMetaType::fromType<T *>(),
            { QtProtobufPrivate::serializeObject<T>, QtProtobufPrivate::deserializeObject<T> });
    QtProtobufPrivate::registerHandler(
            QMetaType::fromType<QList<T>>(),
            { QtProtobufPrivate::serializeList<T>, QtProtobufPrivate::deserializeList<T> });
}

template<typename K, typename V>
inline void qRegisterProtobufMapType()
{
    QtProtobufPrivate::registerHandler(
            QMetaType::fromType<QHash<K, V>>(),
            { QtProtobufPrivate::serializeMap<K, V>, QtProtobufPrivate::deserializeMap<K, V> });
}

#ifdef Q_QDOC
template<typename T>
inline void qRegisterProtobufEnumType();
#else // !Q_QDOC
template<typename T, typename std::enable_if_t<std::is_enum<T>::value, int> = 0>
inline void qRegisterProtobufEnumType()
{
    QtProtobufPrivate::registerHandler(
            QMetaType::fromType<T>(),
            { QtProtobufPrivate::serializeEnum<T>, QtProtobufPrivate::deserializeEnum<T> });
    QtProtobufPrivate::registerHandler(
            QMetaType::fromType<QList<T>>(),
            { QtProtobufPrivate::serializeEnumList<T>, QtProtobufPrivate::deserializeEnumList<T> });
}
#endif // Q_QDOC

QT_END_NAMESPACE
#endif // QPROTOBUFSERIALIZER_H
