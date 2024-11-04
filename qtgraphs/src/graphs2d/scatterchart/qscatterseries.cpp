// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGraphs/qscatterseries.h>
#include <private/qxypoint_p.h>
#include <private/qscatterseries_p.h>
#include <private/qgraphsview_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QScatterSeries
    \inmodule QtGraphs
    \ingroup graphs_2D
    \brief The QScatterSeries class presents data in scatter graphs.

    The scatter data is displayed as a collection of points on the graph. For
    each point, two values are specified that determine its position on the
    horizontal axis and the vertical axis.
*/
/*!
    \qmltype ScatterSeries
    \instantiates QScatterSeries
    \inqmlmodule QtGraphs
    \ingroup graphs_qml_2D
    \inherits XYSeries

    \brief The ScatterSeries type presents data in scatter graphs.

    The scatter data is displayed as a collection of points on the graph. For
    each point, two values are specified that determine its position on the
    horizontal axis and the vertical axis.

    \image graphs2d-scatter.png

    You can represent scatter data by creating a ScatterSeries inside
    GraphsView. Axis types should be then defined for ScatterSeries
    using axisX and axisY properties. Finally data can be added
    to the graph by creating XYPoints as children for the ScatterSeries
    that define the x and y values of each point.

    \code
    GraphsView {
        anchors.fill: parent
        ScatterSeries {
            color: "#00ff00"
            axisX: ValueAxis {
                max: 3
            }
            axisY: ValueAxis {
                max: 3
            }

            XYPoint { x: 0.5; y: 0.5 }
            XYPoint { x: 1; y: 1 }
            XYPoint { x: 2; y: 2 }
            XYPoint { x: 2.5; y: 1.5 }
        }
    }
    \endcode

    Multiple scatter graphs can be created by adding multiple ScatterSeries
    as children of GraphsView. In such cases only one series should define
    the axis used as multiple definitions only override the earlier ones.

    \code
    GraphsView {
        anchors.fill: parent
        ScatterSeries {
            color: "#00ff00"
            axisX: ValueAxis {
                max: 3
            }
            axisY: ValueAxis {
                max: 3
            }

            XYPoint { x: 0.5; y: 0.5 }
            XYPoint { x: 1; y: 1 }
            XYPoint { x: 2; y: 2 }
            XYPoint { x: 2.5; y: 1.5 }
        }

        ScatterSeries {
            color: "#ff0000"
            XYPoint { x: 0.5; y: 3 }
            XYPoint { x: 1; y: 2 }
            XYPoint { x: 2; y: 2.5 }
            XYPoint { x: 2.5; y: 1 }
        }
    }
    \endcode
*/

/*!
    \qmlproperty Component ScatterSeries::pointMarker
    Marks points with the given QML component.

    \code
        pointMarker: Image {
            source: "images/happy_box.png"
        }
    \endcode
*/

QScatterSeries::QScatterSeries(QObject *parent)
    : QXYSeries(*new QScatterSeriesPrivate(this), parent)
{

}

QScatterSeries::QScatterSeries(QScatterSeriesPrivate &d, QObject *parent)
    : QXYSeries(d, parent)
{

}

void QScatterSeries::componentComplete()
{
    for (auto *child : children()) {
        if (auto point = qobject_cast<QXYPoint *>(child))
            append(point->x(), point->y());
    }
}

QScatterSeries::~QScatterSeries()
{
    Q_D(QScatterSeries);
    if (d->m_graph)
        d->m_graph->removeSeries(this);
}


QAbstractSeries::SeriesType QScatterSeries::type() const
{
    return QAbstractSeries::SeriesTypeScatter;
}

QScatterSeriesPrivate::QScatterSeriesPrivate(QScatterSeries *q)
    : QXYSeriesPrivate(q)
{
}


QT_END_NAMESPACE
