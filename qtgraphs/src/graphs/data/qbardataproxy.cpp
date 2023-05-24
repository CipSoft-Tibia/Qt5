// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qbardataproxy_p.h"
#include "qbar3dseries_p.h"

QT_BEGIN_NAMESPACE

/*!
 * \class QBarDataProxy
 * \inmodule QtGraphs
 * \brief The QBarDataProxy class is the data proxy for a 3D bars graph.
 *
 * A bar data proxy handles adding, inserting, changing, and removing rows of
 * data.
 *
 * The data array is a list of vectors (rows) of QBarDataItem instances.
 * Each row can contain a different number of items or even be null.
 *
 * QBarDataProxy takes ownership of all QtGraphs::QBarDataRow objects
 * passed to it, whether directly or in a QtGraphs::QBarDataArray container.
 * If bar data row pointers are used to directly modify data after adding the
 * array to the proxy, the appropriate signal must be emitted to update the
 * graph.
 *
 * QBarDataProxy optionally keeps track of row and column labels, which QCategory3DAxis can utilize
 * to show axis labels. The row and column labels are stored in a separate array from the data and
 * row manipulation methods provide alternate versions that do not affect the row labels.
 * This enables the option of having row labels that relate to the position of the data in the
 * array rather than the data itself.
 *
 * \sa {Qt Graphs Data Handling}
 */

/*!
 * \typealias QBarDataRow
 * \relates QBarDataProxy
 *
 * A list of \l {QBarDataItem} objects.
 */

/*!
 * \typealias QBarDataArray
 * \relates QBarDataProxy
 *
 * A list of pointers to \l {QBarDataRow} objects.
 */

/*!
 * \qmltype BarDataProxy
 * \inqmlmodule QtGraphs
 * \ingroup graphs_qml
 * \instantiates QBarDataProxy
 * \inherits AbstractDataProxy
 * \brief The data proxy for a 3D bars graph.
 *
 * This type handles adding, inserting, changing, and removing rows of data with Qt Quick 2.
 *
 * This type is uncreatable, but contains properties that are exposed via subtypes.
 *
 * For a more complete description, see QBarDataProxy.
 *
 * \sa ItemModelBarDataProxy, {Qt Graphs Data Handling}
 */

/*!
 * \qmlproperty int BarDataProxy::rowCount
 * The number of rows in the array.
 */

/*!
 * \qmlproperty list BarDataProxy::rowLabels
 *
 * The optional row labels for the array. Indexes in this array match the row
 * indexes in the data array.
 * If the list is shorter than the number of rows, all rows will not get labels.
 */

/*!
 * \qmlproperty list BarDataProxy::columnLabels
 *
 * The optional column labels for the array. Indexes in this array match column indexes in rows.
 * If the list is shorter than the longest row, all columns will not get labels.
 */

/*!
 * \qmlproperty Bar3DSeries BarDataProxy::series
 *
 * The series this proxy is attached to.
 */

/*!
 * Constructs a bar data proxy with the given \a parent.
 */
QBarDataProxy::QBarDataProxy(QObject *parent) :
    QAbstractDataProxy(new QBarDataProxyPrivate(this), parent)
{
}

/*!
 * \internal
 */
QBarDataProxy::QBarDataProxy(QBarDataProxyPrivate *d, QObject *parent) :
    QAbstractDataProxy(d, parent)
{
}

/*!
 * Deletes the bar data proxy.
 */
QBarDataProxy::~QBarDataProxy()
{
}

/*!
 * \property QBarDataProxy::series
 *
 * \brief The series this proxy is attached to.
 */
QBar3DSeries *QBarDataProxy::series() const
{
    const Q_D(QBarDataProxy);
    return static_cast<QBar3DSeries *>(d->series());
}

/*!
 * Clears the existing array and row and column labels.
 */
void QBarDataProxy::resetArray()
{
    resetArray(0, QStringList(), QStringList());
    emit rowCountChanged(rowCount());
    emit colCountChanged(colCount());
}

/*!
 * Takes ownership of the array \a newArray. Clears the existing array if the
 * new array differs from it. If the arrays are the same, this function
 * just triggers the arrayReset() signal.
 *
 * Passing a null array deletes the old array and creates a new empty array.
 * Row and column labels are not affected.
 */
void QBarDataProxy::resetArray(QBarDataArray *newArray)
{
    Q_D(QBarDataProxy);
    d->resetArray(newArray, 0, 0);
    emit arrayReset();
    if (rowCount() && colCount()) {
        emit rowCountChanged(rowCount());
        emit colCountChanged(colCount());
    }
}

/*!
 * Takes ownership of the array \a newArray. Clears the existing array if the
 * new array differs from it. If the arrays are the same, this function
 * just triggers the arrayReset() signal.
 *
 * Passing a null array deletes the old array and creates a new empty array.
 *
 * The \a rowLabels and \a columnLabels lists specify the new labels for rows and columns.
 */
void QBarDataProxy::resetArray(QBarDataArray *newArray, const QStringList &rowLabels,
                               const QStringList &columnLabels)
{
    Q_D(QBarDataProxy);
    d->resetArray(newArray, &rowLabels, &columnLabels);
    emit arrayReset();
    emit rowCountChanged(rowCount());
    emit colCountChanged(colCount());
}

/*!
 * Changes an existing row by replacing the row at the position \a rowIndex
 * with the new row specified by \a row. The new row can be
 * the same as the existing row already stored at \a rowIndex.
 * Existing row labels are not affected.
 */
void QBarDataProxy::setRow(int rowIndex, QBarDataRow *row)
{
    Q_D(QBarDataProxy);
    d->setRow(rowIndex, row, 0);
    emit rowsChanged(rowIndex, 1);
}

/*!
 * Changes an existing row by replacing the row at the position \a rowIndex
 * with the new row specified by \a row. The new row can be
 * the same as the existing row already stored at \a rowIndex.
 * Changes the row label to \a label.
 */
void QBarDataProxy::setRow(int rowIndex, QBarDataRow *row, const QString &label)
{
    Q_D(QBarDataProxy);
    d->setRow(rowIndex, row, &label);
    emit rowsChanged(rowIndex, 1);
}

/*!
 * Changes existing rows by replacing the rows starting at the position
 * \a rowIndex with the new rows specifies by \a rows.
 * Existing row labels are not affected. The rows in the \a rows array can be
 * the same as the existing rows already stored at \a rowIndex.
 */
void QBarDataProxy::setRows(int rowIndex, const QBarDataArray &rows)
{
    Q_D(QBarDataProxy);
    d->setRows(rowIndex, rows, 0);
    emit rowsChanged(rowIndex, rows.size());
}

/*!
 * Changes existing rows by replacing the rows starting at the position
 * \a rowIndex with the new rows specifies by \a rows.
 * The row labels are changed to \a labels. The rows in the \a rows array can be
 * the same as the existing rows already stored at \a rowIndex.
 */
void QBarDataProxy::setRows(int rowIndex, const QBarDataArray &rows, const QStringList &labels)
{
    Q_D(QBarDataProxy);
    d->setRows(rowIndex, rows, &labels);
    emit rowsChanged(rowIndex, rows.size());
}

/*!
 * Changes a single item at the position specified by \a rowIndex and
 * \a columnIndex to the item \a item.
 */
void QBarDataProxy::setItem(int rowIndex, int columnIndex, const QBarDataItem &item)
{
    Q_D(QBarDataProxy);
    d->setItem(rowIndex, columnIndex, item);
    emit itemChanged(rowIndex, columnIndex);
}

/*!
 * Changes a single item at the position \a position to the item \a item.
 * The x-value of \a position indicates the row and the y-value indicates the
 * column.
 */
void QBarDataProxy::setItem(const QPoint &position, const QBarDataItem &item)
{
    setItem(position.x(), position.y(), item);
}

/*!
 * Adds the new row \a row to the end of an array.
 * Existing row labels are not affected.
 *
 * Returns the index of the added row.
 */
int QBarDataProxy::addRow(QBarDataRow *row)
{
    Q_D(QBarDataProxy);
    int addIndex = d->addRow(row, 0);
    emit rowsAdded(addIndex, 1);
    emit rowCountChanged(rowCount());
    emit colCountChanged(colCount());
    return addIndex;
}

/*!
 * Adds a the new row \a row with the label \a label to the end of an array.
 *
 * Returns the index of the added row.
 */
int QBarDataProxy::addRow(QBarDataRow *row, const QString &label)
{
    Q_D(QBarDataProxy);
    int addIndex = d->addRow(row, &label);
    emit rowsAdded(addIndex, 1);
    emit rowCountChanged(rowCount());
    emit colCountChanged(colCount());
    return addIndex;
}

/*!
 * Adds the new \a rows to the end of an array.
 * Existing row labels are not affected.
 *
 * Returns the index of the first added row.
 */
int QBarDataProxy::addRows(const QBarDataArray &rows)
{
    Q_D(QBarDataProxy);
    int addIndex = d->addRows(rows, 0);
    emit rowsAdded(addIndex, rows.size());
    emit rowCountChanged(rowCount());
    emit colCountChanged(colCount());
    return addIndex;
}

/*!
 * Adds the new \a rows with \a labels to the end of the array.
 *
 * Returns the index of the first added row.
 */
int QBarDataProxy::addRows(const QBarDataArray &rows, const QStringList &labels)
{
    Q_D(QBarDataProxy);
    int addIndex = d->addRows(rows, &labels);
    emit rowsAdded(addIndex, rows.size());
    emit rowCountChanged(rowCount());
    emit colCountChanged(colCount());
    return addIndex;
}

/*!
 * Inserts the new row \a row into \a rowIndex.
 * If \a rowIndex is equal to the array size, the rows are added to the end of
 * the array.
 * The existing row labels are not affected.
 * \note The row labels array will be out of sync with the row array after this call
 *       if there were labeled rows beyond the inserted row.
 */
void QBarDataProxy::insertRow(int rowIndex, QBarDataRow *row)
{
    Q_D(QBarDataProxy);
    d->insertRow(rowIndex, row, 0);
    emit rowsInserted(rowIndex, 1);
    emit rowCountChanged(rowCount());
    emit colCountChanged(colCount());
}

/*!
 * Inserts the new row \a row with the label \a label into \a rowIndex.
 * If \a rowIndex is equal to array size, rows are added to the end of the
 * array.
 */
void QBarDataProxy::insertRow(int rowIndex, QBarDataRow *row, const QString &label)
{
    Q_D(QBarDataProxy);
    d->insertRow(rowIndex, row, &label);
    emit rowsInserted(rowIndex, 1);
    emit rowCountChanged(rowCount());
    emit colCountChanged(colCount());
}

/*!
 * Inserts new \a rows into \a rowIndex.
 * If \a rowIndex is equal to the array size, the rows are added to the end of
 * the array. The existing row labels are not affected.
 * \note The row labels array will be out of sync with the row array after this call
 *       if there were labeled rows beyond the inserted rows.
 */
void QBarDataProxy::insertRows(int rowIndex, const QBarDataArray &rows)
{
    Q_D(QBarDataProxy);
    d->insertRows(rowIndex, rows, 0);
    emit rowsInserted(rowIndex, rows.size());
    emit rowCountChanged(rowCount());
    emit colCountChanged(colCount());
}

/*!
 * Inserts new \a rows with \a labels into \a rowIndex.
 * If \a rowIndex is equal to the array size, the rows are added to the end of
 * the array.
 */
void QBarDataProxy::insertRows(int rowIndex, const QBarDataArray &rows, const QStringList &labels)
{
    Q_D(QBarDataProxy);
    d->insertRows(rowIndex, rows, &labels);
    emit rowsInserted(rowIndex, rows.size());
    emit rowCountChanged(rowCount());
    emit colCountChanged(colCount());
}

/*!
 * Removes the number of rows specified by \a removeCount starting at the
 * position \a rowIndex. Attempting to remove rows past the end of the
 * array does nothing. If \a removeLabels is \c true, the corresponding row
 * labels are also removed. Otherwise, the row labels are not affected.
 * \note If \a removeLabels is \c false, the row labels array will be out of
 * sync with the row array if there are labeled rows beyond the removed rows.
 */
void QBarDataProxy::removeRows(int rowIndex, int removeCount, bool removeLabels)
{
    Q_D(QBarDataProxy);
    if (rowIndex < rowCount() && removeCount >= 1) {
        d->removeRows(rowIndex, removeCount, removeLabels);
        emit rowsRemoved(rowIndex, removeCount);
        emit rowCountChanged(rowCount());
        emit colCountChanged(colCount());
    }
}

/*!
 * \property QBarDataProxy::colCount
 *
 * \brief The number of columns in the array.
 */
int QBarDataProxy::colCount() const
{
    const Q_D(QBarDataProxy);
    if (d->m_dataArray->size() <= 0)
        return 0;
    return d->m_dataArray->at(0)->size();
}

/*!
 * \property QBarDataProxy::rowCount
 *
 * \brief The number of rows in the array.
 */
int QBarDataProxy::rowCount() const
{
    const Q_D(QBarDataProxy);
    return d->m_dataArray->size();
}

/*!
 * \property QBarDataProxy::rowLabels
 *
 * \brief The optional row labels for the array.
 *
 * Indexes in this array match the row indexes in the data array.
 * If the list is shorter than the number of rows, all rows will not get labels.
 */
QStringList QBarDataProxy::rowLabels() const
{
    const Q_D(QBarDataProxy);
    return d->m_rowLabels;
}

void QBarDataProxy::setRowLabels(const QStringList &labels)
{
    Q_D(QBarDataProxy);
    if (d->m_rowLabels != labels) {
        d->m_rowLabels = labels;
        emit rowLabelsChanged();
    }
}

/*!
 * \property QBarDataProxy::columnLabels
 *
 * \brief The optional column labels for the array.
 *
 * Indexes in this array match column indexes in rows.
 * If the list is shorter than the longest row, all columns will not get labels.
 */
QStringList QBarDataProxy::columnLabels() const
{
    const Q_D(QBarDataProxy);
    return d->m_columnLabels;
}

void QBarDataProxy::setColumnLabels(const QStringList &labels)
{
    Q_D(QBarDataProxy);
    if (d->m_columnLabels != labels) {
        d->m_columnLabels = labels;
        emit columnLabelsChanged();
    }
}

/*!
 * Returns the pointer to the data array.
 */
const QBarDataArray *QBarDataProxy::array() const
{
    const Q_D(QBarDataProxy);
    return d->m_dataArray;
}

/*!
 * Returns the pointer to the row at the position \a rowIndex. It is guaranteed
 * to be valid only until the next call that modifies data.
 */
const QBarDataRow *QBarDataProxy::rowAt(int rowIndex) const
{
    const Q_D(QBarDataProxy);
    const QBarDataArray &dataArray = *d->m_dataArray;
    Q_ASSERT(rowIndex >= 0 && rowIndex < dataArray.size());
    return dataArray[rowIndex];
}

/*!
 * Returns the pointer to the item at the position specified by \a rowIndex and
 * \a columnIndex. It is guaranteed to be valid only
 * until the next call that modifies data.
 */
const QBarDataItem *QBarDataProxy::itemAt(int rowIndex, int columnIndex) const
{
    const Q_D(QBarDataProxy);
    const QBarDataArray &dataArray = *d->m_dataArray;
    Q_ASSERT(rowIndex >= 0 && rowIndex < dataArray.size());
    const QBarDataRow &dataRow = *dataArray[rowIndex];
    Q_ASSERT(columnIndex >= 0 && columnIndex < dataRow.size());
    return &dataRow.at(columnIndex);
}

/*!
 * Returns the pointer to the item at the position \a position. The x-value of
 * \a position indicates the row and the y-value indicates the column. The item
 * is guaranteed to be valid only until the next call that modifies data.
 */
const QBarDataItem *QBarDataProxy::itemAt(const QPoint &position) const
{
    return itemAt(position.x(), position.y());
}

/*!
 * \fn void QBarDataProxy::arrayReset()
 *
 * This signal is emitted when the data array is reset.
 * If the contents of the whole array are changed without calling resetArray(),
 * this signal needs to be emitted to update the graph.
 */

/*!
 * \fn void QBarDataProxy::rowsAdded(int startIndex, int count)
 *
 * This signal is emitted when the number of rows specified by \a count is
 * added starting at the position \a startIndex.
 * If rows are added to the array without calling addRow() or addRows(),
 * this signal needs to be emitted to update the graph.
 */

/*!
 * \fn void QBarDataProxy::rowsChanged(int startIndex, int count)
 *
 * This signal is emitted when the number of rows specified by \a count is
 * changed starting at the position \a startIndex.
 * If rows are changed in the array without calling setRow() or setRows(),
 * this signal needs to be emitted to update the graph.
 */

/*!
 * \fn void QBarDataProxy::rowsRemoved(int startIndex, int count)
 *
 * This signal is emitted when the number of rows specified by \a count is
 * removed starting at the position \a startIndex.
 *
 * The index is the current array size if the rows were removed from the end of
 * the array. If rows are removed from the array without calling removeRows(),
 * this signal needs to be emitted to update the graph.
 */

/*!
 * \fn void QBarDataProxy::rowsInserted(int startIndex, int count)
 *
 * This signal is emitted when the number of rows specified by \a count is
 * inserted at the position \a startIndex.
 *
 * If rows are inserted into the array without calling insertRow() or
 * insertRows(), this signal needs to be emitted to update the graph.
 */

/*!
 * \fn void QBarDataProxy::itemChanged(int rowIndex, int columnIndex)
 *
 * This signal is emitted when the item at the position specified by \a rowIndex
 * and \a columnIndex changes.
 * If the item is changed in the array without calling setItem(),
 * this signal needs to be emitted to update the graph.
 */

// QBarDataProxyPrivate

QBarDataProxyPrivate::QBarDataProxyPrivate(QBarDataProxy *q)
    : QAbstractDataProxyPrivate(q, QAbstractDataProxy::DataTypeBar),
      m_dataArray(new QBarDataArray)
{
}

QBarDataProxyPrivate::~QBarDataProxyPrivate()
{
    clearArray();
}

void QBarDataProxyPrivate::resetArray(QBarDataArray *newArray, const QStringList *rowLabels,
                                      const QStringList *columnLabels)
{
    Q_Q(QBarDataProxy);
    if (rowLabels)
        q->setRowLabels(*rowLabels);
    if (columnLabels)
        q->setColumnLabels(*columnLabels);

    if (!newArray)
        newArray = new QBarDataArray;

    if (newArray != m_dataArray) {
        clearArray();
        m_dataArray = newArray;
    }
}

void QBarDataProxyPrivate::setRow(int rowIndex, QBarDataRow *row, const QString *label)
{
    Q_ASSERT(rowIndex >= 0 && rowIndex < m_dataArray->size());

    if (label)
        fixRowLabels(rowIndex, 1, QStringList(*label), false);
    if (row != m_dataArray->at(rowIndex)) {
        clearRow(rowIndex);
        (*m_dataArray)[rowIndex] = row;
    }
}

void QBarDataProxyPrivate::setRows(int rowIndex, const QBarDataArray &rows,
                                   const QStringList *labels)
{
    QBarDataArray &dataArray = *m_dataArray;
    Q_ASSERT(rowIndex >= 0 && (rowIndex + rows.size()) <= dataArray.size());
    if (labels)
        fixRowLabels(rowIndex, rows.size(), *labels, false);
    for (int i = 0; i < rows.size(); i++) {
        if (rows.at(i) != dataArray.at(rowIndex)) {
            clearRow(rowIndex);
            dataArray[rowIndex] = rows.at(i);
        }
        rowIndex++;
    }
}

void QBarDataProxyPrivate::setItem(int rowIndex, int columnIndex, const QBarDataItem &item)
{
    Q_ASSERT(rowIndex >= 0 && rowIndex < m_dataArray->size());
    QBarDataRow &row = *(*m_dataArray)[rowIndex];
    Q_ASSERT(columnIndex < row.size());
    row[columnIndex] = item;
}

int QBarDataProxyPrivate::addRow(QBarDataRow *row, const QString *label)
{
    int currentSize = m_dataArray->size();
    if (label)
        fixRowLabels(currentSize, 1, QStringList(*label), false);
    m_dataArray->append(row);
    return currentSize;
}

int QBarDataProxyPrivate::addRows(const QBarDataArray &rows, const QStringList *labels)
{
    int currentSize = m_dataArray->size();
    if (labels)
        fixRowLabels(currentSize, rows.size(), *labels, false);
    for (int i = 0; i < rows.size(); i++)
        m_dataArray->append(rows.at(i));
    return currentSize;
}

void QBarDataProxyPrivate::insertRow(int rowIndex, QBarDataRow *row, const QString *label)
{
    Q_ASSERT(rowIndex >= 0 && rowIndex <= m_dataArray->size());
    if (label)
        fixRowLabels(rowIndex, 1, QStringList(*label), true);
    m_dataArray->insert(rowIndex, row);
}

void QBarDataProxyPrivate::insertRows(int rowIndex, const QBarDataArray &rows,
                                      const QStringList *labels)
{
    Q_ASSERT(rowIndex >= 0 && rowIndex <= m_dataArray->size());
    if (labels)
        fixRowLabels(rowIndex, rows.size(), *labels, true);
    for (int i = 0; i < rows.size(); i++)
        m_dataArray->insert(rowIndex++, rows.at(i));
}

void QBarDataProxyPrivate::removeRows(int rowIndex, int removeCount, bool removeLabels)
{
    Q_ASSERT(rowIndex >= 0);
    Q_Q(QBarDataProxy);
    int maxRemoveCount = m_dataArray->size() - rowIndex;
    removeCount = qMin(removeCount, maxRemoveCount);
    bool labelsChanged = false;
    for (int i = 0; i < removeCount; i++) {
        clearRow(rowIndex);
        m_dataArray->removeAt(rowIndex);
        if (removeLabels && m_rowLabels.size() > rowIndex) {
            m_rowLabels.removeAt(rowIndex);
            labelsChanged = true;
        }
    }
    if (labelsChanged)
        emit q->rowLabelsChanged();
}

void QBarDataProxyPrivate::clearRow(int rowIndex)
{
    if (m_dataArray->at(rowIndex)) {
        delete m_dataArray->at(rowIndex);
        (*m_dataArray)[rowIndex] = 0;
    }
}

void QBarDataProxyPrivate::clearArray()
{
    for (int i = 0; i < m_dataArray->size(); i++)
        clearRow(i);
    m_dataArray->clear();
    delete m_dataArray;
}

/*!
 * \internal
 * Fixes the row label array to include specified labels.
 */
void QBarDataProxyPrivate::fixRowLabels(int startIndex, int count, const QStringList &newLabels,
                                        bool isInsert)
{
    Q_Q(QBarDataProxy);
    bool changed = false;
    int currentSize = m_rowLabels.size();

    int newSize = newLabels.size();
    if (startIndex >= currentSize) {
        // Adding labels past old label array, create empty strings to fill intervening space
        if (newSize) {
            for (int i = currentSize; i < startIndex; i++)
                m_rowLabels << QString();
            // Doesn't matter if insert, append, or just change when there were no existing
            // strings, just append new strings.
            m_rowLabels << newLabels;
            changed = true;
        }
    } else {
        if (isInsert) {
            int insertIndex = startIndex;
            if (count)
                changed = true;
            for (int i = 0; i < count; i++) {
                if (i < newSize)
                    m_rowLabels.insert(insertIndex++, newLabels.at(i));
                else
                    m_rowLabels.insert(insertIndex++, QString());
            }
        } else {
            // Either append or change, replace labels up to array end and then add new ones
            int lastChangeIndex = count + startIndex;
            int newIndex = 0;
            for (int i = startIndex; i < lastChangeIndex; i++) {
                if (i >= currentSize) {
                    // Label past the current size, so just append the new label
                    if (newSize < newIndex) {
                        changed = true;
                        m_rowLabels << newLabels.at(newIndex);
                    } else {
                        break; // No point appending empty strings, so just exit
                    }
                } else if (newSize > newIndex) {
                    // Replace existing label
                    if (m_rowLabels.at(i) != newLabels.at(newIndex)) {
                        changed = true;
                        m_rowLabels[i] = newLabels.at(newIndex);
                    }
                } else {
                    // No more new labels, so clear existing label
                    if (!m_rowLabels.at(i).isEmpty()) {
                        changed = true;
                        m_rowLabels[i] = QString();
                    }
                }
                newIndex++;
            }
        }
    }
    if (changed)
        emit q->rowLabelsChanged();
}

QPair<float, float> QBarDataProxyPrivate::limitValues(int startRow, int endRow,
                                                      int startColumn, int endColumn) const
{
    QPair<float, float> limits = qMakePair(0.0f, 0.0f);
    endRow = qMin(endRow, m_dataArray->size() - 1);
    for (int i = startRow; i <= endRow; i++) {
        QBarDataRow *row = m_dataArray->at(i);
        if (row) {
            int lastColumn = qMin(endColumn, row->size() - 1);
            for (int j = startColumn; j <= lastColumn; j++) {
                const QBarDataItem &item = row->at(j);
                float itemValue = item.value();
                if (limits.second < itemValue)
                    limits.second = itemValue;
                if (limits.first > itemValue)
                    limits.first = itemValue;
            }
        }
    }
    return limits;
}

void QBarDataProxyPrivate::setSeries(QAbstract3DSeries *series)
{
    Q_Q(QBarDataProxy);
    QAbstractDataProxyPrivate::setSeries(series);
    QBar3DSeries *barSeries = static_cast<QBar3DSeries *>(series);
    emit q->seriesChanged(barSeries);
}

QT_END_NAMESPACE
