// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qprotobufmessage_p.h"
#include "qprotobufmessage.h"

#include <QtProtobuf/qtprotobuftypes.h>

#include <QtCore/qassert.h>
#include <QtCore/qmetaobject.h>

#include <string>

QT_BEGIN_NAMESPACE

static std::string nullTerminate(QLatin1StringView l1) noexcept
{
    return l1.isNull() ? std::string{} : std::string{l1.data(), size_t(l1.size())};
}

/*!
    \class QProtobufMessage
    \inmodule QtProtobuf

    \brief Base class for all protobuf messages.

    Provides access to the properties of a message, using setProperty()
    and property(), without depending on what the message is.
*/

/*!
    \internal
    Used from generated classes to construct the QProtobufMessage base class.
    Internally the \a metaObject is used to query QMetaProperty
*/
QProtobufMessage::QProtobufMessage(const QMetaObject *metaObject)
    : d_ptr(new QProtobufMessagePrivate)
{
    d_ptr->metaObject = metaObject;
}

/*!
    \internal
    The QMetaObject which was passed to the QProtobufMessage constructor.
*/
const QMetaObject *QProtobufMessage::metaObject() const
{
    return d_ptr->metaObject;
}

/*!
    \internal
*/
int QProtobufMessagePrivate::getPropertyIndex(QAnyStringView propertyName) const
{
    return propertyName.visit([this](auto propertyName) {
        if constexpr (std::is_same_v<QStringView, decltype(propertyName)>) {
            return metaObject->indexOfProperty(propertyName.toLatin1().constData());
        } else if constexpr (std::is_same_v<QUtf8StringView, decltype(propertyName)>) {
            return metaObject->indexOfProperty(propertyName.toString().toLatin1().constData());
        } else if constexpr (std::is_same_v<QLatin1StringView, decltype(propertyName)>) {
            return metaObject->indexOfProperty(nullTerminate(propertyName).data());
        }
        return -1;
    });
}

void QProtobufMessagePrivate::storeUnknownEntry(QByteArrayView entry, int fieldNumber)
{
    unknownEntries[fieldNumber].append(entry.toByteArray());
}

std::optional<QMetaProperty> QProtobufMessagePrivate::metaProperty(QAnyStringView name) const
{
    const int index = getPropertyIndex(name);
    const QMetaProperty property = metaObject->property(index);
    if (property.isValid())
        return property;
    return std::nullopt;
}

std::optional<QMetaProperty>
QProtobufMessagePrivate::metaProperty(QtProtobufPrivate::QProtobufPropertyOrderingInfo info) const
{
    const int propertyIndex = info.getPropertyIndex() + metaObject->propertyOffset();
    const QMetaProperty metaProperty = metaObject->property(propertyIndex);
    if (metaProperty.isValid())
        return metaProperty;
    return std::nullopt;
}

/*!
    Set the property \a propertyName to the value stored in \a value.

    If the \a propertyName isn't a part of the known fields then the value will
    not be written and the function returns \c false.

    Returns \c false if it failed to store the \a value on the property.
    Otherwise \c{true}.
*/
bool QProtobufMessage::setProperty(QAnyStringView propertyName, const QVariant &value)
{
    Q_D(QProtobufMessage);

    if (auto mp = d->metaProperty(propertyName))
        return mp->writeOnGadget(this, value);

    return false;
}

/*!
    \overload
    \since 6.6
*/
bool QProtobufMessage::setProperty(QAnyStringView propertyName, QVariant &&value)
{
    Q_D(QProtobufMessage);

    if (auto mp = d->metaProperty(propertyName))
        return mp->writeOnGadget(this, std::move(value));

    return false;
}

/*!
    Get the value of the property \a propertyName.

    If the \a propertyName isn't known then the returned QVariant is invalid.
*/
QVariant QProtobufMessage::property(QAnyStringView propertyName) const
{
    Q_D(const QProtobufMessage);

    if (const auto mp = d->metaProperty(propertyName))
        return mp->readOnGadget(this);
    return {};
}

/*!
    \internal
*/
QProtobufMessage::QProtobufMessage(const QProtobufMessage &other)
    : d_ptr(other.d_ptr)
{
    d_ptr->ref.ref();
}

/*!
    \internal
*/
QProtobufMessage &QProtobufMessage::operator=(const QProtobufMessage &other)
{
    if (other.d_ptr == d_ptr)
        return *this;

    if (d_ptr && !d_ptr->ref.deref())
        delete d_ptr; // delete d_ptr if it's the last reference
    d_ptr = other.d_ptr;
    if (d_ptr)
        d_ptr->ref.ref();

    return *this;
}

/*!
    \internal
*/
QProtobufMessage::~QProtobufMessage()
{
    if (d_ptr && !d_ptr->ref.deref())
        delete d_ptr;
}

/*!
    \internal
*/
bool QProtobufMessage::isEqual(const QProtobufMessage &lhs, const QProtobufMessage &rhs) noexcept
{
    if (lhs.d_ptr == rhs.d_ptr)
        return true;
    return lhs.d_func()->unknownEntries == rhs.d_func()->unknownEntries;
}

namespace QtProtobufPrivate {
/*!
    \internal
*/
extern QProtobufMessagePointer constructMessageByName(const QString &messageType);
}

/*!
    Constructs QProtobufMessage using \a messageType.
    Returns a pointer to the constructed QProtobufMessage.

    This function attempts to create a message whose type matches \a messageType. If \a messageType
    is unknown, the function returns \nullptr. If the message is not found in the registry, the
    function returns \nullptr.
    Ownership of the constructed message is given to the function caller.
*/
QProtobufMessagePointer QProtobufMessage::constructByName(const QString &messageType)
{
    return QtProtobufPrivate::constructMessageByName(messageType);
}

/*!
    \typedef QProtobufMessagePointer
    \relates QProtobufMessage
    \inmodule QtProtobuf

    Synonym for std::unique_ptr<QProtobufMessage, QProtobufMessageDeleter>.
    Use this to manage the lifetime of dynamically allocated QProtobufMessages,
    such as those created by calling QProtobufMessage::constructByName.
*/

/*!
    \class QProtobufMessageDeleter
    \inmodule QtProtobuf
    \brief Calls the destructor of the child class of a QProtobufMessage.

    This class calls the destructor of a protobuf message using the meta-type
    system. This class is intended to be used with smart pointers, such as
    std::unique_ptr.

    \sa QProtobufMessagePointer
*/

/*!
    Destroys the message pointed to by \a ptr.
    This is intended for use with smart pointers.

    \sa QProtobufMessagePointer
*/
void QProtobufMessageDeleter::operator()(QProtobufMessage *ptr) noexcept
{
    if (!ptr)
        return;
    const QMetaObject *mobj = ptr->metaObject();
    QMetaType type = mobj->metaType();
    type.destroy(ptr);
}

QVariant
QProtobufMessage::property(const QtProtobufPrivate::QProtobufPropertyOrderingInfo &fieldInfo) const
{
    int propertyIndex = fieldInfo.getPropertyIndex() + metaObject()->propertyOffset();
    QMetaProperty metaProperty = metaObject()->property(propertyIndex);

    if (!metaProperty.isValid())
        return {};

    if (fieldInfo.getFieldFlags() & QtProtobufPrivate::Oneof
        || fieldInfo.getFieldFlags() & QtProtobufPrivate::Optional) {
        int hasPropertyIndex = propertyIndex + 1;
        QMetaProperty hasProperty = metaObject()->property(hasPropertyIndex);
        Q_ASSERT_X(hasProperty.isValid() && hasProperty.metaType().id() == QMetaType::Bool,
                   "QProtobufMessage", "The 'oneof' field doesn't have the follow 'has' property.");
        if (!hasProperty.readOnGadget(this).toBool())
            return QVariant(metaProperty.metaType());
    }

    QVariant propertyValue = metaProperty.readOnGadget(this);
    return propertyValue;
}

bool QProtobufMessage::setProperty(
        const QtProtobufPrivate::QProtobufPropertyOrderingInfo &fieldInfo, const QVariant &value)
{
    Q_D(QProtobufMessage);
    const auto mp = d->metaProperty(fieldInfo);
    if (!mp)
        return false;
    return mp->writeOnGadget(this, value);
}

bool QProtobufMessage::setProperty(const QtProtobufPrivate::QProtobufPropertyOrderingInfo &info,
                                   QVariant &&value)
{
    Q_D(QProtobufMessage);
    const auto mp = d->metaProperty(info);
    if (!mp)
        return false;
    return mp->writeOnGadget(this, std::move(value));
}

/*!
    Returns the field numbers that were not known to QtProtobuf during
    deserialization.
    \since 6.7
 */
QList<qint32> QProtobufMessage::unknownFieldNumbers() const
{
    return d_func()->unknownEntries.keys();
}

/*!
    Returns the unknown \a field values sorted as they were received from the
    wire.
    \since 6.7
*/
QList<QByteArray> QProtobufMessage::unknownFieldData(qint32 field) const
{
    return d_func()->unknownEntries.value(field);
}

void QProtobufMessage::detachPrivate()
{
    if (d_ptr->ref.loadAcquire() == 1)
        return;
    QProtobufMessagePrivate *newD = new QProtobufMessagePrivate(*d_ptr);
    if (!d_ptr->ref.deref())
        delete d_ptr;
    d_ptr = newD;
}

QT_END_NAMESPACE

#include "moc_qprotobufmessage.cpp"
