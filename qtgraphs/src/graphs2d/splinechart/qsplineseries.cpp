// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGraphs/qsplineseries.h>
#include <private/qgraphsview_p.h>
#include <private/qsplineseries_p.h>
#include <private/qxypoint_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QSplineSeries
    \inmodule QtGraphs
    \ingroup graphs_2D
    \brief QSplineSeries presents data in spline graphs.

    The graph displays smooth spline segments that moves through all the points
    defined in the graph.
 */
/*!
    \qmltype SplineSeries
    \nativetype QSplineSeries
    \inqmlmodule QtGraphs
    \ingroup graphs_qml_2D
    \inherits XYSeries

    \brief SplineSeries presents data in spline graphs.

    The graph displays smooth spline segments that moves through all the points
    defined in the graph.

    \image graphs2d-spline.png
*/

/*!
    \qmlproperty real SplineSeries::width
    The width of the line. By default, the width is 2.0.
*/

/*!
    \qmlproperty Qt::PenCapStyle SplineSeries::capStyle
    Controls the cap style of the line. Set to one of \l{Qt::FlatCap}{Qt.FlatCap},
    \l{Qt::SquareCap}{Qt.SquareCap} or \l{Qt::RoundCap}{Qt.RoundCap}. By
    default the cap style is Qt.SquareCap.

    \sa Qt::PenCapStyle
*/

/*!
    \qmlsignal SplineSeries::widthChanged()
    This signal is emitted when the spline series width changes.
*/

/*!
    \qmlsignal SplineSeries::capStyleChanged()
    This signal is emitted when the spline series cap style changes.
*/

QSplineSeries::QSplineSeries(QObject *parent)
    : QXYSeries(*(new QSplineSeriesPrivate()), parent)
{}

QSplineSeries::~QSplineSeries() {}

QSplineSeries::QSplineSeries(QSplineSeriesPrivate &dd, QObject *parent)
    : QXYSeries(dd, parent)
{}

void QSplineSeries::componentComplete()
{
    Q_D(QSplineSeries);

    for (auto *child : children()) {
        if (auto point = qobject_cast<QXYPoint *>(child))
            append(point->x(), point->y());
    }

    d->calculateSplinePoints();

    if (d->m_graphTransition)
        d->m_graphTransition->initialize();

    connect(this, &QSplineSeries::pointAdded, this, [d]([[maybe_unused]] int index) {
        d->calculateSplinePoints();
    });

    connect(this, &QSplineSeries::pointRemoved, this, [d]([[maybe_unused]] int index) {
        d->calculateSplinePoints();
    });

    connect(this, &QSplineSeries::pointsRemoved, this
            , [d]([[maybe_unused]] int index, [[maybe_unused]] int count) {
       d->calculateSplinePoints();
   });

    connect(this, &QSplineSeries::pointReplaced, this, [d]([[maybe_unused]] int index) {
        d->calculateSplinePoints();
    });

    connect(this, &QSplineSeries::pointsReplaced, this, [d]() { d->calculateSplinePoints(); });

    QAbstractSeries::componentComplete();
}

QAbstractSeries::SeriesType QSplineSeries::type() const
{
    return QAbstractSeries::SeriesType::Spline;
}

QList<QPointF> &QSplineSeries::getControlPoints()
{
    Q_D(QSplineSeries);
    return d->m_controlPoints;
}

qreal QSplineSeries::width() const
{
    Q_D(const QSplineSeries);
    return d->m_width;
}

void QSplineSeries::setWidth(qreal newWidth)
{
    Q_D(QSplineSeries);

    if (newWidth < 0)
        newWidth = 0;

    if (qFuzzyCompare(d->m_width, newWidth))
        return;
    d->m_width = newWidth;
    emit widthChanged();
    emit update();
}

Qt::PenCapStyle QSplineSeries::capStyle() const
{
    Q_D(const QSplineSeries);
    return d->m_capStyle;
}

void QSplineSeries::setCapStyle(Qt::PenCapStyle newCapStyle)
{
    Q_D(QSplineSeries);
    if (d->m_capStyle == newCapStyle)
        return;
    d->m_capStyle = newCapStyle;
    emit capStyleChanged();
    emit update();
}

QSplineSeriesPrivate::QSplineSeriesPrivate()
    : QXYSeriesPrivate()
    , m_width(1.0)
    , m_capStyle(Qt::PenCapStyle::SquareCap)
    , m_controlPoints()
{}

void QSplineSeriesPrivate::calculateSplinePoints()
{
    if (m_points.size() == 0) {
        m_controlPoints.clear();
        return;
    } else if (m_points.size() == 1) {
        m_controlPoints = {m_points[0], m_points[0]};
        return;
    }

    QList<QPointF> controlPoints;
    controlPoints.resize(m_points.size() * 2 - 2);

    qsizetype n = m_points.size() - 1;

    if (n == 1) {
        //for n==1
        controlPoints[0].setX((2 * m_points[0].x() + m_points[1].x()) / 3);
        controlPoints[0].setY((2 * m_points[0].y() + m_points[1].y()) / 3);
        controlPoints[1].setX(2 * controlPoints[0].x() - m_points[0].x());
        controlPoints[1].setY(2 * controlPoints[0].y() - m_points[0].y());
        m_controlPoints = controlPoints;
    }

    // Calculate first Bezier control points
    // Set of equations for P0 to Pn points.
    //
    //  |   2   1   0   0   ... 0   0   0   ... 0   0   0   |   |   P1_1    |   |   P1 + 2 * P0             |
    //  |   1   4   1   0   ... 0   0   0   ... 0   0   0   |   |   P1_2    |   |   4 * P1 + 2 * P2         |
    //  |   0   1   4   1   ... 0   0   0   ... 0   0   0   |   |   P1_3    |   |   4 * P2 + 2 * P3         |
    //  |   .   .   .   .   .   .   .   .   .   .   .   .   |   |   ...     |   |   ...                     |
    //  |   0   0   0   0   ... 1   4   1   ... 0   0   0   | * |   P1_i    | = |   4 * P(i-1) + 2 * Pi     |
    //  |   .   .   .   .   .   .   .   .   .   .   .   .   |   |   ...     |   |   ...                     |
    //  |   0   0   0   0   0   0   0   0   ... 1   4   1   |   |   P1_(n-1)|   |   4 * P(n-2) + 2 * P(n-1) |
    //  |   0   0   0   0   0   0   0   0   ... 0   2   7   |   |   P1_n    |   |   8 * P(n-1) + Pn         |
    //
    QList<qreal> list;
    list.resize(n);

    list[0] = m_points[0].x() + 2 * m_points[1].x();

    for (int i = 1; i < n - 1; ++i)
        list[i] = 4 * m_points[i].x() + 2 * m_points[i + 1].x();

    list[n - 1] = (8 * m_points[n - 1].x() + m_points[n].x()) / 2.0;

    const QList<qreal> xControl = calculateControlPoints(list);

    list[0] = m_points[0].y() + 2 * m_points[1].y();

    for (int i = 1; i < n - 1; ++i)
        list[i] = 4 * m_points[i].y() + 2 * m_points[i + 1].y();

    list[n - 1] = (8 * m_points[n - 1].y() + m_points[n].y()) / 2.0;

    const QList<qreal> yControl = calculateControlPoints(list);

    for (int i = 0, j = 0; i < n; ++i, ++j) {
        controlPoints[j].setX(xControl[i]);
        controlPoints[j].setY(yControl[i]);

        j++;

        if (i < n - 1) {
            controlPoints[j].setX(2 * m_points[i + 1].x() - xControl[i + 1]);
            controlPoints[j].setY(2 * m_points[i + 1].y() - yControl[i + 1]);
        } else {
            controlPoints[j].setX((m_points[n].x() + xControl[n - 1]) / 2);
            controlPoints[j].setY((m_points[n].y() + yControl[n - 1]) / 2);
        }
    }

    m_controlPoints = controlPoints;
}

QList<qreal> QSplineSeriesPrivate::calculateControlPoints(const QList<qreal> &list)
{
    QList<qreal> result;

    qsizetype count = list.size();
    result.resize(count);
    result[0] = list[0] / 2.0;

    QList<qreal> temp;
    temp.resize(count);
    temp[0] = 0;

    qreal b = 2.0;

    for (int i = 1; i < count; i++) {
        temp[i] = 1 / b;
        b = (i < count - 1 ? 4.0 : 3.5) - temp[i];
        result[i] = (list[i] - result[i - 1]) / b;
    }

    for (int i = 1; i < count; i++)
        result[count - i - 1] -= temp[count - i] * result[count - i];

    return result;
}

QT_END_NAMESPACE
