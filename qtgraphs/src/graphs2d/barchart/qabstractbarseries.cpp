// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGraphs/qabstractbarseries.h>
#include <private/qabstractbarseries_p.h>
#include <QtGraphs/qbarset.h>
#include <private/qbarset_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QAbstractBarSeries
    \inmodule QtGraphs
    \ingroup graphs_2D
    \brief The QAbstractBarSeries class is an abstract parent class for all bar series classes.

    In bar graphs, bars are defined as bar sets that contain one data value for each category.
    The position of a bar is specified by the category and its height by the data value. Bar
    series that contain multiple bar sets group together bars that belong to the same category.
    The way the bars are displayed is determined by the subclass of this class chosen to create
    the bar graph.

    \sa QBarSet, QBarSeries
*/
/*!
    \qmltype AbstractBarSeries
    \instantiates QAbstractBarSeries
    \inqmlmodule QtGraphs
    \ingroup graphs_qml_2D
    \inherits AbstractSeries

    \brief An abstract parent type for all bar series types.

    In bar graphs, bars are defined as bar sets that contain one data value for each category.
    The position of a bar is specified by the category and its height by the data value. Bar
    series that contain multiple bar sets group together bars that belong to the same category.
    The way the bars are displayed is determined by the subclass of this class chosen to create
    the bar graph.
*/

/*!
    \property QAbstractBarSeries::barWidth
    \brief The width of the bars of the series.

    The unit of width is the unit of the x-axis. The minimum width for bars is zero, and negative
    values are treated as zero. Setting the width to zero means that the width of the bar on the
    screen is one pixel regardless of the scale of the x-axis. Bars wider than zero are scaled
    using the x-axis scale.

    By default, the barWidth is 0.5 (bars will take 50% of the available width).
    The valid values range from 0.0 (0%) to 1.0 (100%).

    \note When used with QBarSeries, this value specifies the width of a group of bars instead of
    that of a single bar.
    \sa QBarSeries
*/
/*!
    \qmlproperty real AbstractBarSeries::barWidth
    The unit of width is the unit of the x-axis. The minimum width for bars is zero, and negative
    values are treated as zero. Setting the width to zero means that the width of the bar on the
    screen is one pixel regardless of the scale of the x-axis. Bars wider than zero are scaled
    using the x-axis scale.

    By default, the barWidth is 0.5 (bars will take 50% of the available width).
    The valid values range from 0.0 (0%) to 1.0 (100%).

    \note When used with the BarSeries type, this value specifies the width of a group of bars
    instead of that of a single bar.
*/

/*!
    \property QAbstractBarSeries::count
    \brief The number of bar sets in a bar series.
*/
/*!
    \qmlproperty int AbstractBarSeries::count
    The number of bar sets in a bar series.
*/

/*!
    \property QAbstractBarSeries::labelsVisible
    \brief The visibility of the labels in a bar series.
*/
/*!
    \qmlproperty bool AbstractBarSeries::labelsVisible
    The visibility of the labels in a bar series.
*/

/*!
    \property QAbstractBarSeries::labelsFormat
    \brief The format used for showing labels in a bar series.

    QAbstractBarSeries supports the following format tag:
    \table
      \row
        \li @value      \li The value of the bar
    \endtable

    For example, the following usage of the format tags would produce labels that show the value
    followed by the unit (u):
    \code
    series->setLabelsFormat("@value u");
    \endcode

    By default, the labels show the value of the bar. For the percent bar series, \e % is added
    after the value. The labels are shown on the plot area, if the bars are close to each other,
    the labels may overlap.

    \sa labelsVisible, labelsPosition, labelsPrecision
*/
/*!
    \qmlproperty string AbstractBarSeries::labelsFormat
    The format used for showing labels in a bar series.

    \sa QAbstractBarSeries::labelsFormat, labelsVisible, labelsPosition
*/
/*!
    \fn void QAbstractBarSeries::labelsFormatChanged(const QString &format)
    This signal is emitted when the \a format of data value labels changes.
*/

/*!
 \enum QAbstractBarSeries::LabelsPosition

 This enum value describes the position of the data value labels:

 \value LabelsCenter Label is located in the center of the bar.
 \value LabelsInsideEnd Label is located inside the bar at the top.
 \value LabelsInsideBase Label is located inside the bar at the bottom.
 \value LabelsOutsideEnd Label is located outside the bar at the top.
 */

/*!
    \property QAbstractBarSeries::labelsPosition
    \brief The position of value labels.

    \sa labelsVisible, labelsFormat
*/
/*!
    \qmlproperty enumeration AbstractBarSeries::labelsPosition

    The position of the data value labels:

    \value  AbstractBarSeries.LabelsCenter
            Label is located in the center of the bar.
    \value  AbstractBarSeries.LabelsInsideEnd
            Label is located inside the bar at the top.
    \value  AbstractBarSeries.LabelsInsideBase
            Label is located inside the bar at the bottom.
    \value  AbstractBarSeries.LabelsOutsideEnd
            Label is located outside the bar at the top.

    \sa labelsVisible, labelsFormat
*/
/*!
    \fn void QAbstractBarSeries::labelsPositionChanged(QAbstractBarSeries::LabelsPosition position)
    This signal is emitted when the \a position of value labels changes.
*/

/*!
    \property QAbstractBarSeries::labelsAngle
    \brief The angle of the value labels in degrees.
*/
/*!
    \qmlproperty real AbstractBarSeries::labelsAngle
    The angle of the value labels in degrees.
*/
/*!
    \fn void QAbstractBarSeries::labelsAngleChanged(qreal angle)
    This signal is emitted when the \a angle of the value labels changes.
*/

/*!
    \property QAbstractBarSeries::labelsPrecision
    \brief The maximum amount of significant digits shown in value labels.

    Default value is 6.
*/
/*!
    \qmlproperty real AbstractBarSeries::labelsPrecision
    The maximum amount of significant digits shown in value labels.

    Default value is 6.
*/
/*!
    \fn void QAbstractBarSeries::labelsPrecisionChanged(int precision)
    This signal is emitted when the \a precision of the value labels changes.
*/

/*!
    \fn void QAbstractBarSeries::clicked(int index, QBarSet *barset)
    This signal is emitted when the user clicks the bar specified by \a index
    in the bar set specified by \a barset.
*/
/*!
    \qmlsignal AbstractBarSeries::clicked(int index, BarSet barset)
    This signal is emitted when the user clicks the bar specified by \a index
    in the bar set specified by \a barset.

    The corresponding signal handler is \c onClicked.
*/

/*!
    \fn void QAbstractBarSeries::pressed(int index, QBarSet *barset)
    This signal is emitted when the user clicks the bar specified by \a index
    in the bar set specified by \a barset and holds down the mouse button.
*/
/*!
    \qmlsignal AbstractBarSeries::pressed(int index, BarSet barset)
    This signal is emitted when the user clicks the bar specified by \a index
    in the bar set specified by \a barset and holds down the mouse button.

    The corresponding signal handler is \c onPressed.
*/

/*!
    \fn void QAbstractBarSeries::released(int index, QBarSet *barset)
    This signal is emitted when the user releases the mouse press on the bar
    specified by \a index in the bar set specified by \a barset.
*/
/*!
    \qmlsignal AbstractBarSeries::released(int index, BarSet barset)
    This signal is emitted when the user releases the mouse press on the bar
    specified by \a index in the bar set specified by \a barset.

    The corresponding signal handler is \c onReleased.
*/

/*!
    \fn void QAbstractBarSeries::doubleClicked(int index, QBarSet *barset)
    This signal is emitted when the user double-clicks the bar specified by \a index
    in the bar set specified by \a barset.
*/
/*!
    \qmlsignal AbstractBarSeries::doubleClicked(int index, BarSet barset)
    This signal is emitted when the user double-clicks the bar specified by \a index
    in the bar set specified by \a barset.

    The corresponding signal handler is \c onDoubleClicked.
*/

/*!
    \fn void QAbstractBarSeries::hovered(bool status, int index, QBarSet* barset)

    This signal is emitted when a mouse is hovered over the bar specified by \a index in the
    bar set specified by \a barset. When the mouse moves over the bar, \a status turns \c true,
    and when the mouse moves away again, it turns \c false.
*/
/*!
    \qmlsignal AbstractBarSeries::hovered(bool status, int index, BarSet barset)

    This signal is emitted when a mouse is hovered over the bar specified by \a index in the
    bar set specified by \a barset. When the mouse moves over the bar, \a status turns \c true,
    and when the mouse moves away again, it turns \c false.

    The corresponding signal handler is \c onHovered.
*/

/*!
    \fn void QAbstractBarSeries::countChanged()
    This signal is emitted when the number of bar sets is changed, for example by append() or
    remove().
*/

/*!
    \fn void QAbstractBarSeries::barWidthChanged()
    This signal is emitted when the bar width changes.
*/

/*!
    \fn void QAbstractBarSeries::labelsVisibleChanged()
    This signal is emitted when the labels' visibility changes.
    \sa isLabelsVisible(), setLabelsVisible()
*/

/*!
    \fn void QAbstractBarSeries::barsetsAdded(const QList<QBarSet *> &sets)
    This signal is emitted when the bar sets specified by \a sets are added to the series.
    \sa append(), insert()
*/
/*!
    \qmlsignal AbstractBarSeries::barsetsAdded()
    This signal is emitted when bar sets are added to the series.

    The corresponding signal handler is \c onBarsetsAdded.
*/

/*!
    \fn void QAbstractBarSeries::barsetsRemoved(const QList<QBarSet *> &sets)
    This signal is emitted when the bar sets specified by \a sets are removed from the series.
    \sa remove()
*/
/*!
    \qmlsignal AbstractBarSeries::barsetsRemoved()
    This signal is emitted when bar sets are removed from the series.

    The corresponding signal handler is \c onBarsetsRemoved.
*/

/*!
    \qmlmethod BarSet AbstractBarSeries::at(int index)
    Returns the bar set at \a index. Returns null if the index is not valid.
*/

/*!
    \qmlmethod BarSet AbstractBarSeries::append(string label, VariantList values)
    Adds a new bar set with \a label and \a values to the index. \a values is
    a list of real values.

    For example:
    \code
        myBarSeries.append("set 1", [0, 0.2, 0.2, 0.5, 0.4, 1.5, 0.9]);
    \endcode
*/

/*!
    \qmlmethod BarSet AbstractBarSeries::insert(int index, string label, VariantList values)
    Adds a new bar set with \a label and \a values to \a index. \a values can be a list
    of real values or a list of XYPoint types.

    If the index value is equal to or less than zero, the new bar set is prepended to the bar
    series. If the index value is equal to or greater than the number of bar sets in the bar
    series, the new bar set is appended to the bar series.

    \sa append()
*/

/*!
    \qmlmethod bool AbstractBarSeries::remove(BarSet barset)
    Removes the bar set specified by \a barset from the series. Returns \c true if successful,
    \c false otherwise.
*/

/*!
    \qmlmethod AbstractBarSeries::clear()
    Removes all bar sets from the series.
*/

/*!
    Removes the abstract bar series and the bar sets owned by it.
*/
QAbstractBarSeries::~QAbstractBarSeries()
{

}

/*!
    \internal
*/
QAbstractBarSeries::QAbstractBarSeries(QAbstractBarSeriesPrivate &o, QObject *parent)
    : QAbstractSeries(o, parent)
{
    Q_D(QAbstractSeries);
    QObject::connect(this, SIGNAL(countChanged()), d, SIGNAL(countChanged()));
}

/*!
    Sets the width of the bars of the series to \a width.
*/
void QAbstractBarSeries::setBarWidth(qreal width)
{
    Q_D(QAbstractBarSeries);
    if (d->barWidth() != width) {
        d->setBarWidth(width);
        emit barWidthChanged();
    }
}

/*!
    Returns the width of the bars of the series.
    \sa setBarWidth()
*/
qreal QAbstractBarSeries::barWidth() const
{
    Q_D(const QAbstractBarSeries);
    return d->barWidth();
}

/*!
    Adds a set of bars specified by \a set to the bar series and takes ownership of it. If the set
    is null or it already belongs to the series, it will not be appended.
    Returns \c true if appending succeeded.
*/
bool QAbstractBarSeries::append(QBarSet *set)
{
    Q_D(QAbstractBarSeries);
    bool success = d->append(set);
    if (success) {
        QList<QBarSet *> sets;
        sets.append(set);
        set->setParent(this);
        QObject::connect(set, &QBarSet::update, this, &QAbstractBarSeries::update);
        emit barsetsAdded(sets);
        emit countChanged();
    }
    emit update();
    return success;
}

/*!
    Removes the bar set specified by \a set from the series and permanently deletes it if
    the removal succeeds. Returns \c true if the set was removed.
*/
bool QAbstractBarSeries::remove(QBarSet *set)
{
    Q_D(QAbstractBarSeries);
    bool success = d->remove(set);
    if (success) {
        QList<QBarSet *> sets;
        sets.append(set);
        set->setParent(0);
        QObject::disconnect(set, &QBarSet::update, this, &QAbstractBarSeries::update);
        emit barsetsRemoved(sets);
        emit countChanged();
        delete set;
        set = 0;
    }
    return success;
}

/*!
    Takes a single \a set from the series. Does not delete the bar set object.
    \note The series remains the barset's parent object. You must set the
    parent object to take full ownership.

    Returns \c true if the take operation succeeds.
*/
bool QAbstractBarSeries::take(QBarSet *set)
{
    Q_D(QAbstractBarSeries);
    bool success = d->remove(set);
    if (success) {
        QList<QBarSet *> sets;
        sets.append(set);
        QObject::disconnect(set, &QBarSet::update, this, &QAbstractBarSeries::update);
        emit barsetsRemoved(sets);
        emit countChanged();
    }
    return success;
}

/*!
    Adds a list of bar sets specified by \a sets to a bar series and takes ownership of the sets.
    Returns \c true if all sets were appended successfully. If any of the sets is null or was
    previously appended to the series, nothing is appended and this function returns \c false.
    If any of the sets appears in the list more than once, nothing is appended and this function
    returns \c false.
*/
bool QAbstractBarSeries::append(const QList<QBarSet *> &sets)
{
    Q_D(QAbstractBarSeries);
    if (!d->append(sets))
        return false;

    for (auto *set : sets) {
        set->setParent(this);
        QObject::connect(set, &QBarSet::update, this, &QAbstractBarSeries::update);
    }

    emit barsetsAdded(sets);
    emit countChanged();

    return true;
}

/*!
    Inserts a bar set specified by \a set to a series at the position specified by \a index
    and takes ownership of the set. If the set is null or already belongs to the series, it will
    not be appended. Returns \c true if inserting succeeds.
*/
bool QAbstractBarSeries::insert(int index, QBarSet *set)
{
    Q_D(QAbstractBarSeries);
    bool success = d->insert(index, set);
    if (success) {
        QList<QBarSet *> sets;
        sets.append(set);
        QObject::connect(set, &QBarSet::update, this, &QAbstractBarSeries::update);
        emit barsetsAdded(sets);
        emit countChanged();
    }
    return success;
}

/*!
    Removes all bar sets from the series and permanently deletes them.
*/
void QAbstractBarSeries::clear()
{
    Q_D(QAbstractBarSeries);
    const QList<QBarSet *> sets = barSets();
    bool success = d->remove(sets);
    if (success) {
        emit barsetsRemoved(sets);
        emit countChanged();
        for (QBarSet *set : sets) {
            QObject::disconnect(set, &QBarSet::update, this, &QAbstractBarSeries::update);
            delete set;
        }
    }
}

/*!
    Returns the number of bar sets in a bar series.
*/
int QAbstractBarSeries::count() const
{
    Q_D(const QAbstractBarSeries);
    return d->m_barSets.size();
}

/*!
    Returns a list of bar sets in a bar series. Keeps the ownership of the bar sets.
 */
QList<QBarSet *> QAbstractBarSeries::barSets() const
{
    Q_D(const QAbstractBarSeries);
    return d->m_barSets;
}

/*!
    Sets the visibility of labels in a bar series to \a visible.
*/
void QAbstractBarSeries::setLabelsVisible(bool visible)
{
    Q_D(QAbstractBarSeries);
    if (d->m_labelsVisible != visible) {
        d->setLabelsVisible(visible);
        emit labelsVisibleChanged();
    }
}

/*!
    Returns the visibility of labels.
*/
bool QAbstractBarSeries::isLabelsVisible() const
{
    Q_D(const QAbstractBarSeries);
    return d->m_labelsVisible;
}

void QAbstractBarSeries::setLabelsFormat(const QString &format)
{
    Q_D(QAbstractBarSeries);
    if (d->m_labelsFormat != format) {
        d->m_labelsFormat = format;
        d->setLabelsDirty(true);
        emit labelsFormatChanged(format);
    }
}

QString QAbstractBarSeries::labelsFormat() const
{
    Q_D(const QAbstractBarSeries);
    return d->m_labelsFormat;
}

void QAbstractBarSeries::setLabelsAngle(qreal angle)
{
    Q_D(QAbstractBarSeries);
    if (d->m_labelsAngle != angle) {
        d->m_labelsAngle = angle;
        d->setLabelsDirty(true);
        emit labelsAngleChanged(angle);
    }
}

qreal QAbstractBarSeries::labelsAngle() const
{
    Q_D(const QAbstractBarSeries);
    return d->m_labelsAngle;
}

void QAbstractBarSeries::setLabelsPosition(QAbstractBarSeries::LabelsPosition position)
{
    Q_D(QAbstractBarSeries);
    if (d->m_labelsPosition != position) {
        d->m_labelsPosition = position;
        emit labelsPositionChanged(position);
    }
}

QAbstractBarSeries::LabelsPosition QAbstractBarSeries::labelsPosition() const
{
    Q_D(const QAbstractBarSeries);
    return d->m_labelsPosition;
}

void QAbstractBarSeries::setLabelsPrecision(int precision)
{
    Q_D(QAbstractBarSeries);
    if (d->m_labelsPrecision != precision) {
        d->m_labelsPrecision = precision;
        d->setLabelsDirty(true);
        emit labelsPrecisionChanged(precision);
    }
}

int QAbstractBarSeries::labelsPrecision() const
{
    Q_D(const QAbstractBarSeries);
    return d->m_labelsPrecision;
}


// Select all the elements in the series
void QAbstractBarSeries::selectAll()
{
    Q_D(QAbstractBarSeries);
    for (auto s : d->m_barSets) {
        s->selectAllBars();
    }
}

// Deselect all the elements in the series
void QAbstractBarSeries::deselectAll()
{
    Q_D(QAbstractBarSeries);
    for (auto s : d->m_barSets) {
        s->deselectAllBars();
    }
}

void QAbstractBarSeries::componentComplete()
{
    for (auto *child : children()) {
        if (auto bs = qobject_cast<QBarSet *>(child))
            append(bs);
    }
    QAbstractSeries::componentComplete();
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

QAbstractBarSeriesPrivate::QAbstractBarSeriesPrivate(QAbstractBarSeries *q) :
    QAbstractSeriesPrivate(q),
    m_barWidth(0.5),  // Default value is 50% of category width
    m_labelsVisible(false),
    m_visible(true),
    m_blockBarUpdate(false),
    m_labelsFormat(),
    m_labelsPosition(QAbstractBarSeries::LabelsCenter),
    m_labelsAngle(0),
    m_labelsPrecision(6),
    m_visualsDirty(true),
    m_labelsDirty(true)
{
}

int QAbstractBarSeriesPrivate::categoryCount() const
{
    // No categories defined. return count of longest set.
    int count = 0;
    for (int i = 0; i < m_barSets.size(); i++) {
        if (m_barSets.at(i)->count() > count)
            count = m_barSets.at(i)->count();
    }

    return count;
}

void QAbstractBarSeriesPrivate::setBarWidth(qreal width)
{
    Q_Q(QAbstractBarSeries);
    width = std::clamp(width, 0.0, 1.0);
    if (!qFuzzyCompare(width, m_barWidth)) {
        m_barWidth = width;
        q->update();
    }
}

qreal QAbstractBarSeriesPrivate::barWidth() const
{
    return m_barWidth;
}

QBarSet *QAbstractBarSeriesPrivate::barsetAt(int index)
{
    return m_barSets.at(index);
}

void QAbstractBarSeriesPrivate::setVisible(bool visible)
{
    m_visible = visible;
    emit visibleChanged();
}

void QAbstractBarSeriesPrivate::setLabelsVisible(bool visible)
{
    m_labelsVisible = visible;
    emit labelsVisibleChanged(visible);
}

qreal QAbstractBarSeriesPrivate::min()
{
    if (m_barSets.size() <= 0)
        return 0;

    qreal min = INT_MAX;

    for (int i = 0; i < m_barSets.size(); i++) {
        int categoryCount = m_barSets.at(i)->count();
        for (int j = 0; j < categoryCount; j++) {
            qreal temp = m_barSets.at(i)->at(j);
            if (temp < min)
                min = temp;
        }
    }
    return min;
}

qreal QAbstractBarSeriesPrivate::max()
{
    if (m_barSets.size() <= 0)
        return 0;

    qreal max = INT_MIN;

    for (int i = 0; i < m_barSets.size(); i++) {
        int categoryCount = m_barSets.at(i)->count();
        for (int j = 0; j < categoryCount; j++) {
            qreal temp = m_barSets.at(i)->at(j);
            if (temp > max)
                max = temp;
        }
    }

    return max;
}

qreal QAbstractBarSeriesPrivate::valueAt(int set, int category)
{
    if ((set < 0) || (set >= m_barSets.size()))
        return 0; // No set, no value.
    else if ((category < 0) || (category >= m_barSets.at(set)->count()))
        return 0; // No category, no value.

    return m_barSets.at(set)->at(category);
}

qreal QAbstractBarSeriesPrivate::percentageAt(int set, int category)
{
    if ((set < 0) || (set >= m_barSets.size()))
        return 0; // No set, no value.
    else if ((category < 0) || (category >= m_barSets.at(set)->count()))
        return 0; // No category, no value.

    qreal value = m_barSets.at(set)->at(category);
    qreal sum = categorySum(category);
    if (qFuzzyCompare(sum, 0))
        return 0;

    return value / sum;
}

qreal QAbstractBarSeriesPrivate::categorySum(int category)
{
    qreal sum(0);
    int count = m_barSets.size(); // Count sets
    for (int set = 0; set < count; set++) {
        if (category < m_barSets.at(set)->count())
            sum += m_barSets.at(set)->at(category);
    }
    return sum;
}

qreal QAbstractBarSeriesPrivate::absoluteCategorySum(int category)
{
    qreal sum(0);
    int count = m_barSets.size(); // Count sets
    for (int set = 0; set < count; set++) {
        if (category < m_barSets.at(set)->count())
            sum += qAbs(m_barSets.at(set)->at(category));
    }
    return sum;
}

qreal QAbstractBarSeriesPrivate::maxCategorySum()
{
    qreal max = INT_MIN;
    int count = categoryCount();
    for (int i = 0; i < count; i++) {
        qreal sum = categorySum(i);
        if (sum > max)
            max = sum;
    }
    return max;
}

qreal QAbstractBarSeriesPrivate::minX()
{
    if (m_barSets.size() <= 0)
        return 0;

    qreal min = INT_MAX;

    for (int i = 0; i < m_barSets.size(); i++) {
        int categoryCount = m_barSets.at(i)->count();
        for (int j = 0; j < categoryCount; j++) {
            qreal temp = m_barSets.at(i)->d_ptr.data()->m_values.at(j).x();
            if (temp < min)
                min = temp;
        }
    }
    return min;
}

qreal QAbstractBarSeriesPrivate::maxX()
{
    if (m_barSets.size() <= 0)
        return 0;

    qreal max = INT_MIN;

    for (int i = 0; i < m_barSets.size(); i++) {
        int categoryCount = m_barSets.at(i)->count();
        for (int j = 0; j < categoryCount; j++) {
            qreal temp = m_barSets.at(i)->d_ptr.data()->m_values.at(j).x();
            if (temp > max)
                max = temp;
        }
    }

    return max;
}

qreal QAbstractBarSeriesPrivate::categoryTop(int category)
{
    // Returns top (sum of all positive values) of category.
    // Returns 0, if all values are negative
    qreal top(0);
    int count = m_barSets.size();
    for (int set = 0; set < count; set++) {
        if (category < m_barSets.at(set)->count()) {
            qreal temp = m_barSets.at(set)->at(category);
            if (temp > 0) {
                top += temp;
            }
        }
    }
    return top;
}

qreal QAbstractBarSeriesPrivate::categoryBottom(int category)
{
    // Returns bottom (sum of all negative values) of category
    // Returns 0, if all values are positive
    qreal bottom(0);
    int count = m_barSets.size();
    for (int set = 0; set < count; set++) {
        if (category < m_barSets.at(set)->count()) {
            qreal temp = m_barSets.at(set)->at(category);
            if (temp < 0) {
                bottom += temp;
            }
        }
    }
    return bottom;
}

qreal QAbstractBarSeriesPrivate::top()
{
    // Returns top of all categories
    qreal top(0);
    int count = categoryCount();
    for (int i = 0; i < count; i++) {
        qreal temp = categoryTop(i);
        if (temp > top)
            top = temp;
    }
    return top;
}

qreal QAbstractBarSeriesPrivate::bottom()
{
    // Returns bottom of all categories
    qreal bottom(0);
    int count = categoryCount();
    for (int i = 0; i < count; i++) {
        qreal temp = categoryBottom(i);
        if (temp < bottom)
            bottom = temp;
    }
    return bottom;
}

bool QAbstractBarSeriesPrivate::blockBarUpdate()
{
    return m_blockBarUpdate;
}

qreal QAbstractBarSeriesPrivate::labelsAngle() const
{
    return m_labelsAngle;
}

bool QAbstractBarSeriesPrivate::append(QBarSet *set)
{
    if ((m_barSets.contains(set)) || (set == 0))
        return false; // Fail if set is already in list or set is null.

    m_barSets.append(set);
    QObject::connect(set->d_ptr.data(), &QBarSetPrivate::updatedBars,
                     this, &QAbstractBarSeriesPrivate::updatedBars);
    QObject::connect(set->d_ptr.data(), &QBarSetPrivate::valueChanged,
                     this, &QAbstractBarSeriesPrivate::handleSetValueChange);
    QObject::connect(set->d_ptr.data(), &QBarSetPrivate::valueAdded,
                     this, &QAbstractBarSeriesPrivate::handleSetValueAdd);
    QObject::connect(set->d_ptr.data(), &QBarSetPrivate::valueRemoved,
                     this, &QAbstractBarSeriesPrivate::handleSetValueRemove);
    connect(set, &QBarSet::selectedBarsChanged,
            this, &QAbstractBarSeriesPrivate::updatedBars);

    emit restructuredBars(); // this notifies barchartitem
    return true;
}

bool QAbstractBarSeriesPrivate::remove(QBarSet *set)
{
    if (!m_barSets.contains(set))
        return false; // Fail if set is not in list

    m_barSets.removeOne(set);
    QObject::disconnect(set->d_ptr.data(), &QBarSetPrivate::updatedBars,
                        this, &QAbstractBarSeriesPrivate::updatedBars);
    QObject::disconnect(set->d_ptr.data(), &QBarSetPrivate::valueChanged,
                        this, &QAbstractBarSeriesPrivate::handleSetValueChange);
    QObject::disconnect(set->d_ptr.data(), &QBarSetPrivate::valueAdded,
                        this, &QAbstractBarSeriesPrivate::handleSetValueAdd);
    QObject::disconnect(set->d_ptr.data(), &QBarSetPrivate::valueRemoved,
                        this, &QAbstractBarSeriesPrivate::handleSetValueRemove);
    disconnect(set, &QBarSet::selectedBarsChanged,
               this, &QAbstractBarSeriesPrivate::updatedBars);

    emit restructuredBars(); // this notifies barchartitem
    return true;
}

bool QAbstractBarSeriesPrivate::append(const QList<QBarSet *> &sets)
{
    for (auto *set : sets) {
        if ((set == 0) || (m_barSets.contains(set)))
            return false; // Fail if any of the sets is null or is already appended.
        if (sets.count(set) != 1)
            return false; // Also fail if same set is more than once in given list.
    }

    for (auto *set : sets) {
        m_barSets.append(set);
        QObject::connect(set->d_ptr.data(), &QBarSetPrivate::updatedBars,
                         this, &QAbstractBarSeriesPrivate::updatedBars);
        QObject::connect(set->d_ptr.data(), &QBarSetPrivate::valueChanged,
                         this, &QAbstractBarSeriesPrivate::handleSetValueChange);
        QObject::connect(set->d_ptr.data(), &QBarSetPrivate::valueAdded,
                         this, &QAbstractBarSeriesPrivate::handleSetValueAdd);
        QObject::connect(set->d_ptr.data(), &QBarSetPrivate::valueRemoved,
                         this, &QAbstractBarSeriesPrivate::handleSetValueRemove);
        connect(set, &QBarSet::selectedBarsChanged,
                this, &QAbstractBarSeriesPrivate::updatedBars);
    }

    emit restructuredBars(); // this notifies barchartitem
    return true;
}

bool QAbstractBarSeriesPrivate::remove(const QList<QBarSet *> &sets)
{
    if (sets.size() == 0)
        return false;

    for (QBarSet *set : sets) {
        if ((set == 0) || (!m_barSets.contains(set)))
            return false; // Fail if any of the sets is null or is not in series
        if (sets.count(set) != 1)
            return false; // Also fail if same set is more than once in given list.
    }

    for (QBarSet *set : sets) {
        m_barSets.removeOne(set);
        QObject::disconnect(set->d_ptr.data(), &QBarSetPrivate::updatedBars,
                            this, &QAbstractBarSeriesPrivate::updatedBars);
        QObject::disconnect(set->d_ptr.data(), &QBarSetPrivate::valueChanged,
                            this, &QAbstractBarSeriesPrivate::handleSetValueChange);
        QObject::disconnect(set->d_ptr.data(), &QBarSetPrivate::valueAdded,
                            this, &QAbstractBarSeriesPrivate::handleSetValueAdd);
        QObject::disconnect(set->d_ptr.data(), &QBarSetPrivate::valueRemoved,
                            this, &QAbstractBarSeriesPrivate::handleSetValueRemove);
        disconnect(set, &QBarSet::selectedBarsChanged,
                   this, &QAbstractBarSeriesPrivate::updatedBars);
    }

    emit restructuredBars();        // this notifies barchartitem

    return true;
}

bool QAbstractBarSeriesPrivate::insert(int index, QBarSet *set)
{
    if ((m_barSets.contains(set)) || (set == 0))
        return false; // Fail if set is already in list or set is null.

    m_barSets.insert(index, set);
    QObject::connect(set->d_ptr.data(), &QBarSetPrivate::updatedBars,
                     this, &QAbstractBarSeriesPrivate::updatedBars);
    QObject::connect(set->d_ptr.data(), &QBarSetPrivate::valueChanged,
                     this, &QAbstractBarSeriesPrivate::handleSetValueChange);
    QObject::connect(set->d_ptr.data(), &QBarSetPrivate::valueAdded,
                     this, &QAbstractBarSeriesPrivate::handleSetValueAdd);
    QObject::connect(set->d_ptr.data(), &QBarSetPrivate::valueRemoved,
                     this, &QAbstractBarSeriesPrivate::handleSetValueRemove);
    disconnect(set, &QBarSet::selectedBarsChanged,
               this, &QAbstractBarSeriesPrivate::updatedBars);

    emit restructuredBars();      // this notifies barchartitem
    return true;
}

void QAbstractBarSeriesPrivate::initializeAxes()
{
}

void QAbstractBarSeriesPrivate::handleSetValueChange(int index)
{
    Q_Q(QAbstractBarSeries);
    QBarSetPrivate *priv = qobject_cast<QBarSetPrivate *>(sender());
    if (priv)
        emit setValueChanged(index, priv->q_ptr);
    emit q->update();
}

void QAbstractBarSeriesPrivate::handleSetValueAdd(int index, int count)
{
    Q_Q(QAbstractBarSeries);
    QBarSetPrivate *priv = qobject_cast<QBarSetPrivate *>(sender());
    if (priv)
        emit setValueAdded(index, count, priv->q_ptr);
    emit q->update();
}

void QAbstractBarSeriesPrivate::handleSetValueRemove(int index, int count)
{
    Q_Q(QAbstractBarSeries);
    QBarSetPrivate *priv = qobject_cast<QBarSetPrivate *>(sender());
    if (priv)
        emit setValueRemoved(index, count, priv->q_ptr);
    emit q->update();
}

QT_END_NAMESPACE

#include "moc_qabstractbarseries.cpp"
#include "moc_qabstractbarseries_p.cpp"
