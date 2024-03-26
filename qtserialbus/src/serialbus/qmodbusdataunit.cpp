// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qmodbusdataunit.h"

QT_BEGIN_NAMESPACE

/*!
    \class QModbusDataUnit
    \inmodule QtSerialBus
    \since 5.8

    \brief QModbusDataUnit is a container class representing single bit and
    \c 16 bit word entries in the Modbus register.

    \l QModbusDataUnit can be used for read and write operations. The entries
    are addressed via \l startAddress() and the \l valueCount() number of
    contiguous entries. \l registerType() determines which register is used for
    the operations. Note that some registers are read-only registers.

    The actual \l values() are either single bit or \c 16 bit based.
    \l QModbusDataUnit::DiscreteInputs and \l QModbusDataUnit::Coils
    only accept single bits. Therefore \c 0 is interpreted as \c 0 and anything
    else \c 1.
*/

/*!
    \enum QModbusDataUnit::RegisterType

    This enum describes all supported register types.

    \value Invalid              Set by the default constructor, do not use.
    \value DiscreteInputs       This type of data can be provided by an I/O
                                system.
    \value Coils                This type of data can be alterable by an
                                application program.
    \value InputRegisters       This type of data can be provided by an I/O
                                system.
    \value HoldingRegisters     This type of data can be alterable by an
                                application program.
*/

/*!
    \fn QModbusDataUnit::QModbusDataUnit()

    Constructs an empty, invalid QModbusDataUnit. Start address is set to \c -1
    and the \l registerType is set to \l QModbusDataUnit::Invalid.
*/

/*!
    \fn QModbusDataUnit::QModbusDataUnit(RegisterType type)

    Constructs a unit of data for register \a type. Start address is set to
    \c 0, data range and data values are empty.
*/

/*!
    \fn QModbusDataUnit::QModbusDataUnit(RegisterType type, int address,
                                         quint16 size)

    Constructs a unit of data for register\a type. Start address of the data is
    set to \a address and the size of the unit to \a size.
    The entries of \l values() are initialized with \c 0.
*/

/*!
    \fn QModbusDataUnit::QModbusDataUnit(RegisterType type, int address,
                                         const QList<quint16> &data)

    Constructs a unit of data for register\a type. Start address of the data is
    set to \a address and the unit's values to \a data.
    The value count is implied by the \a data size.
*/

/*!
    \fn void QModbusDataUnit::setRegisterType(QModbusDataUnit::RegisterType type)

    Sets the register \a type.

    \sa registerType(), QModbusDataUnit::RegisterType
*/

/*!
    \fn QModbusDataUnit::RegisterType QModbusDataUnit::registerType() const

    Returns the type of the register.

    \sa setRegisterType(), QModbusDataUnit::RegisterType
*/

/*!
    \fn void QModbusDataUnit::setStartAddress(int address)

    Sets the start \a address of the data unit.

    \sa startAddress()
*/

/*!
    \fn int QModbusDataUnit::startAddress() const

    Returns the start address of data unit in the register.

    \sa setStartAddress()
*/

/*!
    \fn void QModbusDataUnit::setValues(const QList<quint16> &values)

    Sets the \a values of the data unit. \l QModbusDataUnit::DiscreteInputs
    and \l QModbusDataUnit::Coils tables only accept single bit value, so \c 0
    is interpreted as \c 0 and anything else as \c 1.

    \sa values()
*/

/*!
    \fn QList<quint16> QModbusDataUnit::values() const

    Returns the data in the data unit. \l QModbusDataUnit::DiscreteInputs
    and \l QModbusDataUnit::Coils tables only accept single bit value, so \c 0
    is interpreted as \c 0 and anything else as \c 1.

    \sa setValues()
*/

/*!
    \fn qsizetype QModbusDataUnit::valueCount() const

    Returns the size of the requested register's data block or the size of data
    read from the device.

    This function may not always return a count that equals \l values() size.
    Since this class is used to request data from the remote data register, the
    \l valueCount() can be used to indicate the size of the requested register's
    data block. Once the request has been processed, the \l valueCount() is
    equal to the size of \l values().

    \sa setValueCount()
*/

/*!
    \fn void QModbusDataUnit::setValueCount(qsizetype newCount)

    Sets the size of the requested register's data block to \a newCount.

    This may be different from \l values() size as this function is used
    to indicated the size of a data request. Only once the data request
    has been processed \l valueCount() is equal to the size of \l values().
*/

/*!
    \fn void QModbusDataUnit::setValue(qsizetype index, quint16 value)

    Sets the register at position \a index to \a value.
*/

/*!
    \fn quint16 QModbusDataUnit::value(qsizetype index) const

    Return the value at position \a index.
*/

/*!
    \fn bool QModbusDataUnit::isValid() const

    Returns \c true if the \c QModbusDataUnit is valid; otherwise \c false.
    A \c QModbusDataUnit is considered valid if the \l registerType() is not
    \l QModbusDataUnit::Invalid and the \l startAddress() is greater than or
    equal to \c 0.
*/

/*!
    \typedef QModbusDataUnitMap
    \relates QModbusDataUnit
    \since 5.8

    Synonym for QMap<QModbusDataUnit::RegisterType, QModbusDataUnit>.
*/

QT_END_NAMESPACE
