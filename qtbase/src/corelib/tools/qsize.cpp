// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsize.h"
#include "qdatastream.h"

#include <private/qdebug_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QSize
    \inmodule QtCore
    \ingroup painting

    \brief The QSize class defines the size of a two-dimensional
    object using integer point precision.

    A size is specified by a width() and a height().  It can be set in
    the constructor and changed using the setWidth(), setHeight(), or
    scale() functions, or using arithmetic operators. A size can also
    be manipulated directly by retrieving references to the width and
    height using the rwidth() and rheight() functions. Finally, the
    width and height can be swapped using the transpose() function.

    The isValid() function determines if a size is valid (a valid size
    has both width and height greater than or equal to zero). The isEmpty()
    function returns \c true if either of the width and height is less
    than, or equal to, zero, while the isNull() function returns \c true
    only if both the width and the height is zero.

    Use the expandedTo() function to retrieve a size which holds the
    maximum height and width of \e this size and a given
    size. Similarly, the boundedTo() function returns a size which
    holds the minimum height and width of \e this size and a given
    size.

    QSize objects can be streamed as well as compared.

    \sa QSizeF, QPoint, QRect
*/


/*****************************************************************************
  QSize member functions
 *****************************************************************************/

/*!
    \fn QSize::QSize()

    Constructs a size with an invalid width and height (i.e., isValid()
    returns \c false).

    \sa isValid()
*/

/*!
    \fn QSize::QSize(int width, int height)

    Constructs a size with the given \a width and \a height.

    \sa setWidth(), setHeight()
*/

/*!
    \fn bool QSize::isNull() const

    Returns \c true if both the width and height is 0; otherwise returns
    false.

    \sa isValid(), isEmpty()
*/

/*!
    \fn bool QSize::isEmpty() const

    Returns \c true if either of the width and height is less than or
    equal to 0; otherwise returns \c false.

    \sa isNull(), isValid()
*/

/*!
    \fn bool QSize::isValid() const

    Returns \c true if both the width and height is equal to or greater
    than 0; otherwise returns \c false.

    \sa isNull(), isEmpty()
*/

/*!
    \fn int QSize::width() const

    Returns the width.

    \sa height(), setWidth()
*/

/*!
    \fn int QSize::height() const

    Returns the height.

    \sa width(), setHeight()
*/

/*!
    \fn void QSize::setWidth(int width)

    Sets the width to the given \a width.

    \sa rwidth(), width(), setHeight()
*/

/*!
    \fn void QSize::setHeight(int height)

    Sets the height to the given \a height.

    \sa rheight(), height(), setWidth()
*/

/*!
    Swaps the width and height values.

    \sa setWidth(), setHeight(), transposed()
*/

void QSize::transpose() noexcept
{
    qSwap(wd, ht);
}

/*!
  \fn QSize QSize::transposed() const
  \since 5.0

  Returns a QSize with width and height swapped.

  \sa transpose()
*/

/*!
  \fn void QSize::scale(int width, int height, Qt::AspectRatioMode mode)

    Scales the size to a rectangle with the given \a width and \a
    height, according to the specified \a mode:

    \list
    \li If \a mode is Qt::IgnoreAspectRatio, the size is set to (\a width, \a height).
    \li If \a mode is Qt::KeepAspectRatio, the current size is scaled to a rectangle
       as large as possible inside (\a width, \a height), preserving the aspect ratio.
    \li If \a mode is Qt::KeepAspectRatioByExpanding, the current size is scaled to a rectangle
       as small as possible outside (\a width, \a height), preserving the aspect ratio.
    \endlist

    Example:
    \snippet code/src_corelib_tools_qsize.cpp 0

    \sa setWidth(), setHeight(), scaled()
*/

/*!
    \fn void QSize::scale(const QSize &size, Qt::AspectRatioMode mode)
    \overload

    Scales the size to a rectangle with the given \a size, according to
    the specified \a mode.
*/

/*!
    \fn QSize QSize::scaled(int width, int height, Qt::AspectRatioMode mode) const
    \since 5.0

    Return a size scaled to a rectangle with the given \a width and \a
    height, according to the specified \a mode.

    \sa scale()
*/

/*!
    \overload
    \since 5.0

    Return a size scaled to a rectangle with the given size \a s,
    according to the specified \a mode.
*/
QSize QSize::scaled(const QSize &s, Qt::AspectRatioMode mode) const noexcept
{
    if (mode == Qt::IgnoreAspectRatio || wd == 0 || ht == 0) {
        return s;
    } else {
        bool useHeight;
        qint64 rw = qint64(s.ht) * qint64(wd) / qint64(ht);

        if (mode == Qt::KeepAspectRatio) {
            useHeight = (rw <= s.wd);
        } else { // mode == Qt::KeepAspectRatioByExpanding
            useHeight = (rw >= s.wd);
        }

        if (useHeight) {
            return QSize(rw, s.ht);
        } else {
            return QSize(s.wd,
                         qint32(qint64(s.wd) * qint64(ht) / qint64(wd)));
        }
    }
}

/*!
    \fn int &QSize::rwidth()

    Returns a reference to the width.

    Using a reference makes it possible to manipulate the width
    directly. For example:

    \snippet code/src_corelib_tools_qsize.cpp 1

    \sa rheight(), setWidth()
*/

/*!
    \fn int &QSize::rheight()

    Returns a reference to the height.

    Using a reference makes it possible to manipulate the height
    directly. For example:

    \snippet code/src_corelib_tools_qsize.cpp 2

    \sa rwidth(), setHeight()
*/

/*!
    \fn QSize &QSize::operator+=(const QSize &size)

    Adds the given \a size to \e this size, and returns a reference to
    this size. For example:

    \snippet code/src_corelib_tools_qsize.cpp 3
*/

/*!
    \fn QSize &QSize::operator-=(const QSize &size)

    Subtracts the given \a size from \e this size, and returns a
    reference to this size. For example:

    \snippet code/src_corelib_tools_qsize.cpp 4
*/

/*!
    \fn QSize &QSize::operator*=(qreal factor)
    \overload

    Multiplies both the width and height by the given \a factor, and
    returns a reference to the size.

    Note that the result is rounded to the nearest integer.

    \sa scale()
*/

/*!
    \fn bool QSize::operator==(const QSize &s1, const QSize &s2)

    Returns \c true if \a s1 and \a s2 are equal; otherwise returns \c false.
*/

/*!
    \fn bool QSize::operator!=(const QSize &s1, const QSize &s2)

    Returns \c true if \a s1 and \a s2 are different; otherwise returns \c false.
*/

/*!
    \fn QSize QSize::operator+(const QSize &s1, const QSize &s2)

    Returns the sum of \a s1 and \a s2; each component is added separately.
*/

/*!
    \fn QSize QSize::operator-(const QSize &s1, const QSize &s2)

    Returns \a s2 subtracted from \a s1; each component is subtracted
    separately.
*/

/*!
    \fn QSize QSize::operator*(const QSize &size, qreal factor)

    Multiplies the given \a size by the given \a factor, and returns
    the result rounded to the nearest integer.

    \sa QSize::scale()
*/

/*!
    \fn QSize QSize::operator*(qreal factor, const QSize &size)
    \overload

    Multiplies the given \a size by the given \a factor, and returns
    the result rounded to the nearest integer.
*/

/*!
    \fn QSize &QSize::operator/=(qreal divisor)
    \overload

    Divides both the width and height by the given \a divisor, and
    returns a reference to the size.

    Note that the result is rounded to the nearest integer.

    \sa QSize::scale()
*/

/*!
    \fn QSize QSize::operator/(const QSize &size, qreal divisor)
    \overload

    Divides the given \a size by the given \a divisor, and returns the
    result rounded to the nearest integer.

    \sa QSize::scale()
*/

/*!
    \fn QSize QSize::expandedTo(const QSize & otherSize) const

    Returns a size holding the maximum width and height of this size
    and the given \a otherSize.

    \sa boundedTo(), scale()
*/

/*!
    \fn QSize QSize::boundedTo(const QSize & otherSize) const

    Returns a size holding the minimum width and height of this size
    and the given \a otherSize.

    \sa expandedTo(), scale()
*/

/*!
    \fn QSize QSize::grownBy(QMargins margins) const
    \fn QSizeF QSizeF::grownBy(QMarginsF margins) const
    \since 5.14

    Returns the size that results from growing this size by \a margins.

    \sa shrunkBy()
*/

/*!
    \fn QSize QSize::shrunkBy(QMargins margins) const
    \fn QSizeF QSizeF::shrunkBy(QMarginsF margins) const
    \since 5.14

    Returns the size that results from shrinking this size by \a margins.

    \sa grownBy()
*/

/*!
    \fn QSize::toSizeF() const
    \since 6.4

    Returns this size as a size with floating point accuracy.

    \sa QSizeF::toSize()
*/

/*****************************************************************************
  QSize stream functions
 *****************************************************************************/
#ifndef QT_NO_DATASTREAM
/*!
    \fn QDataStream &operator<<(QDataStream &stream, const QSize &size)
    \relates QSize

    Writes the given \a size to the given \a stream, and returns a
    reference to the stream.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator<<(QDataStream &s, const QSize &sz)
{
    if (s.version() == 1)
        s << (qint16)sz.width() << (qint16)sz.height();
    else
        s << (qint32)sz.width() << (qint32)sz.height();
    return s;
}

/*!
    \fn QDataStream &operator>>(QDataStream &stream, QSize &size)
    \relates QSize

    Reads a size from the given \a stream into the given \a size, and
    returns a reference to the stream.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator>>(QDataStream &s, QSize &sz)
{
    if (s.version() == 1) {
        qint16 w, h;
        s >> w;  sz.rwidth() = w;
        s >> h;  sz.rheight() = h;
    }
    else {
        qint32 w, h;
        s >> w;  sz.rwidth() = w;
        s >> h;  sz.rheight() = h;
    }
    return s;
}
#endif // QT_NO_DATASTREAM

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QSize &s)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    dbg << "QSize(";
    QtDebugUtils::formatQSize(dbg, s);
    dbg << ')';
    return dbg;
}
#endif



/*!
    \class QSizeF
    \inmodule QtCore
    \brief The QSizeF class defines the size of a two-dimensional object
    using floating point precision.

    \ingroup painting

    A size is specified by a width() and a height().  It can be set in
    the constructor and changed using the setWidth(), setHeight(), or
    scale() functions, or using arithmetic operators. A size can also
    be manipulated directly by retrieving references to the width and
    height using the rwidth() and rheight() functions. Finally, the
    width and height can be swapped using the transpose() function.

    The isValid() function determines if a size is valid. A valid size
    has both width and height greater than or equal to zero. The
    isEmpty() function returns \c true if either of the width and height
    is \e less than (or equal to) zero, while the isNull() function
    returns \c true only if both the width and the height is zero.

    Use the expandedTo() function to retrieve a size which holds the
    maximum height and width of this size and a given
    size. Similarly, the boundedTo() function returns a size which
    holds the minimum height and width of this size and a given size.

    The QSizeF class also provides the toSize() function returning a
    QSize copy of this size, constructed by rounding the width and
    height to the nearest integers.

    QSizeF objects can be streamed as well as compared.

    \sa QSize, QPointF, QRectF
*/


/*****************************************************************************
  QSizeF member functions
 *****************************************************************************/

/*!
    \fn QSizeF::QSizeF()

    Constructs an invalid size.

    \sa isValid()
*/

/*!
    \fn QSizeF::QSizeF(const QSize &size)

    Constructs a size with floating point accuracy from the given \a
    size.

    \sa toSize(), QSize::toSizeF()
*/

/*!
    \fn QSizeF::QSizeF(qreal width, qreal height)

    Constructs a size with the given finite \a width and \a height.
*/

/*!
    \fn bool QSizeF::isNull() const

    Returns \c true if both the width and height are 0.0 (ignoring the sign);
    otherwise returns \c false.

    \sa isValid(), isEmpty()
*/

/*!
    \fn bool QSizeF::isEmpty() const

    Returns \c true if either of the width and height is less than or
    equal to 0; otherwise returns \c false.

    \sa isNull(), isValid()
*/

/*!
    \fn bool QSizeF::isValid() const

    Returns \c true if both the width and height are equal to or greater
    than 0; otherwise returns \c false.

    \sa isNull(), isEmpty()
*/

/*!
    \fn int QSizeF::width() const

    Returns the width.

    \sa height(), setWidth()
*/

/*!
    \fn int QSizeF::height() const

    Returns the height.

    \sa width(), setHeight()
*/

/*!
    \fn void QSizeF::setWidth(qreal width)

    Sets the width to the given finite \a width.

    \sa width(), rwidth(), setHeight()
*/

/*!
    \fn void QSizeF::setHeight(qreal height)

    Sets the height to the given finite \a height.

    \sa height(), rheight(), setWidth()
*/

/*!
    \fn QSize QSizeF::toSize() const

    Returns an integer based copy of this size.

    Note that the coordinates in the returned size will be rounded to
    the nearest integer.

    \sa QSizeF(), QSize::toSizeF()
*/

/*!
    Swaps the width and height values.

    \sa setWidth(), setHeight(), transposed()
*/

void QSizeF::transpose() noexcept
{
    qSwap(wd, ht);
}

/*!
    \fn QSizeF QSizeF::transposed() const
    \since 5.0

    Returns the size with width and height values swapped.

    \sa transpose()
*/

/*!
  \fn void QSizeF::scale(qreal width, qreal height, Qt::AspectRatioMode mode)

    Scales the size to a rectangle with the given \a width and \a
    height, according to the specified \a mode.

    \list
    \li If \a mode is Qt::IgnoreAspectRatio, the size is set to (\a width, \a height).
    \li If \a mode is Qt::KeepAspectRatio, the current size is scaled to a rectangle
       as large as possible inside (\a width, \a height), preserving the aspect ratio.
    \li If \a mode is Qt::KeepAspectRatioByExpanding, the current size is scaled to a rectangle
       as small as possible outside (\a width, \a height), preserving the aspect ratio.
    \endlist

    Example:
    \snippet code/src_corelib_tools_qsize.cpp 5

    \sa setWidth(), setHeight(), scaled()
*/

/*!
    \fn void QSizeF::scale(const QSizeF &size, Qt::AspectRatioMode mode)
    \overload

    Scales the size to a rectangle with the given \a size, according to
    the specified \a mode.
*/

/*!
    \fn QSizeF QSizeF::scaled(qreal width, qreal height, Qt::AspectRatioMode mode) const
    \since 5.0

    Returns a size scaled to a rectangle with the given \a width and
    \a height, according to the specified \a mode.

    \sa scale()
*/

/*!
    \overload
    \since 5.0

    Returns a size scaled to a rectangle with the given size \a s,
    according to the specified \a mode.
*/
QSizeF QSizeF::scaled(const QSizeF &s, Qt::AspectRatioMode mode) const noexcept
{
    if (mode == Qt::IgnoreAspectRatio || qIsNull(wd) || qIsNull(ht)) {
        return s;
    } else {
        bool useHeight;
        qreal rw = s.ht * wd / ht;

        if (mode == Qt::KeepAspectRatio) {
            useHeight = (rw <= s.wd);
        } else { // mode == Qt::KeepAspectRatioByExpanding
            useHeight = (rw >= s.wd);
        }

        if (useHeight) {
            return QSizeF(rw, s.ht);
        } else {
            return QSizeF(s.wd, s.wd * ht / wd);
        }
    }
}

/*!
    \fn int &QSizeF::rwidth()

    Returns a reference to the width.

    Using a reference makes it possible to manipulate the width
    directly. For example:

    \snippet code/src_corelib_tools_qsize.cpp 6

    \sa rheight(), setWidth()
*/

/*!
    \fn int &QSizeF::rheight()

    Returns a reference to the height.

    Using a reference makes it possible to manipulate the height
    directly. For example:

    \snippet code/src_corelib_tools_qsize.cpp 7

    \sa rwidth(), setHeight()
*/

/*!
    \fn QSizeF &QSizeF::operator+=(const QSizeF &size)

    Adds the given \a size to this size and returns a reference to
    this size. For example:

    \snippet code/src_corelib_tools_qsize.cpp 8
*/

/*!
    \fn QSizeF &QSizeF::operator-=(const QSizeF &size)

    Subtracts the given \a size from this size and returns a reference
    to this size. For example:

    \snippet code/src_corelib_tools_qsize.cpp 9
*/

/*!
    \fn QSizeF &QSizeF::operator*=(qreal factor)
    \overload

    Multiplies both the width and height by the given finite \a factor and
    returns a reference to the size.

    \sa scale()
*/

/*!
    \fn bool QSizeF::operator==(const QSizeF &s1, const QSizeF &s2)

    Returns \c true if \a s1 and \a s2 are approximately equal; otherwise
    returns false.

    \warning This function does not check for strict equality; instead,
    it uses a fuzzy comparison to compare the sizes' extents.

    \sa qFuzzyCompare
*/

/*!
    \fn bool QSizeF::operator!=(const QSizeF &s1, const QSizeF &s2)

    Returns \c true if \a s1 and \a s2 are sufficiently different; otherwise
    returns \c false.

    \warning This function does not check for strict inequality; instead,
    it uses a fuzzy comparison to compare the sizes' extents.
*/

/*!
    \fn QSizeF QSizeF::operator+(const QSizeF &s1, const QSizeF &s2)

    Returns the sum of \a s1 and \a s2; each component is added separately.
*/

/*!
    \fn QSizeF QSizeF::operator-(const QSizeF &s1, const QSizeF &s2)

    Returns \a s2 subtracted from \a s1; each component is subtracted
    separately.
*/

/*!
    \fn QSizeF QSizeF::operator*(const QSizeF &size, qreal factor)

    \overload

    Multiplies the given \a size by the given finite \a factor and returns
    the result.

    \sa QSizeF::scale()
*/

/*!
    \fn QSizeF QSizeF::operator*(qreal factor, const QSizeF &size)

    \overload

    Multiplies the given \a size by the given finite \a factor and returns
    the result.
*/

/*!
    \fn QSizeF &QSizeF::operator/=(qreal divisor)

    \overload

    Divides both the width and height by the given \a divisor and returns a
    reference to the size. The \a divisor must not be either zero or NaN.

    \sa scale()
*/

/*!
    \fn QSizeF QSizeF::operator/(const QSizeF &size, qreal divisor)

    \overload

    Divides the given \a size by the given \a divisor and returns the
    result. The \a divisor must not be either zero or NaN.

    \sa QSizeF::scale()
*/

/*!
    \fn QSizeF QSizeF::expandedTo(const QSizeF & otherSize) const

    Returns a size holding the maximum width and height of this size
    and the given \a otherSize.

    \sa boundedTo(), scale()
*/

/*!
    \fn QSizeF QSizeF::boundedTo(const QSizeF & otherSize) const

    Returns a size holding the minimum width and height of this size
    and the given \a otherSize.

    \sa expandedTo(), scale()
*/



/*****************************************************************************
  QSizeF stream functions
 *****************************************************************************/
#ifndef QT_NO_DATASTREAM
/*!
    \fn QDataStream &operator<<(QDataStream &stream, const QSizeF &size)
    \relates QSizeF

    Writes the given \a size to the given \a stream and returns a
    reference to the stream.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator<<(QDataStream &s, const QSizeF &sz)
{
    s << double(sz.width()) << double(sz.height());
    return s;
}

/*!
    \fn QDataStream &operator>>(QDataStream &stream, QSizeF &size)
    \relates QSizeF

    Reads a size from the given \a stream into the given \a size and
    returns a reference to the stream.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator>>(QDataStream &s, QSizeF &sz)
{
    double w, h;
    s >> w;
    s >> h;
    sz.setWidth(qreal(w));
    sz.setHeight(qreal(h));
    return s;
}
#endif // QT_NO_DATASTREAM

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QSizeF &s)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    dbg << "QSizeF(";
    QtDebugUtils::formatQSize(dbg, s);
    dbg << ')';
    return dbg;
}
#endif

QT_END_NAMESPACE
