// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [labelformat]
proxy->setItemLabelFormat(QStringLiteral("@valueTitle for (@rowLabel, @colLabel): %.1f"));
//! [labelformat]

//! [barmodelproxy]
// By defining row and column categories, you tell the mapping which row and column each item
// belongs to. The categories must match the data stored in the model in the roles you define
// for row and column mapping. In this example we expect "year" role to return four digit year
// and "month" to return three letter designation for the month.
//
// An example of an item in model would be:
// Requested role -> Returned data
// "year" -> "2016" // Matches the first row category, so this item is added to the first row.
// "month" -> "jan" // Matches the first column category, so this item is added as first item in the row.
// "income" -> "12.1"
// "expenses" -> "9.2"
QStringList years;
QStringList months;
years << "2016" << "2017" << "2018" << "2019" << "2020" << "2021" << "2022";
months << "jan" << "feb" << "mar" << "apr" << "may" << "jun" << "jul" << "aug" << "sep" << "oct" << "nov" << "dec";

QItemModelBarDataProxy *proxy = new QItemModelBarDataProxy(customModel,
                                                           QStringLiteral("year"), // Row role
                                                           QStringLiteral("month"), // Column role
                                                           QStringLiteral("income"), // Value role
                                                           years, // Row categories
                                                           months); // Column categories

//...

// To display different data later, you can simply change the mapping.
proxy->setValueRole(QStringLiteral("expenses"));
//! [barmodelproxy]

//! [scattermodelproxy]
// Map "density" value to X-axis, "hardness" to Y-axis and "conductivity" to Z-axis.
QItemModelScatterDataProxy *proxy = new QItemModelScatterDataProxy(customModel,
                                                                   QStringLiteral("density"),
                                                                   QStringLiteral("hardness"),
                                                                   QStringLiteral("conductivity"));
//! [scattermodelproxy]

//! [surfacemodelproxy]
QItemModelSurfaceDataProxy *proxy = new QItemModelSurfaceDataProxy(customModel,
                                                                   QStringLiteral("longitude"), // Row role
                                                                   QStringLiteral("latitude"), // Column role
                                                                   QStringLiteral("height")); // Y-position role
//! [surfacemodelproxy]

//! [proxyexample]
Q3DBars graph;
QBar3DSeries *series = new QBar3DSeries;

for (int i = 0; i < 10; ++i) {
    QBarDataRow dataRow;
    for (int j = 0; j < 5; ++j)
        dataRow.append(myData->getValue(i, j));
    series->dataProxy()->addRow(dataRow);
}

graph.addSeries(series);
//! [proxyexample]

//! [seriesexample]
Q3DBars graph;
QBar3DSeries *series = new QBar3DSeries;

QLinearGradient barGradient(0, 0, 1, 100);
barGradient.setColorAt(1.0, Qt::white);
barGradient.setColorAt(0.0, Qt::black);

series->setBaseGradient(barGradient);
series->setColorStyle(Q3DTheme::ColorStyle::ObjectGradient);
series->setMesh(QAbstract3DSeries::Mesh::Cylinder);

graph.addSeries(series);
//! [seriesexample]
