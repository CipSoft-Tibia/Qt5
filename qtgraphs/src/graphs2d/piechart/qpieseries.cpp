// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGraphs/qpieseries.h>
#include <private/qpieseries_p.h>
#include <QtGraphs/qpieslice.h>
#include <private/qpieslice_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QPieSeries
    \inmodule QtGraphs
    \ingroup graphs_2D
    \brief The QPieSeries class presents data in pie graphs.

    A pie series consists of slices that are defined as QPieSlice objects.
    The slices can have any values as the QPieSeries object calculates
    the percentage of a slice compared with the sum of all slices in the series
    to determine the actual size of the slice in the graph.

    Pie size and position on the graph are controlled by using relative values
    that range from 0.0 to 1.0.
    These relate to the actual graph rectangle.

    By default, the pie is defined as a full pie. A partial pie can be created
    by setting a starting angle and angle span for the series.
    A full pie is 360 degrees, where 0 is at 12 a'clock.

    \sa QPieSlice
*/
/*!
    \qmltype PieSeries
    \nativetype QPieSeries
    \inqmlmodule QtGraphs
    \ingroup graphs_qml_2D
    \inherits AbstractSeries

    \brief Presents data in pie graphs.

    A pie series consists of slices that are defined using the PieSlice type.
    The slices can have any values as the PieSeries type calculates
    the percentage of a slice compared with the sum of all slices in the series
    to determine the actual size of the slice in the graph.

    Pie size and position on the graph are controlled by using relative values
    that range from 0.0 to 1.0.
    These relate to the actual graph rectangle.

    By default, the pie is defined as a full pie. A partial pie can be created
    by setting a starting angle and angle span for the series.
    A full pie is 360 degrees, where 0 is at 12 o'clock.

    The following QML example shows how to create a simple pie graph.
    \qml
    import QtQuick
    import QtGraphs

    Item {
        id: mainView
        width: 1280
        height: 720

        GraphsView {
            anchors.fill: parent
            theme: GraphsTheme {
                 colorScheme: GraphsTheme.ColorScheme.Dark
                 theme: GraphsTheme.Theme.QtGreen
            }
            PieSeries {
                id: pieSeries
                PieSlice {
                    value: 1
                }
                PieSlice {
                    value: 2
                }
            }
        }
    }
    \endqml

    \sa PieSlice, GraphsView
*/

/*!
    \property QPieSeries::horizontalPosition
    \brief The horizontal position of the pie.

    The value is relative to the graph rectangle, so that:

    \list
    \li 0.0 is the absolute left.
    \li 1.0 is the absolute right.
    \endlist
    The default value is 0.5 (center).
    \sa verticalPosition
*/

/*!
    \qmlproperty real PieSeries::horizontalPosition

    The horizontal position of the pie.

    The value is relative to the graph rectangle, so that:

    \list
    \li 0.0 is the absolute left.
    \li 1.0 is the absolute right.
    \endlist
    The default value is 0.5 (center).
    \sa verticalPosition
*/

/*!
    \qmlsignal PieSeries::horizontalPositionChanged()
    This signal is emitted when the horizontal position changes.
    \sa horizontalPosition
*/

/*!
    \property QPieSeries::verticalPosition
    \brief The vertical position of the pie.

    The value is relative to the graph rectangle, so that:

    \list
    \li 0.0 is the absolute top.
    \li 1.0 is the absolute bottom.
    \endlist
    The default value is 0.5 (center).
    \sa horizontalPosition
*/

/*!
    \qmlproperty real PieSeries::verticalPosition

    The vertical position of the pie.

    The value is relative to the graph rectangle, so that:

    \list
    \li 0.0 is the absolute top.
    \li 1.0 is the absolute bottom.
    \endlist
    The default value is 0.5 (center).
    \sa horizontalPosition
*/
/*!
    \qmlsignal PieSeries::verticalPositionChanged()
    This signal is emitted when the vertical position changes.
    \sa verticalPosition
*/

/*!
    \property QPieSeries::pieSize
    \brief The pie size.

    The value is relative to the graph rectangle, so that:

    \list
    \li 0.0 is the minimum pieSize (pie not drawn).
    \li 1.0 is the maximum pieSize that can fit the graph.
    \endlist

    When setting this property, the holeSize property is adjusted if necessary,
    to ensure that the hole size is not greater than the pie size.

    The default value is 0.7.
*/

/*!
    \qmlproperty real PieSeries::pieSize

    The pie size.

    The value is relative to the graph rectangle, so that:

    \list
    \li 0.0 is the minimum pieSize (pie not drawn).
    \li 1.0 is the maximum pieSize that can fit the graph.
    \endlist

    When setting this property, the holeSize property is adjusted if necessary,
    to ensure that the hole size is not greater than the pie size.

    The default value is 0.7.
*/
/*!
    \qmlsignal PieSeries::pieSizeChanged()
    This signal is emitted when the pie size changes.
    \sa pieSize
*/

/*!
    \property QPieSeries::holeSize
    \brief The donut hole size.

    When setting the \l pieSize property, this property is adjusted if necessary,
    to ensure that the hole size is not greater than the pie size.

    The default value is 0.0.
*/

/*!
    \qmlproperty real PieSeries::holeSize

    The donut hole size.

    When setting the \l pieSize property, this property is adjusted if necessary,
    to ensure that the hole size is not greater than the pie size.

    The default value is 0.0.

*/
/*!
    \qmlsignal PieSeries::holeSizeChanged()
    This signal is emitted when the donut hole size changes.
    \sa holeSize
*/

/*!
    \property QPieSeries::startAngle
    \brief The starting angle of the pie.

    A full pie is 360 degrees, where 0 degrees is at 12 o'clock.

    The default value is 0.
*/

/*!
    \qmlproperty real PieSeries::startAngle

    The starting angle of the pie.

    A full pie is 360 degrees, where 0 degrees is at 12 o'clock.

    The default value is 0.
*/
/*!
    \qmlsignal PieSeries::startAngleChanged()
    This signal is emitted when the pie start angle changes.
    \sa startAngle
*/

/*!
    \property QPieSeries::endAngle
    \brief The ending angle of the pie.

    A full pie is 360 degrees, where 0 degrees is at 12 o'clock.

    The default value is 360.
*/

/*!
    \qmlproperty real PieSeries::endAngle

    The ending angle of the pie.

    A full pie is 360 degrees, where 0 degrees is at 12 o'clock.

    The default value is 360.
*/
/*!
    \qmlsignal PieSeries::endAngleChanged()
    This signal is emitted when the pie end angle changes.
    \sa endAngle
*/

/*!
    \property QPieSeries::count

    \brief The number of slices in the series.
*/

/*!
    \qmlproperty int PieSeries::count

    The number of slices in the series.
*/

/*!
    \qmlsignal PieSeries::countChanged()
    This signal is emitted when the slice count changes.
    \sa count
*/

/*!
    \property QPieSeries::sum

    \brief The sum of all slices.

    The series keeps track of the sum of all the slices it holds.
*/

/*!
    \qmlproperty real PieSeries::sum

    The sum of all slices.

    The series keeps track of the sum of all the slices it holds.
*/

/*!
    \qmlsignal PieSeries::sumChanged()
    This signal is emitted when the sum of all slices changes.
    \sa sum
*/

/*!
    \fn void QPieSeries::added(const QList<QPieSlice *> &slices)

    This signal is emitted when the slices specified by \a slices are added to the series.

    \sa append()
*/
/*!
    \qmlsignal PieSeries::added(list<PieSlice> slices)
    This signal is emitted when the slices specified by \a slices are added to the series.
*/

/*!
    \fn void QPieSeries::removed(const QList<QPieSlice *> &slices)
    This signal is emitted when the slices specified by \a slices are removed from the series.
    \sa remove()
*/
/*!
    \qmlsignal PieSeries::removed(list<PieSlice> slices)
    This signal is emitted when the slices specified by \a slices are removed from the series.
*/

/*!
    \qmlmethod PieSlice PieSeries::at(int index)
    Returns the slice at the position specified by \a index. Returns null if the
    index is not valid.
*/

/*!
    \qmlmethod PieSlice PieSeries::find(string label)
    Returns the first slice that has the label \a label. Returns null if the label
    is not found.
*/

/*!
    \qmlmethod PieSlice PieSeries::append(string label, real value)
    Adds a new slice with the label \a label and the value \a value to the pie.
*/

/*!
    \qmlmethod bool PieSeries::remove(PieSlice slice)
    Removes the slice specified by \a slice from the pie. Returns \c true if the
    removal was successful, \c false otherwise.
*/

/*!
    \qmlmethod bool PieSeries::replace(int index, PieSlice slice)
    Replaces the slice specified by \a slice from the pie at \a index. Returns \c true if the
    replace was successful, \c false otherwise.
*/

/*!
    \qmlmethod PieSeries::clear()
    Removes all slices from the pie.
*/

/*!
    \qmlmethod void PieSeries::removeMultiple(int index, int count)
    Removes a range of slices as specified by the \a index and \a count. The call
    traverses over all slices even if removal of one fails.
*/

/*!
    \qmlmethod bool PieSeries::remove(int index)
    Removes the slice specified by \a index from the pie. Returns \c true if the
    removal was successful, \c false otherwise.
*/

/*!
    \qmlmethod bool PieSeries::replace(PieSlice oldSlice, PieSlice newSlice)
    Replaces the slice specified by \a oldSlice with newSlice. Returns \c true if the
    removal was successful, \c false otherwise. \a oldSlice is destroyed if this 
    is successful.
*/

/*!
    \qmlmethod bool PieSeries::replace(list<PieSlice> slices)
    Completely replaces all current slices with \a slices. The size does not need
    to match. Returns false if any of the PieSlice in \a slices is invalid.
*/

/*!
    \qmlmethod bool PieSeries::take(PieSlice slice)
    Takes a single slice, specified by \a slice, from the series. Does not delete
    the slice object. Returns \c true if successful.
*/

/*!
    Constructs a series object that is a child of \a parent.
*/
QPieSeries::QPieSeries(QObject *parent)
    : QAbstractSeries(*(new QPieSeriesPrivate()), parent)
{}

QPieSeries::~QPieSeries() {}

/*!
    \reimp

    Returns the type of the series.
*/
QAbstractSeries::SeriesType QPieSeries::type() const
{
    return QAbstractSeries::SeriesType::Pie;
}

/*!
    Returns the PieSlice at the position \a index. Returns null if no PieSlice was found.
*/
QPieSlice *QPieSeries::at(qsizetype index)
{
    QList<QPieSlice *> sliceList = slices();
    if (index >= 0 && index < sliceList.size())
        return sliceList[index];

    return 0;
}

/*!
    Searches for a PieSlice which contains the label \a label. Returns the PieSlice if found, null otherwise.
*/
QPieSlice *QPieSeries::find(const QString &label)
{
    for (QPieSlice *slice : slices()) {
        if (slice->label() == label)
            return slice;
    }
    return 0;
}

/*!
    Replaces the PieSlice at position \a index with the one specified by \a slice.
    The original PieSlice will be permanently deleted. Returns \c false if replacing
    any of the PieSlices fails.
*/
bool QPieSeries::replace(qsizetype index, QPieSlice *slice)
{
    Q_D(QPieSeries);

    if (index < 0)
        index = 0;
    if (!slice || d->m_slices.contains(slice))
        return false;
    if (slice->series()) // already added to some series
        return false;
    if (qIsNaN(slice->value()) || qIsInf(slice->value()))
        return false;
    if (d->m_slices.size() <= index)
        return false;

    emit removed(QList<QPieSlice *>() << d->m_slices[index]);
    delete d->m_slices[index];

    slice->setParent(this);
    slice->d_func()->m_series = this;

    d->m_slices[index] = slice;

    d->updateData();

    QObject::connect(slice, SIGNAL(sliceChanged()), this, SLOT(handleSliceChange()));
    emit replaced(QList<QPieSlice *>() << slice);

    return true;
}

/*!
    Removes multiple PieSlices from the series starting from \a index to a number of \a count.
    The PieSlices will be permanently deleted.
*/
void QPieSeries::removeMultiple(qsizetype index, int count)
{
    Q_D(QPieSeries);

    if (index + count >= d->m_slices.size())
        return;
    if (index < 0 || count < 0)
        return;

    QList<QPieSlice *> removedList;

    for (qsizetype i = index; i < index + count; ++i) {
        auto slice = d->m_slices[index];
        d->m_slices.removeOne(slice);
        d->updateData();

        removedList << slice;
    }

    emit removed(removedList);

    for (auto slice : removedList) {
        delete slice;
    }

    emit countChanged();
}

/*!
    Removes the PieSlice at the location \a index. The PieSlice will be permanently deleted.
    Returns \c true if removing is successful.
*/
bool QPieSeries::remove(qsizetype index)
{
    Q_D(QPieSeries);

    if (index >= d->m_slices.size())
        return false;
    if (index < 0)
        return false;

    return remove(d->m_slices[index]);
}

/*!
    Replaces the PieSlice \a oldSlice with \a newSlice if found in the series.\a oldSlice will
    be permanently deleted. Returns \c true if replacing is successful.
*/
bool QPieSeries::replace(QPieSlice *oldSlice, QPieSlice *newSlice)
{
    Q_D(QPieSeries);

    if (!oldSlice || !newSlice)
        return false;
    if (oldSlice == newSlice)
        return false;
    if (d->m_slices.contains(newSlice))
        return false;
    if (newSlice->series())
        return false;
    if (qIsNaN(newSlice->value()) || qIsInf(newSlice->value()))
        return false;

    for (int i = 0; i < d->m_slices.size(); ++i) {
        if (d->m_slices[i] == oldSlice) {
            emit removed(QList<QPieSlice *>() << d->m_slices[i]);
            delete d->m_slices[i];

            newSlice->setParent(this);
            newSlice->d_func()->m_series = this;

            d->m_slices[i] = newSlice;

            d->updateData();

            QObject::connect(newSlice, SIGNAL(sliceChanged()), this, SLOT(handleSliceChange()));
            emit replaced(QList<QPieSlice *>() << newSlice);

            return true;
        }
    }

    return false;
}

/*!
    Replaces the entire list of PieSlices in the series with the list specified by \a slices.
    All the original PieSlices will be permanently deleted. Returns \c true if all PieSlices are
    replaced successfully.
*/
bool QPieSeries::replace(const QList<QPieSlice *> &slices)
{
    Q_D(QPieSeries);

    for (const auto slice : slices) {
        if (!slice || d->m_slices.contains(slice))
            return false;
        if (slice->series())
            return false;
        if (qIsNaN(slice->value()) || qIsInf(slice->value()))
            return false;
    }

    emit removed(d->m_slices);
    for (auto &slice : d->m_slices) {
        delete slice;
        slice = nullptr;
    }

    for (auto &slice : slices) {
        slice->setParent(this);
        slice->d_func()->m_series = this;
        QObject::connect(slice, SIGNAL(sliceChanged()), this, SLOT(handleSliceChange()));
    }

    d->m_slices = slices;
    emit replaced(slices);

    return true;
}

/*!
    Appends the slice specified by \a slice to the series.
    Slice ownership is passed to the series.

    Returns \c true if appending succeeds.
*/
bool QPieSeries::append(QPieSlice *slice)
{
    return append(QList<QPieSlice *>() << slice);
}

/*!
    Appends the array of slices specified by \a slices to the series.
    Slice ownership is passed to the series.

    Returns \c true if appending succeeds.
*/
bool QPieSeries::append(const QList<QPieSlice *> &slices)
{
    Q_D(QPieSeries);

    if (slices.size() == 0)
        return false;

    for (auto *s : slices) {
        if (!s || d->m_slices.contains(s))
            return false;
        if (s->series()) // already added to some series
            return false;
        if (qIsNaN(s->value()) || qIsInf(s->value()))
            return false;
    }

    for (auto *s : slices) {
        s->setParent(this);
        s->d_func()->m_series = this;
        d->m_slices << s;
    }

    d->updateData();

    for (auto *s : slices)
        QObject::connect(s, SIGNAL(sliceChanged()), this, SLOT(handleSliceChange()));

    emit added(slices);
    emit countChanged();

    return true;
}

/*!
    Appends the slice specified by \a slice to the series and returns a reference to the series.
    Slice ownership is passed to the series.
*/
QPieSeries &QPieSeries::operator << (QPieSlice *slice)
{
    append(slice);
    return *this;
}

/*!
    Appends a single slice with the specified \a value and \a label to the series.
    Slice ownership is passed to the series.
    Returns null if \a value is \c NaN, \c Inf, or \c -Inf and adds nothing to the
    series.
*/
QPieSlice *QPieSeries::append(const QString &label, qreal value)
{
    if (!(qIsNaN(value) || qIsInf(value))) {
        QPieSlice *slice = new QPieSlice(label, value);
        append(slice);
        return slice;
    } else {
        return nullptr;
    }
}

/*!
    Inserts the slice specified by \a slice to the series before the slice at
    the position specified by \a index.
    Slice ownership is passed to the series.

    Returns \c true if inserting succeeds.
*/
bool QPieSeries::insert(qsizetype index, QPieSlice *slice)
{
    Q_D(QPieSeries);

    if (index < 0 || index > d->m_slices.size())
        return false;

    if (!slice || d->m_slices.contains(slice))
        return false;

    if (slice->series()) // already added to some series
        return false;

    if (qIsNaN(slice->value()) || qIsInf(slice->value()))
        return false;

    slice->setParent(this);
    slice->d_func()->m_series = this;
    d->m_slices.insert(index, slice);

    d->updateData();

    connect(slice, SIGNAL(sliceChanged()), this, SLOT(handleSliceChange()));

    emit added(QList<QPieSlice *>() << slice);
    emit countChanged();

    return true;
}

/*!
    Removes a single slice, specified by \a slice, from the series and deletes it
    permanently.

    The pointer cannot be referenced after this call.

    Returns \c true if the removal succeeds.
*/
bool QPieSeries::remove(QPieSlice *slice)
{
    Q_D(QPieSeries);

    if (!d->m_slices.removeOne(slice))
        return false;

    d->updateData();

    emit removed(QList<QPieSlice *>() << slice);
    emit countChanged();

    delete slice;
    slice = 0;

    return true;
}

/*!
    Takes a single slice, specified by \a slice, from the series. Does not delete
    the slice object.

    \note The series remains the slice's parent object. You must set the
    parent object to take full ownership.

    Returns \c true if the take operation was successful.
*/
bool QPieSeries::take(QPieSlice *slice)
{
    Q_D(QPieSeries);

    if (!d->m_slices.removeOne(slice))
        return false;

    slice->d_func()->m_series = 0;
    slice->disconnect(this);

    d->updateData();

    emit removed(QList<QPieSlice *>() << slice);
    emit countChanged();

    return true;
}

/*!
    Clears all slices from the series.
*/
void QPieSeries::clear()
{
    Q_D(QPieSeries);
    if (d->m_slices.size() == 0)
        return;

    QList<QPieSlice *> slices = d->m_slices;
    for (QPieSlice *s : d->m_slices)
        d->m_slices.removeOne(s);

    d->updateData();

    emit removed(slices);
    emit countChanged();

    for (QPieSlice *s : slices)
        delete s;
}

/*!
    Returns a list of slices that belong to this series.
*/
QList<QPieSlice *> QPieSeries::slices() const
{
    Q_D(const QPieSeries);
    return d->m_slices;
}

/*!
    Returns the number of the slices in this series.
*/
qsizetype QPieSeries::count() const
{
    Q_D(const QPieSeries);
    return d->m_slices.size();
}

/*!
    Returns \c true if the series is empty.
*/
bool QPieSeries::isEmpty() const
{
    Q_D(const QPieSeries);
    return d->m_slices.isEmpty();
}

/*!
    Returns the sum of all slice values in this series.

    \sa QPieSlice::value(), QPieSlice::setValue(), QPieSlice::percentage()
*/
qreal QPieSeries::sum() const
{
    Q_D(const QPieSeries);
    return d->m_sum;
}

void QPieSeries::setHorizontalPosition(qreal relativePosition)
{
    Q_D(QPieSeries);

    if (relativePosition < 0.0)
        relativePosition = 0.0;
    if (relativePosition > 1.0)
        relativePosition = 1.0;

    if (qFuzzyCompare(d->m_pieRelativeHorPos, relativePosition))
        return;

    d->m_pieRelativeHorPos = relativePosition;
    emit horizontalPositionChanged();
    emit update();
}

qreal QPieSeries::horizontalPosition() const
{
    Q_D(const QPieSeries);
    return d->m_pieRelativeHorPos;
}

void QPieSeries::setVerticalPosition(qreal relativePosition)
{
    Q_D(QPieSeries);

    if (relativePosition < 0.0)
        relativePosition = 0.0;
    if (relativePosition > 1.0)
        relativePosition = 1.0;

    if (qFuzzyCompare(d->m_pieRelativeVerPos, relativePosition))
        return;

    d->m_pieRelativeVerPos = relativePosition;
    emit verticalPositionChanged();
    emit update();
}

qreal QPieSeries::verticalPosition() const
{
    Q_D(const QPieSeries);
    return d->m_pieRelativeVerPos;
}

void QPieSeries::setPieSize(qreal relativeSize)
{
    Q_D(QPieSeries);
    relativeSize = qBound((qreal)0.0, relativeSize, (qreal)1.0);
    d->setSizes(qMin(d->m_holeRelativeSize, relativeSize), relativeSize);
}

qreal QPieSeries::pieSize() const
{
    Q_D(const QPieSeries);
    return d->m_pieRelativeSize;
}

/*!
    Sets the start angle of the pie.

    A full pie is 360 degrees, where 0 degrees is at 12 o'clock.

    \a angle must be smaller than the end angle.

    \sa startAngle(), endAngle(), setEndAngle()
*/
void QPieSeries::setStartAngle(qreal angle)
{
    Q_D(QPieSeries);
    if (qFuzzyCompare(d->m_pieStartAngle, angle))
        return;
    d->m_pieStartAngle = angle;
    d->updateData();
    emit startAngleChanged();
    emit update();
}

/*!
    Returns the start angle of the pie.

    A full pie is 360 degrees, where 0 degrees is at 12 o'clock.

    \sa setStartAngle(), endAngle(), setEndAngle()
*/
qreal QPieSeries::startAngle() const
{
    Q_D(const QPieSeries);
    return d->m_pieStartAngle;
}

/*!
    Sets the end angle of the pie.

    A full pie is 360 degrees, where 0 degrees is at 12 o'clock.

    \a angle must be greater than the start angle.

    \sa endAngle(), startAngle(), setStartAngle()
*/
void QPieSeries::setEndAngle(qreal angle)
{
    Q_D(QPieSeries);
    if (qFuzzyCompare(d->m_pieEndAngle, angle))
        return;
    d->m_pieEndAngle = angle;
    d->updateData();
    emit endAngleChanged();
    emit update();
}

/*!
    Returns the end angle of the pie.

    A full pie is 360 degrees, where 0 degrees is at 12 o'clock.

    \sa setEndAngle(), startAngle(), setStartAngle()
*/
qreal QPieSeries::endAngle() const
{
    Q_D(const QPieSeries);
    return d->m_pieEndAngle;
}

void QPieSeries::componentComplete()
{
    for (QObject *child : children()) {
        if (qobject_cast<QPieSlice *>(child))
            QPieSeries::append(qobject_cast<QPieSlice *>(child));
    }
    QAbstractSeries::componentComplete();
}

/*!
    Sets the visibility of all slice labels to \a visible.

    \note This function affects only the current slices in the series.
    If a new slice is added, the default label visibility is \c false.

    \sa QPieSlice::isLabelVisible(), QPieSlice::setLabelVisible()
*/
void QPieSeries::setLabelsVisible(bool visible)
{
    Q_D(QPieSeries);
    for (QPieSlice *s : d->m_slices)
        s->setLabelVisible(visible);
}

/*!
    Sets the position of all the slice labels to \a position.

    \note This function affects only the current slices in the series.
    If a new slice is added, the default label position is QPieSlice::LabelOutside.

    \sa QPieSlice::labelPosition(), QPieSlice::setLabelPosition()
*/
void QPieSeries::setLabelsPosition(QPieSlice::LabelPosition position)
{
    Q_D(QPieSeries);
    for (QPieSlice *s : d->m_slices)
        s->setLabelPosition(position);
}

void QPieSeries::handleSliceChange()
{
    QPieSlice *pSlice = qobject_cast<QPieSlice *>(sender());
    Q_D(QPieSeries);
    Q_ASSERT(d->m_slices.contains(pSlice));
    d->updateData();
}

QPieSeries::QPieSeries(QPieSeriesPrivate &dd, QObject *parent)
    : QAbstractSeries(dd, parent)
{}

void QPieSeries::setHoleSize(qreal holeSize)
{
    Q_D(QPieSeries);
    holeSize = qBound((qreal)0.0, holeSize, (qreal)1.0);
    d->setSizes(holeSize, qMax(d->m_pieRelativeSize, holeSize));
}

qreal QPieSeries::holeSize() const
{
    Q_D(const QPieSeries);
    return d->m_holeRelativeSize;
}

QPieSeriesPrivate::QPieSeriesPrivate()
    : m_pieRelativeHorPos(.5)
    , m_pieRelativeVerPos(.5)
    , m_pieRelativeSize(.7)
    , m_pieStartAngle(0)
    , m_pieEndAngle(360)
    , m_sum(0)
    , m_holeRelativeSize(.0)
{}

void QPieSeriesPrivate::updateData()
{
    Q_Q(QPieSeries);

    // calculate sum of all slices
    qreal sum = 0;
    for (QPieSlice *s : m_slices)
        sum += s->value();

    if (!qFuzzyCompare(m_sum, sum)) {
        m_sum = sum;
        emit q->sumChanged();
    }

    // nothing to show..
    if (qFuzzyCompare(m_sum, 0))
        return;

    // update slice attributes
    qreal sliceAngle = m_pieStartAngle;
    qreal pieSpan = m_pieEndAngle - m_pieStartAngle;
    for (QPieSlice *s : m_slices) {
        QPieSlicePrivate *d = s->d_func();
        d->setPercentage(s->value() / m_sum);
        d->setStartAngle(sliceAngle);
        d->setAngleSpan(pieSpan * s->percentage());
        sliceAngle += s->angleSpan();
    }

    emit q->update();
}

void QPieSeriesPrivate::updateLabels()
{
    Q_Q(QPieSeries);

    emit q->update();
}

void QPieSeriesPrivate::setSizes(qreal innerSize, qreal outerSize)
{
    Q_Q(QPieSeries);
    if (!qFuzzyCompare(m_holeRelativeSize, innerSize)) {
        m_holeRelativeSize = innerSize;
        emit q->holeSizeChanged();
    }

    if (!qFuzzyCompare(m_pieRelativeSize, outerSize)) {
        m_pieRelativeSize = outerSize;
        emit q->pieSizeChanged();
    }
}

QT_END_NAMESPACE
