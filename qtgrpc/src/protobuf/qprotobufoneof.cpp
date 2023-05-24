// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qprotobufoneof.h"

QT_BEGIN_NAMESPACE

namespace QtProtobufPrivate {
/*!
    \internal
    \fn template<typename T> void QProtobufOneof::setValue(const T &value, int fieldNumber)

    Stores the \a value into QProtobufOneof with corresponding \a
    fieldNumber.
*/

/*!
    \internal
    \fn template<typename T, IsNonMessageProtobufType<T> = 0> T QProtobufOneof::value() const

    Returns the value of non-message protobuf type stored in the
    QProtobufOneof object.
*/

/*!
    \internal
    \fn template<typename T, IsProtobufMessageType<T> = 0> T &QProtobufOneof::value() const

    Returns the reference to a protobuf message that is stored inside the
    QProtobufOneof object.
*/

/*!
    \internal
    \fn template<typename T> bool QProtobufOneof::isEqual(const T &value, int fieldNumber) const

    Compares the data stored in QProtobufOneof with the value/fieldNumber
    pair.
*/

class QProtobufOneofPrivate final
{
public:
    QVariant value;
    int fieldNumber = QtProtobuf::InvalidFieldNumber;
};

/*!
    \internal
    \brief The default constructor, constructs uninitialized QProtobufOneof.
*/
QProtobufOneof::QProtobufOneof() : d_ptr(new QProtobufOneofPrivate) { }

/*!
    \internal
    Constructs a copy of other. This operation takes constant time, because
    QProtobufOneof is implicitly shared. If a shared instance is modified, it
    will be copied (copy-on-write).
*/
QProtobufOneof::QProtobufOneof(const QProtobufOneof &other)
    : d_ptr(new QProtobufOneofPrivate(*other.d_ptr))
{
}

/*!
    \internal
    Assigns other to this QProtobufOneof and returns a reference to this
    QProtobufOneof.
*/

QProtobufOneof &QProtobufOneof::operator=(const QProtobufOneof &other)
{
    if (this != &other)
        *d_ptr = *other.d_ptr;
    return *this;
}

/*!
    \internal
    \fn QProtobufOneof::QProtobufOneof(QProtobufOneof &&other) noexcept
    Move-constructs a QProtobufOneof instance, making it point at the same
    object that other was pointing to.
*/

/*!
    \internal
    \fn QProtobufOneof &QProtobufOneof::operator=(QProtobufOneof &&other) noexcept
    Move-assigns other to this QProtobufOneof instance.
*/

/*!
    \internal
    Checks if values stored in oneof is equal \a other.
*/
bool QProtobufOneof::isEqual(const QProtobufOneof &other) const
{
    if (d_ptr == other.d_ptr)
        return true;
    return d_func()->fieldNumber == other.d_func()->fieldNumber
            && d_func()->value == other.d_func()->value;
}

/*!
    \internal
    Destroys the oneof.
*/
QProtobufOneof::~QProtobufOneof()
{
    delete d_ptr;
}

QVariant &QProtobufOneof::rawValue() const
{
    return d_ptr->value;
}

/*
   Setting of the QProtobufOneof data makes sense only using
   value/fieldNumber pair. Instead of non-constant dereferencing of the
   QSharedDataPointer we simply rewrite it, that keep its copies untouched but
   creates a new data with the provided values. This avoids redundant data
   copying when updating the data.
*/
void QProtobufOneof::setValue(const QVariant &value, int fieldNumber)
{
    d_ptr->value = value;
    d_ptr->fieldNumber = fieldNumber;
}

/*!
    \internal
    Returns \c true if QProtobufOneof holds the field with \a fieldNumber.
*/
bool QProtobufOneof::holdsField(int fieldNumber) const
{
    Q_D(const QProtobufOneof);
    return d->fieldNumber == fieldNumber && fieldNumber != QtProtobuf::InvalidFieldNumber
            && !d->value.isNull();
}

/*!
    \internal
    Returns the number of a protobuf field that is stored in the object
*/
int QProtobufOneof::fieldNumber() const
{
    Q_D(const QProtobufOneof);
    return d->fieldNumber;
}

} // namespace QtProtobufPrivate

QT_END_NAMESPACE
