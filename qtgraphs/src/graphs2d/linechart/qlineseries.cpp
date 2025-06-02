// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGraphs/qlineseries.h>
#include <private/qgraphsview_p.h>
#include <private/qlineseries_p.h>
#include <private/qxypoint_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QLineSeries
    \inmodule QtGraphs
    \ingroup graphs_2D
    \brief The QLineSeries class presents data in line graphs.

    A line graph is used to show information as a series of data points
    connected by straight lines.
*/
/*!
    \qmltype LineSeries
    \nativetype QLineSeries
    \inqmlmodule QtGraphs
    \ingroup graphs_qml_2D
    \inherits XYSeries

    \brief Presents data in line graphs.

    A line graph is used to show information as a series of data points
    connected by straight lines.

    \image graphs2d-line.png

    LineSeries uses mostly the same API as ScatterSeries so see ScatterSeries
    documentation for further usage examples.

    \sa ScatterSeries
*/

/*!
    \qmlproperty real LineSeries::width
    The width of the line. By default, the width is 2.0. Widths lower than 0
    are invalid and are automatically set to 0.
*/

/*!
    \qmlproperty Qt::PenCapStyle LineSeries::capStyle
    Controls the cap style of the line. Set to one of \l{Qt::FlatCap}{Qt.FlatCap},
    \l{Qt::SquareCap}{Qt.SquareCap} or \l{Qt::RoundCap}{Qt.RoundCap}. By
    default the cap style is Qt.SquareCap. Invalid values are automatically set
    to the default value.

    \sa Qt::PenCapStyle
*/

/*!
    \qmlproperty Component LineSeries::pointDelegate
    Marks the point with the given QML component.

    \code
        pointDelegate: Image {
            source: "images/happy_box.png"
        }
    \endcode
*/

/*!
    \qmlsignal LineSeries::widthChanged()
    This signal is emitted when the line series width changes.
*/

/*!
    \qmlsignal LineSeries::capStyleChanged()
    This signal is emitted when the line series cap style changes.
*/

QLineSeries::QLineSeries(QObject *parent)
    : QXYSeries(*(new QLineSeriesPrivate()), parent)
{}

QLineSeries::~QLineSeries() {}

QLineSeries::QLineSeries(QLineSeriesPrivate &dd, QObject *parent)
    : QXYSeries(dd, parent)
{}

void QLineSeries::componentComplete()
{
    Q_D(QLineSeries);

    for (auto *child : children()) {
        if (auto point = qobject_cast<QXYPoint *>(child))
            append(point->x(), point->y());
    }

    if (d->m_graphTransition)
        d->m_graphTransition->initialize();

    QAbstractSeries::componentComplete();
}

QAbstractSeries::SeriesType QLineSeries::type() const
{
    return QAbstractSeries::SeriesType::Line;
}

QLineSeriesPrivate::QLineSeriesPrivate() {}

qreal QLineSeries::width() const
{
    Q_D(const QLineSeries);
    return d->m_width;
}

void QLineSeries::setWidth(qreal newWidth)
{
    Q_D(QLineSeries);
    if (newWidth < 0.0)
        newWidth = 0.0;
    if (qFuzzyCompare(d->m_width, newWidth))
        return;
    d->m_width = newWidth;
    emit widthChanged();
    emit update();
}

Qt::PenCapStyle QLineSeries::capStyle() const
{
    Q_D(const QLineSeries);
    return d->m_capStyle;
}

void QLineSeries::setCapStyle(Qt::PenCapStyle newCapStyle)
{
    Q_D(QLineSeries);
    Qt::PenCapStyle validCapStyle = newCapStyle;
    if (validCapStyle != Qt::PenCapStyle::FlatCap && validCapStyle != Qt::PenCapStyle::SquareCap
        && validCapStyle != Qt::PenCapStyle::RoundCap
        && validCapStyle != Qt::PenCapStyle::MPenCapStyle) {
        validCapStyle = Qt::PenCapStyle::SquareCap;
    }
    if (d->m_capStyle == validCapStyle)
        return;
    d->m_capStyle = validCapStyle;
    emit capStyleChanged();
    emit update();
}

QT_END_NAMESPACE
