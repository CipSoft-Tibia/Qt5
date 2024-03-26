// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "imagecomposer.h"

#include <QApplication>

//! [0]
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    ImageComposer composer;
    composer.show();
    return app.exec();
}
//! [0]
