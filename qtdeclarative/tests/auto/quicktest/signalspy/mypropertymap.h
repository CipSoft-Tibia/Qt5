// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QQmlPropertyMap>

class MyPropertyMap : public QQmlPropertyMap
{
    Q_OBJECT

public:
    MyPropertyMap(QObject *parent = nullptr);
    ~MyPropertyMap();

Q_SIGNALS:
    void mySignal();
};
