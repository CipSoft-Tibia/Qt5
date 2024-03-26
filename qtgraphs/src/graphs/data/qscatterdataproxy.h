// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSCATTERDATAPROXY_H
#define QSCATTERDATAPROXY_H

#include <QtGraphs/qabstractdataproxy.h>
#include <QtGraphs/qscatterdataitem.h>

Q_MOC_INCLUDE(<QtGraphs/qscatter3dseries.h>)

QT_BEGIN_NAMESPACE

class QScatterDataProxyPrivate;
class QScatter3DSeries;

using QScatterDataArray = QList<QScatterDataItem>;

class Q_GRAPHS_EXPORT QScatterDataProxy : public QAbstractDataProxy
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QScatterDataProxy)
    Q_PROPERTY(int itemCount READ itemCount NOTIFY itemCountChanged)
    Q_PROPERTY(QScatter3DSeries *series READ series NOTIFY seriesChanged)

public:
    explicit QScatterDataProxy(QObject *parent = nullptr);
    virtual ~QScatterDataProxy();

    QScatter3DSeries *series() const;
    int itemCount() const;
    const QScatterDataArray *array() const;
    const QScatterDataItem *itemAt(int index) const;

    void resetArray(QScatterDataArray *newArray);

    void setItem(int index, const QScatterDataItem &item);
    void setItems(int index, const QScatterDataArray &items);

    int addItem(const QScatterDataItem &item);
    int addItems(const QScatterDataArray &items);

    void insertItem(int index, const QScatterDataItem &item);
    void insertItems(int index, const QScatterDataArray &items);

    void removeItems(int index, int removeCount);

Q_SIGNALS:
    void arrayReset();
    void itemsAdded(int startIndex, int count);
    void itemsChanged(int startIndex, int count);
    void itemsRemoved(int startIndex, int count);
    void itemsInserted(int startIndex, int count);

    void itemCountChanged(int count);
    void seriesChanged(QScatter3DSeries *series);

protected:
    explicit QScatterDataProxy(QScatterDataProxyPrivate *d, QObject *parent = nullptr);

private:
    Q_DISABLE_COPY(QScatterDataProxy)

    friend class Scatter3DController;
};

QT_END_NAMESPACE

#endif
