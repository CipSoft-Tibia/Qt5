// Copyright (C) 2020 The Qt Company Ltd.
// Copyright (C) 2019 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qbitarray.h"
#include <qalgorithms.h>
#include <qdatastream.h>
#include <qdebug.h>
#include <qendian.h>

#include <limits>

#include <string.h>

QT_BEGIN_NAMESPACE

/*!
    \class QBitArray
    \inmodule QtCore
    \brief The QBitArray class provides an array of bits.

    \ingroup tools
    \ingroup shared
    \reentrant

    A QBitArray is an array that gives access to individual bits and
    provides operators (\l{operator&()}{AND}, \l{operator|()}{OR},
    \l{operator^()}{XOR}, and \l{operator~()}{NOT}) that work on
    entire arrays of bits. It uses \l{implicit sharing} (copy-on-write)
    to reduce memory usage and to avoid the needless copying of data.

    The following code constructs a QBitArray containing 200 bits
    initialized to false (0):

    \snippet code/src_corelib_tools_qbitarray.cpp 0

    To initialize the bits to true, either pass \c true as second
    argument to the constructor, or call fill() later on.

    QBitArray uses 0-based indexes, just like C++ arrays. To access
    the bit at a particular index position, you can use operator[]().
    On non-const bit arrays, operator[]() returns a reference to a
    bit that can be used on the left side of an assignment. For
    example:

    \snippet code/src_corelib_tools_qbitarray.cpp 1

    For technical reasons, it is more efficient to use testBit() and
    setBit() to access bits in the array than operator[](). For
    example:

    \snippet code/src_corelib_tools_qbitarray.cpp 2

    QBitArray supports \c{&} (\l{operator&()}{AND}), \c{|}
    (\l{operator|()}{OR}), \c{^} (\l{operator^()}{XOR}),
    \c{~} (\l{operator~()}{NOT}), as well as
    \c{&=}, \c{|=}, and \c{^=}. These operators work in the same way
    as the built-in C++ bitwise operators of the same name. For
    example:

    \snippet code/src_corelib_tools_qbitarray.cpp 3

    For historical reasons, QBitArray distinguishes between a null
    bit array and an empty bit array. A \e null bit array is a bit
    array that is initialized using QBitArray's default constructor.
    An \e empty bit array is any bit array with size 0. A null bit
    array is always empty, but an empty bit array isn't necessarily
    null:

    \snippet code/src_corelib_tools_qbitarray.cpp 4

    All functions except isNull() treat null bit arrays the same as
    empty bit arrays; for example, QBitArray() compares equal to
    QBitArray(0). We recommend that you always use isEmpty() and
    avoid isNull().

    \sa QByteArray, QList
*/

/*!
    \fn QBitArray::QBitArray(QBitArray &&other)

    Move-constructs a QBitArray instance, making it point at the same
    object that \a other was pointing to.

    \since 5.2
*/

/*! \fn QBitArray::QBitArray()

    Constructs an empty bit array.

    \sa isEmpty()
*/

/*
 * QBitArray construction note:
 *
 * We overallocate the byte array by 1 byte. The first user bit is at
 * d.data()[1]. On the extra first byte, we store the difference between the
 * number of bits in the byte array (including this byte) and the number of
 * bits in the bit array. Therefore, for a non-empty QBitArray, it's always a
 * number between 8 and 15. For the empty one, d is the an empty QByteArray and
 * *d.constData() is the QByteArray's terminating NUL (0) byte.
 *
 * This allows for fast calculation of the bit array size:
 *    inline qsizetype size() const { return (d.size() << 3) - *d.constData(); }
 */

static constexpr qsizetype storage_size(qsizetype size)
{
    // avoid overflow when adding 7, by doing the arithmetic in unsigned space:
    return qsizetype((size_t(size) + 7) / 8);
}

static constexpr qsizetype allocation_size(qsizetype size)
{
    return size <= 0 ? 0 : storage_size(size) + 1;
}

static void adjust_head_and_tail(char *data, qsizetype storageSize, qsizetype logicalSize)
{
    quint8 *c = reinterpret_cast<quint8 *>(data);
    // store the difference between storage and logical size in d[0]:
    *c = quint8(size_t(storageSize) * 8 - logicalSize);
    // reset unallocated bits to 0:
    if (logicalSize & 7)
        *(c + 1 + logicalSize / 8) &= (1 << (logicalSize & 7)) - 1;
}

/*!
    Constructs a bit array containing \a size bits. The bits are
    initialized with \a value, which defaults to false (0).
*/
QBitArray::QBitArray(qsizetype size, bool value)
    : d(allocation_size(size), value ? 0xFF : 0x00)
{
    Q_ASSERT_X(size >= 0, "QBitArray::QBitArray", "Size must be greater than or equal to 0.");
    if (size <= 0)
        return;

    adjust_head_and_tail(d.data(), d.size(), size);
}

/*! \fn qsizetype QBitArray::size() const

    Returns the number of bits stored in the bit array.

    \sa resize()
*/

/*! \fn qsizetype QBitArray::count() const

    Same as size().
*/

/*!
    If \a on is true, this function returns the number of
    1-bits stored in the bit array; otherwise the number
    of 0-bits is returned.
*/
qsizetype QBitArray::count(bool on) const
{
    qsizetype numBits = 0;
    const quint8 *bits = reinterpret_cast<const quint8 *>(d.data()) + 1;

    // the loops below will try to read from *end
    // it's the QByteArray implicit NUL, so it will not change the bit count
    const quint8 *const end = reinterpret_cast<const quint8 *>(d.end());

    while (bits + 7 <= end) {
        quint64 v = qFromUnaligned<quint64>(bits);
        bits += 8;
        numBits += qsizetype(qPopulationCount(v));
    }
    if (bits + 3 <= end) {
        quint32 v = qFromUnaligned<quint32>(bits);
        bits += 4;
        numBits += qsizetype(qPopulationCount(v));
    }
    if (bits + 1 < end) {
        quint16 v = qFromUnaligned<quint16>(bits);
        bits += 2;
        numBits += qsizetype(qPopulationCount(v));
    }
    if (bits < end)
        numBits += qsizetype(qPopulationCount(bits[0]));

    return on ? numBits : size() - numBits;
}

/*!
    Resizes the bit array to \a size bits.

    If \a size is greater than the current size, the bit array is
    extended to make it \a size bits with the extra bits added to the
    end. The new bits are initialized to false (0).

    If \a size is less than the current size, bits are removed from
    the end.

    \sa size()
*/
void QBitArray::resize(qsizetype size)
{
    Q_ASSERT_X(size >= 0, "QBitArray::resize", "Size must be greater than or equal to 0.");
    if (size <= 0) {
        d.resize(0);
    } else {
        d.resize(allocation_size(size), 0x00);
        adjust_head_and_tail(d.data(), d.size(), size);
    }
}

/*! \fn bool QBitArray::isEmpty() const

    Returns \c true if this bit array has size 0; otherwise returns
    false.

    \sa size()
*/

/*! \fn bool QBitArray::isNull() const

    Returns \c true if this bit array is null; otherwise returns \c false.

    Example:
    \snippet code/src_corelib_tools_qbitarray.cpp 5

    Qt makes a distinction between null bit arrays and empty bit
    arrays for historical reasons. For most applications, what
    matters is whether or not a bit array contains any data,
    and this can be determined using isEmpty().

    \sa isEmpty()
*/

/*! \fn bool QBitArray::fill(bool value, qsizetype size = -1)

    Sets every bit in the bit array to \a value, returning true if successful;
    otherwise returns \c false. If \a size is different from -1 (the default),
    the bit array is resized to \a size beforehand.

    Example:
    \snippet code/src_corelib_tools_qbitarray.cpp 6

    \sa resize()
*/

/*!
    \overload

    Sets bits at index positions \a begin up to (but not including) \a end
    to \a value.

    \a begin must be a valid index position in the bit array
    (0 <= \a begin < size()).

    \a end must be either a valid index position or equal to size(), in
    which case the fill operation runs until the end of the array
    (0 <= \a end <= size()).

    Example:
    \snippet code/src_corelib_tools_qbitarray.cpp 15
*/

void QBitArray::fill(bool value, qsizetype begin, qsizetype end)
{
    while (begin < end && begin & 0x7)
        setBit(begin++, value);
    qsizetype len = end - begin;
    if (len <= 0)
        return;
    qsizetype s = len & ~qsizetype(0x7);
    uchar *c = reinterpret_cast<uchar *>(d.data());
    memset(c + (begin >> 3) + 1, value ? 0xff : 0, s >> 3);
    begin += s;
    while (begin < end)
        setBit(begin++, value);
}

/*!
    \fn const char *QBitArray::bits() const
    \since 5.11

    Returns a pointer to a dense bit array for this QBitArray. Bits are counted
    upwards from the least significant bit in each byte. The number of bits
    relevant in the last byte is given by \c{size() % 8}.

    \sa fromBits(), size()
 */

/*!
    \since 5.11

    Creates a QBitArray with the dense bit array located at \a data, with \a
    size bits. The byte array at \a data must be at least \a size / 8 (rounded up)
    bytes long.

    If \a size is not a multiple of 8, this function will include the lowest
    \a size % 8 bits from the last byte in \a data.

    \sa bits()
 */
QBitArray QBitArray::fromBits(const char *data, qsizetype size)
{
    Q_ASSERT_X(size >= 0, "QBitArray::fromBits", "Size must be greater than or equal to 0.");
    QBitArray result;
    if (size <= 0)
        return result;

    auto &d = result.d;
    d.resize(allocation_size(size));
    memcpy(d.data() + 1, data, d.size() - 1);
    adjust_head_and_tail(d.data(), d.size(), size);
    return result;
}

/*!
    \since 6.0

    Returns the array of bit converted to an int. The conversion is based on \a endianness.
    Converts up to the first 32 bits of the array to \c quint32 and returns it,
    obeying \a endianness. If \a ok is not a null pointer, and the array has more
    than 32 bits, \a ok is set to false and this function returns zero; otherwise,
    it's set to true.
*/
quint32 QBitArray::toUInt32(QSysInfo::Endian endianness, bool *ok) const noexcept
{
    const qsizetype _size = size();
    if (_size > 32) {
        if (ok)
            *ok = false;
        return 0;
    }

    if (ok)
        *ok = true;

    quint32 factor = 1;
    quint32 total = 0;
    for (qsizetype i = 0; i < _size; ++i, factor *= 2) {
        const auto index = endianness == QSysInfo::Endian::LittleEndian ? i : (_size - i - 1);
        if (testBit(index))
            total += factor;
    }

    return total;
}

/*! \fn bool QBitArray::isDetached() const

    \internal
*/

/*! \fn void QBitArray::detach()

    \internal
*/

/*! \fn void QBitArray::clear()

    Clears the contents of the bit array and makes it empty.

    \sa resize(), isEmpty()
*/

/*! \fn void QBitArray::truncate(qsizetype pos)

    Truncates the bit array at index position \a pos.

    If \a pos is beyond the end of the array, nothing happens.

    \sa resize()
*/

/*! \fn bool QBitArray::toggleBit(qsizetype i)

    Inverts the value of the bit at index position \a i, returning the
    previous value of that bit as either true (if it was set) or false (if
    it was unset).

    If the previous value was 0, the new value will be 1. If the
    previous value was 1, the new value will be 0.

    \a i must be a valid index position in the bit array (i.e., 0 <=
    \a i < size()).

    \sa setBit(), clearBit()
*/

/*! \fn bool QBitArray::testBit(qsizetype i) const

    Returns \c true if the bit at index position \a i is 1; otherwise
    returns \c false.

    \a i must be a valid index position in the bit array (i.e., 0 <=
    \a i < size()).

    \sa setBit(), clearBit()
*/

/*! \fn bool QBitArray::setBit(qsizetype i)

    Sets the bit at index position \a i to 1.

    \a i must be a valid index position in the bit array (i.e., 0 <=
    \a i < size()).

    \sa clearBit(), toggleBit()
*/

/*! \fn void QBitArray::setBit(qsizetype i, bool value)

    \overload

    Sets the bit at index position \a i to \a value.
*/

/*! \fn void QBitArray::clearBit(qsizetype i)

    Sets the bit at index position \a i to 0.

    \a i must be a valid index position in the bit array (i.e., 0 <=
    \a i < size()).

    \sa setBit(), toggleBit()
*/

/*! \fn bool QBitArray::at(qsizetype i) const

    Returns the value of the bit at index position \a i.

    \a i must be a valid index position in the bit array (i.e., 0 <=
    \a i < size()).

    \sa operator[]()
*/

/*! \fn QBitRef QBitArray::operator[](qsizetype i)

    Returns the bit at index position \a i as a modifiable reference.

    \a i must be a valid index position in the bit array (i.e., 0 <=
    \a i < size()).

    Example:
    \snippet code/src_corelib_tools_qbitarray.cpp 7

    The return value is of type QBitRef, a helper class for QBitArray.
    When you get an object of type QBitRef, you can assign to
    it, and the assignment will apply to the bit in the QBitArray
    from which you got the reference.

    The functions testBit(), setBit(), and clearBit() are slightly
    faster.

    \sa at(), testBit(), setBit(), clearBit()
*/

/*! \fn bool QBitArray::operator[](qsizetype i) const

    \overload
*/

/*! \fn QBitArray::QBitArray(const QBitArray &other) noexcept

    Constructs a copy of \a other.

    This operation takes \l{constant time}, because QBitArray is
    \l{implicitly shared}. This makes returning a QBitArray from a
    function very fast. If a shared instance is modified, it will be
    copied (copy-on-write), and that takes \l{linear time}.

    \sa operator=()
*/

/*! \fn QBitArray &QBitArray::operator=(const QBitArray &other) noexcept

    Assigns \a other to this bit array and returns a reference to
    this bit array.
*/

/*! \fn QBitArray &QBitArray::operator=(QBitArray &&other)
    \since 5.2

    Moves \a other to this bit array and returns a reference to
    this bit array.
*/

/*! \fn void QBitArray::swap(QBitArray &other)
    \since 4.8

    Swaps bit array \a other with this bit array. This operation is very
    fast and never fails.
*/

/*! \fn bool QBitArray::operator==(const QBitArray &other) const

    Returns \c true if \a other is equal to this bit array; otherwise
    returns \c false.

    \sa operator!=()
*/

/*! \fn bool QBitArray::operator!=(const QBitArray &other) const

    Returns \c true if \a other is not equal to this bit array;
    otherwise returns \c false.

    \sa operator==()
*/

// Returns a new QBitArray that has the same size as the bigger of \a a1 and
// \a a2, but whose contents are uninitialized.
static QBitArray sizedForOverwrite(const QBitArray &a1, const QBitArray &a2)
{
    QBitArray result;
    const QByteArrayData &d1 = a1.data_ptr();
    const QByteArrayData &d2 = a2.data_ptr();
    qsizetype n1 = d1.size;
    qsizetype n2 = d2.size;
    qsizetype n = qMax(n1, n2);

    QByteArrayData bytes(n, n);

    // initialize the count of bits in the last byte (see construction note)
    // and the QByteArray null termination (some of our algorithms read it)
    if (n1 > n2) {
        *bytes.ptr = *d1.ptr;
        bytes.ptr[n1] = 0;
    } else if (n2 > n1) {
        *bytes.ptr = *d2.ptr;
        bytes.ptr[n2] = 0;
    } else if (n1) {    // n1 == n2
        *bytes.ptr = qMin(*d1.ptr, *d2.ptr);
        bytes.ptr[n1] = 0;
    }

    result.data_ptr() = std::move(bytes);
    return result;
}

template <typename BitwiseOp> static Q_NEVER_INLINE
QBitArray &performBitwiseOperationHelper(QBitArray &out, const QBitArray &a1,
                                         const QBitArray &a2, BitwiseOp op)
{
    const QByteArrayData &d1 = a1.data_ptr();
    const QByteArrayData &d2 = a2.data_ptr();

    // Sizes in bytes (including the initial bit difference counter)
    qsizetype n1 = d1.size;
    qsizetype n2 = d2.size;
    Q_ASSERT(out.data_ptr().size == qMax(n1, n2));
    Q_ASSERT(out.data_ptr().size == 0 || !out.data_ptr().needsDetach());

    // Bypass QByteArray's emptiness verification; we won't dereference
    // these pointers if their size is zero.
    auto dst = reinterpret_cast<uchar *>(out.data_ptr().data());
    auto p1 = reinterpret_cast<const uchar *>(d1.data());
    auto p2 = reinterpret_cast<const uchar *>(d2.data());

    // Main: perform the operation in the range where both arrays have data
    if (n1 < n2) {
        std::swap(n1, n2);
        std::swap(p1, p2);
    }
    for (qsizetype i = 1; i < n2; ++i)
        dst[i] = op(p1[i], p2[i]);

    // Tail: operate as if both arrays had the same data by padding zeroes to
    // the end of the shorter of the two (for std::bit_or and std::bit_xor, this is
    // a memmove; for std::bit_and, it's memset to 0).
    for (qsizetype i = qMax(n2, qsizetype(1)); i < n1; ++i)
        dst[i] = op(p1[i], uchar(0));

    return out;
}

template <typename BitwiseOp> static Q_NEVER_INLINE
QBitArray &performBitwiseOperationInCopy(QBitArray &self, const QBitArray &other, BitwiseOp op)
{
    QBitArray tmp(std::move(self));
    self = sizedForOverwrite(tmp, other);
    return performBitwiseOperationHelper(self, tmp, other, op);
}

template <typename BitwiseOp> static Q_NEVER_INLINE
QBitArray &performBitwiseOperationInPlace(QBitArray &self, const QBitArray &other, BitwiseOp op)
{
    if (self.size() < other.size())
        self.resize(other.size());
    return performBitwiseOperationHelper(self, self, other, op);
}

template <typename BitwiseOp> static
QBitArray &performBitwiseOperation(QBitArray &self, const QBitArray &other, BitwiseOp op)
{
    if (self.data_ptr().needsDetach())
        return performBitwiseOperationInCopy(self, other, op);
    return performBitwiseOperationInPlace(self, other, op);
}

// SCARY helper
enum { InCopy, InPlace };
static auto prepareForBitwiseOperation(QBitArray &self, QBitArray &other)
{
    QByteArrayData &d1 = self.data_ptr();
    QByteArrayData &d2 = other.data_ptr();
    bool detached1 = !d1.needsDetach();
    bool detached2 = !d2.needsDetach();
    if (!detached1 && !detached2)
        return InCopy;

    // at least one of the two is detached, we'll reuse its buffer
    bool swap = false;
    if (detached1 && detached2) {
        // both are detached, so choose the larger of the two
        swap = d1.allocatedCapacity() < d2.allocatedCapacity();
    } else if (detached2) {
        // we can re-use other's buffer but not self's, so swap the two
        swap = true;
    }
    if (swap)
        self.swap(other);
    return InPlace;
}

template <typename BitwiseOp> static
QBitArray &performBitwiseOperation(QBitArray &self, QBitArray &other, BitwiseOp op)
{
    auto choice = prepareForBitwiseOperation(self, other);
    if (choice == InCopy)
        return performBitwiseOperationInCopy(self, other, std::move(op));
    return performBitwiseOperationInPlace(self, other, std::move(op));
}

/*!
    \fn QBitArray &QBitArray::operator&=(const QBitArray &other)
    \fn QBitArray &QBitArray::operator&=(QBitArray &&other)

    Performs the AND operation between all bits in this bit array and
    \a other. Assigns the result to this bit array, and returns a
    reference to it.

    The result has the length of the longest of the two bit arrays,
    with any missing bits (if one array is shorter than the other)
    taken to be 0.

    Example:
    \snippet code/src_corelib_tools_qbitarray.cpp 8

    \sa operator&(), operator|=(), operator^=(), operator~()
*/

QBitArray &QBitArray::operator&=(QBitArray &&other)
{
    return performBitwiseOperation(*this, other, std::bit_and<uchar>());
}

QBitArray &QBitArray::operator&=(const QBitArray &other)
{
    return performBitwiseOperation(*this, other, std::bit_and<uchar>());
}

/*!
    \fn QBitArray &QBitArray::operator|=(const QBitArray &other)
    \fn QBitArray &QBitArray::operator|=(QBitArray &&other)

    Performs the OR operation between all bits in this bit array and
    \a other. Assigns the result to this bit array, and returns a
    reference to it.

    The result has the length of the longest of the two bit arrays,
    with any missing bits (if one array is shorter than the other)
    taken to be 0.

    Example:
    \snippet code/src_corelib_tools_qbitarray.cpp 9

    \sa operator|(), operator&=(), operator^=(), operator~()
*/

QBitArray &QBitArray::operator|=(QBitArray &&other)
{
    return performBitwiseOperation(*this, other, std::bit_or<uchar>());
}

QBitArray &QBitArray::operator|=(const QBitArray &other)
{
    return performBitwiseOperation(*this, other, std::bit_or<uchar>());
}

/*!
    \fn QBitArray &QBitArray::operator^=(const QBitArray &other)
    \fn QBitArray &QBitArray::operator^=(QBitArray &&other)

    Performs the XOR operation between all bits in this bit array and
    \a other. Assigns the result to this bit array, and returns a
    reference to it.

    The result has the length of the longest of the two bit arrays,
    with any missing bits (if one array is shorter than the other)
    taken to be 0.

    Example:
    \snippet code/src_corelib_tools_qbitarray.cpp 10

    \sa operator^(), operator&=(), operator|=(), operator~()
*/

QBitArray &QBitArray::operator^=(QBitArray &&other)
{
    return performBitwiseOperation(*this, other, std::bit_xor<uchar>());
}

QBitArray &QBitArray::operator^=(const QBitArray &other)
{
    return performBitwiseOperation(*this, other, std::bit_xor<uchar>());
}

/*!
    \fn QBitArray QBitArray::operator~(QBitArray a)
    Returns a bit array that contains the inverted bits of the bit
    array \a a.

    Example:
    \snippet code/src_corelib_tools_qbitarray.cpp 11

    \sa operator&(), operator|(), operator^()
*/

Q_NEVER_INLINE QBitArray QBitArray::inverted_inplace() &&
{
    qsizetype n = d.size();
    uchar *dst = reinterpret_cast<uchar *>(data_ptr().data());
    const uchar *src = dst;
    QBitArray result([&] {
        if (d.isDetached() || n == 0)
            return std::move(d.data_ptr());     // invert in-place

        QByteArrayData tmp(n, n);
        dst = reinterpret_cast<uchar *>(tmp.data());
        return tmp;
    }());

    uchar bitdiff = 8;
    if (n)
        bitdiff = dst[0] = src[0];      // copy the count of bits in the last byte

    for (qsizetype i = 1; i < n; ++i)
        dst[i] = ~src[i];

    if (int tailCount = 16 - bitdiff; tailCount != 8) {
        // zero the bits beyond our size in the last byte
        Q_ASSERT(n > 1);
        uchar tailMask = (1U << tailCount) - 1;
        dst[n - 1] &= tailMask;
    }

    return result;
}

/*!
    \fn QBitArray QBitArray::operator&(const QBitArray &a1, const QBitArray &a2)
    \fn QBitArray QBitArray::operator&(QBitArray &&a1, const QBitArray &a2)
    \fn QBitArray QBitArray::operator&(const QBitArray &a1, QBitArray &&a2)
    \fn QBitArray QBitArray::operator&(QBitArray &&a1, QBitArray &&a2)

    Returns a bit array that is the AND of the bit arrays \a a1 and \a
    a2.

    The result has the length of the longest of the two bit arrays,
    with any missing bits (if one array is shorter than the other)
    taken to be 0.

    Example:
    \snippet code/src_corelib_tools_qbitarray.cpp 12

    \sa {QBitArray::}{operator&=()}, {QBitArray::}{operator|()}, {QBitArray::}{operator^()}
*/

QBitArray operator&(const QBitArray &a1, const QBitArray &a2)
{
    QBitArray tmp = sizedForOverwrite(a1, a2);
    performBitwiseOperationHelper(tmp, a1, a2, std::bit_and<uchar>());
    return tmp;
}

/*!
    \fn QBitArray QBitArray::operator|(const QBitArray &a1, const QBitArray &a2)
    \fn QBitArray QBitArray::operator|(QBitArray &&a1, const QBitArray &a2)
    \fn QBitArray QBitArray::operator|(const QBitArray &a1, QBitArray &&a2)
    \fn QBitArray QBitArray::operator|(QBitArray &&a1, QBitArray &&a2)

    Returns a bit array that is the OR of the bit arrays \a a1 and \a
    a2.

    The result has the length of the longest of the two bit arrays,
    with any missing bits (if one array is shorter than the other)
    taken to be 0.

    Example:
    \snippet code/src_corelib_tools_qbitarray.cpp 13

    \sa QBitArray::operator|=(), operator&(), operator^()
*/

QBitArray operator|(const QBitArray &a1, const QBitArray &a2)
{
    QBitArray tmp = sizedForOverwrite(a1, a2);
    performBitwiseOperationHelper(tmp, a1, a2, std::bit_or<uchar>());
    return tmp;
}

/*!
    \fn QBitArray QBitArray::operator^(const QBitArray &a1, const QBitArray &a2)
    \fn QBitArray QBitArray::operator^(QBitArray &&a1, const QBitArray &a2)
    \fn QBitArray QBitArray::operator^(const QBitArray &a1, QBitArray &&a2)
    \fn QBitArray QBitArray::operator^(QBitArray &&a1, QBitArray &&a2)

    Returns a bit array that is the XOR of the bit arrays \a a1 and \a
    a2.

    The result has the length of the longest of the two bit arrays,
    with any missing bits (if one array is shorter than the other)
    taken to be 0.

    Example:
    \snippet code/src_corelib_tools_qbitarray.cpp 14

    \sa {QBitArray}{operator^=()}, {QBitArray}{operator&()}, {QBitArray}{operator|()}
*/

QBitArray operator^(const QBitArray &a1, const QBitArray &a2)
{
    QBitArray tmp = sizedForOverwrite(a1, a2);
    performBitwiseOperationHelper(tmp, a1, a2, std::bit_xor<uchar>());
    return tmp;
}

/*!
    \class QBitRef
    \inmodule QtCore
    \reentrant
    \brief The QBitRef class is an internal class, used with QBitArray.

    \internal

    The QBitRef is required by the indexing [] operator on bit arrays.
    It is not for use in any other context.
*/

/*! \fn QBitRef::QBitRef (QBitArray& a, qsizetype i)

    Constructs a reference to element \a i in the QBitArray \a a.
    This is what QBitArray::operator[] constructs its return value
    with.
*/

/*! \fn QBitRef::operator bool() const

    Returns the value referenced by the QBitRef.
*/

/*! \fn bool QBitRef::operator!() const

    \internal
*/

/*! \fn QBitRef& QBitRef::operator= (const QBitRef& v)

    Sets the value referenced by the QBitRef to that referenced by
    QBitRef \a v.
*/

/*! \fn QBitRef& QBitRef::operator= (bool v)
    \overload

    Sets the value referenced by the QBitRef to \a v.
*/

/*****************************************************************************
  QBitArray stream functions
 *****************************************************************************/

#ifndef QT_NO_DATASTREAM
/*!
    \relates QBitArray

    Writes bit array \a ba to stream \a out.

    \sa {Serializing Qt Data Types}{Format of the QDataStream operators}
*/

QDataStream &operator<<(QDataStream &out, const QBitArray &ba)
{
    const qsizetype len = ba.size();
    if (out.version() < QDataStream::Qt_6_0) {
        if (Q_UNLIKELY(len > qsizetype{(std::numeric_limits<qint32>::max)()})) {
            out.setStatus(QDataStream::Status::SizeLimitExceeded);
            return out;
        }
        out << quint32(len);
    } else {
        out << quint64(len);
    }
    if (len > 0)
        out.writeRawData(ba.d.data() + 1, ba.d.size() - 1);
    return out;
}

/*!
    \relates QBitArray

    Reads a bit array into \a ba from stream \a in.

    \sa {Serializing Qt Data Types}{Format of the QDataStream operators}
*/

QDataStream &operator>>(QDataStream &in, QBitArray &ba)
{
    ba.clear();
    qsizetype len;
    if (in.version() < QDataStream::Qt_6_0) {
        quint32 tmp;
        in >> tmp;
        if (Q_UNLIKELY(tmp > quint32((std::numeric_limits<qint32>::max)()))) {
            in.setStatus(QDataStream::ReadCorruptData);
            return in;
        }
        len = tmp;
    } else {
        quint64 tmp;
        in >> tmp;
        if (Q_UNLIKELY(tmp > quint64((std::numeric_limits<qsizetype>::max)()))) {
            in.setStatus(QDataStream::Status::SizeLimitExceeded);
            return in;
        }
        len = tmp;
    }
    if (len == 0) {
        ba.clear();
        return in;
    }

    const qsizetype Step = 8 * 1024 * 1024;
    const qsizetype totalBytes = storage_size(len);
    qsizetype allocated = 0;

    while (allocated < totalBytes) {
        qsizetype blockSize = qMin(Step, totalBytes - allocated);
        ba.d.resize(allocated + blockSize + 1);
        if (in.readRawData(ba.d.data() + 1 + allocated, blockSize) != blockSize) {
            ba.clear();
            in.setStatus(QDataStream::ReadPastEnd);
            return in;
        }
        allocated += blockSize;
    }

    const auto fromStream = ba.d.back();
    adjust_head_and_tail(ba.d.data(), ba.d.size(), len);
    if (ba.d.back() != fromStream) {
        ba.clear();
        in.setStatus(QDataStream::ReadCorruptData);
        return in;
    }
    return in;
}
#endif // QT_NO_DATASTREAM

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QBitArray &array)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "QBitArray(";
    for (qsizetype i = 0; i < array.size();) {
        if (array.testBit(i))
            dbg << '1';
        else
            dbg << '0';
        i += 1;
        if (!(i % 4) && (i < array.size()))
            dbg << ' ';
    }
    dbg << ')';
    return dbg;
}
#endif

/*!
    \fn DataPtr &QBitArray::data_ptr()
    \internal
*/

/*!
    \typedef QBitArray::DataPtr
    \internal
*/

QT_END_NAMESPACE
