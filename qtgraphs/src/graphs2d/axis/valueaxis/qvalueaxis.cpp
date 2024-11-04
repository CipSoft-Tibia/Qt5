// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <private/qvalueaxis_p.h>
#include <private/charthelpers_p.h>
#include <QtCore/QtMath>

QT_BEGIN_NAMESPACE
/*!
    \class QValueAxis
    \inmodule QtGraphs
    \ingroup graphs_2D
    \brief The QValueAxis class adds values to a graph's axes.

    A value axis can be set up to show an axis line with tick marks, grid lines, and shades.
    The values on the axis are drawn at the positions of tick marks.
*/
/*!
    \qmltype ValueAxis
    \instantiates QValueAxis
    \inqmlmodule QtGraphs
    \ingroup graphs_qml_2D
    \inherits AbstractAxis
    \brief Adds values to a graph's axes.

    The ValueAxis type can be set up to show an axis line with tick marks, grid lines, and shades.
    The values on the axis are drawn at the positions of tick marks.

    The following example code illustrates how to use the ValueAxis type:
    \code
    GraphsView {
        LineSeries {
            axisX: ValueAxis {
                max: 10
                tickInterval: 1
            }
            axisY: ValueAxis {
                min -20
                max: 40
            }
            // Add a few XYPoint data...
        }
    }
    \endcode
*/

/*!
  \property QValueAxis::min
  \brief The minimum value on the axis.

  When setting this property, the maximum value is adjusted if necessary, to ensure that
  the range remains valid.
  The default value is 0.0
*/
/*!
  \qmlproperty real ValueAxis::min
  The minimum value on the axis.

  When setting this property, the maximum value is adjusted if necessary, to ensure that
  the range remains valid.
  The default value is 0.0
*/

/*!
  \property QValueAxis::max
  \brief The maximum value on the axis.

  When setting this property, the minimum value is adjusted if necessary, to ensure that
  the range remains valid.
  The default value is 10.0
*/
/*!
  \qmlproperty real ValueAxis::max
  The maximum value on the axis.

  When setting this property, the minimum value is adjusted if necessary, to ensure that
  the range remains valid.
  The default value is 10.0
*/

/*!
  \property QValueAxis::minorTickCount
  \brief The number of minor tick marks on the axis. This indicates how many grid lines are drawn
  between major ticks on the graph. Labels are not drawn for minor ticks. The default value is 0.
*/
/*!
  \qmlproperty int ValueAxis::minorTickCount
  The number of minor tick marks on the axis. This indicates how many grid lines are drawn
  between major ticks on the graph. Labels are not drawn for minor ticks. The default value is 0.
*/

/*!
  \property QValueAxis::tickAnchor
  \brief The base value where the dynamically placed tick marks and labels are started from.
  The default value is 0.
 */
/*!
  \qmlproperty real ValueAxis::tickAnchor
  The base value where the dynamically placed tick marks and labels are started from.
  The default value is 0.
 */

/*!
  \property QValueAxis::tickInterval
  \brief The interval between dynamically placed tick marks and labels.
  The default value is 0, which means that intervals are automatically calculated
  based on the min and max range.
*/
/*!
  \qmlproperty real ValueAxis::tickInterval
  The interval between dynamically placed tick marks and labels.
  The default value is 0, which means that intervals are automatically calculated
  based on the min and max range.
*/

/*!
  \property QValueAxis::labelFormat
  \brief The label format of the axis.

  The format string supports the following conversion specifiers, length modifiers, and flags
  provided by \c printf() in the standard C++ library: d, i, o, x, X, f, F, e, E, g, G, c.

  The default value is empty, in which case 'f' format is used.

  \sa QString::asprintf()
*/
/*!
  \qmlproperty string ValueAxis::labelFormat

  The format string supports the following conversion specifiers, length modifiers, and flags
  provided by \c printf() in the standard C++ library: d, i, o, x, X, f, F, e, E, g, G, c.

  The default value is empty, in which case 'f' format is used.

  \sa QString::asprintf()
*/

/*!
  \property QValueAxis::labelDecimals
  \brief The number of decimals used for showing the labels. When set to -1, decimal amount
  is adjusted automatically based on the values range. The default value is -1.
*/
/*!
  \qmlproperty int ValueAxis::labelDecimals
  The number of decimals used for showing the labels. When set to -1, decimal amount
  is adjusted automatically based on the values range. The default value is -1.
*/

/*!
  \fn void QValueAxis::minChanged(qreal min)
  This signal is emitted when the minimum value of the axis, specified by \a min, changes.
*/

/*!
  \fn void QValueAxis::maxChanged(qreal max)
  This signal is emitted when the maximum value of the axis, specified by \a max, changes.
*/

/*!
  \fn void QValueAxis::minorTickCountChanged(int minorTickCount)
  This signal is emitted when the number of minor tick marks on the axis, specified by
  \a minorTickCount, changes.
*/

/*!
  \fn void QValueAxis::rangeChanged(qreal min, qreal max)
  This signal is emitted when the minimum or maximum value of the axis, specified by \a min
  and \a max, changes.
*/

/*!
    \qmlsignal ValueAxis::rangeChanged(string min, string max)
    This signal is emitted when \a min or \a max value of the axis changes.

    The corresponding signal handler is \c onRangeChanged.
*/

/*!
  \fn void QValueAxis::labelFormatChanged(const QString &format)
  This signal is emitted when the \a format of axis labels changes.
*/

/*!
  \fn void QValueAxis::labelDecimalsChanged(int decimals)
  This signal is emitted when the \a decimals amount of axis labels changes.
*/

/*!
  \fn void QValueAxis::tickAnchorChanged(qreal tickAnchor)
  This signal is emitted when the tick anchoring value, specified by
  \a tickAnchor, changes.
*/

/*!
  \fn void QValueAxis::tickIntervalChanged(qreal tickInterval)
  This signal is emitted when the tick interval value, specified by
  \a tickInterval, changes.
*/

/*!
    Constructs an axis object that is a child of \a parent.
*/
QValueAxis::QValueAxis(QObject *parent) :
    QAbstractAxis(*new QValueAxisPrivate(this), parent)
{

}

/*!
    \internal
*/
QValueAxis::QValueAxis(QValueAxisPrivate &d, QObject *parent)
    : QAbstractAxis(d, parent)
{

}

/*!
    Destroys the object.
*/
QValueAxis::~QValueAxis()
{
    Q_D(QValueAxis);
    if (d->m_graph)
        d->m_graph->removeAxis(this);
}

void QValueAxis::setMin(qreal min)
{
    Q_D(QValueAxis);
    setRange(min, qMax(d->m_max, min));
}

qreal QValueAxis::min() const
{
    Q_D(const QValueAxis);
    return d->m_min;
}

void QValueAxis::setMax(qreal max)
{
    Q_D(QValueAxis);
    setRange(qMin(d->m_min, max), max);
}

qreal QValueAxis::max() const
{
    Q_D(const QValueAxis);
    return d->m_max;
}

/*!
  Sets the range from \a min to \a max on the axis.
  If \a min is greater than \a max, this function returns without making any changes.
*/
void QValueAxis::setRange(qreal min, qreal max)
{
    Q_D(QValueAxis);
    d->setRange(min,max);
    emit update();
}

void QValueAxis::setMinorTickCount(int count)
{
    Q_D(QValueAxis);
    if (d->m_minorTickCount != count && count >= 0) {
        d->m_minorTickCount = count;
        emit update();
        emit minorTickCountChanged(count);
    }
}

int QValueAxis::minorTickCount() const
{
    Q_D(const QValueAxis);
    return d->m_minorTickCount;
}

void QValueAxis::setTickAnchor(qreal anchor)
{
    Q_D(QValueAxis);
    if (d->m_tickAnchor != anchor) {
        d->m_tickAnchor = anchor;
        emit update();
        emit tickAnchorChanged(anchor);
    }
}

qreal QValueAxis::tickAnchor() const
{
    Q_D(const QValueAxis);
    return d->m_tickAnchor;
}

void QValueAxis::setTickInterval(qreal interval)
{
    Q_D(QValueAxis);
    if (d->m_tickInterval != interval) {
        d->m_tickInterval = interval;
        emit update();
        emit tickIntervalChanged(interval);
    }
}

qreal QValueAxis::tickInterval() const
{
    Q_D(const QValueAxis);
    return d->m_tickInterval;
}

void QValueAxis::setLabelFormat(const QString &format)
{
    Q_D(QValueAxis);
    d->m_format = format;
    emit update();
    emit labelFormatChanged(format);
}

QString QValueAxis::labelFormat() const
{
    Q_D(const QValueAxis);
    return d->m_format;
}

void QValueAxis::setLabelDecimals(int decimals)
{
    Q_D(QValueAxis);
    if (d->m_decimals != decimals) {
        d->m_decimals = decimals;
        emit update();
        emit labelDecimalsChanged(decimals);
    }
}

int QValueAxis::labelDecimals() const
{
    Q_D(const QValueAxis);
    return d->m_decimals;
}

/*!
  Returns the type of the axis.
*/
QAbstractAxis::AxisType QValueAxis::type() const
{
    return AxisTypeValue;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

QValueAxisPrivate::QValueAxisPrivate(QValueAxis *q)
    : QAbstractAxisPrivate(q),
      m_min(0),
      m_max(10),
      m_minorTickCount(0),
      m_format(),
      m_decimals(-1),
      m_tickAnchor(0.0),
      m_tickInterval(0.0)
{

}

QValueAxisPrivate::~QValueAxisPrivate()
{

}

void QValueAxisPrivate::setMin(const QVariant &min)
{
    Q_Q(QValueAxis);
    bool ok;
    qreal value = min.toReal(&ok);
    if (ok)
        q->setMin(value);
}

void QValueAxisPrivate::setMax(const QVariant &max)
{
    Q_Q(QValueAxis);
    bool ok;
    qreal value = max.toReal(&ok);
    if (ok)
        q->setMax(value);
}

void QValueAxisPrivate::setRange(const QVariant &min, const QVariant &max)
{
    Q_Q(QValueAxis);
    bool ok1;
    bool ok2;
    qreal value1 = min.toReal(&ok1);
    qreal value2 = max.toReal(&ok2);
    if (ok1 && ok2)
        q->setRange(value1, value2);
}

void QValueAxisPrivate::setRange(qreal min, qreal max)
{
    Q_Q(QValueAxis);
    bool changed = false;

    if (min > max)
        return;

    if (!isValidValue(min, max)) {
        qWarning() << "Attempting to set invalid range for value axis: ["
                   << min << " - " << max << "]";
        return;
    }

    if (m_min != min) {
        m_min = min;
        changed = true;
        emit q->minChanged(min);
    }

    if (m_max != max) {
        m_max = max;
        changed = true;
        emit q->maxChanged(max);
    }

    if (changed) {
        emit rangeChanged(min,max);
        emit q->rangeChanged(min, max);
    }
}

QT_END_NAMESPACE

#include "moc_qvalueaxis.cpp"
#include "moc_qvalueaxis_p.cpp"
