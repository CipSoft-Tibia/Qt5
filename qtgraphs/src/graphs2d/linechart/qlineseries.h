// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QLINESERIES_H
#define QLINESERIES_H

#if 0
#  pragma qt_class(QLineSeries)
#endif

#include <QtGraphs/qxyseries.h>
#include <QtGraphs/qgraphsglobal.h>

QT_BEGIN_NAMESPACE

class QLineSeriesPrivate;

class QT_TECH_PREVIEW_API Q_GRAPHS_EXPORT QLineSeries : public QXYSeries
{
    Q_OBJECT
    Q_PROPERTY(qreal width READ width WRITE setWidth NOTIFY widthChanged)
    Q_PROPERTY(Qt::PenCapStyle capStyle READ capStyle WRITE setCapStyle NOTIFY capStyleChanged)

public:
    explicit QLineSeries(QObject *parent = nullptr);
    ~QLineSeries();
    QAbstractSeries::SeriesType type() const override;

    QML_NAMED_ELEMENT(LineSeries)

    qreal width() const;
    void setWidth(qreal newWidth);

    Qt::PenCapStyle capStyle() const;
    void setCapStyle(const Qt::PenCapStyle &newCapStyle);

Q_SIGNALS:
    void widthChanged();
    void capStyleChanged();

protected:
    QLineSeries(QLineSeriesPrivate &d, QObject *parent = nullptr);

    void componentComplete() override;

private:
    Q_DECLARE_PRIVATE(QLineSeries)
    Q_DISABLE_COPY(QLineSeries)
};

QT_END_NAMESPACE

#endif // QLINESERIES_H
