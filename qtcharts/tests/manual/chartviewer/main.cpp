// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "window.h"
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <iostream>

QVariantHash parseArgs(QStringList args)
{
    QVariantHash parameters;

    while (!args.isEmpty()) {

        QString param = args.takeFirst();
        if (param.startsWith("--")) {
            param.remove(0, 2);

            if (args.isEmpty() || args.first().startsWith("--")) {
                parameters[param] = true;
            } else {
                QString value = args.takeFirst();
                if (value == "true" || value == "on" || value == "enabled") {
                    parameters[param] = true;
                } else if (value == "false" || value == "off" || value == "disable") {
                    parameters[param] = false;
                } else {
                    if (value.endsWith('"'))
                        value.chop(1);
                    if (value.startsWith('"'))
                        value.remove(0, 1);
                    parameters[param] = value;
                }
            }
        }
    }

    return parameters;
}

void printHelp()
{
    std::cout << "chartviewer <options> " << std::endl;
    std::cout << "  --view <1/2/3/4>  - set size of charts' grid" << std::endl;
    std::cout << "  --chart <categoryName::subCategory::chartName>  - set template to be show " << std::endl;
    std::cout << "  --opengl <enabled/disbaled>  - set opengl mode" << std::endl;
    std::cout << "  --theme <name>  - set theme" << std::endl;
    std::cout << "  --legend <alignment>  - set legend alignment" << std::endl;
    std::cout << "  --help  - prints this help" << std::endl;
    std::cout << "Examples: " << std::endl;
    std::cout << "  chartviewer --view 4 --chart Axis" << std::endl;
    std::cout << "  chartviewer --view 1 --chart Axis::BarCategoryAxis::Axis " << std::endl;
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QVariantHash parameters = parseArgs(QApplication::arguments());
    if(parameters.contains("help"))
    {
        printHelp();
        return 0;
    }
    Window window(parameters);
    window.show();
    return a.exec();
}

