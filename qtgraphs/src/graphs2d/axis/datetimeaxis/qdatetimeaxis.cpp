// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/QObject>
#include <QtGraphs/QDateTimeAxis>
#include "private/qdatetimeaxis_p.h"

QT_BEGIN_NAMESPACE
/*!
 \class QDateTimeAxis
 \inmodule QtGraphs
 \ingroup graphs_2D
 \brief The QDateTimeAxis adds support for DateTime values to be added to a graph's axis.

 A DateTime Axis can be used to display DateTime representations with tick marks and grid lines.
 The DateTime items on the axis are displayed at the position of the ticks.
*/

/*!
 \qmltype DateTimeAxis
 \nativetype QDateTimeAxis
 \inqmlmodule QtGraphs
 \ingroup graphs_qml_2D
 \inherits AbstractAxis
 \brief Adds DateTime items to a graph's axis.

 A DateTime Axis can be used to display DateTime representations with tick marks and grid lines.
 The DateTime items on the axis are displayed at the position of the ticks.

 The following example code illustrates how to use the DateTimeAxis type:
 \code
 GraphsView {
     axisX: DateTimeAxis {
         min: new Date(2000,1,1)
         max: new Date(1970,1,1)
     }
     LineSeries {
         // Add a few XYPoint data...
     }
 }
 \endcode
*/

/*!
 \property QDateTimeAxis::min
 \brief The minimum value on the axis

 This value can be lower or higher than the maximum.
 The DateTime is stored as UTC internally.
 The default value is new Date(1970,1,1)
*/
/*!
 \qmlproperty real DateTimeAxis::min
 The minimum value on the axis.

 This value can be lower or higher than the maximum.
 The DateTime is stored as UTC internally.
 The default value is new Date(1970,1,1)
*/

/*!
 \property QDateTimeAxis::max
 \brief The maximum value on the axis

 This value can be lower or higher than the minimum.
 The DateTime is stored as UTC internally.
 The default value is new Date(1980,1,1)
*/
/*!
 \qmlproperty real DateTimeAxis::max
 The maximum value on the axis.

 This value can be lower or higher than the minimum.
 The DateTime is stored as UTC internally.
 The default value is new Date(1980,1,1)
*/
/*!
 \property QDateTimeAxis::subTickCount
 \brief The number of subticks on the axis. This indicates how many subticks are drawn
 between major lines on the graph. Labels are not drawn for subticks. The default value is 0.
*/
/*!
 \qmlproperty int DateTimeAxis::subTickCount
 The number of subticks on the axis. This indicates how many subticks are drawn
 between major lines on the graph. Labels are not drawn for subticks. The default value is 0.
*/
/*!
 \property QDateTimeAxis::tickInterval
 \brief The interval between dynamically placed tick marks and labels.
 The default value is 0, which means that intervals are automatically calculated
 based on the min and max range.
*/
/*!
 \qmlproperty real DateTimeAxis::tickInterval
 The interval between dynamically placed tick marks and labels.
 The default value is 0, which means that intervals are automatically calculated
 based on the min and max range.
*/

/*!
 \property QDateTimeAxis::labelFormat
 \brief The format of the DateTime labels on the axis.
 The format property allows to signify the visual representation of the DateTime object, in days,
  months, and years. The default value is dd-MMMM-yy.
*/

/*!
 \qmlproperty string DateTimeAxis::labelFormat
 The format of the DateTime labels on the axis
 The format property allows to signify the visual representation of the DateTime object, in days,
  months, and years. The default value is dd-MMMM-yy.
 */
/*!
 \qmlsignal DateTimeAxis::minChanged(DateTime min)
 This signal is emitted when the minimum value of the axis, specified by \a min, changes.
*/
/*!
 \qmlsignal DateTimeAxis::maxChanged(DateTime max)
 This signal is emitted when the maximum value of the axis, specified by \a max, changes.
*/
/*!
 \qmlsignal DateTimeAxis::subTickCountChanged(int subTickCount)
 This signal is emitted when the number of subticks on the axis, specified by
 \a subTickCount, changes.
*/
/*!
 \qmlsignal DateTimeAxis::rangeChanged(real min, real max)
 This signal is emitted when the minimum or maximum value of the axis, specified by \a min
 and \a max, changes.
*/
/*!
 \qmlsignal DateTimeAxis::labelFormatChanged(string format)
 This signal is emitted when the \a format of axis labels changes.
*/
/*!
 \qmlsignal DateTimeAxis::tickIntervalChanged(real tickInterval)
 This signal is emitted when the tick interval value, specified by
 \a tickInterval, changes.
*/

QDateTimeAxis::QDateTimeAxis(QObject *parent)
    : QAbstractAxis(*(new QDateTimeAxisPrivate), parent)
{}

QDateTimeAxis::~QDateTimeAxis()
{
}

QAbstractAxis::AxisType QDateTimeAxis::type() const
{
    return QAbstractAxis::AxisType::DateTime;
}

void QDateTimeAxis::setMin(const QDateTime &min)
{
    Q_D(QDateTimeAxis);
    if (min.isValid()) {
        d->setRange(min.toMSecsSinceEpoch(), d->m_max);
        emit update();
    }
}

QDateTime QDateTimeAxis::min() const
{
    Q_D(const QDateTimeAxis);
    return QDateTime::fromMSecsSinceEpoch(d->m_min, QTimeZone::UTC);
}

void QDateTimeAxis::setMax(const QDateTime &max)
{
    Q_D(QDateTimeAxis);
    if (max.isValid()) {
        d->setRange(d->m_min, max.toMSecsSinceEpoch());
        emit update();
    }
}

QDateTime QDateTimeAxis::max() const
{
    Q_D(const QDateTimeAxis);
    return QDateTime::fromMSecsSinceEpoch(d->m_max, QTimeZone::UTC);
}

void QDateTimeAxis::setLabelFormat(const QString &format)
{
    Q_D(QDateTimeAxis);
    if (d->m_format != format) {
        d->m_format = format;
        emit labelFormatChanged(format);
        emit update();
    }
}

QString QDateTimeAxis::labelFormat() const
{
    Q_D(const QDateTimeAxis);
    return d->m_format;
}

qreal QDateTimeAxis::tickInterval() const
{
    Q_D(const QDateTimeAxis);
    return d->m_tickInterval;
}

void QDateTimeAxis::setTickInterval(qreal newTickInterval)
{
    Q_D(QDateTimeAxis);

    if (newTickInterval < 0.0)
        newTickInterval = 0.0;

    if (qFuzzyCompare(d->m_tickInterval, newTickInterval))
        return;
    d->m_tickInterval = newTickInterval;
    emit tickIntervalChanged();
    emit update();
}

int QDateTimeAxis::subTickCount() const
{
    Q_D(const QDateTimeAxis);
    return d->m_subTickCount;
}

void QDateTimeAxis::setSubTickCount(int newSubTickCount)
{
    Q_D(QDateTimeAxis);

    if (newSubTickCount < 0.0)
        newSubTickCount = 0.0;

    if (d->m_subTickCount == newSubTickCount)
        return;
    d->m_subTickCount = newSubTickCount;
    emit subTickCountChanged();
    emit update();
}

///////////////////////////////////////////////////////////////////////////////

QDateTimeAxisPrivate::QDateTimeAxisPrivate() {}

QDateTimeAxisPrivate::~QDateTimeAxisPrivate() {}

void QDateTimeAxisPrivate::setMin(const QVariant &min)
{
    Q_Q(QDateTimeAxis);
    if (min.canConvert<QDateTime>())
        q->setMin(min.toDateTime());
}

void QDateTimeAxisPrivate::setMax(const QVariant &max)
{
    Q_Q(QDateTimeAxis);
    if (max.canConvert<QDateTime>())
        q->setMax(max.toDateTime());
}

void QDateTimeAxisPrivate::setRange(const QVariant &min, const QVariant &max)
{
    Q_Q(QDateTimeAxis);
    if (min.canConvert<QDateTime>() && max.canConvert<QDateTime>())
        q->setRange(min.toDateTime(), max.toDateTime());
}

void QDateTimeAxisPrivate::setRange(qreal min, qreal max)
{
    Q_Q(QDateTimeAxis);

    bool changed = false;

    if (m_min != min) {
        m_min = min;
        changed = true;
        emit q->minChanged(QDateTime::fromMSecsSinceEpoch(min, QTimeZone::UTC));
    }

    if (m_max != max) {
        m_max = max;
        changed = true;
        emit q->maxChanged(QDateTime::fromMSecsSinceEpoch(max, QTimeZone::UTC));
    }

    if (changed)
        emit q->rangeChanged(min, max);
}

QT_END_NAMESPACE
