/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/qdebug.h>

#include "qmediatimerange.h"

QT_BEGIN_NAMESPACE

/*!
    \class QMediaTimeInterval
    \brief The QMediaTimeInterval class represents a time interval with integer precision.
    \inmodule QtMultimedia

    \ingroup multimedia
    \ingroup multimedia_core

    An interval is specified by an inclusive start() and end() time.  These
    must be set in the constructor, as this is an immutable class.  The
    specific units of time represented by the class have not been defined - it
    is suitable for any times which can be represented by a signed 64 bit
    integer.

    The isNormal() method determines if a time interval is normal (a normal
    time interval has start() <= end()). A normal interval can be received
    from an abnormal interval by calling the normalized() method.

    The contains() method determines if a specified time lies within the time
    interval.

    The translated() method returns a time interval which has been translated
    forwards or backwards through time by a specified offset.

    \sa QMediaTimeRange
*/

/*!
    \fn QMediaTimeInterval::QMediaTimeInterval()

    Constructs an empty interval.
*/
QMediaTimeInterval::QMediaTimeInterval()
    : s(0)
    , e(0)
{

}

/*!
    \fn QMediaTimeInterval::QMediaTimeInterval(qint64 start, qint64 end)

    Constructs an interval with the specified \a start and \a end times.
*/
QMediaTimeInterval::QMediaTimeInterval(qint64 start, qint64 end)
    : s(start)
    , e(end)
{

}

/*!
    \fn QMediaTimeInterval::QMediaTimeInterval(const QMediaTimeInterval &other)

    Constructs an interval by taking a copy of \a other.
*/
QMediaTimeInterval::QMediaTimeInterval(const QMediaTimeInterval &other)
    : s(other.s)
    , e(other.e)
{

}

/*!
    \fn QMediaTimeInterval::start() const

    Returns the start time of the interval.

    \sa end()
*/
qint64 QMediaTimeInterval::start() const
{
    return s;
}

/*!
    \fn QMediaTimeInterval::end() const

    Returns the end time of the interval.

    \sa start()
*/
qint64 QMediaTimeInterval::end() const
{
    return e;
}

/*!
    \fn QMediaTimeInterval::contains(qint64 time) const

    Returns true if the time interval contains the specified \a time.
    That is, start() <= time <= end().
*/
bool QMediaTimeInterval::contains(qint64 time) const
{
    return isNormal() ? (s <= time && time <= e)
        : (e <= time && time <= s);
}

/*!
    \fn QMediaTimeInterval::isNormal() const

    Returns true if this time interval is normal.
    A normal time interval has start() <= end().

    \sa normalized()
*/
bool QMediaTimeInterval::isNormal() const
{
    return s <= e;
}

/*!
    \fn QMediaTimeInterval::normalized() const

    Returns a normalized version of this interval.

    If the start() time of the interval is greater than the end() time,
    then the returned interval has the start and end times swapped.
*/
QMediaTimeInterval QMediaTimeInterval::normalized() const
{
    if(s > e)
        return QMediaTimeInterval(e, s);

    return *this;
}

/*!
    \fn QMediaTimeInterval::translated(qint64 offset) const

    Returns a copy of this time interval, translated by a value of \a offset.
    An interval can be moved forward through time with a positive offset, or backward
    through time with a negative offset.
*/
QMediaTimeInterval QMediaTimeInterval::translated(qint64 offset) const
{
    return QMediaTimeInterval(s + offset, e + offset);
}

/*!
    \relates QMediaTimeRange

    Returns true if \a a is exactly equal to \a b.
*/
bool operator==(const QMediaTimeInterval &a, const QMediaTimeInterval &b)
{
    return a.start() == b.start() && a.end() == b.end();
}

/*!
    \relates QMediaTimeRange

    Returns true if \a a is not exactly equal to \a b.
*/
bool operator!=(const QMediaTimeInterval &a, const QMediaTimeInterval &b)
{
    return a.start() != b.start() || a.end() != b.end();
}

class QMediaTimeRangePrivate : public QSharedData
{
public:

    QMediaTimeRangePrivate();
    QMediaTimeRangePrivate(const QMediaTimeRangePrivate &other);
    QMediaTimeRangePrivate(const QMediaTimeInterval &interval);

    QList<QMediaTimeInterval> intervals;

    void addInterval(const QMediaTimeInterval &interval);
    void removeInterval(const QMediaTimeInterval &interval);
};

QMediaTimeRangePrivate::QMediaTimeRangePrivate()
    : QSharedData()
{

}

QMediaTimeRangePrivate::QMediaTimeRangePrivate(const QMediaTimeRangePrivate &other)
    : QSharedData()
    , intervals(other.intervals)
{

}

QMediaTimeRangePrivate::QMediaTimeRangePrivate(const QMediaTimeInterval &interval)
    : QSharedData()
{
    if(interval.isNormal())
        intervals << interval;
}

void QMediaTimeRangePrivate::addInterval(const QMediaTimeInterval &interval)
{
    // Handle normalized intervals only
    if(!interval.isNormal())
        return;

    // Find a place to insert the interval
    int i;
    for (i = 0; i < intervals.count(); i++) {
        // Insert before this element
        if(interval.s < intervals[i].s) {
            intervals.insert(i, interval);
            break;
        }
    }

    // Interval needs to be added to the end of the list
    if (i == intervals.count())
        intervals.append(interval);

    // Do we need to correct the element before us?
    if(i > 0 && intervals[i - 1].e >= interval.s - 1)
        i--;

    // Merge trailing ranges
    while (i < intervals.count() - 1
          && intervals[i].e >= intervals[i + 1].s - 1) {
        intervals[i].e = qMax(intervals[i].e, intervals[i + 1].e);
        intervals.removeAt(i + 1);
    }
}

void QMediaTimeRangePrivate::removeInterval(const QMediaTimeInterval &interval)
{
    // Handle normalized intervals only
    if(!interval.isNormal())
        return;

    for (int i = 0; i < intervals.count(); i++) {
        QMediaTimeInterval r = intervals[i];

        if (r.e < interval.s) {
            // Before the removal interval
            continue;
        } else if (interval.e < r.s) {
            // After the removal interval - stop here
            break;
        } else if (r.s < interval.s && interval.e < r.e) {
            // Split case - a single range has a chunk removed
            intervals[i].e = interval.s -1;
            addInterval(QMediaTimeInterval(interval.e + 1, r.e));
            break;
        } else if (r.s < interval.s) {
            // Trimming Tail Case
            intervals[i].e = interval.s - 1;
        } else if (interval.e < r.e) {
            // Trimming Head Case - we can stop after this
            intervals[i].s = interval.e + 1;
            break;
        } else {
            // Complete coverage case
            intervals.removeAt(i);
            --i;
        }
    }
}

/*!
    \class QMediaTimeRange
    \brief The QMediaTimeRange class represents a set of zero or more disjoint
    time intervals.
    \ingroup multimedia
    \inmodule QtMultimedia

    \reentrant

    The earliestTime(), latestTime(), intervals() and isEmpty()
    methods are used to get information about the current time range.

    The addInterval(), removeInterval() and clear() methods are used to modify
    the current time range.

    When adding or removing intervals from the time range, existing intervals
    within the range may be expanded, trimmed, deleted, merged or split to ensure
    that all intervals within the time range remain distinct and disjoint. As a
    consequence, all intervals added or removed from a time range must be
    \l{QMediaTimeInterval::isNormal()}{normal}.

    \sa QMediaTimeInterval
*/

/*!
    \fn QMediaTimeRange::QMediaTimeRange()

    Constructs an empty time range.
*/
QMediaTimeRange::QMediaTimeRange()
    : d(new QMediaTimeRangePrivate)
{

}

/*!
    \fn QMediaTimeRange::QMediaTimeRange(qint64 start, qint64 end)

    Constructs a time range that contains an initial interval from
    \a start to \a end inclusive.

    If the interval is not \l{QMediaTimeInterval::isNormal()}{normal},
    the resulting time range will be empty.

    \sa addInterval()
*/
QMediaTimeRange::QMediaTimeRange(qint64 start, qint64 end)
    : d(new QMediaTimeRangePrivate(QMediaTimeInterval(start, end)))
{

}

/*!
    \fn QMediaTimeRange::QMediaTimeRange(const QMediaTimeInterval &interval)

    Constructs a time range that contains an initial interval, \a interval.

    If \a interval is not \l{QMediaTimeInterval::isNormal()}{normal},
    the resulting time range will be empty.

    \sa addInterval()
*/
QMediaTimeRange::QMediaTimeRange(const QMediaTimeInterval &interval)
    : d(new QMediaTimeRangePrivate(interval))
{

}

/*!
    \fn QMediaTimeRange::QMediaTimeRange(const QMediaTimeRange &range)

    Constructs a time range by copying another time \a range.
*/
QMediaTimeRange::QMediaTimeRange(const QMediaTimeRange &range)
    : d(range.d)
{

}

/*!
    \fn QMediaTimeRange::~QMediaTimeRange()

    Destructor.
*/
QMediaTimeRange::~QMediaTimeRange()
{

}

/*!
    Takes a copy of the \a other time range and returns itself.
*/
QMediaTimeRange &QMediaTimeRange::operator=(const QMediaTimeRange &other)
{
    d = other.d;
    return *this;
}

/*!
    Sets the time range to a single continuous interval, \a interval.
*/
QMediaTimeRange &QMediaTimeRange::operator=(const QMediaTimeInterval &interval)
{
    d = new QMediaTimeRangePrivate(interval);
    return *this;
}

/*!
    \fn QMediaTimeRange::earliestTime() const

    Returns the earliest time within the time range.

    For empty time ranges, this value is equal to zero.

    \sa latestTime()
*/
qint64 QMediaTimeRange::earliestTime() const
{
    if (!d->intervals.isEmpty())
        return d->intervals[0].s;

    return 0;
}

/*!
    \fn QMediaTimeRange::latestTime() const

    Returns the latest time within the time range.

    For empty time ranges, this value is equal to zero.

    \sa earliestTime()
*/
qint64 QMediaTimeRange::latestTime() const
{
    if (!d->intervals.isEmpty())
        return d->intervals[d->intervals.count() - 1].e;

    return 0;
}

/*!
    \fn QMediaTimeRange::addInterval(qint64 start, qint64 end)
    \overload

    Adds the interval specified by \a start and \a end
    to the time range.

    \sa addInterval()
*/
void QMediaTimeRange::addInterval(qint64 start, qint64 end)
{
    d->addInterval(QMediaTimeInterval(start, end));
}

/*!
    \fn QMediaTimeRange::addInterval(const QMediaTimeInterval &interval)

    Adds the specified \a interval to the time range.

    Adding intervals which are not \l{QMediaTimeInterval::isNormal()}{normal}
    is invalid, and will be ignored.

    If the specified interval is adjacent to, or overlaps existing
    intervals within the time range, these intervals will be merged.

    This operation takes linear time.

    \sa removeInterval()
*/
void QMediaTimeRange::addInterval(const QMediaTimeInterval &interval)
{
    d->addInterval(interval);
}

/*!
    Adds each of the intervals in \a range to this time range.

    Equivalent to calling addInterval() for each interval in \a range.
*/
void QMediaTimeRange::addTimeRange(const QMediaTimeRange &range)
{
    const auto intervals = range.intervals();
    for (const QMediaTimeInterval &i : intervals) {
        d->addInterval(i);
    }
}

/*!
    \fn QMediaTimeRange::removeInterval(qint64 start, qint64 end)
    \overload

    Removes the interval specified by \a start and \a end
    from the time range.

    \sa removeInterval()
*/
void QMediaTimeRange::removeInterval(qint64 start, qint64 end)
{
    d->removeInterval(QMediaTimeInterval(start, end));
}

/*!
    \fn QMediaTimeRange::removeInterval(const QMediaTimeInterval &interval)

    Removes the specified \a interval from the time range.

    Removing intervals which are not \l{QMediaTimeInterval::isNormal()}{normal}
    is invalid, and will be ignored.

    Intervals within the time range will be trimmed, split or deleted
    such that no intervals within the time range include any part of the
    target interval.

    This operation takes linear time.

    \sa addInterval()
*/
void QMediaTimeRange::removeInterval(const QMediaTimeInterval &interval)
{
    d->removeInterval(interval);
}

/*!
    Removes each of the intervals in \a range from this time range.

    Equivalent to calling removeInterval() for each interval in \a range.
*/
void QMediaTimeRange::removeTimeRange(const QMediaTimeRange &range)
{
    const auto intervals = range.intervals();
    for (const QMediaTimeInterval &i : intervals) {
        d->removeInterval(i);
    }
}

/*!
    Adds each interval in \a other to the time range and returns the result.
*/
QMediaTimeRange& QMediaTimeRange::operator+=(const QMediaTimeRange &other)
{
    addTimeRange(other);
    return *this;
}

/*!
    Adds the specified \a interval to the time range and returns the result.
*/
QMediaTimeRange& QMediaTimeRange::operator+=(const QMediaTimeInterval &interval)
{
    addInterval(interval);
    return *this;
}

/*!
    Removes each interval in \a other from the time range and returns the result.
*/
QMediaTimeRange& QMediaTimeRange::operator-=(const QMediaTimeRange &other)
{
    removeTimeRange(other);
    return *this;
}

/*!
    Removes the specified \a interval from the time range and returns the result.
*/
QMediaTimeRange& QMediaTimeRange::operator-=(const QMediaTimeInterval &interval)
{
    removeInterval(interval);
    return *this;
}

/*!
    \fn QMediaTimeRange::clear()

    Removes all intervals from the time range.

    \sa removeInterval()
*/
void QMediaTimeRange::clear()
{
    d->intervals.clear();
}

/*!
    \fn QMediaTimeRange::intervals() const

    Returns the list of intervals covered by this time range.
*/
QList<QMediaTimeInterval> QMediaTimeRange::intervals() const
{
    return d->intervals;
}

/*!
    \fn QMediaTimeRange::isEmpty() const

    Returns true if there are no intervals within the time range.

    \sa intervals()
*/
bool QMediaTimeRange::isEmpty() const
{
    return d->intervals.isEmpty();
}

/*!
    \fn QMediaTimeRange::isContinuous() const

    Returns true if the time range consists of a continuous interval.
    That is, there is one or fewer disjoint intervals within the time range.
*/
bool QMediaTimeRange::isContinuous() const
{
    return (d->intervals.count() <= 1);
}

/*!
    \fn QMediaTimeRange::contains(qint64 time) const

    Returns true if the specified \a time lies within the time range.
*/
bool QMediaTimeRange::contains(qint64 time) const
{
    for (int i = 0; i < d->intervals.count(); i++) {
        if (d->intervals[i].contains(time))
            return true;

        if (time < d->intervals[i].s)
            break;
    }

    return false;
}

/*!
    \relates QMediaTimeRange

    Returns true if all intervals in \a a are present in \a b.
*/
bool operator==(const QMediaTimeRange &a, const QMediaTimeRange &b)
{
    return a.intervals() == b.intervals();
}

/*!
    \relates QMediaTimeRange

    Returns true if one or more intervals in \a a are not present in \a b.
*/
bool operator!=(const QMediaTimeRange &a, const QMediaTimeRange &b)
{
    return !(a == b);
}

/*!
    \relates QMediaTimeRange

    Returns a time range containing the union between \a r1 and \a r2.
 */
QMediaTimeRange operator+(const QMediaTimeRange &r1, const QMediaTimeRange &r2)
{
    return (QMediaTimeRange(r1) += r2);
}

/*!
    \relates QMediaTimeRange

    Returns a time range containing \a r2 subtracted from \a r1.
 */
QMediaTimeRange operator-(const QMediaTimeRange &r1, const QMediaTimeRange &r2)
{
    return (QMediaTimeRange(r1) -= r2);
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QMediaTimeRange &range)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    dbg << "QMediaTimeRange( ";
    const auto intervals = range.intervals();
    for (const QMediaTimeInterval &interval : intervals)
        dbg << '(' <<  interval.start() << ", " << interval.end() << ") ";
    dbg.space();
    dbg << ')';
    return dbg;
}
#endif

QT_END_NAMESPACE

