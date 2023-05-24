// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2019 Alexey Edelev <semlanik@gmail.com>, Viktor Kopp <vifactor@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qprotobufserializer.h"
#include "qprotobufserializer_p.h"

#include "qtprotobuftypes.h"

#include <QtCore/qmetatype.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qvariant.h>
#include <QtCore/qreadwritelock.h>

#include <QtProtobuf/private/qprotobufserializer_p.h>
#include <QtProtobuf/private/qprotobufmessage_p.h>


QT_BEGIN_NAMESPACE

namespace {

/*
    \internal
    \brief The HandlersRegistry is a container to store mapping between metatype
    identifier and serialization handlers.
*/
struct HandlersRegistry
{
    void registerHandler(QMetaType type, const QtProtobufPrivate::SerializationHandler &handlers)
    {
        QWriteLocker locker(&m_lock);
        m_registry[type] = handlers;
    }

    QtProtobufPrivate::SerializationHandler findHandler(QMetaType type)
    {
        QtProtobufPrivate::SerializationHandler handler;
        QReadLocker locker(&m_lock);
        auto it = m_registry.constFind(type);
        if (it != m_registry.constEnd())
            handler = it.value();
        return handler;
    }

private:
    QReadWriteLock m_lock;
    QHash<QMetaType, QtProtobufPrivate::SerializationHandler> m_registry;
};
Q_GLOBAL_STATIC(HandlersRegistry, handlersRegistry)

inline bool isOneofField(const QtProtobufPrivate::QProtobufPropertyOrderingInfo &fieldInfo)
{
    // TODO: Add the check for the optional flag once the functionality is
    // implemented.
    return fieldInfo.getFieldFlags() & QtProtobufPrivate::Oneof;
}

} // namespace

void QtProtobufPrivate::registerHandler(QMetaType type,
                                        const QtProtobufPrivate::SerializationHandler &handlers)
{
    handlersRegistry->registerHandler(type, handlers);
}

QtProtobufPrivate::SerializationHandler QtProtobufPrivate::findHandler(QMetaType type)
{
    if (!handlersRegistry.exists())
        return {};
    return handlersRegistry->findHandler(type);
}

/*!
    \class QProtobufSerializer
    \inmodule QtProtobuf
    \since 6.5
    \brief The QProtobufSerializer class is interface that represents
           basic functions for serialization/deserialization.

    The QProtobufSerializer class registers serializers/deserializers for
    classes implementing a protobuf message, inheriting \l QProtobufMessage. These
    classes are generated automatically, based on a \c{.proto} file, using the CMake
    function \l qt_add_protobuf or by running
    \l {The qtprotobufgen Tool} {qtprotobufgen} directly.
*/

/*!
    \fn QProtobufSerializer::DeserializationError QProtobufSerializer::deserializationError() const

    Returns the last deserialization error.
*/

/*!
    \fn QString QProtobufSerializer::deserializationErrorString() const

    Returns a human-readable string describing the last deserialization error.
    If there was no error, an empty string is returned.
*/

/*!
    \relates QProtobufSerializer
    \fn template<typename T> inline void qRegisterProtobufType()

    Registers a Protobuf type \e T.
    This function is normally called by generated code.
*/

/*!
    \relates QProtobufSerializer
    \fn template<typename K, typename V> inline void qRegisterProtobufMapType();

    Registers a Protobuf map type \c K and \c V.
    \c V must be a QProtobufMessage.
    This function is normally called by generated code.
*/

/*!
    \relates QProtobufSerializer
    \fn template<typename T> inline void qRegisterProtobufEnumType();

    Registers serializers for enumeration type \c T in QtProtobuf global
    serializers registry.

    This function is normally called by generated code.
*/

using namespace Qt::StringLiterals;
using namespace QtProtobufPrivate;

template<std::size_t N>
using SerializerRegistryType =
        std::array<QProtobufSerializerPrivate::ProtobufSerializationHandler, N>;

namespace {
#define QT_CONSTRUCT_PROTOBUF_SERIALIZATION_HANDLER(Type, WireType)          \
  {                                                                          \
    QMetaType::fromType<Type>(),                                             \
            QProtobufSerializerPrivate::serializeWrapper<                    \
                    Type, QProtobufSerializerPrivate::serializeBasic<Type>>, \
            QProtobufSerializerPrivate::deserializeBasic<Type>,              \
            QProtobufSerializerPrivate::isPresent<Type>, WireType        \
  }
#define QT_CONSTRUCT_PROTOBUF_LIST_SERIALIZATION_HANDLER(ListType, Type)            \
  {                                                                                 \
    QMetaType::fromType<ListType>(),                                                \
            QProtobufSerializerPrivate::serializeWrapper<                           \
                    ListType, QProtobufSerializerPrivate::serializeListType<Type>>, \
            QProtobufSerializerPrivate::deserializeList<Type>,                      \
            QProtobufSerializerPrivate::isPresent<ListType>,                    \
            QtProtobuf::WireTypes::LengthDelimited                                  \
  }

#define QT_CONSTRUCT_PROTOBUF_NON_PACKED_LIST_SERIALIZATION_HANDLER(ListType, Type, WireType) \
{                                                                                           \
QMetaType::fromType<ListType>(),                                                          \
        QProtobufSerializerPrivate::serializeNonPackedWrapper<                            \
                ListType, QProtobufSerializerPrivate::serializeNonPackedList<Type>>,      \
        QProtobufSerializerPrivate::deserializeNonPackedList<Type>,                       \
        QProtobufSerializerPrivate::isPresent<ListType>, WireType                     \
}

constexpr SerializerRegistryType<30> IntegratedTypesSerializers = { {
        QT_CONSTRUCT_PROTOBUF_SERIALIZATION_HANDLER(float, QtProtobuf::WireTypes::Fixed32),
        QT_CONSTRUCT_PROTOBUF_SERIALIZATION_HANDLER(double, QtProtobuf::WireTypes::Fixed64),
        QT_CONSTRUCT_PROTOBUF_SERIALIZATION_HANDLER(QtProtobuf::int32,
                                                    QtProtobuf::WireTypes::Varint),
        QT_CONSTRUCT_PROTOBUF_SERIALIZATION_HANDLER(QtProtobuf::int64,
                                                    QtProtobuf::WireTypes::Varint),
        QT_CONSTRUCT_PROTOBUF_SERIALIZATION_HANDLER(QtProtobuf::uint32,
                                                    QtProtobuf::WireTypes::Varint),
        QT_CONSTRUCT_PROTOBUF_SERIALIZATION_HANDLER(QtProtobuf::uint64,
                                                    QtProtobuf::WireTypes::Varint),
        QT_CONSTRUCT_PROTOBUF_SERIALIZATION_HANDLER(QtProtobuf::sint32,
                                                    QtProtobuf::WireTypes::Varint),
        QT_CONSTRUCT_PROTOBUF_SERIALIZATION_HANDLER(QtProtobuf::sint64,
                                                    QtProtobuf::WireTypes::Varint),
        QT_CONSTRUCT_PROTOBUF_SERIALIZATION_HANDLER(QtProtobuf::fixed32,
                                                    QtProtobuf::WireTypes::Fixed32),
        QT_CONSTRUCT_PROTOBUF_SERIALIZATION_HANDLER(QtProtobuf::fixed64,
                                                    QtProtobuf::WireTypes::Fixed64),
        QT_CONSTRUCT_PROTOBUF_SERIALIZATION_HANDLER(QtProtobuf::sfixed32,
                                                    QtProtobuf::WireTypes::Fixed32),
        QT_CONSTRUCT_PROTOBUF_SERIALIZATION_HANDLER(QtProtobuf::sfixed64,
                                                    QtProtobuf::WireTypes::Fixed64),
        QT_CONSTRUCT_PROTOBUF_SERIALIZATION_HANDLER(QtProtobuf::boolean,
                                                    QtProtobuf::WireTypes::Varint),
        QT_CONSTRUCT_PROTOBUF_SERIALIZATION_HANDLER(QString,
                                                    QtProtobuf::WireTypes::LengthDelimited),
        QT_CONSTRUCT_PROTOBUF_SERIALIZATION_HANDLER(QByteArray,
                                                    QtProtobuf::WireTypes::LengthDelimited),
        QT_CONSTRUCT_PROTOBUF_LIST_SERIALIZATION_HANDLER(QtProtobuf::floatList, float),
        QT_CONSTRUCT_PROTOBUF_LIST_SERIALIZATION_HANDLER(QtProtobuf::doubleList, double),
        QT_CONSTRUCT_PROTOBUF_LIST_SERIALIZATION_HANDLER(QtProtobuf::int32List, QtProtobuf::int32),
        QT_CONSTRUCT_PROTOBUF_LIST_SERIALIZATION_HANDLER(QtProtobuf::int64List, QtProtobuf::int64),
        QT_CONSTRUCT_PROTOBUF_LIST_SERIALIZATION_HANDLER(QtProtobuf::uint32List,
                                                         QtProtobuf::uint32),
        QT_CONSTRUCT_PROTOBUF_LIST_SERIALIZATION_HANDLER(QtProtobuf::uint64List,
                                                         QtProtobuf::uint64),
        QT_CONSTRUCT_PROTOBUF_LIST_SERIALIZATION_HANDLER(QtProtobuf::sint32List,
                                                         QtProtobuf::sint32),
        QT_CONSTRUCT_PROTOBUF_LIST_SERIALIZATION_HANDLER(QtProtobuf::sint64List,
                                                         QtProtobuf::sint64),
        QT_CONSTRUCT_PROTOBUF_LIST_SERIALIZATION_HANDLER(QtProtobuf::fixed32List,
                                                         QtProtobuf::fixed32),
        QT_CONSTRUCT_PROTOBUF_LIST_SERIALIZATION_HANDLER(QtProtobuf::fixed64List,
                                                         QtProtobuf::fixed64),
        QT_CONSTRUCT_PROTOBUF_LIST_SERIALIZATION_HANDLER(QtProtobuf::sfixed32List,
                                                         QtProtobuf::sfixed32),
        QT_CONSTRUCT_PROTOBUF_LIST_SERIALIZATION_HANDLER(QtProtobuf::sfixed64List,
                                                         QtProtobuf::sfixed64),
        QT_CONSTRUCT_PROTOBUF_LIST_SERIALIZATION_HANDLER(QtProtobuf::boolList, QtProtobuf::boolean),
        QT_CONSTRUCT_PROTOBUF_NON_PACKED_LIST_SERIALIZATION_HANDLER(
                QStringList, QString, QtProtobuf::WireTypes::LengthDelimited),
        QT_CONSTRUCT_PROTOBUF_NON_PACKED_LIST_SERIALIZATION_HANDLER(
                QByteArrayList, QByteArray, QtProtobuf::WireTypes::LengthDelimited),
} };

constexpr SerializerRegistryType<13> IntegratedNonPackedSerializers = { {
        QT_CONSTRUCT_PROTOBUF_NON_PACKED_LIST_SERIALIZATION_HANDLER(QtProtobuf::floatList, float,
                                                                    QtProtobuf::WireTypes::Fixed32),
        QT_CONSTRUCT_PROTOBUF_NON_PACKED_LIST_SERIALIZATION_HANDLER(QtProtobuf::doubleList, double,
                                                                    QtProtobuf::WireTypes::Fixed64),
        QT_CONSTRUCT_PROTOBUF_NON_PACKED_LIST_SERIALIZATION_HANDLER(
                QtProtobuf::int32List, QtProtobuf::int32, QtProtobuf::WireTypes::Varint),
        QT_CONSTRUCT_PROTOBUF_NON_PACKED_LIST_SERIALIZATION_HANDLER(
                QtProtobuf::int64List, QtProtobuf::int64, QtProtobuf::WireTypes::Varint),
        QT_CONSTRUCT_PROTOBUF_NON_PACKED_LIST_SERIALIZATION_HANDLER(
                QtProtobuf::uint32List, QtProtobuf::uint32, QtProtobuf::WireTypes::Varint),
        QT_CONSTRUCT_PROTOBUF_NON_PACKED_LIST_SERIALIZATION_HANDLER(
                QtProtobuf::uint64List, QtProtobuf::uint64, QtProtobuf::WireTypes::Varint),
        QT_CONSTRUCT_PROTOBUF_NON_PACKED_LIST_SERIALIZATION_HANDLER(
                QtProtobuf::sint32List, QtProtobuf::sint32, QtProtobuf::WireTypes::Varint),
        QT_CONSTRUCT_PROTOBUF_NON_PACKED_LIST_SERIALIZATION_HANDLER(
                QtProtobuf::sint64List, QtProtobuf::sint64, QtProtobuf::WireTypes::Varint),
        QT_CONSTRUCT_PROTOBUF_NON_PACKED_LIST_SERIALIZATION_HANDLER(
                QtProtobuf::fixed32List, QtProtobuf::fixed32, QtProtobuf::WireTypes::Fixed32),
        QT_CONSTRUCT_PROTOBUF_NON_PACKED_LIST_SERIALIZATION_HANDLER(
                QtProtobuf::fixed64List, QtProtobuf::fixed64, QtProtobuf::WireTypes::Fixed64),
        QT_CONSTRUCT_PROTOBUF_NON_PACKED_LIST_SERIALIZATION_HANDLER(
                QtProtobuf::sfixed32List, QtProtobuf::sfixed32, QtProtobuf::WireTypes::Fixed32),
        QT_CONSTRUCT_PROTOBUF_NON_PACKED_LIST_SERIALIZATION_HANDLER(
                QtProtobuf::sfixed64List, QtProtobuf::sfixed64, QtProtobuf::WireTypes::Fixed64),
        QT_CONSTRUCT_PROTOBUF_NON_PACKED_LIST_SERIALIZATION_HANDLER(
                QtProtobuf::boolList, QtProtobuf::boolean, QtProtobuf::WireTypes::Varint),
} };

template<std::size_t N>
std::optional<QProtobufSerializerPrivate::ProtobufSerializationHandler>
findIntegratedTypeHandlerImpl(QMetaType metaType, const SerializerRegistryType<N> &registry)
{
    typename SerializerRegistryType<N>::const_iterator it = std::find_if(
            registry.begin(), registry.end(),
            [&metaType](const QProtobufSerializerPrivate::ProtobufSerializationHandler &handler) {
                return handler.metaType == metaType;
            });
    if (it == registry.end())
        return std::nullopt;
    return { *it };
}

std::optional<QProtobufSerializerPrivate::ProtobufSerializationHandler>
findIntegratedTypeHandler(QMetaType metaType, bool nonPacked)
{
    if (nonPacked)
        return findIntegratedTypeHandlerImpl(metaType, IntegratedNonPackedSerializers);

    return findIntegratedTypeHandlerImpl(metaType, IntegratedTypesSerializers);
}
}

/*!
    Constructs a new serializer instance.
*/
QProtobufSerializer::QProtobufSerializer() : d_ptr(new QProtobufSerializerPrivate(this))
{
}

/*!
    Destroys the serializer instance.
*/
QProtobufSerializer::~QProtobufSerializer() = default;

/*!
    This is called by serialize() to serialize a registered Protobuf message
    \a message with \a ordering. \a message must not be
    \nullptr.
    Returns a QByteArray containing the serialized message.
*/
QByteArray QProtobufSerializer::serializeMessage(
        const QProtobufMessage *message,
        const QtProtobufPrivate::QProtobufPropertyOrdering &ordering) const
{
    QByteArray result;
    for (int index = 0; index < ordering.fieldCount(); ++index) {
        int fieldIndex = ordering.getFieldNumber(index);
        Q_ASSERT_X(fieldIndex < 536870912 && fieldIndex > 0, "", "fieldIndex is out of range");
        QProtobufPropertyOrderingInfo fieldInfo(ordering, index);
        QVariant propertyValue = message->property(fieldInfo);
        result.append(d_ptr->serializeProperty(propertyValue, fieldInfo));
    }

    // Restore any unknown fields we have stored away:
    const QProtobufMessagePrivate *messagePrivate = QProtobufMessagePrivate::get(message);
    for (const auto &fields : messagePrivate->unknownEntries)
        result += fields.join();

    return result;
}

void QProtobufSerializerPrivate::setUnexpectedEndOfStreamError()
{
    setDeserializationError(QAbstractProtobufSerializer::UnexpectedEndOfStreamError,
                            QCoreApplication::translate("QtProtobuf", "Unexpected end of stream"));
}

void QProtobufSerializerPrivate::clearError()
{
    deserializationError = QAbstractProtobufSerializer::NoError;
    deserializationErrorString.clear();
}

/*!
    This is called by deserialize() to deserialize a registered Protobuf message
    \a message with \a ordering, from a QByteArrayView \a data. \a message
    can be assumed to not be \nullptr.
    Returns \c true if deserialization was successful, otherwise \c false.
*/
bool QProtobufSerializer::deserializeMessage(
        QProtobufMessage *message, const QtProtobufPrivate::QProtobufPropertyOrdering &ordering,
        QByteArrayView data) const
{
    d_ptr->clearError();
    auto it = QProtobufSelfcheckIterator::fromView(data);
    while (it.isValid() && it != data.end()) {
        if (!d_ptr->deserializeProperty(message, ordering, it))
            return false;
    }
    if (!it.isValid())
        d_ptr->setUnexpectedEndOfStreamError();
    return it.isValid();
}

/*!
    Serialize an \a message with \a ordering and \a fieldInfo.
    Returns a QByteArray containing the serialized message.

    You should not call this function directly.
*/
QByteArray
QProtobufSerializer::serializeObject(const QProtobufMessage *message,
                                     const QtProtobufPrivate::QProtobufPropertyOrdering &ordering,
                                     const QProtobufPropertyOrderingInfo &fieldInfo) const
{
    QByteArray result = QProtobufSerializerPrivate::prependLengthDelimitedSize(
            serializeMessage(message, ordering));
    result.prepend(QProtobufSerializerPrivate::encodeHeader(
            fieldInfo.getFieldNumber(), QtProtobuf::WireTypes::LengthDelimited));
    return result;
}

/*!
    Deserialize an \a message with \a ordering from a QProtobufSelfcheckIterator
    \a it. Returns \c true if deserialization was successful, otherwise
    \c false.

    You should not call this function directly.
*/
bool QProtobufSerializer::deserializeObject(QProtobufMessage *message,
        const QtProtobufPrivate::QProtobufPropertyOrdering &ordering,
        QProtobufSelfcheckIterator &it) const
{
    if (it.bytesLeft() == 0) {
        d_ptr->setUnexpectedEndOfStreamError();
        return false;
    }
    std::optional<QByteArray> array = QProtobufSerializerPrivate::deserializeLengthDelimited(it);
    if (!array) {
        d_ptr->setUnexpectedEndOfStreamError();
        return false;
    }
    return deserializeMessage(message, ordering, array.value());
}

/*!
    This function is called to serialize \a message as a part of list property
    with \a ordering and \a fieldInfo.

    You should not call this function directly.
*/
QByteArray QProtobufSerializer::serializeListObject(
        const QProtobufMessage *message,
        const QtProtobufPrivate::QProtobufPropertyOrdering &ordering,
        const QProtobufPropertyOrderingInfo &fieldInfo) const
{
    return serializeObject(message, ordering, fieldInfo);
}

/*!
    This function deserializes an \a message from byte stream as part of list property, with
    \a ordering from a QProtobufSelfcheckIterator \a it.
    Returns \c true if deserialization was successful, otherwise \c false.

    You should not call this function directly.
*/
bool QProtobufSerializer::deserializeListObject(QProtobufMessage *message,
        const QtProtobufPrivate::QProtobufPropertyOrdering &ordering,
        QProtobufSelfcheckIterator &it) const
{
    return deserializeObject(message, ordering, it);
}

/*!
    This function serializes QMap pair of \a key and \a value with
    \a fieldInfo to a QByteArray

    You should not call this function directly.
*/
QByteArray
QProtobufSerializer::serializeMapPair(const QVariant &key, const QVariant &value,
                                      const QProtobufPropertyOrderingInfo &fieldInfo) const
{
    auto keyHandler = findIntegratedTypeHandler(key.metaType(), false);
    Q_ASSERT_X(keyHandler, "QProtobufSerializer", "Map key is not an integrated type.");

    QByteArray result = QProtobufSerializerPrivate::encodeHeader(
            fieldInfo.getFieldNumber(), QtProtobuf::WireTypes::LengthDelimited);

    result.append(QProtobufSerializerPrivate::prependLengthDelimitedSize(
            keyHandler->serializer(
                    key, QProtobufSerializerPrivate::encodeHeader(1, keyHandler->wireType))
            + d_ptr->serializeProperty(value, fieldInfo.infoForMapValue())));
    return result;
}

/*!
    This function deserializes QMap pair of \a key and \a value from a
    QProtobufSelfcheckIterator \a it.
    Returns \c true if deserialization was successful, otherwise \c false.

    You should not call this function directly.
*/
bool QProtobufSerializer::deserializeMapPair(QVariant &key, QVariant &value, QProtobufSelfcheckIterator &it) const
{
    return d_ptr->deserializeMapPair(key, value, it);
}

/*!
    This function serializes \a value as a QByteArray for enum associated with
    property \a fieldInfo.

    You should not call this function directly.
*/
QByteArray QProtobufSerializer::serializeEnum(QtProtobuf::int64 value,
                                              const QProtobufPropertyOrderingInfo &fieldInfo) const
{
    if (value == 0 && !isOneofField(fieldInfo))
        return {};

    QtProtobuf::WireTypes type = QtProtobuf::WireTypes::Varint;
    int fieldNumber = fieldInfo.getFieldNumber();
    return QProtobufSerializerPrivate::encodeHeader(fieldNumber, type)
            + QProtobufSerializerPrivate::serializeBasic<QtProtobuf::int64>(value);
}

/*!
    This function serializes a list, \a value, as a QByteArray for enum
    associated with property \a fieldInfo.

    You should not call this function directly.
*/
QByteArray
QProtobufSerializer::serializeEnumList(const QList<QtProtobuf::int64> &value,
                                       const QProtobufPropertyOrderingInfo &fieldInfo) const
{
    if (value.isEmpty()) {
        return {};
    }

    auto header = QProtobufSerializerPrivate::encodeHeader(fieldInfo.getFieldNumber(),
                                                           QtProtobuf::WireTypes::LengthDelimited);

    if (fieldInfo.getFieldFlags() & QtProtobufPrivate::NonPacked)
        return QProtobufSerializerPrivate::serializeNonPackedList<QtProtobuf::int64>(value, header);

    return header + QProtobufSerializerPrivate::serializeListType<QtProtobuf::int64>(value);
}

/*!
    This function deserializes an enum \a value from a QProtobufSelfcheckIterator \a it.
    Returns \c true if deserialization was successful, otherwise \c false.

    You should not call this function directly.
*/
bool QProtobufSerializer::deserializeEnum(QtProtobuf::int64 &value, QProtobufSelfcheckIterator &it) const
{
    QVariant variantValue;
    if (!QProtobufSerializerPrivate::deserializeBasic<QtProtobuf::int64>(it, variantValue)) {
        d_ptr->setUnexpectedEndOfStreamError();
        return false;
    }
    value = variantValue.value<QtProtobuf::int64>();
    return true;
}

/*!
    This function deserializes a list of enum \a value from a QProtobufSelfcheckIterator \a it.
    Returns \c true if deserialization was successful, otherwise \c false.

    You should not call this function directly.
*/
bool QProtobufSerializer::deserializeEnumList(QList<QtProtobuf::int64> &value, QProtobufSelfcheckIterator &it) const
{
    QVariant variantValue;
    if (!QProtobufSerializerPrivate::deserializeList<QtProtobuf::int64>(it, variantValue)) {
        d_ptr->setUnexpectedEndOfStreamError();
        return false;
    }
    value = variantValue.value<QList<QtProtobuf::int64>>();
    return true;
}

QProtobufSerializerPrivate::QProtobufSerializerPrivate(QProtobufSerializer *q) : q_ptr(q)
{
}

/*!
    Encode a property field index and its type into output bytes.

    Header byte
    Meaning    |  Field index  |  Type
    ---------- | ------------- | --------
    bit number | 7  6  5  4  3 | 2  1  0

    fieldIndex: The index of a property in parent object
    wireType: Serialization type used for the property with index @p fieldIndex

    Returns a varint-encoded fieldIndex and wireType
 */
QByteArray QProtobufSerializerPrivate::encodeHeader(int fieldIndex,
                                                           QtProtobuf::WireTypes wireType)
{
    uint32_t header = (fieldIndex << 3) | int(wireType);
    return serializeVarintCommon<uint32_t>(header);
}

/*!
    Decode a property field index and its serialization type from input bytes

    Iterator: that points to header with encoded field index and serialization type
    fieldIndex: Decoded index of a property in parent object
    wireType: Decoded serialization type used for the property with index
 *
 \return true if both decoded wireType and fieldIndex have "allowed" values and false, otherwise
 */
bool QProtobufSerializerPrivate::decodeHeader(QProtobufSelfcheckIterator &it,
                                                     int &fieldIndex,
                                                     QtProtobuf::WireTypes &wireType)
{
    if (it.bytesLeft() == 0)
        return false;
    auto opt = deserializeVarintCommon<uint32_t>(it);
    if (!opt)
        return false;
    uint32_t header = opt.value();
    wireType = static_cast<QtProtobuf::WireTypes>(header & 0b00000111);
    fieldIndex = header >> 3;

    constexpr int maxFieldIndex = (1 << 29) - 1;
    return fieldIndex <= maxFieldIndex && fieldIndex > 0
            && (wireType == QtProtobuf::WireTypes::Varint
                || wireType == QtProtobuf::WireTypes::Fixed64
                || wireType == QtProtobuf::WireTypes::Fixed32
                || wireType == QtProtobuf::WireTypes::LengthDelimited);
}

void QProtobufSerializerPrivate::skipVarint(QProtobufSelfcheckIterator &it)
{
    while ((*it) & 0x80)
        ++it;
    ++it;
}

void QProtobufSerializerPrivate::skipLengthDelimited(QProtobufSelfcheckIterator &it)
{
    //Get length of length-delimited field
    auto opt = QProtobufSerializerPrivate::deserializeVarintCommon<QtProtobuf::uint64>(it);
    if (!opt) {
        it += it.bytesLeft() + 1;
        return;
    }
    QtProtobuf::uint64 length = opt.value();
    it += length;
}

qsizetype QProtobufSerializerPrivate::skipSerializedFieldBytes(QProtobufSelfcheckIterator &it, QtProtobuf::WireTypes type)
{
    const auto *initialIt = QByteArray::const_iterator(it);
    switch (type) {
    case QtProtobuf::WireTypes::Varint:
        skipVarint(it);
        break;
    case QtProtobuf::WireTypes::Fixed32:
        it += sizeof(decltype(QtProtobuf::fixed32::_t));
        break;
    case QtProtobuf::WireTypes::Fixed64:
        it += sizeof(decltype(QtProtobuf::fixed64::_t));
        break;
    case QtProtobuf::WireTypes::LengthDelimited:
        skipLengthDelimited(it);
        break;
    case QtProtobuf::WireTypes::Unknown:
    default:
        Q_UNREACHABLE();
        return 0;
    }

    return std::distance(initialIt, QByteArray::const_iterator(it));
}

QByteArray
QProtobufSerializerPrivate::serializeProperty(const QVariant &propertyValue,
                                              const QProtobufPropertyOrderingInfo &fieldInfo)
{
    QMetaType metaType = propertyValue.metaType();

    qProtoDebug() << "propertyValue" << propertyValue << "fieldIndex" << fieldInfo.getFieldNumber()
                  << "metaType" << metaType.name();

    if (metaType.id() == QMetaType::UnknownType || propertyValue.isNull()) {
        // Empty value
        return {};
    }

    auto basicHandler = findIntegratedTypeHandler(
            metaType, fieldInfo.getFieldFlags() & QtProtobufPrivate::NonPacked);
    if (basicHandler) {
        bool serializeUninitialized = isOneofField(fieldInfo);
        if (!basicHandler->isPresent(propertyValue) && !serializeUninitialized) {
            return {};
        }

        QByteArray header = QProtobufSerializerPrivate::encodeHeader(fieldInfo.getFieldNumber(),
                                                                     basicHandler->wireType);
        return basicHandler->serializer(propertyValue, header);
    }

    auto handler = QtProtobufPrivate::findHandler(metaType);
    if (!handler.serializer) {
        qProtoWarning() << "No serializer for type" << propertyValue.typeName();
        return {};
    }
    QByteArray result;
    handler.serializer(q_ptr, propertyValue, fieldInfo, result);

    return result;
}

bool QProtobufSerializerPrivate::deserializeProperty(
        QProtobufMessage *message,
        const QtProtobufPrivate::QProtobufPropertyOrdering &ordering,
        QProtobufSelfcheckIterator &it)
{
    Q_ASSERT(it.isValid() && it.bytesLeft() > 0);
    //Each iteration we expect iterator is setup to beginning of next chunk
    int fieldNumber = QtProtobuf::InvalidFieldNumber;
    QtProtobuf::WireTypes wireType = QtProtobuf::WireTypes::Unknown;
    const QProtobufSelfcheckIterator itBeforeHeader = it; // copy this, we may need it later
    if (!QProtobufSerializerPrivate::decodeHeader(it, fieldNumber, wireType)) {
        setDeserializationError(
                QAbstractProtobufSerializer::InvalidHeaderError,
                QCoreApplication::translate("QtProtobuf",
                                     "Message received doesn't contain valid header byte."));
        return false;
    }

    int index = ordering.indexOfFieldNumber(fieldNumber);
    if (index == -1) {
        // This is an unknown field, it may have been added in a later revision
        // of the Message we are currently deserializing. We must store the
        // bytes for this field and re-emit them later if this message is
        // serialized again.
        qsizetype length = std::distance(itBeforeHeader, it); // size of header
        length += QProtobufSerializerPrivate::skipSerializedFieldBytes(it, wireType);

        if (!it.isValid()) {
            setUnexpectedEndOfStreamError();
            return false;
        }

        QProtobufMessagePrivate *messagePrivate = QProtobufMessagePrivate::get(message);
        messagePrivate->storeUnknownEntry(QByteArrayView(itBeforeHeader.data(), length),
                                          fieldNumber);
        return true;
    }

    QProtobufPropertyOrderingInfo fieldInfo(ordering, index);
    QVariant newPropertyValue = message->property(fieldInfo);
    QMetaType metaType = newPropertyValue.metaType();

    qProtoDebug() << "wireType:" << wireType << "metaType:" << metaType.name()
                  << "currentByte:" << QString::number((*it), 16);

    bool isNonPacked = ordering.getFieldFlags(index) & QtProtobufPrivate::NonPacked;
    auto basicHandler = findIntegratedTypeHandler(metaType, isNonPacked);

    if (basicHandler) {
        if (basicHandler->wireType != wireType) {
            // If the handler wiretype mismatches the wiretype received from the
            // wire that most probably means that we received the list in wrong
            // format. This can happen because of mismatch of the field packed
            // option in the protobuf schema on the wire ends. Invert the
            // isNonPacked flag and try to find the handler one more time to make
            // sure that we cover this exceptional case.
            // See the conformance tests
            // Required.Proto3.ProtobufInput.ValidDataRepeated.*.UnpackedInput
            // for details.
            basicHandler = findIntegratedTypeHandler(metaType, !isNonPacked);
            if (!basicHandler || basicHandler->wireType != wireType) {
                setDeserializationError(
                        QAbstractProtobufSerializer::InvalidHeaderError,
                        QCoreApplication::translate("QtProtobuf",
                                                    "Message received has invalid wiretype for the "
                                                    "field number %1. Expected %2, received %3")
                                .arg(fieldNumber)
                                .arg(static_cast<int>(basicHandler->wireType))
                                .arg(static_cast<int>(wireType)));
                return false;
            }
        }

        if (!basicHandler->deserializer(it, newPropertyValue)) {
            setUnexpectedEndOfStreamError();
            return false;
        }
    } else {
        auto handler = QtProtobufPrivate::findHandler(metaType);
        if (!handler.deserializer) {
            qProtoWarning() << "No deserializer for type" << metaType.name();
            setDeserializationError(
                    QAbstractProtobufSerializer::NoDeserializerError,
                    QCoreApplication::translate("QtProtobuf",
                                         "No deserializer is registered for type %1"));
            return false;
        }
        handler.deserializer(q_ptr, it, newPropertyValue);
    }

    return message->setProperty(fieldInfo, std::move(newPropertyValue));
}

bool QProtobufSerializerPrivate::deserializeMapPair(QVariant &key, QVariant &value, QProtobufSelfcheckIterator &it)
{
    int mapIndex = 0;
    QtProtobuf::WireTypes type = QtProtobuf::WireTypes::Unknown;
    auto opt = QProtobufSerializerPrivate::deserializeVarintCommon<QtProtobuf::uint32>(it);
    if (!opt) {
        setUnexpectedEndOfStreamError();
        return false;
    }
    unsigned int count = opt.value();
    qProtoDebug("count: %u", count);
    QProtobufSelfcheckIterator last = it + count;
    while (it.isValid() && it != last) {
        if (!QProtobufSerializerPrivate::decodeHeader(it, mapIndex, type)) {
            setDeserializationError(
                        QAbstractProtobufSerializer::InvalidHeaderError,
                        QCoreApplication::translate("QtProtobuf",
                                             "Message received doesn't contain valid header byte."));
            return false;
        }
        if (mapIndex == 1) {
            //Only simple types are supported as keys
            QMetaType metaType = key.metaType();
            auto basicHandler = findIntegratedTypeHandler(metaType, false);
            if (!basicHandler) {
                // clang-format off
                QString errorStr = QCoreApplication::translate("QtProtobuf",
                                                        "Either there is no deserializer for type "
                                                        "%1 or it is not a builtin type")
                        .arg(QLatin1String(key.typeName()));
                // clang-format on
                setDeserializationError(QAbstractProtobufSerializer::NoDeserializerError, errorStr);
                return false;
            }
            basicHandler->deserializer(it, key);
        } else {
            //TODO: replace with some common function
            QMetaType metaType = value.metaType();
            auto basicHandler = findIntegratedTypeHandler(metaType, false);
            if (basicHandler) {
                basicHandler->deserializer(it, value);
            } else {
                auto handler = QtProtobufPrivate::findHandler(metaType);
                if (!handler.deserializer) {
                    qProtoWarning() << "No deserializer for type" << value.typeName();
                    setDeserializationError(
                                QAbstractProtobufSerializer::NoDeserializerError,
                                QCoreApplication::translate("QtProtobuf",
                                                     "No deserializer is registered for type %1")
                                .arg(QLatin1String(value.typeName())));
                    return false;
                }
                handler.deserializer(q_ptr, it, value);
            }
        }
    }
    return it == last;
}

QAbstractProtobufSerializer::DeserializationError QProtobufSerializer::deserializationError() const
{
    return d_ptr->deserializationError;
}

QString QProtobufSerializer::deserializationErrorString() const
{
    return d_ptr->deserializationErrorString;
}

void QProtobufSerializerPrivate::setDeserializationError(
        QAbstractProtobufSerializer::DeserializationError error, const QString &errorString)
{
    deserializationError = error;
    deserializationErrorString = errorString;
}

QT_END_NAMESPACE
