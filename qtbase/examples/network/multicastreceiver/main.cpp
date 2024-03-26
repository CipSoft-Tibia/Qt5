// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>

#include "receiver.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Receiver receiver;
    receiver.show();
    return app.exec();
}
