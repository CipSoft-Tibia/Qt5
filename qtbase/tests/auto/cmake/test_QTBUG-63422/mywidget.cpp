// Copyright (C) 2017 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Kevin Funk <kevin.funk@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mywidget.h"
#include "ui_mywidget.h"

MyWidget::MyWidget(QWidget *parent)
    : QWidget(parent)
{
    emit someSignal();
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    MyWidget myWidget;
    return 0;
}
