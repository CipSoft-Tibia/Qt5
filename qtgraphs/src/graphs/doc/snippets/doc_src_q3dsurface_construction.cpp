// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

//! [5]
#include <QtGraphs>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    //! [0]
    Q3DSurface surface;
    surface.setMinimumSize(QSize(256, 256));
    surface.setResizeMode(QQuickWidget::SizeRootObjectToView);
    //! [0]
    //! [1]
    QSurfaceDataArray *data = new QSurfaceDataArray;
    QSurfaceDataRow *dataRow1 = new QSurfaceDataRow;
    QSurfaceDataRow *dataRow2 = new QSurfaceDataRow;
    //! [1]

    //! [2]
    *dataRow1 << QVector3D(0.0f, 0.1f, 0.5f) << QVector3D(1.0f, 0.5f, 0.5f);
    *dataRow2 << QVector3D(0.0f, 1.8f, 1.0f) << QVector3D(1.0f, 1.2f, 1.0f);
    *data << dataRow1 << dataRow2;
    //! [2]

    //! [3]
    QSurface3DSeries *series = new QSurface3DSeries;
    series->dataProxy()->resetArray(data);
    surface.addSeries(series);
    //! [3]
    //! [4]
    surface.show();
    //! [4]

    return app.exec();
}
//! [5]
