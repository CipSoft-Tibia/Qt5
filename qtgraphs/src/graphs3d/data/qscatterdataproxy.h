// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSCATTERDATAPROXY_H
#define QSCATTERDATAPROXY_H

#if 0
#  pragma qt_class(QScatterDataProxy)
#endif

#include <QtGraphs/qabstractdataproxy.h>
#include <QtGraphs/qscatterdataitem.h>

Q_MOC_INCLUDE(<QtGraphs/qscatter3dseries.h>)

QT_BEGIN_NAMESPACE

class QScatterDataProxyPrivate;
class QScatter3DSeries;

using QScatterDataArray = QList<QScatterDataItem>;

class QT_TECH_PREVIEW_API Q_GRAPHS_EXPORT QScatterDataProxy : public QAbstractDataProxy
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QScatterDataProxy)
    Q_PROPERTY(int itemCount READ itemCount NOTIFY itemCountChanged)
    Q_PROPERTY(QScatter3DSeries *series READ series NOTIFY seriesChanged)

public:
    explicit QScatterDataProxy(QObject *parent = nullptr);
    ~QScatterDataProxy() override;

    QScatter3DSeries *series() const;
    int itemCount() const;
    const QScatterDataArray &array() const;
    const QScatterDataItem &itemAt(int index) const;

    void resetArray();
    void resetArray(QScatterDataArray newArray);

    void setItem(int index, QScatterDataItem item);
    void setItems(int index, QScatterDataArray items);

    int addItem(QScatterDataItem item);
    int addItem(QScatterDataItem &&item);
    int addItems(QScatterDataArray items);
    int addItems(QScatterDataArray &&items);

    void insertItem(int index, QScatterDataItem item);
    void insertItems(int index, QScatterDataArray items);

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
    explicit QScatterDataProxy(QScatterDataProxyPrivate &d, QObject *parent = nullptr);

private:
    Q_DISABLE_COPY(QScatterDataProxy)

    friend class QQuickGraphsScatter;
};

QT_END_NAMESPACE

#endif
