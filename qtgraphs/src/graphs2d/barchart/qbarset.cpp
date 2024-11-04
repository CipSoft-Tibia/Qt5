// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGraphs/qbarset.h>
#include <private/qbarset_p.h>
#include <private/charthelpers_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QBarSet
    \inmodule QtGraphs
    \ingroup graphs_2D
    \brief The QBarSet class represents one set of bars in a bar graph.

    A bar set contains one data value for each category. The first value of a set is assumed to
    belong to the first category, the second one to the second category, and so on. If the set has
    fewer values than there are categories, the missing values are assumed to be located at the end
    of the set. For missing values in the middle of a set, the numerical value of zero is used.
    Labels for zero value sets are not shown.

    \sa QAbstractBarSeries, QBarSeries
*/
/*!
    \qmltype BarSet
    \instantiates QBarSet
    \inqmlmodule QtGraphs
    \ingroup graphs_qml_2D
    \brief Represents one set of bars in a bar graph.

    A bar set contains one data value for each category. The first value of a set is assumed to
    belong to the first category, the second one to the second category, and so on. If the set has
    fewer values than there are categories, the missing values are assumed to be located at the end
    of the set. For missing values in the middle of a set, the numerical value of zero is used.
    Labels for zero value sets are not shown.

    \sa AbstractBarSeries, BarSeries
*/

/*!
    \property QBarSet::label
    \brief The label of the bar set.
*/
/*!
    \qmlproperty string BarSet::label
    The label of the bar set.
*/

/*!
    \property QBarSet::color
    \brief The fill color of the bar set.
*/
/*!
    \qmlproperty color BarSet::color
    The fill color of the bar set.
*/

/*!
    \property QBarSet::borderColor
    \brief The border color of the bar set.
*/
/*!
    \qmlproperty color BarSet::borderColor
    The border color of the bar set.
*/

/*!
    \property QBarSet::borderWidth
    \brief The width of the border line.
    By default, the width is -1, meaning the border width is defined by the theme.
*/
/*!
    \qmlproperty real BarSet::borderWidth
     By default, the width is -1, meaning the border width is defined by the theme.
*/

/*!
    \property QBarSet::count
    \brief The number of values in the bar set.
*/
/*!
    \qmlproperty int BarSet::count
    The number of values in the bar set.
*/

/*!
    \property QBarSet::labelColor
    \brief The text (label) color of the bar set.
*/
/*!
    \qmlproperty color BarSet::labelColor
    The text (label) color of the bar set.
*/

/*!
    \property QBarSet::selectedBars
    \brief The indexes of the bars which are currently selected.
*/
/*!
    \qmlproperty list BarSet::selectedBars
    The indexes of the bars which are currently selected.
*/

/*!
    \qmlproperty QVariantList BarSet::values
    The values of the bar set. You can set a list of either \l [QML]{real} or \l [QML]{point}
    types as values.

    If you set a list of real types as values, they directly define the bar set values.

    If you set a list of point types as values, the x-coordinate of the point specifies its
    zero-based index in the bar set. The size of the bar set is the highest x-coordinate value + 1.
    If a point is missing for any x-coordinate between zero and the highest value,
    it gets the value zero.

    For example, the following bar sets have equal values:
    \code
        myBarSet1.values = [5, 0, 1, 5];
        myBarSet2.values = [Qt.point(0, 5), Qt.point(2, 1), Qt.point(3, 5)];
    \endcode
*/

/*!
    \fn void QBarSet::valuesChanged()
    This signal is emitted when the values of the bar set change.
*/

/*!
    \fn void QBarSet::labelChanged()
    This signal is emitted when the label of the bar set changes.
    \sa label
*/

/*!
    \fn void QBarSet::colorChanged(QColor)
    This signal is emitted when the fill color of the bar set changes to \a color.
*/

/*!
    \fn void QBarSet::borderColorChanged(QColor)
    This signal is emitted when the border color of the bar set changes to \a color.
*/

/*!
    \fn void QBarSet::labelColorChanged(QColor)
    This signal is emitted when the text (label) color of the bar set changes to \a color.
*/

/*!
    \fn void QBarSet::valuesAdded(int index, int count)
    This signal is emitted when new values are added to the bar set.
    \a index indicates the position of the first inserted value, and \a count is the number
    of inserted values.
    \sa append(), insert()
*/
/*!
    \qmlsignal BarSet::valuesAdded(int index, int count)
    This signal is emitted when new values are added to the bar set.
    \a index indicates the position of the first inserted value, and \a count is the number
    of inserted values.

    The corresponding signal handler is \c onValuesAdded.
*/

/*!
    \fn void QBarSet::valuesRemoved(int index, int count)
    This signal is emitted when values are removed from the bar set.
    \a index indicates the position of the first removed value, and \a count is the number
    of removed values.
    \sa remove()
*/
/*!
    \qmlsignal BarSet::valuesRemoved(int index, int count)
    This signal is emitted when values are removed from the bar set.
    \a index indicates the position of the first removed value, and \a count is the number
    of removed values.

    The corresponding signal handler is \c onValuesRemoved.
*/

/*!
    \fn void QBarSet::valueChanged(int index)
    This signal is emitted when the value at the position specified by \a index is modified.
    \sa at()
*/
/*!
    \qmlsignal BarSet::valueChanged(int index)
    This signal is emitted when the value at the position specified by \a index is modified.

    The corresponding signal handler is \c onValueChanged.
*/

QBarSet::QBarSet(QObject *parent)
    : QObject(parent),
      d_ptr(new QBarSetPrivate(QString(), this))
{
}

/*!
    Constructs a bar set with the label \a label and the parent \a parent.
*/
QBarSet::QBarSet(const QString label, QObject *parent)
    : QObject(parent),
      d_ptr(new QBarSetPrivate(label, this))
{
}

/*!
    Removes the bar set.
*/
QBarSet::~QBarSet()
{
    // NOTE: d_ptr destroyed by QObject
}

/*!
    Sets \a label as the new label for the bar set.
*/
void QBarSet::setLabel(const QString label)
{
    if (d_ptr->m_label != label) {
        d_ptr->m_label = label;
        d_ptr->setLabelsDirty(true);
        emit update();
        emit labelChanged();
    }
}

/*!
    Returns the label of the bar set.
*/
QString QBarSet::label() const
{
    return d_ptr->m_label;
}

/*!
    \qmlmethod BarSet::append(real value)
    Appends the new value specified by \a value to the end of the bar set.
*/

/*!
    Appends the new value specified by \a value to the end of the bar set.
*/
void QBarSet::append(const qreal value)
{
    // Convert to QPointF
    int index = d_ptr->m_values.size();
    d_ptr->append(QPointF(d_ptr->m_values.size(), value));
    emit valuesAdded(index, 1);
}

/*!
    Appends the list of real values specified by \a values to the end of the bar set.

    \sa append()
*/
void QBarSet::append(const QList<qreal> &values)
{
    int index = d_ptr->m_values.size();
    d_ptr->append(values);
    emit valuesAdded(index, values.size());
}

/*!
    A convenience operator for appending the real value specified by \a value to the end of the
    bar set.

    \sa append()
*/
QBarSet &QBarSet::operator << (const qreal &value)
{
    append(value);
    return *this;
}

/*!
    Inserts \a value in the position specified by \a index.
    The values following the inserted value are moved up one position.

    \sa remove()
*/
void QBarSet::insert(const int index, const qreal value)
{
    d_ptr->insert(index, value);

    bool callSignal = false;
    if (!d_ptr->m_selectedBars.isEmpty()) {
        // if value was inserted we need to move already selected bars by 1
        QSet<int> selectedAfterInsert;
        for (const auto &value : std::as_const(d_ptr->m_selectedBars)) {
            if (value >= index) {
                selectedAfterInsert << value + 1;
                callSignal = true;
            } else {
                selectedAfterInsert << value;
            }
        }
        d_ptr->m_selectedBars = selectedAfterInsert;
    }

    emit valuesAdded(index, 1);
    if (callSignal)
        emit selectedBarsChanged(selectedBars());
}

/*!
    \qmlmethod BarSet::remove(int index, int count)
    Removes the number of values specified by \a count from the bar set starting
    with the value specified by \a index.

    If you leave out \a count, only the value specified by \a index is removed.
*/

/*!
    Removes the number of values specified by \a count from the bar set starting with
    the value specified by \a index.
    \sa insert()
*/
void QBarSet::remove(const int index, const int count)
{
    int removedCount = d_ptr->remove(index, count);
    if (removedCount > 0)
        emit valuesRemoved(index, removedCount);
    return;
}

/*!
    \qmlmethod BarSet::replace(int index, real value)
    Adds the value specified by \a value to the bar set at the position
    specified by \a index.
*/

/*!
    Adds the value specified by \a value to the bar set at the position specified by \a index.
*/
void QBarSet::replace(const int index, const qreal value)
{
    if (index >= 0 && index < d_ptr->m_values.size()) {
        d_ptr->replace(index, value);
        emit valueChanged(index);
    }
}

/*!
    \qmlmethod BarSet::at(int index)
    Returns the value specified by \a index from the bar set.
    If the index is out of bounds, 0.0 is returned.
*/

/*!
    Returns the value specified by \a index from the bar set.
    If the index is out of bounds, 0.0 is returned.
*/
qreal QBarSet::at(const int index) const
{
    if (index < 0 || index >= d_ptr->m_values.size())
        return 0;
    return d_ptr->m_values.at(index).y();
}

/*!
    Returns the value of the bar set specified by \a index.
    If the index is out of bounds, 0.0 is returned.
*/
qreal QBarSet::operator [](const int index) const
{
    return at(index);
}

/*!
    Returns the number of values in a bar set.
*/
int QBarSet::count() const
{
    return d_ptr->m_values.size();
}

/*!
    Returns the sum of all values in the bar set.
*/
qreal QBarSet::sum() const
{
    qreal total(0);
    for (int i = 0; i < d_ptr->m_values.size(); i++)
        total += d_ptr->m_values.at(i).y();
    return total;
}

/*!
    Returns the fill color for the bar set.
*/
QColor QBarSet::color()
{
    return d_ptr->m_color;
}

/*!
    Sets the fill color for the bar set to \a color.
*/
void QBarSet::setColor(QColor color)
{
    if (d_ptr->m_color != color) {
        d_ptr->m_color = color;
        emit update();
        emit colorChanged(color);
    }
}

/*!
    Returns the line color for the bar set.
*/
QColor QBarSet::borderColor()
{
    return d_ptr->m_borderColor;
}

/*!
    Sets the line color for the bar set to \a color.
*/
void QBarSet::setBorderColor(QColor color)
{
    if (d_ptr->m_borderColor != color) {
        d_ptr->m_borderColor = color;
        emit update();
        emit borderColorChanged(color);
    }
}

/*!
    Returns the text color for the bar set.
*/
QColor QBarSet::labelColor()
{
    return d_ptr->m_labelColor;
}

/*!
    Sets the text color for the bar set to \a color.
*/
void QBarSet::setLabelColor(QColor color)
{
    if (d_ptr->m_labelColor != color) {
        d_ptr->m_labelColor = color;
        emit update();
        emit labelColorChanged(color);
    }
}

/*!
    Returns the color of the selected bars.

    This is the fill (brush) color of bars marked as selected. If not specified,
    value of QBarSet::color is used as default.
    \sa color
*/
QColor QBarSet::selectedColor() const
{
    return d_ptr->m_selectedColor;
}

/*!
    Sets the \a color of the selected bars.
    \sa selectedColor
*/
void QBarSet::setSelectedColor(const QColor &color)
{
    if (d_ptr->m_selectedColor != color) {
        d_ptr->m_selectedColor = color;
        d_ptr->setLabelsDirty(true);
        emit update();
        emit d_ptr->updatedBars();
        emit selectedColorChanged(color);
    }
}


qreal QBarSet::borderWidth() const
{
    return d_ptr->m_borderWidth;
}

void QBarSet::setBorderWidth(qreal width)
{
    if (d_ptr->m_borderWidth != width) {
        d_ptr->m_borderWidth = width;
        emit update();
        emit borderWidthChanged(width);
    }
}

QVariantList QBarSet::values()
{
    QVariantList values;
    for (int i(0); i < count(); i++)
        values.append(QVariant(QBarSet::at(i)));
    return values;
}

void QBarSet::setValues(QVariantList values)
{
    bool valuesUpdated = false;
    // See if we can replace values instead of remove & add all.
    // This way e.g. selections remain.
    const bool doReplace = count() == values.size();

    if (!doReplace) {
        while (count())
            remove(count() - 1);
        valuesUpdated = true;
    }

    if (values.size() > 0 && values.at(0).canConvert<QPoint>()) {
        // Create list of values for appending if the first item is Qt.point
        int maxValue = 0;
        for (int i = 0; i < values.size(); i++) {
            if (values.at(i).canConvert<QPoint>() &&
                values.at(i).toPoint().x() > maxValue) {
                maxValue = values.at(i).toPoint().x();
            }
        }

        QList<qreal> indexValueList;
        indexValueList.resize(maxValue + 1);

        for (int i = 0; i < values.size(); i++) {
            if (values.at(i).canConvert<QPoint>())
                indexValueList.replace(values.at(i).toPoint().x(), values.at(i).toPointF().y());
        }

        for (int i = 0; i < indexValueList.size(); i++) {
            if (doReplace)
                QBarSet::replace(i, indexValueList.at(i));
            else
                QBarSet::append(indexValueList.at(i));
            valuesUpdated = true;
        }

    } else {
        for (int i(0); i < values.size(); i++) {
            if (values.at(i).canConvert<double>()) {
                if (doReplace)
                    QBarSet::replace(i, values[i].toDouble());
                else
                    QBarSet::append(values[i].toDouble());
                valuesUpdated = true;
            }
        }
    }
    emit update();
    if (valuesUpdated)
        emit valuesChanged();
}


/*!
   Returns \c true if the bar at the given \a index is among selected bars and \c false otherwise.
   \note Selected bars are drawn using the selected color if it was specified using QBarSet::setSelectedColor.
   \sa selectedBars(), setBarSelected(), setSelectedColor()
 */
bool QBarSet::isBarSelected(int index) const
{
    return d_ptr->isBarSelected(index);
}

/*!
  Marks the bar at \a index as selected.
  \note Emits QBarSet::selectedBarsChanged.
  \sa setBarSelected()
 */
void QBarSet::selectBar(int index)
{
    setBarSelected(index, true);
}

/*!
  Deselects the bar at \a index.
  \note Emits QBarSet::selectedBarsChanged.
  \sa setBarSelected()
 */
void QBarSet::deselectBar(int index)
{
    setBarSelected(index, false);
}

/*!
  Marks the bar at \a index as either selected or deselected as specified by \a selected.
  \note Selected bars are drawn using the selected color if it was specified. Emits QBarSet::selectedBarsChanged.
  \sa setSelectedColor()
 */
void QBarSet::setBarSelected(int index, bool selected)
{
    bool callSignal = false;
    d_ptr->setBarSelected(index, selected, callSignal);

    if (callSignal)
        emit selectedBarsChanged(selectedBars());
    emit update();
}

/*!
  Marks all bars in the series as selected.
  \note Emits QBarSet::selectedBarsChanged.
  \sa setBarSelected()
 */
void QBarSet::selectAllBars()
{
    bool callSignal = false;
    for (int i = 0; i < d_ptr->m_values.size(); ++i)
        d_ptr->setBarSelected(i, true, callSignal);

    if (callSignal)
        emit selectedBarsChanged(selectedBars());
    emit update();
}

/*!
  Deselects all bars in the series.
  \note Emits QBarSet::selectedBarsChanged.
  \sa setBarSelected()
 */
void QBarSet::deselectAllBars()
{
    bool callSignal = false;
    for (int i = 0; i < d_ptr->m_values.size(); ++i)
        d_ptr->setBarSelected(i, false, callSignal);

    if (callSignal)
        emit selectedBarsChanged(selectedBars());
    emit update();
}

/*!
  Marks multiple bars passed in an \a indexes list as selected.
  \note Emits QBarSet::selectedBarsChanged.
  \sa setBarSelected()
 */
void QBarSet::selectBars(const QList<int> &indexes)
{
    bool callSignal = false;
    for (const int &index : indexes)
        d_ptr->setBarSelected(index, true, callSignal);

    if (callSignal)
        emit selectedBarsChanged(selectedBars());
    emit update();
}

/*!
  Marks multiple bars passed in an \a indexes list as deselected.
  \note Emits QBarSet::selectedBarsChanged.
  \sa setBarSelected()
 */
void QBarSet::deselectBars(const QList<int> &indexes)
{
    bool callSignal = false;
    for (const int &index : indexes)
        d_ptr->setBarSelected(index, false, callSignal);

    if (callSignal)
        emit selectedBarsChanged(selectedBars());
    emit update();
}

/*!
  Changes the selection state of bars at the given \a indexes to the opposite one.
  \note Emits QBarSet::selectedBarsChanged.
  \sa setBarSelected()
 */
void QBarSet::toggleSelection(const QList<int> &indexes)
{
    bool callSignal = false;
    for (const int &index : indexes)
        d_ptr->setBarSelected(index, !isBarSelected(index), callSignal);

    if (callSignal)
        emit selectedBarsChanged(selectedBars());
    emit update();
}

/*!
  Returns a list of bars marked as selected.
  \sa setBarSelected()
 */
QList<int> QBarSet::selectedBars() const
{
    return QList<int>(d_ptr->m_selectedBars.begin(), d_ptr->m_selectedBars.end());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

QBarSetPrivate::QBarSetPrivate(const QString label, QBarSet *parent) : QObject(parent),
    q_ptr(parent),
    m_label(label),
    m_visualsDirty(true)
{
}

QBarSetPrivate::~QBarSetPrivate()
{
}

void QBarSetPrivate::append(QPointF value)
{
    if (isValidValue(value)) {
        m_values.append(value);
        emit valueAdded(m_values.size() - 1, 1);
    }
}

void QBarSetPrivate::append(const QList<QPointF> &values)
{
    int originalIndex = m_values.size();
    for (const auto &value : values) {
        if (isValidValue(value))
            m_values.append(value);
    }
    emit valueAdded(originalIndex, values.size());
}

void QBarSetPrivate::append(const QList<qreal> &values)
{
    int originalIndex = m_values.size();
    int index = originalIndex;
    for (const auto value : values) {
        if (isValidValue(value)) {
            m_values.append(QPointF(index, value));
            index++;
        }
    }
    emit valueAdded(originalIndex, values.size());
}

void QBarSetPrivate::insert(const int index, const qreal value)
{
    m_values.insert(index, QPointF(index, value));
    emit valueAdded(index, 1);
}

void QBarSetPrivate::insert(const int index, const QPointF value)
{
    m_values.insert(index, value);
    emit valueAdded(index, 1);
}

int QBarSetPrivate::remove(const int index, const int count)
{
    int removeCount = count;

    if ((index < 0) || (m_values.size() == 0))
        return 0; // Invalid index or not values in list, remove nothing.
    else if ((index + count) > m_values.size())
        removeCount = m_values.size() - index; // Trying to remove more items than list has. Limit amount to be removed.

    int c = 0;
    while (c < removeCount) {
        m_values.removeAt(index);
        c++;
    }

    bool callSignal = false;
    if (!m_selectedBars.empty()) {
        QSet<int> selectedAfterRemoving;

        for (const int &selectedBarIndex : std::as_const(m_selectedBars)) {
            if (selectedBarIndex < index) {
                selectedAfterRemoving << selectedBarIndex;
            } else if (selectedBarIndex >= index + removeCount) {
                selectedAfterRemoving << selectedBarIndex - removeCount;
                callSignal = true;
            } else {
                callSignal = true;
            }
        }

        m_selectedBars = selectedAfterRemoving;
    }

    emit valueRemoved(index, removeCount);
    if (callSignal)
        emit q_ptr->selectedBarsChanged(q_ptr->selectedBars());

    return removeCount;
}

void QBarSetPrivate::replace(const int index, const qreal value)
{
    if (index < 0 || index >= m_values.size())
        return;

    m_values.replace(index, QPointF(index, value));
    emit valueChanged(index);
}

qreal QBarSetPrivate::pos(const int index)
{
    if (index < 0 || index >= m_values.size())
        return 0;
    return m_values.at(index).x();
}

qreal QBarSetPrivate::value(const int index)
{
    if (index < 0 || index >= m_values.size())
        return 0;
    return m_values.at(index).y();
}

void QBarSetPrivate::setBarSelected(int index, bool selected, bool &callSignal)
{
    if (index < 0 || index > m_values.size() - 1)
        return;

    if (selected) {
        if (!isBarSelected(index)) {
            m_selectedBars << index;
            callSignal = true;
        }
    } else {
        if (isBarSelected(index)) {
            m_selectedBars.remove(index);
            callSignal = true;
        }
    }

    if (callSignal)
        setVisualsDirty(true);
}

bool QBarSetPrivate::isBarSelected(int index) const
{
    return m_selectedBars.contains(index);
}

QT_END_NAMESPACE

#include "moc_qbarset.cpp"
#include "moc_qbarset_p.cpp"
