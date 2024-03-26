// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "charts.h"
#include <QtCharts/QChart>
#include <QtCharts/QStackedBarSeries>
#include <QtCharts/QBarSet>

class VerticalStackedBarChart: public Chart
{
public:
    QString name() { return QObject::tr("VerticalStackedBarChart"); }
    QString category()  { return QObject::tr("BarSeries"); }
    QString subCategory() { return QObject::tr("Vertical"); }

    QChart *createChart(const DataTable &table)
    {
        QChart *chart = new QChart();
        chart->setTitle("Stacked bar chart");
        QStackedBarSeries *series = new QStackedBarSeries(chart);
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

DECLARE_CHART(VerticalStackedBarChart)

