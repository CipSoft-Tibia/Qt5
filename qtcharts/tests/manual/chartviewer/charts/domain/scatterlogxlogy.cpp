// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "charts.h"
#include <QtCharts/QChart>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QLogValueAxis>

class ScatterLogXLogY: public Chart
{
public:
    QString name() { return "Scatter LogX LogY"; }
    QString category()  { return QObject::tr("Domain"); }
    QString subCategory() { return QObject::tr("Both Log"); }

    QChart *createChart(const DataTable &table)
    {
        QChart *chart = new QChart();
        chart->setTitle("Scatter: Log X, Log Y");

        QString name("Series ");
        int nameIndex = 0;
        foreach (DataList list, table) {
            QScatterSeries *series = new QScatterSeries(chart);
            foreach (Data data, list)
                series->append(data.first);
            series->setName(name + QString::number(nameIndex));
            nameIndex++;
            chart->addSeries(series);
        }

        QLogValueAxis *axisX= new QLogValueAxis();
        axisX->setBase(2);
        QLogValueAxis *axisY= new QLogValueAxis();
        axisY->setBase(2);
        foreach (QAbstractSeries *series, chart->series()) {
            chart->setAxisX(axisX, series);
            chart->setAxisY(axisY, series);
        }

        return chart;
    }
};

DECLARE_CHART(ScatterLogXLogY);
