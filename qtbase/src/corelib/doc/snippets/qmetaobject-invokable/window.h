// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>

//! [Window class with invokable method]
class Window : public QWidget
{
    Q_OBJECT

public:
    Window();
    void normalMethod();
    Q_INVOKABLE void invokableMethod();
};
//! [Window class with invokable method]

#endif
