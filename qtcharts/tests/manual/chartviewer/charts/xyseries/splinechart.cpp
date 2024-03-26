// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "charts.h"
#include <QtCharts/QChart>
#include <QtCharts/QSplineSeries>

class SplineChart: public Chart
{
public:
    QString name() { return QObject::tr("SplineChart"); }
    QString category()  { return QObject::tr("XYSeries"); }
    QString subCategory() { return QString(); }

    QChart *createChart(const DataTable &table)
    {
        QChart *chart = new QChart();
        chart->setTitle("Spline chart");
        QString name("Series ");
        int nameIndex = 0;
        foreach (DataList list, table) {
            QSplineSeries *series = new QSplineSeries(chart);
            foreach (Data data, list)
                series->append(data.first);
            series->setName(name + QString::number(nameIndex));
            nameIndex++;
            chart->addSeries(series);
        }
        chart->createDefaultAxes();
        return chart;
    }
};

DECLARE_CHART(SplineChart)

