// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "serverrunner.h"

#include <QCoreApplication>
#include <memory>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    auto server = std::make_unique<ExampleServer>();
    server->run();
    return a.exec();
}
