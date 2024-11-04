// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGraphs/qxyseries.h>
#include <private/qxyseries_p.h>
#include <private/charthelpers_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QXYSeries
    \inmodule QtGraphs
    \ingroup graphs_2D
    \brief The QXYSeries class is a parent class for all x & y series classes.

    In QXYSeries, data points are defined as a list of QPointF, defining X and Y positions.

 \sa QLineSeries, QScatterSeries
*/
/*!
    \qmltype XYSeries
    \instantiates QXYSeries
    \inqmlmodule QtGraphs
    \ingroup graphs_qml_2D
    \inherits AbstractSeries
    \brief A parent type for all x & y series types.

    In XYSeries, data points are defined as a list of point types, defining X and Y positions.
*/

/*!
    \fn void QXYSeries::pointReplaced(int index)
    This signal is emitted when a point is replaced at the position specified by
    \a index.
    \sa replace()
*/
/*!
    \qmlsignal XYSeries::pointReplaced(int index)
    This signal is emitted when a point is replaced at the position specified by
    \a index.

    The corresponding signal handler is \c onPointReplaced().
*/

QXYSeries::QXYSeries(QXYSeriesPrivate &d, QObject *parent)
    : QAbstractSeries(d, parent)
{
}

QXYSeries::~QXYSeries()
{
}

/*!
    Appends a point with the coordinates \a x and \a y to the series.
*/
void QXYSeries::append(qreal x, qreal y)
{
    append(QPointF(x, y));
}

/*!
    Appends a point with the coordinates \a point to the series.
*/
void QXYSeries::append(const QPointF &point)
{
    Q_D(QXYSeries);

    if (isValidValue(point)) {
        d->m_points << point;
        emit pointAdded(d->m_points.size() - 1);
    }
}

/*!
    Appends points with the coordinates \a points to the series.
*/
void QXYSeries::append(const QList<QPointF> &points)
{
    for (const QPointF &point : points)
        append(point);
}

/*!
    Replaces the point with the coordinates \a oldX and \a oldY with the point
    with the coordinates \a newX and \a newY. Does nothing if the old point does
    not exist.
*/
void QXYSeries::replace(qreal oldX, qreal oldY, qreal newX, qreal newY)
{
    replace(QPointF(oldX, oldY), QPointF(newX, newY));
}

/*!
    Replaces the point with the coordinates \a oldPoint with the point
    with the coordinates \a newPoint. Does nothing if the old point does
    not exist.
*/
void QXYSeries::replace(const QPointF &oldPoint, const QPointF &newPoint)
{
    Q_D(QXYSeries);
    int index = d->m_points.indexOf(oldPoint);
    if (index == -1)
        return;
    replace(index, newPoint);
}

/*!
    Replaces the point at the position specified by \a index with the point
    that has the coordinates \a newX and \a newY.
*/

void QXYSeries::replace(int index, qreal newX, qreal newY)
{
    replace(index, QPointF(newX, newY));
}

/*!
    Replaces the point at the position specified by \a index with the point
    that has the coordinates \a newPoint.
*/
void QXYSeries::replace(int index, const QPointF &newPoint)
{
    Q_D(QXYSeries);
    if (isValidValue(newPoint)) {
        d->m_points[index] = newPoint;
        emit pointReplaced(index);
    }
}

/*!
    Replaces the current points with the points specified by \a points
    \note This is much faster than replacing data points one by one, or first
    clearing all data, and then appending the new data. Emits \l pointReplaced
    when the points have been replaced.
*/
void QXYSeries::replace(const QList<QPointF> &points)
{
    Q_D(QXYSeries);
    d->m_points = points;
    emit pointsReplaced();
}

/*!
    Returns true if point at given \a index is among selected points and false otherwise.
    \note Selected points are drawn using the selected color if it was specified.
    \sa selectedPoints(), setPointSelected(), setSelectedColor()
 */
bool QXYSeries::isPointSelected(int index)
{
    Q_D(QXYSeries);
    return d->isPointSelected(index);
}

/*!
    Marks point at \a index as selected.
    \note Emits QXYSeries::selectedPointsChanged
    \sa setPointSelected()
 */
void QXYSeries::selectPoint(int index)
{
    setPointSelected(index, true);
}

/*!
    Deselects point at given \a index.
    \note Emits QXYSeries::selectedPointsChanged
    \sa setPointSelected()
 */
void QXYSeries::deselectPoint(int index)
{
    setPointSelected(index, false);
}

/*!
    Marks point at given \a index as either selected or deselected as specified by \a selected.
    \note Selected points are drawn using the selected color if it was specified. Emits QXYSeries::selectedPointsChanged
    \sa selectAllPoints(), setSelectedColor()
 */
void QXYSeries::setPointSelected(int index, bool selected)
{
    Q_D(QXYSeries);

    bool callSignal = false;
    d->setPointSelected(index, selected, callSignal);

    if (callSignal)
        emit selectedPointsChanged();
}

/*!
    Marks all points in the series as selected,
    \note Emits QXYSeries::selectedPointsChanged
    \sa setPointSelected()
 */
void QXYSeries::selectAllPoints()
{
    Q_D(QXYSeries);

    bool callSignal = false;
    for (int i = 0; i < d->m_points.size(); ++i)
        d->setPointSelected(i, true, callSignal);

    if (callSignal)
        emit selectedPointsChanged();
}

/*!
    Deselects all points in the series.
    \note Emits QXYSeries::selectedPointsChanged
    \sa setPointSelected()
 */
void QXYSeries::deselectAllPoints()
{
    Q_D(QXYSeries);

    bool callSignal = false;
    for (int i = 0; i < d->m_points.size(); ++i)
        d->setPointSelected(i, false, callSignal);

    if (callSignal)
        emit selectedPointsChanged();
}

/*!
    Marks multiple points passed in a \a indexes list as selected.
    \note Emits QXYSeries::selectedPointsChanged
    \sa setPointSelected()
 */
void QXYSeries::selectPoints(const QList<int> &indexes)
{
    Q_D(QXYSeries);

    bool callSignal = false;
    for (const int &index : indexes)
        d->setPointSelected(index, true, callSignal);

    if (callSignal)
        emit selectedPointsChanged();
}

/*!
    Marks multiple points passed in a \a indexes list as deselected.
    \note Emits QXYSeries::selectedPointsChanged
    \sa setPointSelected()
 */
void QXYSeries::deselectPoints(const QList<int> &indexes)
{
    Q_D(QXYSeries);

    bool callSignal = false;
    for (const int &index : indexes)
        d->setPointSelected(index, false, callSignal);

    if (callSignal)
        emit selectedPointsChanged();
}

/*!
    Changes selection state of points at given \a indexes to the opposite one.
    \note Emits QXYSeries::selectedPointsChanged
    \sa setPointSelected()
 */
void QXYSeries::toggleSelection(const QList<int> &indexes)
{
    Q_D(QXYSeries);

    bool callSignal = false;
    for (const int &index : indexes)
        d->setPointSelected(index, !isPointSelected(index), callSignal);

    if (callSignal)
        emit selectedPointsChanged();
}

/*!
    Returns a list of points indexes marked as selected.
    Selected points are visible regardless of points visibility.
    \sa setPointSelected()
 */
QList<int> QXYSeries::selectedPoints() const
{
    Q_D(const QXYSeries);
    return QList<int>(d->m_selectedPoints.begin(), d->m_selectedPoints.end());
}

/*!
    Removes the point with the coordinates \a x and \a y from the series. Does
    nothing if the point does not exist.
*/
void QXYSeries::remove(qreal x, qreal y)
{
    remove(QPointF(x, y));
}

/*!
    Removes the point with the coordinates \a point from the series. Does
    nothing if the point does not exist.
*/
void QXYSeries::remove(const QPointF &point)
{
    Q_D(QXYSeries);
    int index = d->m_points.indexOf(point);
    if (index == -1)
        return;
    remove(index);
}

/*!
    Removes the point at the position specified by \a index from the series.
*/
void QXYSeries::remove(int index)
{
    Q_D(QXYSeries);
    d->m_points.remove(index);

    bool callSignal = false;
    d->setPointSelected(index, false, callSignal);

    emit pointRemoved(index);
    if (callSignal)
        emit selectedPointsChanged();
}

/*!
    Removes the number of points specified by \a count from the series starting
    at the position specified by \a index.
*/
void QXYSeries::removePoints(int index, int count)
{
    // This function doesn't overload remove as there is chance for it to get mixed up with
    // remove(qreal, qreal) overload in some implicit casting cases.
    Q_D(QXYSeries);
    if (count > 0) {
        d->m_points.remove(index, count);

        bool callSignal = false;
        if (!d->m_selectedPoints.empty()) {
            QSet<int> selectedAfterRemoving;

            for (const int &selectedPointIndex : std::as_const(d->m_selectedPoints)) {
                if (selectedPointIndex < index) {
                    selectedAfterRemoving << selectedPointIndex;
                } else if (selectedPointIndex >= index + count) {
                    selectedAfterRemoving << selectedPointIndex - count;
                    callSignal = true;
                } else {
                    callSignal = true;
                }
            }

            d->m_selectedPoints = selectedAfterRemoving;
        }

        emit pointsRemoved(index, count);
        if (callSignal)
            emit selectedPointsChanged();
    }
}

/*!
    Inserts a point with the coordinates \a point to the position specified
    by \a index in the series. If the index is 0 or less than 0, the point is
    prepended to the list of points. If the index is equal to or greater than
    than the number of points in the series, the point is appended to the
    list of points.
*/
void QXYSeries::insert(int index, const QPointF &point)
{
    Q_D(QXYSeries);
    if (isValidValue(point)) {
        index = qMax(0, qMin(index, d->m_points.size()));

        d->m_points.insert(index, point);

        bool callSignal = false;
        if (!d->m_selectedPoints.isEmpty()) {
            // if point was inserted we need to move already selected points by 1
            QSet<int> selectedAfterInsert;
            for (const auto &value : std::as_const(d->m_selectedPoints)) {
                if (value >= index) {
                    selectedAfterInsert << value + 1;
                    callSignal = true;
                } else {
                    selectedAfterInsert << value;
                }
            }
            d->m_selectedPoints = selectedAfterInsert;
        }

        emit pointAdded(index);
        if (callSignal)
            emit selectedPointsChanged();
    }
}

/*!
    Removes all points from the series.
*/
void QXYSeries::clear()
{
    Q_D(QXYSeries);
    removePoints(0, d->m_points.size());
}

/*!
    Returns the points in the series.
*/
QList<QPointF> QXYSeries::points() const
{
    Q_D(const QXYSeries);
    return d->m_points;
}

/*!
    Returns the point at the position specified by \a index. Returns (0, 0) if
    the index is not valid.
*/
const QPointF &QXYSeries::at(int index) const
{
    Q_D(const QXYSeries);
    return d->m_points.at(index);
}

/*!
    Returns the number of data points in a series.
*/
int QXYSeries::count() const
{
    Q_D(const QXYSeries);
    return d->m_points.size();
}

/*!
    \property QXYSeries::color
    \brief The main color of the series. For QLineSeries this means the line color and
    for QScatterSeries the color of the point.
*/
/*!
    \qmlproperty color XYSeries::color
    The main color of the series. For LineSeries this means the line color and
    for ScatterSeries the color of the point
*/
void QXYSeries::setColor(const QColor &newColor)
{
    Q_D(QXYSeries);
    if (color() != newColor) {
        d->m_color = newColor;
        emit colorChanged(newColor);
    }
}

QColor QXYSeries::color() const
{
    Q_D(const QXYSeries);
    return d->m_color;
}

/*!
    \property QXYSeries::selectedColor
    \brief The main color of the selected series. For QLineSeries this means the line color and
    for QScatterSeries the color of the point.
*/
/*!
    \qmlproperty color XYSeries::selectedColor
    The main color of the selected series. For LineSeries this means the line color and
    for ScatterSeries the color of the point
*/
void QXYSeries::setSelectedColor(const QColor &color)
{
    Q_D(QXYSeries);
    if (selectedColor() != color) {
        d->m_selectedColor = color;
        emit selectedColorChanged(color);
    }
}

QColor QXYSeries::selectedColor() const
{
    Q_D(const QXYSeries);
    return d->m_selectedColor;
}

/*!
    \property QXYSeries::markerSize
    \brief The size of the point marker in pixels when using a default circle as a marker.
    When using \l pointMarker, the size is defined by the \l pointMarker conponent.
*/
/*!
    \qmlproperty real XYSeries::markerSize
    The size of the pointMarker in pixels when using a default circle as a marker.
    When using \l pointMarker, the size is defined by the \l pointMarker conponent.
*/
void QXYSeries::setMarkerSize(qreal size)
{
    Q_D(QXYSeries);

    if (!qFuzzyCompare(d->m_markerSize, size)) {
        d->m_markerSizeDefault = false;
        d->setMarkerSize(size);
        emit d->seriesUpdated();
        emit markerSizeChanged(size);
    }
}

qreal QXYSeries::markerSize() const
{
    Q_D(const QXYSeries);
    return d->m_markerSize;
}

/*!
    \property QXYSeries::axisX
    \brief X-axis of the series.

    The x-axis used for the series. This should be QValueAxis.
*/
/*!
    \qmlproperty AbstractAxis XYSeries::axisX
    The x-axis used for the series. This should be ValueAxis.
    \sa axisY
*/
QAbstractAxis *QXYSeries::axisX() const
{
    Q_D(const QXYSeries);
    return d->m_axisX;
}

void QXYSeries::setAxisX(QAbstractAxis *axis)
{
    Q_D(QXYSeries);
    detachAxis(d->m_axisX);
    d->m_axisX = axis;
    if (axis) {
        axis->setOrientation(Qt::Horizontal);
        attachAxis(axis);
    }
    emit update();
}

/*!
    \property QXYSeries::axisY
    \brief Y-axis of the series.

    The y-axis used for the series. This should be QValueAxis.
*/
/*!
    \qmlproperty AbstractAxis XYSeries::axisY
    The y-axis used for the series. This should be ValueAxis.
    \sa axisX
*/
QAbstractAxis *QXYSeries::axisY() const
{
    Q_D(const QXYSeries);
    return d->m_axisY;
}

void QXYSeries::setAxisY(QAbstractAxis *axis)
{
    Q_D(QXYSeries);
    detachAxis(d->m_axisY);
    d->m_axisY = axis;
    if (axis) {
        axis->setOrientation(Qt::Vertical);
        attachAxis(axis);
    }
    emit update();
}

/*!
    \property QXYSeries::pointMarker
    \brief A custom QML Component used as a marker for data points.
*/
/*!
    \qmlproperty Component XYSeries::pointMarker
    A custom QML Component used as a marker for data points.
*/
QQmlComponent *QXYSeries::pointMarker() const
{
    Q_D(const QXYSeries);
    return d->m_marker;
}

void QXYSeries::setPointMarker(QQmlComponent *newPointMarker)
{
    Q_D(QXYSeries);
    if (d->m_marker == newPointMarker)
        return;
    d->m_marker = newPointMarker;
    emit pointMarkerChanged();
    emit update();
}

QXYSeries &QXYSeries::operator<< (const QPointF &point)
{
    append(point);
    return *this;
}

QXYSeries &QXYSeries::operator<< (const QList<QPointF>& points)
{
    append(points);
    return *this;
}

QXYSeriesPrivate::QXYSeriesPrivate(QXYSeries *q)
    : QAbstractSeriesPrivate(q),
      m_markerSize(15.0)
{
}

void QXYSeriesPrivate::initializeAxes()
{
}

void QXYSeriesPrivate::setPointSelected(int index, bool selected, bool &callSignal)
{
    if (index < 0 || index > m_points.size() - 1)
        return;

    if (selected) {
        if (!isPointSelected(index)) {
            m_selectedPoints << index;
            callSignal = true;
        }
    } else {
        if (isPointSelected(index)) {
            m_selectedPoints.remove(index);
            callSignal = true;
        }
    }
}

bool QXYSeriesPrivate::isPointSelected(int index)
{
    return m_selectedPoints.contains(index);
}

bool QXYSeriesPrivate::isMarkerSizeDefault()
{
    return m_markerSizeDefault;
}

void QXYSeriesPrivate::setMarkerSize(qreal markerSize)
{
    m_markerSize = markerSize;
}


QT_END_NAMESPACE
