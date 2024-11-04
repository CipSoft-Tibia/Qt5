// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QBARSERIES_H
#define QBARSERIES_H

#if 0
#  pragma qt_class(QBarSeries)
#endif

#include <QtGraphs/qabstractbarseries.h>
#include <QtGraphs/qabstractaxis.h>
#include <QtGraphs/qgraphsglobal.h>

QT_BEGIN_NAMESPACE

class QBarSeriesPrivate;

class QT_TECH_PREVIEW_API Q_GRAPHS_EXPORT QBarSeries : public QAbstractBarSeries
{
    Q_OBJECT
    Q_PROPERTY(QAbstractAxis *axisX READ axisX WRITE setAxisX NOTIFY axisXChanged)
    Q_PROPERTY(QAbstractAxis *axisY READ axisY WRITE setAxisY NOTIFY axisYChanged)
    QML_NAMED_ELEMENT(BarSeries)

public:
    explicit QBarSeries(QObject *parent = nullptr);
    ~QBarSeries();
    QAbstractSeries::SeriesType type() const override;

    QAbstractAxis *axisX();
    void setAxisX(QAbstractAxis *axis);
    QAbstractAxis *axisY();
    void setAxisY(QAbstractAxis *axis);

Q_SIGNALS:
    void axisXChanged(QAbstractAxis *axis);
    void axisYChanged(QAbstractAxis *axis);

private:
    Q_DECLARE_PRIVATE(QBarSeries)
    Q_DISABLE_COPY(QBarSeries)
};

QT_END_NAMESPACE

#endif // QBARSERIES_H
