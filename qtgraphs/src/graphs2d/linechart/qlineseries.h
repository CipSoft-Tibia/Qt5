// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QTGRAPHS_QLINESERIES_H
#define QTGRAPHS_QLINESERIES_H

#include <QtGraphs/qxyseries.h>
#include <QtGraphs/qgraphsglobal.h>

QT_BEGIN_NAMESPACE

class QLineSeriesPrivate;

class Q_GRAPHS_EXPORT QLineSeries : public QXYSeries
{
    Q_OBJECT
    Q_PROPERTY(qreal width READ width WRITE setWidth NOTIFY widthChanged FINAL)
    Q_PROPERTY(Qt::PenCapStyle capStyle READ capStyle WRITE setCapStyle NOTIFY capStyleChanged FINAL)

    QML_NAMED_ELEMENT(LineSeries)
public:
    explicit QLineSeries(QObject *parent = nullptr);
    ~QLineSeries() override;
    QAbstractSeries::SeriesType type() const override;


    qreal width() const;
    void setWidth(qreal newWidth);

    Qt::PenCapStyle capStyle() const;
    void setCapStyle(Qt::PenCapStyle newCapStyle);

    Q_REVISION(6, 10) Q_INVOKABLE QPointF dataPointCoordinatesAt(qreal x, qreal y);

Q_SIGNALS:
    void widthChanged();
    void capStyleChanged();

protected:
    QLineSeries(QLineSeriesPrivate &dd, QObject *parent = nullptr);

    void componentComplete() override;

private:
    Q_DECLARE_PRIVATE(QLineSeries)
    Q_DISABLE_COPY(QLineSeries)
};

QT_END_NAMESPACE

#endif // QTGRAPHS_QLINESERIES_H
