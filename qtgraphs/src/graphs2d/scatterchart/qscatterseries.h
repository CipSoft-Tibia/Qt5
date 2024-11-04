// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSCATTERSERIES_H
#define QSCATTERSERIES_H

#if 0
#  pragma qt_class(QScatterSeries)
#endif

#include <QtGraphs/qxyseries.h>
#include <QtGraphs/qgraphsglobal.h>

QT_BEGIN_NAMESPACE

class QScatterSeriesPrivate;

class QT_TECH_PREVIEW_API Q_GRAPHS_EXPORT QScatterSeries : public QXYSeries
{
    Q_OBJECT

public:
    explicit QScatterSeries(QObject *parent = nullptr);
    ~QScatterSeries();
    QAbstractSeries::SeriesType type() const override;

    QML_NAMED_ELEMENT(ScatterSeries)

protected:
    QScatterSeries(QScatterSeriesPrivate &d, QObject *parent = nullptr);

    void componentComplete() override;

private:
    Q_DECLARE_PRIVATE(QScatterSeries)
    Q_DISABLE_COPY(QScatterSeries)
};

QT_END_NAMESPACE

#endif // QSCATTERSERIES_H
