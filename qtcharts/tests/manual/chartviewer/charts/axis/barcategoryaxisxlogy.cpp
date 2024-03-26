// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "charts.h"
#include <QtCharts/QChart>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QLogValueAxis>
#include <QtCharts/QBarCategoryAxis>

class BarCategoryAxisXLogY: public Chart
{
public:
    QString name() { return "AxisX"; }
    QString category()  { return QObject::tr("Axis"); }
    QString subCategory() { return "BarCategoryAxis"; }

    QChart *createChart(const DataTable &table)
    {
        QChart *chart = new QChart();
        chart->setTitle(" BarCateogry X , LogValue Y");

        QString name("Series ");
        QBarSeries *series = new QBarSeries(chart);
        QLogValueAxis *logvalueaxis = new QLogValueAxis();
        logvalueaxis->setBase(2);
        QBarCategoryAxis *barcategory = new QBarCategoryAxis();
        for (int i(0); i < table.count(); i++) {
            QBarSet *set = new QBarSet("Bar set " + QString::number(i));
            foreach (Data data, table[i])
                *set << data.first.y();
            series->append(set);
        }
        chart->addSeries(series);

        int count = series->barSets().first()->count();


        for (int i = 0; i < count; i++) {
            barcategory->append("BarSet " + QString::number(i));
        }

        chart->setAxisY(logvalueaxis, series);
        chart->setAxisX(barcategory, series);

        return chart;
    }
};

class BarCategoryAxisXLogYTitle: public BarCategoryAxisXLogY
{
    QString name() { return "AxisX Title"; }
    QChart *createChart(const DataTable &table)
    {
        QChart *chart = BarCategoryAxisXLogY::createChart(table);
        chart->axisX()->setTitleText("Axis X");
        chart->axisY()->setTitleText("Axis Y");
        chart->setTitle(" BarCateogry X , LogValue Y, title");
        return chart;
    }
};

DECLARE_CHART(BarCategoryAxisXLogY);
DECLARE_CHART(BarCategoryAxisXLogYTitle);
