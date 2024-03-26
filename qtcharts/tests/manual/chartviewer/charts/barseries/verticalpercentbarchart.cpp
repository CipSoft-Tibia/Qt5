// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "charts.h"
#include <QtCharts/QChart>
#include <QtCharts/QPercentBarSeries>
#include <QtCharts/QBarSet>

class VerticalPercentBarChart: public Chart
{
public:
    QString name() { return QObject::tr("VerticalPercentBarChart"); }
    QString category()  { return QObject::tr("BarSeries"); }
    QString subCategory() { return QObject::tr("Vertical"); }

    QChart *createChart(const DataTable &table)
    {
        QChart *chart = new QChart();
        chart->setTitle("Percent bar chart");
        QPercentBarSeries *series = new QPercentBarSeries(chart);
        for (int i(0); i < table.count(); i++) {
            QBarSet *set = new QBarSet("Bar set " + QString::number(i));
            foreach (Data data, table[i])
                *set << data.first.y();
            series->append(set);
        }
        chart->addSeries(series);
        chart->createDefaultAxes();
        return chart;
    }
};

DECLARE_CHART(VerticalPercentBarChart)

