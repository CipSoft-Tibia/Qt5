// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtProtobufWellKnownTypes/qtprotobufwellknowntypesglobal.h>

#include <QtProtobuf/qprotobufbaseserializer.h>
#include <QtProtobuf/private/qprotobufserializer_p.h>

#include "qprotobufanysupport.h"

#include <google/protobuf/any.qpb.h>

QT_BEGIN_NAMESPACE

namespace QtProtobuf {
using namespace QtProtobufPrivate;

class AnyPrivate
{
public:
    QString typeUrl;
    QByteArray value;

    static AnyPrivate *get(const Any *any) { return any->d_ptr; }
};

static void serializerProxy(const QProtobufBaseSerializer *serializer, const QVariant &object,
                            const QProtobufPropertyOrderingInfo &fieldInfo)
{
    if (object.isNull())
        return;

    Any any = object.value<Any>();
    if (any.typeUrl().isEmpty())
        return;

    google::protobuf::Any realAny;
    realAny.setValue(any.value());
    realAny.setTypeUrl(any.typeUrl());
    serializer->serializeObject(&realAny, google::protobuf::Any::propertyOrdering, fieldInfo);
}

static void listSerializerProxy(const QProtobufBaseSerializer *serializer, const QVariant &object,
                                const QProtobufPropertyOrderingInfo &fieldInfo)
{
    const auto anyList = object.value<QList<Any>>();
    for (const Any &any : anyList) {
        google::protobuf::Any realAny;
        realAny.setValue(any.value());
        realAny.setTypeUrl(any.typeUrl());
        serializer->serializeListObject(&realAny, google::protobuf::Any::propertyOrdering,
                                        fieldInfo);
    }
}

static void listDeserializerProxy(const QProtobufBaseSerializer *deserializer, QVariant &object)
{
    auto anyList = object.value<QList<Any>>();
    const auto &ordering = google::protobuf::Any::propertyOrdering;
    google::protobuf::Any realAny;
    if (deserializer->deserializeObject(&realAny, ordering)) {
        Any any;
        any.setTypeUrl(realAny.typeUrl());
        any.setValue(realAny.value());
        anyList.append(std::move(any));
    } else {
        return; // unexpected end of data
    }
    object.setValue(std::move(anyList));
}

static void deserializerProxy(const QProtobufBaseSerializer *deserializer, QVariant &object)
{
    google::protobuf::Any realAny;
    if (deserializer->deserializeObject(&realAny, google::protobuf::Any::propertyOrdering)) {
        Any any;
        any.setTypeUrl(realAny.typeUrl());
        any.setValue(realAny.value());
        object.setValue(std::move(any));
    }
}

void Any::registerTypes()
{
    QtProtobufPrivate::registerHandler(
            QMetaType::fromType<Any>(),
            SerializationHandler{ &serializerProxy, &deserializerProxy });
    QtProtobufPrivate::registerHandler(
            QMetaType::fromType<QList<Any>>(),
            { &listSerializerProxy, &listDeserializerProxy });
}

/*!
    \class QtProtobuf::Any
    \brief A helper class to simplify working with the protobuf Any type.
    \inmodule QtProtobufWellKnownTypes

    The Any class enables easy casting to and from other types using as() and
    fromMessage().

    It is the type used in code generated by \l{The qtprotobufgen Tool}
    {qtprotobufgen} when a message contains a field of the \c
    google.protobuf.Any type.
*/

/*!
    Constructs a defaulted, empty, instance of Any.
*/
Any::Any() : QProtobufMessage(&Any::staticMetaObject), d_ptr(new AnyPrivate())
{
}

/*!
    Destroys this instance of Any
*/
Any::~Any()
{
    delete d_ptr;
}

/*!
    Constructs a copy of \a other.
*/
Any::Any(const Any &other)
    : QProtobufMessage(other),
      d_ptr(new AnyPrivate(*other.d_ptr))
{
}

/*!
    Copies the data of \a other into this instance.
*/
Any &Any::operator=(const Any &other)
{
    if (this == &other)
        return *this;
    QProtobufMessage::operator=(other);
    *d_ptr = *other.d_ptr;
    return *this;
}

/*!
    Returns the type URL of the Any object.
    The URL is meant as a hint for what the contained data really is.

    \note Qt has no support for dynamically obtaining any potential recipes for
    deconstructing types and simply uses the type URL to verify that types
    passed as T to \c{as<T>()} are of the correct type.

    \sa setTypeUrl(), value()
*/
QString Any::typeUrl() const
{
    return d_func()->typeUrl;
}

/*!
    Returns the raw bytes that make up the value stored.

    Consult typeUrl() to determine how to interpret these bytes.

    \sa setValue(), typeUrl()
*/
QByteArray Any::value() const
{
    return d_func()->value;
}

/*!
    Sets the type URL for the data contained to \a typeUrl.

    \note No verification is performed on the input to test if it's correct or
    in a valid format.

    \sa typeUrl(), fromMessage()
*/
void Any::setTypeUrl(const QString &typeUrl)
{
    Q_D(Any);
    if (d->typeUrl != typeUrl)
        d->typeUrl = typeUrl;
}

/*!
    Sets the raw bytes of the value stored to \a value.

    \a value must be the output of serializing a message.

    \sa value(), fromMessage()
*/
void Any::setValue(const QByteArray &value)
{
    Q_D(Any);
    if (d->value != value)
        d->value = value;
}

/*!
    \fn template <typename T> std::optional<T> Any::as(QAbstractProtobufSerializer *serializer) const

    This function compares the message name of T with the value of typeUrl()
    before deserializing the data using \a serializer.

    If the verification or deserialization fails it will return
    \c{std::nullopt}.

    \note T must be a class derived from QProtobufMessage with the
    \c{Q_PROTOBUF_OBJECT} macro or (for a nested Any message) be Any itself.
*/

bool Any::asImpl(QAbstractProtobufSerializer *serializer, QProtobufMessage *message,
                 QtProtobufPrivate::QProtobufPropertyOrdering ordering) const
{
    Q_ASSERT_X(serializer != nullptr, "Any::asImpl", "serializer is null");
    QString tUrl = typeUrl();
    qsizetype lastSegmentIndex = tUrl.lastIndexOf(u'/') + 1;
    if (QStringView(tUrl).mid(lastSegmentIndex).compare(ordering.getMessageFullName()) != 0)
        return false;
    return serializer->deserializeMessage(message, ordering, value());
}

std::optional<Any> Any::asAnyImpl(QAbstractProtobufSerializer *serializer) const
{
    google::protobuf::Any realAny;
    if (!asImpl(serializer, &realAny, google::protobuf::Any::propertyOrdering))
        return std::nullopt;
    Any any;
    any.setTypeUrl(realAny.typeUrl());
    any.setValue(realAny.value());
    return {std::move(any)};
}

/*!
    \fn template <typename T> static Any Any::fromMessage(QAbstractProtobufSerializer *serializer,
        const T &message, QAnyStringView typeUrlPrefix)

    This function serializes the given \a message as the value of the returned
    Any instance. This instance's typeUrl() is constructed from a prefix, a
    forward slash and the message name obtained from
    \c{T::propertyOrdering.getMessageFullName()} using \a serializer. If \a
    typeUrlPrefix is supplied, it is used as prefix, otherwise
    \c{"type.googleapis.com"} is used.

    \note T must be a class derived from QProtobufMessage with the
    \c{Q_PROTOBUF_OBJECT} macro or (for a nested Any message) be Any itself.
*/

Any Any::fromMessageImpl(QAbstractProtobufSerializer *serializer, const QProtobufMessage *message,
                         QtProtobufPrivate::QProtobufPropertyOrdering ordering,
                         QAnyStringView typeUrlPrefix)
{
    Any any;
    any.setValue(serializer->serializeMessage(message, ordering));
    any.setTypeUrl(typeUrlPrefix.toString() + u'/' + ordering.getMessageFullName().toString());
    return { any };
}

// Used to handle nested Any messages.
Any Any::fromAnyMessageImpl(QAbstractProtobufSerializer *serializer,
                            const Any *message, QAnyStringView typeUrlPrefix)
{
    using RealAny = google::protobuf::Any;
    RealAny realAny;
    realAny.setTypeUrl(message->typeUrl());
    realAny.setValue(message->value());
    return fromMessageImpl(serializer, &realAny, RealAny::propertyOrdering, typeUrlPrefix);
}

QAnyStringView Any::defaultUrlPrefix()
{
    // The URL should describe the type of the serialized message.
    // We don't have support for this, so users have to provide the correct URL.
    // We won't check if it's correct.
    return u"type.googleapis.com";
}

bool Any::equals(const Any &other) const noexcept
{
    return typeUrl() == other.typeUrl() && value() == other.value();
}

/*!
    \fn bool Any::operator==(const Any &lhs, const Any &rhs)
    Returns true if the two instances of Any, \a lhs and \a rhs, compare equal.
*/

/*!
    \fn bool Any::operator!=(const Any &lhs, const Any &rhs)
    Returns true if the two instances of Any, \a lhs and \a rhs, are distinct.
*/
}

QT_END_NAMESPACE

#include "moc_qprotobufanysupport.cpp"
