// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef TEST_H
#define TEST_H

#include <QtQml/qqml.h>
#include <QObject>

class Test : public QObject
{
    Q_OBJECT
    QML_ELEMENT
public:
    Test();
};

#endif // TEST_H
