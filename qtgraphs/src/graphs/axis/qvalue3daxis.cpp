// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qvalue3daxis_p.h"
#include "qvalue3daxisformatter_p.h"

QT_BEGIN_NAMESPACE

/*!
 * \class QValue3DAxis
 * \inmodule QtGraphs
 * \brief The QValue3DAxis class manipulates an axis of a graph.
 *
 * A value axis can be given a range of values and segment and subsegment
 * counts to divide the range into.
 *
 * Labels are drawn between each segment. Grid lines are drawn between each segment and each
 * subsegment. \note If visible, there will always be at least two grid lines and labels indicating
 * the minimum and the maximum values of the range, as there is always at least one segment.
 */

/*!
 * \qmltype ValueAxis3D
 * \inqmlmodule QtGraphs
 * \ingroup graphs_qml
 * \instantiates QValue3DAxis
 * \inherits AbstractAxis3D
 * \brief Manipulates an axis of a graph.
 *
 * This type provides an axis that can be given a range of values and segment and subsegment
 * counts to divide the range into.
 */


/*!
 * \qmlproperty int ValueAxis3D::segmentCount
 *
 * The number of segments on the axis. This indicates how many labels are drawn. The number
 * of grid lines to be drawn is calculated with the following formula:
 * \c {segments * subsegments + 1}.
 * The preset default is \c 5. The value cannot be below \c 1.
 */

/*!
 * \qmlproperty int ValueAxis3D::subSegmentCount
 *
 * The number of subsegments inside each segment on the axis. Grid lines are drawn between
 * each subsegment, in addition to each segment.
 * The preset default is \c 1. The value cannot be below \c 1.
 */

/*!
 * \qmlproperty string ValueAxis3D::labelFormat
 *
 * The label format to be used for the labels on this axis.
 *
 * The format string supports the following conversion specifiers, length
 * modifiers, and flags provided by \c printf() in the standard C++ library:
 * d, i, o, x, X, f, F, e, E, g, G, c.
 *
 * If AbstractGraph3D::locale is anything else than \c{"C"}, the supported
 * specifiers are limited to: d, e, E, f, g, G, and i. Also, only the precision
 * modifier is supported. The rest of the formatting comes from the default
 * \l [QML] Locale of the application.
 *
 * \sa AbstractGraph3D::locale
 */

/*!
 * \qmlproperty ValueAxis3DFormatter ValueAxis3D::formatter
 *
 * The axis formatter to be used. Any existing formatter is deleted when a new formatter
 * is set.
 *
 */

/*!
 * \qmlproperty bool ValueAxis3D::reversed
 *
 * If \c{true}, the axis will be rendered in reverse. That is, the positions of
 * the minimum and maximum values are swapped when the graph is rendered. This
 * property does not affect the actual minimum and maximum values of the axis.
 */

/*!
 * Constructs QValue3DAxis with the given \a parent.
 */
QValue3DAxis::QValue3DAxis(QObject *parent) :
    QAbstract3DAxis(new QValue3DAxisPrivate(this), parent)
{
    setFormatter(new QValue3DAxisFormatter);
}

/*!
 * Destroys QValue3DAxis.
 */
QValue3DAxis::~QValue3DAxis()
{
}

/*!
 * \property QValue3DAxis::segmentCount
 *
 * \brief The number of segments on the axis.
 *
 * This indicates how many labels are drawn. The number
 * of grid lines to be drawn is calculated with formula: \c {segments * subsegments + 1}.
 * The preset default is \c 5. The value cannot be below \c 1.
 *
 * \sa setSubSegmentCount()
 */
void QValue3DAxis::setSegmentCount(int count)
{
    Q_D(QValue3DAxis);
    if (count <= 0) {
        qWarning() << "Warning: Illegal segment count automatically adjusted to a legal one:"
                   << count << "-> 1";
        count = 1;
    }
    if (d->m_segmentCount != count) {
        d->m_segmentCount = count;
        d->emitLabelsChanged();
        emit segmentCountChanged(count);
    }
}

int QValue3DAxis::segmentCount() const
{
    const Q_D(QValue3DAxis);
    return d->m_segmentCount;
}

/*!
 * \property QValue3DAxis::subSegmentCount
 *
 * \brief The number of subsegments inside each segment on the axis.
 *
 * Grid lines are drawn between
 * each subsegment, in addition to each segment.
 * The preset default is \c 1. The value cannot be below \c 1.
 *
 * \sa setSegmentCount()
 */
void QValue3DAxis::setSubSegmentCount(int count)
{
    Q_D(QValue3DAxis);
    if (count <= 0) {
        qWarning() << "Warning: Illegal subsegment count automatically adjusted to a legal one:"
                   << count << "-> 1";
        count = 1;
    }
    if (d->m_subSegmentCount != count) {
        d->m_subSegmentCount = count;
        emit subSegmentCountChanged(count);
    }
}

int QValue3DAxis::subSegmentCount() const
{
    const Q_D(QValue3DAxis);
    return d->m_subSegmentCount;
}

/*!
 * \property QValue3DAxis::labelFormat
 *
 * \brief The label format to be used for the labels on this axis.
 *
 * The format string supports the following conversion specifiers, length
 * modifiers, and flags provided by \c printf() in the standard C++ library:
 * d, i, o, x, X, f, F, e, E, g, G, c.
 *
 * If QAbstract3DGraph::locale is anything else than \c{"C"}, the supported
 * specifiers are limited to: d, e, E, f, g, G, and i. Also, only the precision
 * modifier is supported. The rest of the formatting comes from the default
 * QLocale of the application.
 *
 * Usage example:
 *
 * \c {axis->setLabelFormat("%.2f mm");}
 *
 * \sa formatter, QAbstract3DGraph::locale
 */
void QValue3DAxis::setLabelFormat(const QString &format)
{
    Q_D(QValue3DAxis);
    if (d->m_labelFormat != format) {
        d->m_labelFormat = format;
        d->emitLabelsChanged();
        emit labelFormatChanged(format);
    }
}

QString QValue3DAxis::labelFormat() const
{
    const Q_D(QValue3DAxis);
    return d->m_labelFormat;
}

/*!
 * \property QValue3DAxis::formatter
 *
 * \brief The axis formatter to be used.
 *
 * Any existing formatter is deleted when a new formatter
 * is set.
 */
void QValue3DAxis::setFormatter(QValue3DAxisFormatter *formatter)
{
    Q_ASSERT(formatter);
    Q_D(QValue3DAxis);

    if (formatter != d->m_formatter) {
        delete d->m_formatter;
        d->m_formatter = formatter;
        formatter->setParent(this);
        formatter->setAxis(this);
        emit formatterChanged(formatter);
        emit formatterDirty();
    }
}

QValue3DAxisFormatter *QValue3DAxis::formatter() const
{
    const Q_D(QValue3DAxis);
    return d->m_formatter;
}

/*!
 * \property QValue3DAxis::reversed
 *
 * \brief Whether the axis is rendered in reverse.
 *
 * If \c{true}, the axis will be rendered in reverse, i.e. the positions of minimum and maximum
 * values are swapped when the graph is rendered. This property doesn't affect the actual
 * minimum and maximum values of the axis.
 */
void QValue3DAxis::setReversed(bool enable)
{
    Q_D(QValue3DAxis);
    if (d->m_reversed != enable) {
        d->m_reversed = enable;
        emit reversedChanged(enable);
    }
}

bool QValue3DAxis::reversed() const
{
    const Q_D(QValue3DAxis);
    return d->m_reversed;
}

void QValue3DAxis::recalculate()
{
    formatter()->d_func()->recalculate();
}

int QValue3DAxis::gridSize()
{
    return formatter()->gridPositions().size();
}

int QValue3DAxis::subGridSize()
{
    return formatter()->subGridPositions().size();
}

float QValue3DAxis::gridPositionAt(int gridLine)
{
    return formatter()->gridPositions().at(gridLine);
}

float QValue3DAxis::subGridPositionAt(int gridLine)
{
    return formatter()->subGridPositions().at(gridLine);
}

float QValue3DAxis::labelPositionAt(int index)
{
    return formatter()->labelPositions().at(index);
}

float QValue3DAxis::positionAt(float x)
{
    return formatter()->positionAt(x);
}

QString QValue3DAxis::stringForValue(float x)
{
    return formatter()->stringForValue(x, labelFormat());
}

QValue3DAxisPrivate::QValue3DAxisPrivate(QValue3DAxis *q)
    : QAbstract3DAxisPrivate(q, QAbstract3DAxis::AxisTypeValue),
      m_segmentCount(5),
      m_subSegmentCount(1),
      m_labelFormat(Utils::defaultLabelFormat()),
      m_labelsDirty(true),
      m_formatter(0),
      m_reversed(false)
{
}

QValue3DAxisPrivate::~QValue3DAxisPrivate()
{
}

void QValue3DAxisPrivate::setRange(float min, float max, bool suppressWarnings)
{
    bool dirty = (min != m_min || max != m_max);

    QAbstract3DAxisPrivate::setRange(min, max, suppressWarnings);

    if (dirty)
        emitLabelsChanged();
}

void QValue3DAxisPrivate::setMin(float min)
{
    bool dirty = (min != m_min);

    QAbstract3DAxisPrivate::setMin(min);

    if (dirty)
        emitLabelsChanged();
}

void QValue3DAxisPrivate::setMax(float max)
{
    bool dirty = (max != m_max);

    QAbstract3DAxisPrivate::setMax(max);

    if (dirty)
        emitLabelsChanged();
}

void QValue3DAxisPrivate::emitLabelsChanged()
{
    Q_Q(QValue3DAxis);
    m_labelsDirty = true;
    emit q->labelsChanged();
}

void QValue3DAxisPrivate::emitFormatterDirty()
{
    Q_Q(QValue3DAxis);
    m_labelsDirty = true;
    emit q->formatterDirty();
}

void QValue3DAxisPrivate::updateLabels()
{
    if (!m_labelsDirty)
        return;

    m_labelsDirty = false;

    m_formatter->d_func()->recalculate();

    m_labels = m_formatter->labelStrings();
}

bool QValue3DAxisPrivate::allowZero()
{
    return m_formatter->allowZero();
}

bool QValue3DAxisPrivate::allowNegatives()
{
    return m_formatter->allowNegatives();
}

bool QValue3DAxisPrivate::allowMinMaxSame()
{
    return false;
}

QT_END_NAMESPACE
